#!/bin/bash
#------------------------------------------------------------------------------
# haFunctions.sh
# 
# Description:
#   This script is used for controlling the state of appliances used during
#   High Availability testing.
#   ISMsetup.sh or testEnv must have at least the following defined:
#       A1_IPv4_1, A1_IPv4_HA0, A1_IPv4_HA1, A1_USER
#       A2_IPv4_1, A2_IPv4_HA0, A2_IPv4_HA1, A2_USER
#
#   NOTE: This script isn't meant for screwing up the HA pair. All commands
#         expect the servers to be in some known state. Things are likely
#         to break if the servers aren't in the state that a command
#         is expecting them to be in.
#
# Input:
#   $1 - required: -a action
#        Available actions include:
#          setupHA - Configure the 2 servers as an HA pair, both in AutoDetect
#          setupPrimary - Configure a server as StandAlone primary
#          setupStandby - Configure a server as Standby AutoDetect
#          disableHA - Return A1 and A2 to individually running servers
#          autoSetup - setup HA using new simplified configuration
#
#          stopPrimary - Kill the primary server with "imaserver stop force"
#          stopPrimaryNoCheck - Kill the primary server with "imaserver stop force"
#                               without checking that standby server takes over
#          stopStandby - Kill the standby server
#          crashBothServers - Kill both servers
#
#          startStandby - Restart the currently dead server into standby
#
#          swapPrimary - Kill primary server and bring it back up as standby
#          startAndPickPrimary - Start both servers and decide which should
#                                take over as primary. Choose with -p option or
#                                the most recent primary is chosen.
#
#          verifyBothRunning - Verifies that the 2 servers are up in Running
#                              and standby.
#
#          clean_store - clean_store on both servers in the HA pair
#
#          deviceRestartPrimary - Kill the primary server with "device restart force"
#          powerCyclePrimary - power cycle the primary with ipmitool  . Does verify
#                            - that standby takes over, but does NOT verify that old
#                            - primary has come back up. That is left to the user to
#                            - do if necessary at a later time. ONLY works if AX_IMMIP
#                            - is defined.
#
#   $2 - required: -f logfile
#
#   $3 - optional: -p primary
#   $4 - optional: -s standby
#        Available Primary/Standby values:
#          1: Server A1
#          2: Server A2
#
#   $5 - optional: -i IPversion
#        Available IPversion values:
#          4: use IPv4
#          6: use IPv6
#        NOTE: Automation appliances aren't set up for IPv6 HA interfaces.
#
# Return Code:
#   0 - Pass
#   1 - Fail
#
# Changes:
#   3/05/14  - Updated some version checking for Group. autoSetup requires clean
#              stores on each appliance, or a fresh pair of appliances. 
#   2/21/14  - Add autoSetup action for new HA simplified configuration
#   2/20/14  - Add "Group=${MQKEY}HAPair" to update HighAvailability commands.
#   2/14/14  - Add stopStandby and swapPrimary actions
#   2/13/14  - Fixed startAndPickPrimary to give servers time to start.
#   1/13/14  - Update startAndPickPrimary to check if servers are already running
#              in a proper HA configuration. This is new behavior as of V2.0
#              to automatically detect which server should become primary when
#              both have non-empty stores.
#   10/13/13 - Add clean_store action. This expects both servers to be running when
#              it is called.
#   9/20/13  - Updates for differences in status messages between V1.0 and V1.1
# 
#------------------------------------------------------------------------------
# Sample run-scenarios input file:
# #----------------------------------------------------
# # Scenario 4 - setup StandAlone primary server
# #----------------------------------------------------
# xml[${n}]="scenario4"
# timeouts[${n}]=400
# scenario[${n}]="${xml[${n}]} - Configure StandAlone Primary Server"
# component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|setupPrimary|-p|2"
# components[${n}]="${component[1]}"
# ((n+=1))
#------------------------------------------------------------------------------

# HA Configuration Functions

#------------------------------------------------------------------------------
# Configures and enables HA, then restarts the servers
# The Group parameter uses ${MQKEY} from testEnv / ISMsetup to give the pair
# a name unique to the M1 controller.
#   $1 - "manual" or "auto"
#      Manual configures HA the original way, specifying the NIC properties
#      Auto configures HA the new way, with just the Group property
#        Auto also expects either a fresh pair of servers, or both stores
#        to be clean.
# Exits on failure
#------------------------------------------------------------------------------
function configure_ha {
    trace "Configuring HA on A${sID} (Standby)"
    if [[ "$1" == "auto" ]] ; then
        run_cli_command 0 "${standby}" "imaserver update HighAvailability ${USEGROUP} EnableHA=True"
        trace "Configuring HA on A${pID} (Primary)"
        run_cli_command 0 "${primary}" "imaserver update HighAvailability ${USEGROUP} EnableHA=True PreferredPrimary=True"
        server_stop
    elif [[ "$1" == "manual" ]] ; then
        run_cli_command 0 "${standby}" "imaserver update HighAvailability EnableHA=True PreferredPrimary=False StartupMode=AutoDetect RemoteDiscoveryNIC=${s1ha0Val} LocalDiscoveryNIC=${s2ha0Val} LocalReplicationNIC=${s2ha1Val} ${USEGROUP}"
        clean_store ${sID}
        trace "Configuring HA on A${pID} (Primary)"
        run_cli_command 0 "${primary}" "imaserver update HighAvailability EnableHA=True PreferredPrimary=True StartupMode=AutoDetect RemoteDiscoveryNIC=${s2ha0Val} LocalDiscoveryNIC=${s1ha0Val} LocalReplicationNIC=${s1ha1Val} ${USEGROUP}"
        stop_individual ${pID}
    fi
    server_start
}

#------------------------------------------------------------------------------
# Configure HA on just primary server to act in StandAlone mode
#
# 1. Configure HA on primary
# 2. Restart primary
#
# NOTE: When bringing in a standby server, StartupMode must be updated to
#       AutoDetect, otherwise restarting this server will cause splitbrain.
# Returns 0 on success
#------------------------------------------------------------------------------
function configure_primary {
    trace "Configuring HA on A${pId} (StandAlone)"
    run_cli_command 0 "${primary}" "imaserver update HighAvailability EnableHA=True StartupMode=StandAlone PreferredPrimary=True RemoteDiscoveryNIC=${s2ha0Val} LocalDiscoveryNIC=${s1ha0Val} LocalReplicationNIC=${s1ha1Val} ${USEGROUP}"
    stop_individual ${pID}
    verify_status_individual ${pID} "${STOPPED}" "120"
    start_individual ${pID}
}

