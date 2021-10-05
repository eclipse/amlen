#!/bin/bash
#------------------------------------------------------------------------------
# 
# putServerMaintenance.sh
#
# Description:
#   Put an appliance server into maintenance mode
#   
#
# 
# Usage:
#   ./putServerN2devicerestart.sh <user@server> <MAINTENANCE|PRODUCTION>
#
# Dependencies: none
#
#------------------------------------------------------------------------------

function print_help {
    echo "Put a server into a 'device restart' state. After restart put server into"
    echo "requested runmode. This script will put server into maintenance before"
    echo "the 'device restart' is sent"
    echo " "
    echo "Usage: putServerN2devicerestart.sh <user@server> <MAINTENANCE|PRODUCTION>"
    echo "         user@server = the userid and server ip to issue a 'device restart'"
    echo "         PRODUCTION|MAINTENANCE = requested server runmode after successful 'device restart' "
}

function is_alive {
#
# lets check upto 10mins (ie 600s) and poll every 30s
#
    timeout=600
    elapsed=0
    ADDRESS=`echo $1  |gawk -F@ {'print \$2'}`
    echo "testing ${ADDRESS} is_alive" 
    while [ $elapsed -lt $timeout ] ; do
        reply=`ping -c 5 ${ADDRESS} |grep "5 packets transmitted"`
	echo "reply:$reply"
        if [[ "$reply" == *" 0% packet loss"* ]] ; then
            echo " $reply"
	    echo "server looks alive"
            return 0
        fi
	echo "sleeping for 30s before next ping"
        sleep 30
        elapsed=`expr $elapsed + 30`
    done
    
    return 1

}

function verify_status {
#
# set timeout to 5mins
#
    timeout=300
    elapsed=0
 
    while [ $elapsed -lt $timeout ] ; do
        sleep 1
        reply=`ssh ${SERVER} status imaserver 2>&1`

#        echo "reply:$reply"
        case $# in
            1)  echo "testing 1arg '$1'"
		if [[ "$reply" == *"$1"* ]] ; then
                   echo " status '$reply' reached"
                   return 0
        	fi
 		;;
            2) echo "testing 2arg '$1' '$2'"
		if [[ "$reply" == *"$1" || "$reply" == *"$2"* ]] ; then
                   echo " status '$reply' reached"
                   return 0
        	fi
		;;
            3) echo "testing 3arg '$1' '$2' '$3'"
		if [[ "$reply" == *"$1"* || "$reply" == *"$2"* || "$reply" == *"$3"* ]] ; then
                   echo " status '$reply' reached"
                   return 0
        	fi
		;;
	    *)  echo "Unknown verification"
		return 1
		;;
	
        esac

        elapsed=`expr $elapsed + 1`
    done

    #
    # we timedout on status. we should fail
    #
    case $# in
        1) echo "Timed out on verifying status:'$1'"
	   ;;
        2) echo "Timed out on verifying status:'$1' '$2'"
	   ;;
        3) echo "Timed out on verifying status:'$1' '$2' '$3'"
	   ;;
    esac

    cmd="ssh ${SERVER} status imaserver"
    reply=`$cmd`
    echo "Status: ${reply}"
    return 1
}

MAINTENANCE='Status = Running (maintenance)'
STOPPED='Status = Stopped'

#
# Below are valid production for standalone and HA
#
RUNNING='Status = Running (production)'
STANDBY='Status = Standby'
STORESTARTING='Status = StoreStarting'