#------------------------------------------------------------------------------
# Configure HA on standby server to add it into existing HA configuration
#
# 1. Configure HA on standby
# 2. Clean store on standby
# 3. Start standby
# 4. Update HA on priamry to AutoDetect
#
# NOTE: After starting the standby server, StartupMode on the primary server
#       is updated to AutoDetect.
# Exits on failure
#------------------------------------------------------------------------------
function configure_standby {
    trace "Configuring HA on A${sID} (Adding as Standby)"
    run_cli_command 0 "${standby}" "imaserver update HighAvailability EnableHA=True StartupMode=AutoDetect PreferredPrimary=False RemoteDiscoveryNIC=${s1ha0Val} LocalDiscoveryNIC=${s2ha0Val} LocalReplicationNIC=${s2ha1Val} ${USEGROUP}"
    clean_store ${sID}
    start_individual ${sID}
    trace "Updating primary server configuration StartupMode to AutoDetect"
    run_cli_command 0 "${primary}" "imaserver update HighAvailability StartupMode=AutoDetect"
}

#------------------------------------------------------------------------------
# Disables HA config, then restarts the servers
# 
# 1. Disable HA on server1.
# 2. Disable HA on server2.
# 3. Restart both servers.
#------------------------------------------------------------------------------
function disable_ha {
    trace "Disabling HA on A1"
    if [[ "${USEGROUP}" != "" ]] ; then
        run_cli_command 0 "${server1}" "imaserver update HighAvailability EnableHA=False \"LocalReplicationNIC=\" \"LocalDiscoveryNIC=\" \"RemoteDiscoveryNIC=\" \"Group=\""
        trace "Disabling HA on A2"
        run_cli_command 0 "${server2}" "imaserver update HighAvailability EnableHA=False \"LocalReplicationNIC=\" \"LocalDiscoveryNIC=\" \"RemoteDiscoveryNIC=\" \"Group=\""
    else
        run_cli_command 0 "${server1}" "imaserver update HighAvailability EnableHA=False"
        trace "Disabling HA on A2"
        run_cli_command 0 "${server2}" "imaserver update HighAvailability EnableHA=False"
    fi
    # Stop both servers
    server_stop_force
    verify_status "${STOPPED}" "${STOPPED}" "120"
    # Restart both servers
    server_start
}

# IMA stop/start functions

#------------------------------------------------------------------------------
# Force stop both servers and verify they are stopped
#------------------------------------------------------------------------------
function server_stop_force {
    trace "Force stopping A1"
    run_cli_command 0 "${server1}" "imaserver stop force"
    trace "Force stopping on A2"
    run_cli_command 0 "${server2}" "imaserver stop force"
    sleep 3
    verify_status "${STOPPED}" "${STOPPED}" "120"
}

#------------------------------------------------------------------------------
# Stop both servers
#------------------------------------------------------------------------------
function server_stop {
    stop_individual 1
    stop_individual 2
}

#------------------------------------------------------------------------------
# Stop just one server
#   $1 - ID of the server to stop
#
# Stop the server specified by $1, and verify that "Status = Stopped".
#------------------------------------------------------------------------------
function stop_individual {
    trace "Stopping server A$1. Status will be checked for up to 60 seconds."
    set_appliance $1
    run_cli_command 0 "${appliance}" "imaserver stop"
    trace "Waiting for stop command to take effect."
    statusloop "60" "${appliance}" "${STOPPED}"

    # If the server would not stop for some reason, generate a core file
    # This takes a long time on a real appliance
    if ! [[ "${reply}" =~ "${STOPPED}" ]] ; then
        trace "RC=1 Server stop failed for server A$1. Taking a coredump of the server!"

        cmd="${appliance} advanced-pd-options dumpcore imaserver"
        reply=`${cmd}` > haFunctions.dumpcore.log
        echo -e "${reply}\n" | tee -a ${LOG}
        path=`pwd`
        run_cli_command 0 "${appliance}" "advanced-pd-options list"
        for name in ${reply} ; do
            if [[ "${name}" =~ "imaserver_core" ]] ; then
                trace "Found imaserver corefile: ${name}"
                filename=${name}
            fi
        done
        run_cli_command 0 "${appliance}" "advanced-pd-options export ${filename} scp://root@${M1_IPv4_1}:${path}"
        run_cli_command 0 "${appliance}" "advanced-pd-options delete ${filename}"

        # Force stop the server so that we can continue
        run_cli_command x "${appliance}" "imaserver stop force"
    fi
    sleep 3
}

#------------------------------------------------------------------------------
# Start both servers and verify they are no longer stopped
#------------------------------------------------------------------------------
function server_start {
    # Detach the first server start, so that both start at the same time.
    start_individual 1 &
    start_individual 2
    sleep 3
}

#------------------------------------------------------------------------------
# Start just one server
#   $1 - ID of the server to start
#
# Start the server specified by $1.
#
# NOTE: This won't exit as a failure if the server is already running.
#
# Exits on failure
#------------------------------------------------------------------------------
function start_individual {
    trace "Starting server A$1"
    set_appliance $1
    run_cli_command x "${appliance}" "imaserver start"
    if [[ "${reply}" =~ "${ALREADY_RUNNING}" ]] ; then
        # Server is already running, just return
        trace "The server is already running"
        return
    elif [[ "${reply}" =~ "${SERVER_STARTING}" ]] ; then
        # Command worked. Hopefully this means the server is coming up.
        trace "The server is starting"
        return
    else
        # The server failed to start
        trace "The server failed to start"
        abort
    fi

    verify_status_not $1 "${STOPPED}"
}

# Status verification functions

#------------------------------------------------------------------------------
# Verify status of an individual server is "$2"
#   $1 - ID of server to verify status of
#   $2 - Expected server status to compare against
#   $3 - timeout in seconds
#   $4 - Optional, "ignore_fail" to keep script from exiting if verify fails.
# Exits on failure unless "ignore_fail" specified
# Returns 0 on success, 1 on failure
#------------------------------------------------------------------------------
function verify_status_individual {
    set_appliance $1
    trace "Verifying status for appliance A$1. Status will be checked for up to ${timeout} seconds\nExpected status: $2"
    trace "Waiting for status to update."
    statusloop "$3" "${appliance}" "$2"

    if ! [[ "${reply}" =~ "$2" ]] ; then
        if [[ "$4" == "ignore_fail" ]] ; then
            return 1
        fi
        trace "FAILURE: A$1 Status:\n${reply}."
        abort
    else
        return 0
    fi
}

#------------------------------------------------------------------------------
# Verify status of an individual server is not "$2"
#   $1 - ID of the server
#   $2 - Status to check for
#   $3 - Optional, "ignore_fail" to keep script from exiting if verify fails
# Exits on failure unless "ignore_fail" specified
# Returns 0 on success, 1 on failure
#------------------------------------------------------------------------------
function verify_status_not {
    trace "Verifying server A$1 is not \"$2\""
    set_appliance $1

    run_cli_command x "${appliance}" "status imaserver"
    if [[ "${reply}" =~ "$2" ]] ; then
        if [[ "$3" == "ignore_fail" ]] ; then
            return 1
        fi
        trace "FAILURE: A$1 Status:\n${reply}."
        abort
    else
        return 0
    fi
}

#------------------------------------------------------------------------------
# Verify status of both servers
#   $1 - Expected status of servers to compare against
#   $2 - Expected status of servers to compare against
#   $3 - timeout in seconds
#   $4 - Optional, "ignore_fail" to keep script from exiting if verify fails.
# Exits on failure unless "ignore_fail" specified
# Returns 0 on success, 1 on failure
#------------------------------------------------------------------------------
function verify_status {
    a1_success=1
    a2_success=1
    trace "Verifying status for both servers. Status will be checked for up to $3 seconds each.\nExpected status 1: $1\nExpected status 2: $2"
    trace "Waiting for status to update."
    statusloop "$3" "${server1}" "$1" "$2"
    a1_reply=${reply}
    if [ ${rc} -eq 0 ] ; then
        a1_success=0
    fi
    statusloop "$3" "${server2}" "$1" "$2"
    a2_reply=${reply}
    if [ ${rc} -eq 0 ] ; then
        a2_success=0
    fi
    if [[ ${a1_success} != 0 || ${a2_success} != 0 ]] ; then
        if [[ "$4" == "ignore_fail" ]] ; then
            return 1
        fi
        trace "FAILED: RC=1 Verify status failed.\nA1 status:\n${a1_reply}\nA2 status:\n${a2_reply}."
        abort
    else
        return 0
    fi
}

#------------------------------------------------------------------------------
# Verify the HARole of the specified server
#   $1 - Server ID (1=A1, 2=A2)
#   $2 - New Role (PRIMARY, STANDBY, UNSYNC, HADISABLED)
#   $3 - Old Role (PRIMARY, STANDBY, UNSYNC, HADISABLED)
#   $4 - Active Nodes (0, 1, 2)
#   $5 - Reason Code (1 - 5)
# Returns 0 on success
#------------------------------------------------------------------------------
function verify_harole {
    local rc=0
    trace "Verify HARole on server A$1"
    set_appliance $1
    run_cli_command 0 "${appliance}" "imaserver harole"
    newRole=`echo "${reply}" | grep NewRole | cut -d'=' -f 2 | cut -c2- | cut -d' ' -f 1`
    oldRole=`echo "${reply}" | grep OldRole | cut -d'=' -f 2 | cut -c2- | cut -d' ' -f 1`
    activeNodes=`echo "${reply}" | grep ActiveNodes | cut -d'=' -f 2 | cut -c2- | cut -d' ' -f 1`
    reasonCode=`echo "${reply}" | grep ReasonCode | cut -d'=' -f 2 | cut -c2- | cut -d' ' -f 1`
    if [[ "$2" != "${newRole}" ]] ; then
        trace "Verify HARole failed for A$1: Expected NewRole=$2 : Actual NewRole=${newRole}"
        ((rc+=1))
    fi
    if [[ "$3" != "${oldRole}" ]] ; then
        trace "Verify HARole failed for A$1: Expected OldRole=$3 : Actual OldRole=${oldRole}"
        ((rc+=1))
    fi
    if [[ "$4" != "${activeNodes}" ]] ; then
        trace "Verify HARole failed for A$1: Expected ActiveNodes=$4 : Actual ActiveNodes=${activeNodes}"
        ((rc+=1))
    fi
    if [[ "$5" != "${reasonCode}" ]] ; then
        trace "Verify HARole failed for A$1: Expected ReasonCode=$5 : Actual ReasonCode=${reasonCode}"
        ((rc+=1))
    fi
    return ${rc}
}

# HA event functions

#------------------------------------------------------------------------------
# Start the standby server
#
# Figure out which server is in "Status = Stopped" and start that server.
# Verify that the server is no longer stopped.
#
# NOTE: Expects one of the servers to already be in a running state.
#------------------------------------------------------------------------------
function start_standby {
    cmd="${server1} status imaserver"
    reply=`${cmd}`
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    if [[ "${reply}" =~ "${STOPPED}" ]] ; then
        trace "A1 Status:\n${reply}"
        trace "Starting standby server A1"
        pID=2
        sID=1
        start_individual ${sID}
        return 0
    elif [[ "${reply1}" =~ "${STOPPED}" ]] ; then
        trace "A2 Status:\n${reply1}"
        trace "Starting standby server A2"
        pID=1
        sID=2
        start_individual ${sID}
        return 0
    else
        return 2
    fi
}