if [[ $# == 2 ]]; then
    SERVER=$1
    if [[ "$2" != "MAINTENANCE" && "$2" != "PRODUCTION" ]]; then
	print_help
	exit 1
    else
	REQMODE=$2
    fi
else
    print_help
    exit 1
fi

d_start=`date +%s`
cmd="ssh ${SERVER} status imaserver"
reply=`$cmd`

#
# if stopped need to start to figure out status
#
if [[ "$reply" =~ "${STOPPED}" ]]; then
    ssh ${SERVER} imaserver start
    verify_status "${MAINTENANCE}" "${STANDBY}" "${RUNNING}"
fi

cmd="ssh ${SERVER} status imaserver"
reply=`$cmd`

if [[ "$reply" =~ "${MAINTENANCE}" ]]; then
    #
    # We are in running maintenance mode. we just need to send device restart
    #
    echo "we already in maintenance" 

elif [[ "$reply" =~ "${RUNNING}" || "$reply" =~ "${STANDBY}" ]]; then
    #
    # We are in production running or standby. Lets be nice and put server
    # into running maintence before device restart
    #
    echo "Running as production or standby. Lets put into running maintenance mode"
    d_startmaintenance=`date +%s`
    ssh ${SERVER} imaserver runmode maintenance
    ssh ${SERVER} imaserver stop
    verify_status "${STOPPED}"
    ssh ${SERVER} imaserver start
    d_donemaintenance=`date +%s`
    verify_status "${MAINTENANCE}"

else
    echo "Server in unknown status"
    exit 1
fi

#
# lets send device restart
#   
echo "sending 'device restart' to $SERVER"
d_startdevrestart=`date +%s`
cmd="ssh ${SERVER} device restart"
reply=`$cmd`

#
# Sleep for 5mins before continuing
#
echo "Sleeping for 5mins before checking server is back up"
sleep 300

#
# lets if server is alive 
#
is_alive "${SERVER}"
d_stopdevrestart=`date +%s`

if [[ $? == 1 ]]; then
    echo "FAILED:timeout occured retriving status from server"
    exit 1
fi

#
# server should be alive, but might need some time
# to be fully up. verify_status will keep asking for target status 
# upto a timeout
#
verify_status "${MAINTENANCE}"

#
# if server is backup check status one more time
#
cmd="ssh ${SERVER} status imaserver"
reply=`$cmd`


if [[ "$reply" =~ "${MAINTENANCE}" ]]; then
    if [[ "$REQMODE" == "MAINTENANCE" ]]; then
	#
	# We are in maintenane and requested maintanence. we're done
	#
	echo "already in requested maintenance after 'device restart'"
    else 
	#
	# lets change runmode of server to production
	#
	echo "moving server from maintenance to production"
    	d_productionstart=`date +%s`
    	ssh ${SERVER} imaserver runmode production
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	ssh ${SERVER} imaserver start
    	d_productiondone=`date +%s`
        d_done=`date +%s`

        verify_status "${STANDBY}" "${RUNNING}" "${STORESTARTING}"
	
	if [ $? == 1 ]; then
	    #
	    # we failed lets exit
	    #
	    echo "Couldnt reach expected status within timeout"
	    exit 1
	fi

	
     	let d_total=${d_done}-${d_start}
    	let d_totalproduction=${d_productiondone}-${d_productionstart}
    	let d_totaldevrestart=${d_stopdevrestart}-${d_startdevrestart}
    	echo "Move to production Total time: ${d_totalproduction} seconds"
    	echo "Device restart Total time: ${d_totaldevrestart} seconds"
    	echo "SUCCESS Total Time: ${d_total} seconds"
    	exit 0
    fi
else
    echo "Server not in running maintenance status after 'device restart'"
    echo "Excpected server to be in maintenance"
    d_stop=`date +%s`
    let d_total=${d_stop}-${d_start}
    let d_totaldevrestart=${d_stopdevrestart}-${d_startdevrestart}
    echo "Device restart Total time: ${d_totaldevrestart} seconds"
    echo "FAILED: Total time ${d_total} seconds"
    exit 1
fi

d_stop=`date +%s`
let d_total=${d_stop}-${d_start}
let d_totaldevrestart=${d_stopdevrestart}-${d_startdevrestart}
# in requested runmode
echo "Device restart Total time: ${d_totaldevrestart} seconds"
echo "SUCCESS: Total time ${d_total} seconds"