#------------------------------------------------------------------------------
# Powercycle  the primary server
#
# Determine which server is in "Status = Running", and power cycle that
# server using the ipmi tool.
#
# Verify that the standby server has taken over as primary.
#
# NOTE: Expects there to be a running standby server ready to take over
#       as primary, or else returns a failure.
# Exits on failure
#------------------------------------------------------------------------------
function powercycle_primary {
    local rc=1 rc1 rc2
    local immip

    cmd="${server1} status imaserver"
    reply=`${cmd}`
    rc1=$?
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    rc2=$?

    if [ "${rc1}" != "0" -o "${rc2}" != "0" ] ;then 
        trace "FAILURE: Could not get status for both servers. rc1:${rc1} rc2:${rc2}. It is not a good idea to powercycle unless everything is working"
        abort
    fi

    if [[ "${reply}" =~ "${RUNNING}" ]] ; then
        primary=${server1}
        primaryID=1
        standby=${server2}
        standbyID=2
        if [ "${A1_IMMIP}" ] ;then 
            immip=${A1_IMMIP}
        else
            trace "FAILURE: A1_IMMIP environment variable is not set"
            abort
        fi
    elif [[ "${reply1}" =~ "${RUNNING}" ]] ; then
        primary=${server2}
        primaryID=2
        standby=${server1}
        standbyID=1
        if [ "${A2_IMMIP}" ] ;then 
            immip=${A2_IMMIP}
        else
            trace "FAILURE: A2_IMMIP environment variable is not set"
            abort
        fi
    fi
    trace "Power Cycle primary server A${primaryID} using ${immip}. Standby status will be checked for up to 180 seconds."
    trace "Running command: imaserver stop force"
    cmd=" ipmitool -I lanplus -H ${immip} -U USERID -P PASSW0RD chassis power cycle"
    ${cmd}
    rc=$?
    if [ "$rc" != "0" ] ;then
        trace "FAILURE: non zero rc ${rc} from ipmitool command: ${cmd}"
        abort
    fi
    timeout=180
    #elapsed=0
    trace "Waiting for previous standby to assume the role of primary"
    statusloop "${timeout}" "${standby}" "${RUNNING}"
    verify_harole ${standbyID} PRIMARY STANDBY 1 0
    ((rc+=$?))

    if [[ ${rc} != 0 ]] ; then
        trace "FAILURE: RC=1 New primary A${standbyID} server failed to take over."
        abort
    fi
}
#------------------------------------------------------------------------------
# Crash the primary server
#
# Determine which server is in "Status = Running", and stop that server.
# Verify that the standby server has taken over as primary.
#
# NOTE: Expects there to be a running standby server ready to take over
#       as primary, or else returns a failure.
# Exits on failure
#------------------------------------------------------------------------------
function stop_primary {
    local rc=1
    cmd="${server1} status imaserver"
    reply=`${cmd}`
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    if [[ "${reply}" =~ "${RUNNING}" ]] ; then
        primary=${server1}
        primaryID=1
        standby=${server2}
        standbyID=2
    elif [[ "${reply1}" =~ "${RUNNING}" ]] ; then
        primary=${server2}
        primaryID=2
        standby=${server1}
        standbyID=1
    fi
    trace "Killing primary server A${primaryID}. Standby status will be checked for up to 30 seconds."
    run_cli_command x "${primary}" "imaserver stop force"
    timeout=30
    #elapsed=0
    trace "Waiting for stop force command to take effect."
    statusloop "${timeout}" "${standby}" "${RUNNING}"
    verify_harole ${standbyID} PRIMARY STANDBY 1 0
    ((rc+=$?))

    if [[ ${rc} != 0 ]] ; then
        trace "FAILURE: RC=1 New primary A${standbyID} server failed to take over."
        abort
    fi
}

#------------------------------------------------------------------------------
# Crash the standby server
#------------------------------------------------------------------------------
function stop_standby {
    cmd="${server1} status imaserver"
    reply=`${cmd}`
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    if [[ "${reply}" =~ "${RUNNING}" ]] ; then
        primary=${server1}
        primaryID=1
        standby=${server2}
        standbyID=2
    elif [[ "${reply1}" =~ "${RUNNING}" ]] ; then
        primary=${server2}
        primaryID=2
        standby=${server1}
        standbyID=1
    fi
    trace "Killing standby server A${standbyID}."
    run_cli_command x "${standby}" "imaserver stop force"
    run_cli_command 0 "${standby}" "status imaserver"
}

#------------------------------------------------------------------------------
# Crash the primary server without checking for standby to take over
#------------------------------------------------------------------------------
function stop_primary_no_check {
    cmd="${server1} status imaserver"
    reply=`${cmd}`
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    if [[ "${reply}" =~ "${RUNNING}" ]] ; then
        primary=${server1}
        primaryID=1
    elif [[ "${reply1}" =~ "${RUNNING}" ]] ; then
        primary=${server2}
        primaryID=2
    fi
    trace "Killing primary server A${primaryID}"
    trace "Running command: imaserver stop force"
    run_cli_command x "${primary}" "imaserver stop force"
    verify_status_individual ${primaryID} "${STOPPED}" "120"
}

#------------------------------------------------------------------------------
# Run device restart on the primary server
#
# Determine which server is in "Status = Running", and restart that server.
# Verify that the standby server has taken over as primary.
#
# NOTE: Expects there to be a running standby server ready to take over
#       as primary, or else returns a failure.
# Exits on failure
#------------------------------------------------------------------------------
function device_restart_primary {
    local rc=1
    cmd="${server1} status imaserver"
    reply=`${cmd}`
    cmd1="${server2} status imaserver"
    reply1=`${cmd1}`
    if [[ "${reply}" =~ "${RUNNING}" ]] ; then
        primary=${server1}
        primaryID=1
        standby=${server2}
        standbyID=2
    elif [[ "${reply1}" =~ "${RUNNING}" ]] ; then
        primary=${server2}
        primaryID=2
        standby=${server1}
        standbyID=1
    fi
    trace "Killing primary server A${primaryID}"
    # device restart force command never returns when ssh'd to the server
    # Once we have run command, kill the ssh process so that the script
    # can continue
    trace "Running command: device restart force"
    ${primary} device restart force &
    PID=$!
    sleep 5
    trace "Killing Process: ${PID}. Standby status will be checked for up to 30 seconds."
    killCmd=`kill -9 ${PID}`
    timeout=30
    #elapsed=0
    reply=''
    trace "Waiting for standby to become primary."
    statusloop "${timeout}" "${standby}" "${RUNNING}"
    verify_harole ${standbyID} PRIMARY STANDBY 1 0
    ((rc+=$?))

    if [[ ${rc} != 0 ]] ; then
        trace "RC=1 New primary A${standbyID} server failed to take over."
        abort
    fi
}

#------------------------------------------------------------------------------
# Figure out which server acted as primary most recently
#
# Get the timestamp from PrimaryLastTime for both servers, and determine
# which server was the most recent primary.
#
# Returns ID of server with latest primary timestamp
#------------------------------------------------------------------------------
function get_last_primary_times {
    trace "Picking which server should take over as primary"
    s1lastTime=`echo "status imaserver" | ${server1} | grep PrimaryLastTime | cut -d'=' -f 2 | cut -c1-`
    s2lastTime=`echo "status imaserver" | ${server2} | grep PrimaryLastTime | cut -d'=' -f 2 | cut -c1-`
    s1TimeStamp=$(date -d "${s1lastTime}" '+%s')
    s2TimeStamp=$(date -d "${s2lastTime}" '+%s')
    if [[ ${s1TimeStamp} -gt ${s2TimeStamp} ]] ; then
        latest=1
    else
        latest=2
    fi
    trace "A1 PrimaryLastTime: ${s1lastTime} (${s1TimeStamp})"
    trace "A2 PrimaryLastTime: ${s2lastTime} (${s2TimeStamp})"
    trace "Server A${latest} has the most recent store"
    return ${latest}
}

#------------------------------------------------------------------------------
# Setup the servers to both be brought up at the same time when HA
# was already configured
# 
# Set ${primary} back to production mode, and stop it.
# Clean the store of ${standby} and return it to production mode, and stop it.
#------------------------------------------------------------------------------
function pick_primary {
    if [[ ${latest} = 1 ]] ; then
        pID=1
        sID=2
        primary=${server1}
        standby=${server2}
    else
        pID=2
        sID=1
        primary=${server2}
        standby=${server1}
    fi
    run_cli_command 0 "${primary}" "imaserver runmode production"
    stop_individual ${pID}
    clean_store ${sID}
}

# Helper functions

#------------------------------------------------------------------------------
# Print formattted trace statements with timestamps
#------------------------------------------------------------------------------
function trace {
    timestamp=`date +"%Y-%m-%d %H:%M:%S"`
    echo -e "[${timestamp}] $1\n" | tee -a ${LOG}
}

#------------------------------------------------------------------------------
# Run an imaserver cli command
# $1 = Expected RC
# $2 = ssh user@host
# $3+ = command
#------------------------------------------------------------------------------
function run_cli_command {
    cmd="$2 ${@:3}"
    trace "${cmd}"
    reply=`${cmd} 2>&1`
    result=$?
    echo "${reply}" | tee -a ${LOG}
    echo -e "RC=${result}\n" | tee -a ${LOG}
    if [[ "$1" == "x" ]] ; then
        # ignore return code
        return
    elif [ ${result} -ne $1 ] ; then
        echo "FAILED: Expected RC: $1" | tee -a ${LOG}
        abort
    fi
}

#------------------------------------------------------------------------------
# Set the appliance variable used in many functions
#------------------------------------------------------------------------------
function set_appliance {
    if [ $1 -eq 1 ] ; then
        appliance=${server1}
    else
        appliance=${server2}
    fi
}

#------------------------------------------------------------------------------
# Clean store on just the specified server
#   $1 - ID of server to clean the store of
#
# Run clean_store on the server.
# Restart the server and verify it is in maintenance mode.
# Set the server back to production mode, and leave it stopped.
#------------------------------------------------------------------------------
function clean_store {
    trace "Clean store on A$1"
    set_appliance $1
    run_cli_command 0 "${appliance}" "imaserver runmode Maintenance"
    stop_individual $1
    verify_status_individual $1 "${STOPPED}" "120"
    start_individual $1
    verify_status_individual $1 "${MAINTENANCE}" "120"
    run_cli_command 0 "${appliance}" "imaserver runmode clean_store"
    stop_individual $1
    verify_status_individual $1 "${STOPPED}" "120"
    start_individual $1
    verify_status_individual $1 "${MAINTENANCE}" "120"
    run_cli_command 0 "${appliance}" "imaserver runmode production"
    stop_individual $1
}

function clean_store_both {
    trace "Clean store on both servers"
    run_cli_command 0 "${server1}" "imaserver runmode Maintenance"
    run_cli_command 0 "${server2}" "imaserver runmode Maintenance"
    server_stop
    verify_status "${STOPPED}" "${STOPPED}" "120"
    server_start
    verify_status "${MAINTENANCE}" "${MAINTENANCE}" "120"
    run_cli_command 0 "${server1}" "imaserver runmode clean_store"
    run_cli_command 0 "${server2}" "imaserver runmode clean_store"
    server_stop
    verify_status "${STOPPED}" "${STOPPED}" "120"
    server_start
    verify_status "${MAINTENANCE}" "${MAINTENANCE}" "120"
    run_cli_command 0 "${server1}" "imaserver runmode production"
    run_cli_command 0 "${server2}" "imaserver runmode production"
    server_stop
    verify_status "${STOPPED}" "${STOPPED}" "120"
    #server_start
}

#------------------------------------------------------------------------------
# Loop and check the status of the specified server.
# $1 = timeout
# $2 = appliance
# $3 = status
# $4 = status
# rc=0 on Success
#------------------------------------------------------------------------------
function statusloop {
    rc=1
    elapsed=0
    timeout=$1
    appliance=$2
    expectedA=$3
    if [ $# == 4 ] ; then
        expectedB=$4
    else
        expectedB="fakestatusforconditional"
    fi
    while [ ${elapsed} -lt ${timeout} ] ; do
        echo -n "*"
        sleep 1
        cmd="${appliance} status imaserver"
        reply=`${cmd}`
        if [[ "${reply}" =~ "${expectedA}" || "${reply}" =~ "${expectedB}" ]] ; then
            rc=0
            echo ""
            break
        fi
        elapsed=`expr ${elapsed} + 1`
    done
    trace "${appliance} Status:\n${reply}"
}

function abort {
    trace "haFunctions.sh Test result is Failure. Aborting!"
    if [[ "${action}" =~ "etup" ]] ; then
        # If we are aborting any of setupHA, autoSetup, setupPrimary, setupStandby,
        # then disable HA and restart the servers
        run_cli_command 0 "${primary}" "imaserver update HighAvailability EnableHA=False LocalDiscoveryNIC= LocalReplicationNIC= RemoteDiscoveryNIC="
        run_cli_command 0 "${standby}" "imaserver update HighAvailability EnableHA=False LocalDiscoveryNIC= LocalReplicationNIC= RemoteDiscoveryNIC="
        server_stop
        server_start
    fi
    exit 1
}

function print_usage {
    echo './haFunctions -a <action> -f <logfile> [-p <#>] [-s <#>] [-i <#>]'
    echo '    a )'
    echo '        action:       setupHA, setupPrimary, setupStandby, disableHA, autoSetup'
    echo '                      stopPrimary, stopPrimaryNoCheck, stopStandby, crashBothServers'
    echo '                      startStandby, swapPrimary, startAndPickPrimary'
    echo '                      clean_store, verifyBothRunning'
    echo '                      deviceRestartPrimary, powerCyclePrimary'
    echo '    f )'
    echo '        logfile:      Name of log file'
    echo '    i )'
    echo '        IPversion:    4, 6 (Optional: Default=4)'
    echo '    p )'
    echo '        primary:      1, 2 (Optional: Default=1)'
    echo '    s )'
    echo '        standby:      1, 2 (Optional: Default=2)'
    echo ' '
    echo '  Use the following to setup or disable HA on a pair of servers:'
    echo '  Ex. ./haFunctions -a setupHA -f setupHA.log'
    echo '  Ex. ./haFunctions -a setupHA -p 2 -s 1 -i 6 -f setupHA.log'
    echo '  Ex. ./haFunctions -a disableHA -f disableHA.log'
    echo ' '
    echo '  Use the following to setup a StandAlone primary server:'
    echo '  Ex. ./haFunctions -a setupPrimary -p 2 -f setupPrimary.log'
    echo ' '
    echo '  Use the following to add a standby server to an existing StandAlone server'
    echo '  Ex. ./haFunctions -a setupStandby -s 1 -f setupStandby.log'
    echo ' '
    echo '  Use the following to bring an individual server in the HA pair up or down:'
    echo '  Ex. ./haFunctions -a stopPrimary -f stopPrimary.log'
    echo '  Ex. ./haFunctions -a stopPrimaryNoCheck -f stopOnlyServer.log'
    echo '  Ex. ./haFunctions -a stopStandby -f stopStandby.log'
    echo '  Ex. ./haFunctions -a crashBothServers -f crashBothServers.log'
    echo '  Ex. ./haFunctions -a deviceRestartPrimary -f deviceRestartPrimary.log'
    echo '  Ex. ./haFunctions -a powerCyclePrimary -f powerCyclePrimary.log'
    echo '  Ex. ./haFunctions -a startStandby -f startStandby.log'
    echo ' '
    echo '  Use the following to start both servers into maintenance mode, and pick'
    echo '  which one should take over as primary'
    echo '  Ex. ./haFunctions -a startAndPickPrimary -p 2 -f startAndPickPrimary.log'
    echo ' '
    echo '  Use the following to verify the servers are up and running'
    echo '  Ex. ./haFunctions -a verifyBothRunning -f verify.log'
    echo ' '
    echo '  Use the following to clean the stores of both appliances in the HA pair'
    echo '  EX. ./haFunctions -a clean_store'
    echo ' '
    echo '  Use the following to switch the current primary to be the standby'
    echo '  Ex. ./haFunctions -a swapPrimary -f swapPrimary.log'
    echo ' '
    exit 1
}

function parse_args {
    if [[ -z "${action}" ]]; then
        print_usage
    fi

    if [[ ${pID} = 0 && ${sID} != 1 ]] ; then
        pID=1
    else
        pID=2
    fi
    if [[ ${pID} -lt 1 || ${pID} -gt 2 ]] ; then
        print_usage
    fi

    if [[ ${sID} = 0 && ${pID} != 2 ]] ; then
        sID=2
    else
        sID=1
    fi
    if [[ ${sID} -lt 1 || ${sID} -gt 2 ]] ; then
        print_usage
    fi

    if [[ ${ip} != 4 && ${ip} != 6 ]] ; then
        print_usage
    fi

    if [[ ${choice} -lt 0 || ${choice} -gt 2 ]] ; then
        print_usage
    fi

    if [[ -z "${LOG}" ]] ; then
        print_usage
    fi

    if [[ ${A1_IPv4_HA0} == "" || ${A1_IPv4_HA1} == "" ]] ; then
        echo "A1_IPv4_HA0 and A1_IPv4_HA1 must be defined"
        exit 1
    fi

    if [[ ${A2_IPv4_HA0} == "" || ${A2_IPv4_HA1} == "" ]] ; then
        echo "A2_IPv4_HA0 and A2_IPv4_HA1 must be defined"
        exit 1
    fi
}

#set -x
pID=0
sID=0
ip=4
busybox=0
choice=0
while getopts "a:f:i:p:s:" opt; do
    case $opt in
    a )
        action=${OPTARG}
        ;;
    f )
        LOG=${OPTARG}
        ;;
    i )
        ip=${OPTARG}
        ;;
    p )
        pID=${OPTARG}
        choice=${OPTARG}
        ;;
    s )
        sID=${OPTARG}
        ;;
    * )
        exit 1
        ;;
    esac
done

if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
    source  ../scripts/ISMsetup.sh
else
    echo "neither testEnv.sh or ISMsetup.sh was used"
fi

parse_args

echo -e "Entering $0 with $# arguments: $@\n" >> ${LOG}

s1user="A${pID}_USER"
s1userVal=$(eval echo -e \$${s1user})
s1host="A${pID}_HOST"
s1hostVal=$(eval echo -e \$${s1host})
s1ha0="A${pID}_IPv${ip}_HA0"
s1ha0Val=$(eval echo -e \$${s1ha0})
s1ha1="A${pID}_IPv${ip}_HA1"
s1ha1Val=$(eval echo -e \$${s1ha1})
primary="ssh ${s1userVal}@${s1hostVal}"

s2user="A${sID}_USER"
s2userVal=$(eval echo -e \$${s2user})
s2host="A${sID}_HOST"
s2hostVal=$(eval echo -e \$${s2host})
s2ha0="A${sID}_IPv${ip}_HA0"
s2ha0Val=$(eval echo -e \$${s2ha0})
s2ha1="A${sID}_IPv${ip}_HA1"
s2ha1Val=$(eval echo -e \$${s2ha1})
standby="ssh ${s2userVal}@${s2hostVal}"

a1user="A1_USER"
a1userVal=$(eval echo -e \$${a1user})
a1host="A1_HOST"
a1hostVal=$(eval echo -e \$${a1host})
a2user="A2_USER"
a2userVal=$(eval echo -e \$${a2user})
a2host="A2_HOST"
a2hostVal=$(eval echo -e \$${a2host})

server1="ssh ${a1userVal}@${a1hostVal}"
server2="ssh ${a2userVal}@${a2hostVal}"

############################
###   STATUS VARIABLES   ###
###   & version checks   ###
############################
# On V1.0.x.x, show imaserver only works if the server is running.
# So if versioncmd=~'1.0' or is empty, set V1.0.x.x values.
# Otherwise, set V1.1.x.x+ values.
# On V2.x.x.x+, Group is a required HA parameter.
versioncmd=`${server1} show imaserver`
if [[ "${versioncmd}" == "" || "${versioncmd}" =~ "version is 1.0" ]] ; then
    # Old status messages 1.0.x.x
    MAINTENANCE='Status = Maintenance'
    RUNNING='Status = Running'
    STOPPED='Status = Stopped'
    STANDBY='Status = Standby'
    SERVER_STARTING='Start IBM MessageSight server'
    ALREADY_RUNNING='server process is already running'
    USEGROUP=""
else
    # New status messages version 1.1+
    MAINTENANCE='Status = Running (maintenance)'
    RUNNING='Status = Running (production)'
    STOPPED='Status = Stopped'
    STANDBY='Status = Standby'
    SERVER_STARTING='server is starting'
    ALREADY_RUNNING='already running'
    if [[ "${versioncmd}" =~ "version is 1.1" ]] ; then
        USEGROUP=""
    else
        # V2.x.x.x+ requires Group to be set on HighAvailability config.
        USEGROUP="Group=${MQKEY}HAPair"
    fi
fi

############################

total_fails=0

#------------------------------------------------------------------------------
# Configure servers for HA
# Procedure for configuring High Availability
#
# Default: A1=Primary, A2=Standby
#
# 1. Update high availability on the appliance which will act as standby (A2)
# 2. Clean the store of the server wa want to act as standby (A2)
# 3. Set the standby server (A2) back to production, and stop it
# 4. Update high availability on the appliance which will act as priamry (A1)
# 5. Stop the server which will act as primary (A1)
# 6. Start both servers
# 5. Verify A1 is primary and A2 is standby
#------------------------------------------------------------------------------
if [[ "${action}" = "setupHA" ]]; then
    d_start=`date +%s`
    configure_ha "manual"
    d_config_done=`date +%s`
    verify_status "${RUNNING}" "${STANDBY}" "120"
    d_servers_up=`date +%s`
    verify_harole ${pID} PRIMARY PRIMARY 2 0
    rc=$?
    verify_harole ${sID} STANDBY UNSYNC 2 0
    ((rc+=$?))

    # Calculate total time and time for synchronization
    let d_total=${d_servers_up}-${d_start}
    let d_sync=${d_servers_up}-${d_config_done}
    trace "Total Time: ${d_total} seconds"
    trace "Sync Time: ${d_sync} seconds"

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# New auto HA configuration!
# Still requires doing a clean store on the standby server.
# Only real difference is we don't need to specify LocalReplicationNIC,
# LocalDiscoveryNIC or RemoteDiscoveryNIC anymore.
#------------------------------------------------------------------------------
elif [[ "${action}" = "autoSetup" ]] ; then
    if [[ "${USEGROUP}" != "" ]] ; then
        d_start=`date +%s`
        configure_ha "auto"
        verify_status "${RUNNING}" "${STANDBY}" "120"
        d_servers_up=`date +%s`

        verify_harole ${pID} PRIMARY PRIMARY 2 0
        rc=$?
        verify_harole ${sID} STANDBY UNSYNC 2 0
        ((rc+=$?))

        # Calculate total time and time for synchronization
        let d_sync=${d_servers_up}-${d_start}
        trace "Total Time: ${d_sync} seconds"

        if [[ ${rc} -gt 0 ]] ; then
            let total_fails+=1
        fi
    else
        trace "autoSetup action is not allowed on this version"
        ((total_fails+=1))
    fi

#------------------------------------------------------------------------------
# Setup server as HA server in StandAlone mode (Default: A1)
#------------------------------------------------------------------------------
elif [[ "${action}" = "setupPrimary" ]] ; then
    d_start=`date +%s`
    configure_primary
    d_config_done=`date +%s`
    verify_status_individual ${pID} "${RUNNING}" "120"
    d_servers_up=`date +%s`
    verify_harole ${pID} PRIMARY UNSYNC 1 0
    rc=$?

    # Calculate total time and time for synchronization
    let d_total=${d_servers_up}-${d_start}
    let d_sync=${d_servers_up}-${d_config_done}
    trace "Total Time: ${d_total} seconds"
    trace "Sync Time: ${d_sync} seconds"

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Setup server as standby for an existing StandAlone server (Default: A2)
#------------------------------------------------------------------------------
elif [[ "${action}" = "setupStandby" ]] ; then
    d_start=`date +%s`
    configure_standby
    d_config_done=`date +%s`
    verify_status "${RUNNING}" "${STANDBY}" "120"
    d_servers_up=`date +%s`
    verify_harole ${pID} PRIMARY PRIMARY 2 0
    rc=$?
    verify_harole ${sID} STANDBY UNSYNC 2 0
    ((rc+=$?))

    # Calculate total time and time for synchronization
    let d_total=${d_servers_up}-${d_start}
    let d_sync=${d_servers_up}-${d_config_done}
    trace "Total Time: ${d_total} seconds"
    trace "Sync Time: ${d_sync} seconds"

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Clean store and return to non-HA Configuration
#------------------------------------------------------------------------------
elif [[ "${action}" = "disableHA" ]]; then
    server_start
    verify_status_not 1 "${STOPPED}"
    verify_status_not 2 "${STOPPED}"

    # If verify_not_maintenance returns 1, at least 1 server
    # is in maintenance mode. Continue disabling HA but
    # exit the script as a failure when done.
    disable_ha
    clean_store_both
    server_start
    verify_status "${RUNNING}" "${RUNNING}" "120"

    verify_harole 1 HADISABLED HADISABLED 0 0
    rc=$?
    verify_harole 2 HADISABLED HADISABLED 0 0
    ((rc+=$?))

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Restart the crashed server into Standby mode
#------------------------------------------------------------------------------
elif [[ "${action}" = "startStandby" ]]; then
    d_start=`date +%s`
    start_standby
    result=$?
    if [ ${result} -eq 2 ] ; then
        trace "Both servers were already running"
    else
	    d_start_returned=`date +%s`
	    verify_status "${RUNNING}" "${STANDBY}" "120"
	    d_servers_up=`date +%s`
	    verify_harole ${pID} PRIMARY PRIMARY 2 0
	    rc=$?
	    verify_harole ${sID} STANDBY UNSYNC 2 0
	    ((rc+=$?))
	
	    # Calculate total time and time for synchronization
	    let d_total=${d_servers_up}-${d_start}
	    let d_sync=${d_servers_up}-${d_start_returned}
	    trace "Total Time: ${d_total} seconds"
	    trace "Sync Time: ${d_sync} seconds"
	
	    if [[ ${rc} -gt 0 ]] ; then
	        let total_fails+=1
	    fi
    fi
    
#------------------------------------------------------------------------------
# Crash the current primary server
#------------------------------------------------------------------------------
elif [[ "${action}" = "powerCyclePrimary" ]]; then
    powercycle_primary

#------------------------------------------------------------------------------
# Crash the current primary server
#------------------------------------------------------------------------------
elif [[ "${action}" = "stopPrimary" ]]; then
    stop_primary
    verify_status "${STOPPED}" "${RUNNING}" "120"

#------------------------------------------------------------------------------
# Kill the primary and being it back up as standby
#------------------------------------------------------------------------------
elif [[ "${action}" = "swapPrimary" ]]; then
    stop_primary
    verify_status "${STOPPED}" "${RUNNING}" "45"
    start_standby 
    verify_status "${RUNNING}" "${STANDBY}" "70"

#------------------------------------------------------------------------------
# Kill the standby server in the pair
#------------------------------------------------------------------------------
elif [[ "${action}" = "stopStandby" ]]; then
    stop_standby
    verify_status "${STOPPED}" "${RUNNING}" "45"

#------------------------------------------------------------------------------
# Kill primary without checking that standby takes over
#------------------------------------------------------------------------------
elif [[ "${action}" = "stopPrimaryNoCheck" ]] ; then
    stop_primary_no_check

#------------------------------------------------------------------------------
# Run device restart on the current primary server
#------------------------------------------------------------------------------
elif [[ "${action}" = "deviceRestartPrimary" ]]; then
    device_restart_primary
    verify_harole ${primaryID} PRIMARY STANDBY 1 0
    rc=$?

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Crash both servers (Wait for 2nd server to become primary before stopping)
#------------------------------------------------------------------------------
elif [[ "${action}" = "crashBothServers" ]] ; then
    stop_primary
    stop_primary_no_check

#------------------------------------------------------------------------------
# Start both servers with HA already configured, and decide which
# server should take over as primary
# As of version 2.0, the servers should automatically resolve which one
# should take over as primary. If they resolve automatically, leave them as
# they are.
#------------------------------------------------------------------------------
elif [[ "${action}" = "startAndPickPrimary" ]] ; then
    d_start=`date +%s`
    server_start
    d_start_done=`date +%s`
    # if version >= 2.0, primary/standby picked automatically
    verify_status "${RUNNING}" "${STANDBY}" "45" "ignore_fail"
    do_pick=$?
    if [ ${do_pick} -ne 0 ] ; then
        trace "The servers did not automatically resolve which should become primary."
        verify_status "${MAINTENANCE}" "${MAINTENANCE}" "120"
        verify_harole 1 UNSYNC_ERROR UNSYNC 1 5
        rc=$?
        verify_harole 2 UNSYNC_ERROR UNSYNC 1 5
        ((rc+=$?))
        if [[ ${choice} = 0 ]] ; then
            get_last_primary_times
            latest=$?
            trace "No choice specified. Setting server A${latest} as primary"
        else
            get_last_primary_times
            latest=${choice}
            trace "User choice is to set server A${choice} as primary"
        fi
        pick_primary $latest
        d_restart=`date +%s`
        server_start
        verify_status "${RUNNING}" "${STANDBY}" "120"
        verify_harole ${pID} PRIMARY PRIMARY 2 0
        ((rc+=$?))
        verify_harole ${sID} STANDBY UNSYNC 2 0
        ((rc+=$?))

        # Calculate total time and time for synchronization
        d_servers_up=`date +%s`
        let d_total=${d_servers_up}-${d_start}
        let d_sync=${d_servers_up}-${d_restart}
        trace "Total Time: ${d_total} seconds"
        trace "Sync Time: ${d_sync} seconds"
        if [[ "${versioncmd}" =~ "version is 1." ]] ; then
        	# If version is < 2.0, then it is acceptable to have to pick the primary
        	# manually.
        	trace "Primary server was picked manually"
        else
        	# But if version is >= 2.0, then the primary server should have been picked
        	# automatically.
            trace "FAILURE: Primary server was picked manually"
            ((rc+=1))
        fi
    else
        trace "Primary and standby have been picked automatically"
    fi

    if [[ ${rc} -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# This just checks that the 2 servers are running as an HA pair.
#------------------------------------------------------------------------------
elif [[ "${action}" = "verifyBothRunning" ]] ; then
    verify_status "${RUNNING}" "${STANDBY}" "120"

#------------------------------------------------------------------------------
# Clean the store of both appliances in the HA pair.
# This will expect both servers to be running when called.
# It makes no assumption of which server is primary or standby.
# Simply put both servers into maintenance mode and then clean the stores.
# When complete, whichever server has PreferredPrimary=True will most likely
# become the primary server.
#------------------------------------------------------------------------------
elif [[ "${action}" = "clean_store" ]] ; then
    verify_status_not 1 "${STOPPED}"
    verify_status_not 2 "${STOPPED}"
    clean_store_both
    verify_status "${STOPPED}" "${STOPPED}" "30"
    server_start

#------------------------------------------------------------------------------
# Invalid action entered
#------------------------------------------------------------------------------
else
    print_usage
fi

if [ ${total_fails} -gt 0 ] ; then
    trace "haFunctions.sh Test result is Failure! Number of failures: ${total_fails}"
else
    trace "haFunctions.sh Test result is Success!"
fi
