#!/bin/bash -x

# TODO: Need to implement doing things to both appliances, like stopping both appliances, network outages on both appliances ,etc...
# TODO: Need function to check if interfaces are enabled / enable them  before starting test. - partially implemented
# TODO: Need IMM power off test.
# TODO: Need to add messaging to test. - implemented 
# TODO: Delay between tests would be good so that some messaging could get through.

# WHY: ssh -nx  
#  rsh and/or ssh can break a while read loop due to it using stdin. 
#  Put a -n into the ssh line which stops it from trying to use stdin and it fixed the problem.

# echo "ifconfig" | ssh  admin@10.10.10.10 "busybox"
# let iteration=0; while((1)); do ./ha_fail.sh; ((iteration++)); echo current iteration is $iteration; date; done

. ./svt_test.sh

ima1=${1-10.10.10.10}
ima2=${2-10.10.10.10}
delay_before_start=0
delay_between_tests=0
#FIXME : replication and discovery interfaces need to be auto-detected
ima1_replication_interface=ha1
ima2_replication_interface=ha1
#ima1_discovery_interface=ha1
#ima2_discovery_interface=ha1
ima1_discovery_interface=eth7
ima2_discovery_interface=eth7 

#delay_between_tests=2;

ssh_wrapper() {
    echo ssh $@
    ssh $@
    return $?
}

toggle_server_interface (){ 
    local server=$1
    local action=$2
    local interface=$3
    echo ssh_wrapper -nx admin@$server $action ethernet-interface $interface
    ssh_wrapper -nx admin@$server $action ethernet-interface $interface
    echo rc=$?
    if [ "$action" == "disable" ] ; then
        echo "Warning: Do not interrupt until interface is reenabled"
    fi
    echo "Return code rc= $?"
}

enable_interfaces(){
    local rc=0;
    #-------------------------------------
    # TODO Make this better, the next 4 should not be hardcoded.
    #-------------------------------------
    
    while ((1)) ; do
        let rc=0;
        toggle_server_interface $ima1 enable $ima1_replication_interface
        let rc=$rc+$?
        toggle_server_interface $ima1 enable $ima1_discovery_interface
        let rc=$rc+$?
        toggle_server_interface $ima2 enable $ima2_replication_interface
        let rc=$rc+$?
        toggle_server_interface $ima2 enable $ima2_discovery_interface
        let rc=$rc+$?
        if [ "$rc" ==  "0" ] ;then
            break;
        else
            echo "Failed to enable interfaces"
            sleep 60;
        fi
    done


}
    
segfault_server() {
    local server=$1;
    local data;
    local rc;
    if exist_server_core ; then 
        echo "WARNING: Core files detected on one or more appliances. Before injecting segfault. Will not inject a segfault. Please stop and debug"
        return 1;  
    fi
    echo "Inject segfault on imaserver on $server"
    echo "killall -11 imaserver" | ssh_wrapper admin@$server busybox
    echo rc=$?
    echo "Return code rc= $?"

    while((1)) ; do
        echo "Waiting for bt. file to show up"
        data=`ssh_wrapper -nx  admin@$server advanced-pd-options list`
        if echo "$data" |grep bt\. > /dev/null ;then
            echo "core data found is : $data" >> /dev/stderr
            break;
        fi
    done

    echo "Clear the bt file we just created, we don't want it to count as a failure." 
    clear_server_core; #hopefully clear the core we just created
    
    
    return 0;
}
device_restart_server() {
    local server=$1;
    echo "Inject device restart on $server"
    ssh_wrapper -nx admin@$server device restart
    rc=$?
    echo "Return code rc= $?"
   
    echo "Wait for server to stop pinging..." 
    while ping -c 1 $server -w 10; do echo "`date` Server has not yet restarted...";  sleep 1; done

}
kill_server() {
    local server=$1;
    echo ssh_wrapper  -nx admin@$server imaserver stop force
    ssh_wrapper  -nx admin@$server imaserver stop force
    rc=$?
    echo "Return code rc= $?"
}
kill_servers() {
    kill_server $ima1 
    kill_server $ima2 
}

stop_server() {
    local server=$1;
    echo ssh_wrapper  -nx admin@$server imaserver stop
    ssh_wrapper  -nx admin@$server imaserver stop 
    rc=$?
    echo "Return code rc= $?"
}


clear_server_core()  {
    local line
    local data
    local var
    local rc
    for server in $ima1 $ima2 ; do
        data=`ssh_wrapper -nx  admin@$server advanced-pd-options list | awk '{print $1}'`
        for var in $data ;do
            #line=`echo "$var" | grep -oE 'bt.*|.*_core.*' |awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*|.*_core.*' >/dev/null ; then
            #line=`echo "$var" | grep -oE 'bt.*' |awk '{print $1}' `
            line=`echo "$var" | awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*' >/dev/null ; then
                echo ssh_wrapper  -nx admin@$server advanced-pd-options delete $line
                ssh_wrapper  -nx admin@$server advanced-pd-options delete $line
                rc=$?
                echo rc is $rc
            #fi
        done
    done
}
exist_server_core()  {
    local data
    local regex="bt.*"
    data=` ssh_wrapper -nx  admin@$ima1 advanced-pd-options list`
    data+=$'\n'
    data+=` ssh_wrapper -nx  admin@$ima2 advanced-pd-options list`
    #if echo "$data" | grep -E 'bt.*|.*_core.*'  > /dev/null ;then
    if echo "$data" | grep -E 'bt.*'  > /dev/null ;then
        echo "WARNING: core data found is : $data" >> /dev/stderr
    fi
    if [[ $data =~ $regex ]] ; then
        return 0; 
    fi
    return 1;
}
get_server_time()  {
    echo ssh_wrapper -nx  admin@$ima1 datetime get
    ssh_wrapper  -nx admin@$ima1 datetime get
    echo "";
    echo ssh_wrapper  -nx admin@$ima2 datetime get 
    ssh_wrapper -nx  admin@$ima2 datetime get
}

synchronize_server_time()  {
 local setdate=`date "+%Y-%m-%d %H:%M:%S"`
 local pidlist=""
    echo ssh_wrapper -nx  admin@$ima1 datetime set $setdate
    echo ssh_wrapper  -nx admin@$ima2 datetime set $setdate
    ssh_wrapper -nx admin@$ima1 datetime set $setdate &
    pidlist="$! " 
    ssh_wrapper -nx admin@$ima2 datetime set $setdate &
    pidlist+="$! " 
    wait $pidlist
}

get_harole(){
    local server=$1;
    local rc;
    local data;
    data=`run_command_with_timeout "ssh_wrapper -nx  admin@$server imaserver harole " 60`
    if [ "$rc" == "0" ] ;then
        echo "$data"
        return 0;
    else
        echo "WARNING: harole command failed: ssh_wrapper -nx  admin@$server imaserver harole after 60 second timeout"
        return 1;
    fi
    return 0;
}

start_server() {
    local server=$1;
    echo ssh_wrapper  -nx admin@$server imaserver start
    ssh_wrapper  -nx admin@$server imaserver start
    rc=$?
    echo "Return code rc= $?"
}
start_servers() {
    start_server $ima1
    start_server $ima2
}

clear_environment() {
    unset IMA_HA_PRIMARY
    unset IMA_HA_STANDBY
    unset IMA1_HA_STATUS
    unset IMA1_HA_ROLE
    unset IMA2_HA_STATUS
    unset IMA2_HA_ROLE
}

set_environment () {
    local ima_num=${1-""}
    local ima_ip=${2-""}
    local ima_status=${3-""}
    local ima_harole=${4-""}
    local rc=0;

    export ${ima_num}_HA_STATUS=$ima_status
    export ${ima_num}_HA_ROLE=$ima_harole

    if [ "$ima_status" == "Running" ] ;then
        if [ "$IMA_HA_PRIMARY" != "$ima_ip" -a "$IMA_HA_PRIMARY" != "" ] ;then
            echo "WARNING: PRIMARY was already set to $IMA_HA_PRIMARY, and now trying to set it to $ima_ip" >> /dev/stderr
            let rc=$rc+1
        fi
        export IMA_HA_PRIMARY="$ima_ip"
        if [ "$ima_harole" == "PRIMARY" ] ;then
            export IMA_HA_PRIMARY_ROLE="$ima_ip"
        else
            echo "WARNING: Set PRIMARY to $IMA_HA_PRIMARY, but harole is $ima_harole" >> /dev/stderr
            let rc=$rc+1
        fi
    elif [ "$ima_status" == "Standby" ] ;then
        if [ "$IMA_HA_STANDBY" != "$ima_ip" -a  "$IMA_HA_STANDBY" != ""  ] ;then
            echo "WARNING: STANDBY was already set to $IMA_HA_STANDBY, and now trying to set it to $ima_ip" >> /dev/stderr
            let rc=$rc+1
        fi
        export IMA_HA_STANDBY="$ima_ip"
        if [ "$ima_harole" == "STANDBY" ] ;then
            export IMA_HA_STANDBY_ROLE="$ima_ip"
        else
            echo "WARNING: Set STANDBY to $IMA_HA_STANDBY, but harole is $ima_harole" >> /dev/stderr
            let rc=$rc+1
        fi
    fi
    return $rc
}

print_status_output_msg() {
    local msg=${1-""}
    local extramsg=${2-""}
    msg+=$'\n'
    msg+="StateTransition|`date`|IMA1_HA_STATUS|${IMA1_HA_STATUS}|IMA1_HA_ROLE|${IMA1_HA_ROLE}|IMA2_HA_STATUS|${IMA2_HA_STATUS}|IMA2_HA_ROLE|${IMA2_HA_ROLE}|"
    msg+=$'\n'
    if [ "$extramsg" != "" ] ;then
        msg+=$extramsg
        msg+=$'\n'
    fi
    echo "$msg"
}

get_status () {
    local needed_status_1=${1-""}
    local needed_status_count_1=${2-0}
    local needed_status_2=${3-""}
    local needed_status_count_2=${4-0}
    local timeout_cycles=${5-86600};
    local total_status_1=0
    local total_status_2=0
    local s1;
    local s2;
    local h1;
    local h2;
    local msg="";
    local prevmsg="";
    local cycles;

    clear_environment
    
    let cycles=1;
   
    while((1)) ; do
        let total_status_1=0;
        let total_status_2=0;

        if exist_server_core ; then 
            echo "WARNING: Core files detected on one or more appliances. Please stop and debug"
            return 1;  
        fi
    
        msg=$'\n==================================================='
        msg+=$'\nharole of ima1 is : '
        h1=`ssh_wrapper -nx  admin@$ima1 imaserver harole 2>&1`
        msg+="$h1"
        msg+=$'\nstatus of ima1 is : '
        s1=`ssh_wrapper -nx  admin@$ima1 status imaserver | grep "Status = "`
        msg+="$s1"
        msg+=$'\n'
        msg+="needed_status_1 is $needed_status_1"
        msg+=$'\n'
        msg+="needed_status_2 is $needed_status_2"
        msg+=$'\n'
        if [[ "$s1" =~ "$needed_status_1" ]] ;then
            msg+="needed_status_1 is met"
            msg+=$'\n'
            ((total_status_1++));
        elif [[ "$s1" =~ "$needed_status_2" ]] ;then
            msg+="needed_status_2 is met"
            msg+=$'\n'
            ((total_status_2++));
        else
            msg+="s1 matches no needed status"
            msg+=$'\n'
        fi

        set_environment "IMA1" $ima1 `echo "$s1" | grep ^Status | head -1 | awk '{printf("%s", $3);}'` `echo "$h1" | grep ^NewRole | head -1 | awk '{printf("%s", $3);}'`

        msg+=$'\n'
        msg+=$'\nharole of ima2 is : '
        h2=`ssh_wrapper -nx  admin@$ima2 imaserver harole 2>&1`
        msg+="$h2"
        msg+=$'\nstatus of ima2 is : '
        s2=`ssh_wrapper -nx  admin@$ima2 status imaserver  | grep "Status = "`
        msg+="$s2"
        msg+=$'\n'
        msg+="needed_status_1 is $needed_status_1"
        msg+=$'\n'
        msg+="needed_status_2 is $needed_status_2"
        msg+=$'\n'
        if [[ "$s2" =~ "$needed_status_1" ]] ;then
            msg+="needed_status_1 is met"
            msg+=$'\n'
            ((total_status_1++));
        elif [[ "$s2" =~ "$needed_status_2" ]] ;then
            msg+="needed_status_2 is met"
            msg+=$'\n'
            ((total_status_2++));
        else
            msg+="s2 matches no needed status"
            msg+=$'\n'
        fi

        set_environment "IMA2" $ima2 `echo "$s2" | grep ^Status | head -1 | awk '{printf("%s", $3);}'` `echo "$h2" | grep ^NewRole | head -1 | awk '{printf("%s", $3);}'`
    
        msg+=$'\n'
        msg+="total_status_1 = $total_status_1 , total_status_2 = $total_status_2"
        msg+=$'\n'
        msg+="IMA_HA_PRIMARY|$IMA_HA_PRIMARY|IMA_HA_PRIMARY_ROLE|$IMA_HA_PRIMARY_ROLE|IMA_HA_STANDBY|$IMA_HA_STANDBY|IMA_HA_STANDBY_ROLE|$IMA_HA_STANDBY_ROLE"
        msg+=$'\n'
            
        if (($needed_status_count_1>0)); then
            if (($total_status_1==$needed_status_count_1)); then
                if (($needed_status_count_2>0)); then
                    if (($total_status_2==$needed_status_count_2)); then
                        print_status_output_msg "$msg" "Expected status reached on both servers. $ima1 $ima2"
                        break;
                    fi
                elif (($needed_status_count_2==0)); then
                    print_status_output_msg "$msg" "Expected status reached on one server. $ima1 "
                    echo needed_status_count_2 is $needed_status_count_2
                    break;
                fi
            fi 
        else
            print_status_output_msg "$msg" "Expected status reached on zero servers. "
            break;
        fi 
        if [ "$msg" != "$prevmsg" ] ;then
            print_status_output_msg "$msg" 
            prevmsg="$msg"
        else
            echo -n "."
        fi
        if(($cycles>$timeout_cycles)); then
            echo "WARNING:(TIMEOUT) Did not reach expected status. "
            echo "$msg"
            echo "WARNING:(TIMEOUT) Did not reach expected status. Timeout $cycles > $timeout_cycles"
            return 1;             
        fi
        sleep 1;
        let cycles=$cycles+1;
    done

    return 0;
    
}

run_command_with_timeout() {
    local command="$1"
    local timeout=${2-60}
    local running_test;
    local test_exit;
    $command &
    running_test=$!
    echo "Running $command with timeout $timeout, as pid: running_test=$running_test"
    while(($timeout>0)); do
        if ! [ -e /proc/$running_test/cmdline  ] ;then
            echo "INFO: /proc/$running_test/cmdline does not exist anymore" 
            break;
        fi
        let timeout=$timeout-1;
        sleep 1;
    done
    echo "INFO: After breaking from loop, timeout value is now $timeout"
    if (($timeout<=0)); then
        echo "ERROR: TIMEOUT running command $command, kill -9 $running_test" 
        kill -9 $running_test;
        return 1;
    fi
    wait $running_test
    test_exit=$?
    echo "INFO: test exit is $test_exit for $running_test"
    return $test_exit;
}

#----------------------------------------
# Call this function to force the ha pair into a known good state.
#----------------------------------------
ha_pair_up_old() {
    local rc=0;

    enable_interfaces
    let rc=$rc+$?;

    if run_command_with_timeout "get_status "Running" 1 "Standby" 1 5 " 10 ; then
        echo "HA Pair is already up at Running and Standby"
        return 0;
    else
        echo "HA pair - force into known good state"
    fi
    date
    kill_servers
    let rc=$rc+$?;
    get_status "Stopped" 2 
    let rc=$rc+$?;
    start_servers
    let rc=$rc+$?;
    get_status "Running" 1 "Standby" 1
    let rc=$rc+$?;
    return $rc;
}
#----------------------------------------
# Call this function to force the ha pair into a known good state. New as of Mar 29th 2013
# configure HA on A1, configure HA on A2. stop A1.
# run clean_store on A2
# stop A2, start A2, runmode production on A2, stop A2
# then start A1 and A2
#----------------------------------------



ha_pair_up_part1() {
    local rc=0;

    enable_interfaces
    let rc=$rc+$?;

    if run_command_with_timeout "get_status "Running" 1 "Standby" 1 60 " 120 ; then
        echo "HA Pair is already up at Running and Standby"
        return 0;
    else
        echo "HA pair - force into known good state"
        return 1;
    fi
    return 1;
}

ha_pair_up_part2() {
    echo "=================================="
    echo "=================================="
    echo "WARNING: HA PAIR CLEANUP is needed"
    echo "=================================="
    echo "=================================="
    date
    while((1)); do
        kill_servers
        let rc=$rc+$?;
        if ! get_status "Stopped" 2 "" 0 10 ; then
            echo "Still waiting for both of the servers to go into Stopped" 
        else
            break;
        fi
    done
}
ha_pair_up_part3() {
    # expect both servers to be in Stopped now.
    start_servers
    let rc=$rc+$?;
    while((1)); do
        kill_server $ima1
        if ! get_status "Stopped" 1 "" 0 10 ; then
            echo "Still waiting for one of the servers to go into Stopped" 
        else
            break;
        fi
    done
    ssh_wrapper -nx  admin@$ima2 "imaserver runmode clean_store"
    stop_server $ima2
    if ! get_status "Stopped" 2 "" 0 60 ; then
        kill_server $ima2 
        get_status "Stopped" 2
    fi
    start_server $ima2
    get_status "Maintenance" 1
    ssh_wrapper -nx  admin@$ima2 "imaserver runmode production"
    stop_server $ima2
    get_status "Stopped" 2
    start_server $ima1
    start_server $ima2
    get_status "Running" 1 "Standby" 1
    let rc=$rc+$?;
    return $rc;
}

ha_pair_up() {
    local rc; 
    ha_pair_up_part1
    rc=$?
    if [ "$rc" != "0" ] ;then
        ha_pair_up_part2
        ha_pair_up_part3
    fi  

}

get_primary(){
    get_status "Running" 1  > /dev/null
    if [ "$?" != "0" ] ;then
        return 1; 
    fi
    echo $IMA_HA_PRIMARY
    return 0;
}

get_standby(){
    get_status "Standby" 1 > /dev/null
    if [ "$?" != "0" ] ;then
        return 1; 
    fi
    echo $IMA_HA_STANDBY
    return 0;
}


svt_ha_test_X_disrupt_server_on_Y_check_result(){

    local old_primary_ip=${1}  ; # Y
    local target=${2}  ; # Y
    local tmp_ip;

    tmp_ip=`get_primary`
    if [ "$?" != "0" ] ;then
        echo "ERROR: Could not detect primary"
        return 1; 
    fi
    if [ "$target" == "PRIMARY" ] ;then
        if [ "$tmp_ip" == "$old_primary_ip" ]; then 
            echo "ERROR: Primary ip after test did not change: $tmp_ip == $old_primary_ip"
            echo "FAILURE: Test failed."
            return 1;
        fi
    elif [ "$target" == "STANDBY" -o "$target" == "SPLITBRAIN" ] ;then
        if [ "$tmp_ip" != "$old_primary_ip" ]; then 
            echo "ERROR: Primary ip after test changed: $tmp_ip != $old_primary_ip"
            echo "FAILURE: Test failed."
            return 1;
        fi
    else
        echo "ERROR: Unexpected exit location C"
        echo "FAILURE: Test failed. Internal test bug."
        return 1;
    fi
    echo "SUCCESS: Test Ok."
    return 0;
}

#--------------------------------------------
# "_v2" state transition pattern expected for device restart and segfault injection.
#--------------------------------------------
svt_ha_test_X_disrupt_server_on_Y_finish_test_v2(){
#used for SIGSEGV, DEVICE to finish the test
    local primary_ip=${1}  
    local target=${2}  ; 
    local method=${3}  ; 
    local ret;

    echo "---------------------------"
    echo "`date` step 2.1a for method $method, expect state to return to Running Standby"
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1
    let ret=$ret+$?
    echo "`date` step 2.2 for method: $method, verfiy final result."
    echo "---------------------------"
    svt_ha_test_X_disrupt_server_on_Y_check_result $primary_ip $target
    let ret=$ret+$?
    return $ret;

}


#---------------------------------------
#
# Procedure:
# 1. HA pair up and running in "Running" "Standby"
# 2. Inject failure X on Y primary or standby w/ supported method :
#        X      - failures  description
#   -----------------------------------------
#       KILL    - imaserver stop force
#       STOP    - imaserver stop 
#    SIGSEGV    - kill -11 on imaserver
# NETWORK- interrupt the network interface network_interface for time specified by network_outage_time
# 3. Expect status to reach "Stopped" and "Running"
# 4. Start imaserver again on new standby
# 5. Expect status to return "Standby" and "Running"
# 6. Expect primary ip to change to what was previously the secondary ip
#
# Expecation: If I kill the server on standby the primary should stay running
#
#---------------------------------------
svt_ha_test_X_disrupt_server_on_Y(){
    local method=${1-"KILL"}     ; # X
    local target=${2-"PRIMARY"}  ; # Y
    local network_interface=${3-""}    ; # Required only if method is "NETWORK"
    local network_outage_time=${4-60}    
    local outage_time=${3-60}    
    local outage_time=${3-60}    
    local ret=0;
    
    echo "---------------------------"
    echo "`date` step 1 -setup for test"
    echo "---------------------------"
    local standby_ip=`get_standby`
    if [ "$?" != "0" ] ;then
        echo "ERROR: Could not detect standby"
        return 1; 
    fi
    echo "---------------------------"
    echo "`date` step 1 b continued..."
    echo "---------------------------"
    local primary_ip=`get_primary`
    if [ "$?" != "0" ] ;then
        echo "ERROR: Could not detect primary"
        return 1; 
    fi
    local tmp_ip;
    local target_ip;
    if [ "$target" == "PRIMARY" ] ;then
        target_ip=$primary_ip;
    elif [ "$target" == "STANDBY" ] ;then
        target_ip=$standby_ip;
    elif [ "$target" == "N/A" ] ;then
        echo "INFO: target is not applicable for this test... example (split brain test)"
    else       
        echo "ERROR: unsupported target $target"
        return 1; 
    fi

    echo "---------------------------"
    echo "`date` step 1 c continued.. verify system is in Running / Standby state before starting test."
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1

    if [ "$method" == "KILL" ] ;then
        echo "---------------------------"
        echo "`date` step 2 inject KILL server $target_ip"
        echo "---------------------------"
        kill_server $target_ip
    elif [ "$method" == "STOP" ] ;then
        echo "---------------------------"
        echo "`date` step 2 inject STOP server $target_ip"
        echo "---------------------------"
        stop_server $target_ip
    elif [ "$method" == "SIGSEGV" ] ;then
        echo "---------------------------"
        echo "`date` step 2 inject segfault on imaserver running on server $target_ip"
        echo "---------------------------"
        segfault_server $target_ip
        let ret=$ret+$?
        svt_ha_test_X_disrupt_server_on_Y_finish_test_v2 $primary_ip $target $method
        let ret=$ret+$?
        return $ret
    elif [ "$method" == "DEVICE" ] ;then
        echo "---------------------------"
        echo "`date` step 2 inject device restart on appliance $target_ip"
        echo "---------------------------"
        device_restart_server $target_ip
        svt_ha_test_X_disrupt_server_on_Y_finish_test_v2 $primary_ip $target $method
        return $?
    elif [ "$method" == "SPLITBRAIN" ] ;then
        echo "---------------------------"
        echo "`date` step 2b force split brain by disabling both replication and discovery interface - start split brain in random order"
        echo "---------------------------"
        toggle_server_interface $ima1 disable $ima1_replication_interface
        toggle_server_interface $ima2 disable $ima2_replication_interface
        toggle_server_interface $ima1 disable $ima1_discovery_interface
        toggle_server_interface $ima2 disable $ima2_discovery_interface 

        echo "---------------------------"
        echo "`date` step 2c expect Running Running (split brain) state to occur"
        echo "---------------------------"
        get_status "Running" 2
        let ret=$ret+$?

        sleep $network_outage_time

        echo "---------------------------"
        echo "`date` step 2d Renable network"
        echo "---------------------------"
        toggle_server_interface $ima1 enable $ima1_replication_interface
        toggle_server_interface $ima2 enable $ima2_replication_interface
        toggle_server_interface $ima1 enable $ima1_discovery_interface
        toggle_server_interface $ima2 enable $ima2_discovery_interface
        
        echo "---------------------------"
        echo "`date` step 2e expect State to return to Running Standby"
        echo "---------------------------"
        get_status "Running" 1 "Standby" 1
        let ret=$ret+$?

        #svt_ha_test_X_disrupt_server_on_Y_check_result $primary_ip "SPLITBRAIN"
        #let ret=$ret+$?
        #if [ "$ret" == "0" ]  ; then
        #    echo "SUCCESS: Test Ok."
        #else
        #    echo "FAILURE: Test Failed."
        #fi
        return $ret;
        
    elif [ "$method" == "NETWORK" ] ;then
        echo "---------------------------"
        echo "`date` step 2 inject NETWORK outage of time $network_outage_time on network_interface $network_interface"
        echo "---------------------------"
        if [ "$network_interface" == "" ] ; then
            echo "ERROR: network_interface $network_interface"
            return 1; 
        fi
        toggle_server_interface $target_ip disable $network_interface
        sleep $network_outage_time
        toggle_server_interface $target_ip enable $network_interface
        echo "---------------------------"
        echo "`date` step 2 b. network outage must reach Running, Maintenance, as long as the outage was longer than the discovery timeout."
        echo "         or... if it was the network outage was less than the discovery timeout and it was not on a replication interface"
        echo "         the server may remain in Running,Standby . If that is the case it must stay Running standby for at least 60 seconds "
        echo "---------------------------"
        if ! get_status "Running" 1 "Maintenance" 1  60 ; then
            get_status "Running" 1 "Standby" 1 
            echo "There was no change in state, the outage must have been on a discovery interface  and outage lasted < heartbeat timout."
            echo "TODO: Verify that if needed"
            echo "SUCCESS: Test Ok."
            return 0;
        fi
        echo "---------------------------"
        echo "`date` step 2 c. stop backup "
        echo "---------------------------"
        #kill_server $target_ip
        kill_server $standby_ip
        get_status "Running" 1 "Stopped" 1
        echo "---------------------------"
        echo "`date` step 2 d. start backup "
        echo "---------------------------"
        start_server $standby_ip
        echo "---------------------------"
        echo "`date` step 2 e. get Running Standby status "
        echo "---------------------------"
        get_status "Running" 1 "Standby" 1
        let ret=$ret+$?
        if [ "$ret" == "0" ]  ; then
            echo "SUCCESS: Test Ok."
        else
            echo "FAILURE: Test Failed."
        fi
        return $ret;
    else
        echo "ERROR: unsupported method $method"
        return 1; 
    fi
    echo "---------------------------"
    echo "`date` step 3 - expect ha pair to go to the Running / Stopped state"
    echo "---------------------------"
    get_status "Running" 1 "Stopped" 1
    let ret=$ret+$?
    echo "---------------------------"
    echo "`date` step 4 - imaserver start on $target_ip"
    echo "---------------------------"
    start_server $target_ip
    echo "---------------------------"
    echo "`date` step 5 - expect ha pair to go to the Running / Standby state"
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1
    let ret=$ret+$?
    echo "---------------------------"
    echo "`date` step 6 - Complete - do final checks for errors. "
    echo "---------------------------"
    svt_ha_test_X_disrupt_server_on_Y_check_result $primary_ip $target
    let ret=$ret+$?
    return $ret;
}

svt_ha_print_tests(){
    for method in KILL STOP SIGSEGV DEVICE ; do 
        for target in PRIMARY STANDBY ; do
            if [ "$method" == "NETWORK" ] ; then
                echo svt_ha_test_X_disrupt_server_on_Y $method $target $network_interface $network_outage
            else
                echo svt_ha_test_X_disrupt_server_on_Y $method $target 
            fi
        done
    done
    #for network_outage in 1 2 3 4 5 8 9 10 11 12 15 20 25 30 60 100 1000 ; do 
    for network_outage in 1 2 3 4 5 8 9 10 11 12 15 20 25 30 60 100 ; do 
    #for network_outage in 1 2 3 4 5 8 9 10 11 12 15 20 25 30 60 ; do 
        echo svt_ha_test_X_disrupt_server_on_Y SPLITBRAIN "N/A" "N/A" $network_outage
        #for network_interface in ha1 ha2 ; do 
        #for network_interface in ha0 ha1 ; do 
        for network_interface in $ima1_replication_interface $ima1_discovery_interface ; do
    #        for method in KILL SIGSEGV NETWORK ; do     # currently SIGSEGV has defect 23786
    #        for method in KILL SIGSEGV NETWORK DEVICE POWER ; do     # currently SIGSEGV has defect 23786
            #for method in KILL SIGSEGV NETWORK DEVICE WER ; do     # currently SIGSEGV has defect 23786
            #for method in KILL NETWORK ; do 
            for method in NETWORK ; do 
                for target in PRIMARY STANDBY ; do
                    if [ "$method" == "NETWORK" ] ; then
                        echo svt_ha_test_X_disrupt_server_on_Y $method $target $network_interface $network_outage
                    else
                        echo svt_ha_test_X_disrupt_server_on_Y $method $target 
                    fi
                done
            done
        done
    done
}
svt_ha_print_tests_loop(){
    local iteration=1000
    while (($iteration>0)); do
        #svt_ha_print_tests | sort -R | grep KILL | grep STANDBY
        svt_ha_print_tests | sort -R | grep KILL  
        #svt_ha_print_tests | sort -R | grep -v BRAIN
        #svt_ha_print_tests  | grep -E 'NETWORK PRIMARY|DEVICE STANDBY' | grep -E 'eth6|DEVICE' | grep -E '10|DEVICE'
        #svt_ha_print_tests  | grep -E 'NETWORK PRIMARY' | grep -E 'eth6|DEVICE' | grep -E '10|DEVICE'
        #svt_ha_print_tests  | grep BRAIN
        #svt_ha_print_tests  | grep -E 'NETWORK' | grep -E '15' 
        let iteration=$iteration-1;
    done  | head -n 15
}

svt_ha_cleanup_sighup(){
echo svt_ha_cleanup sighup invoked for PID: $$
kill -1 $$
}
svt_ha_cleanup_sigint(){
echo svt_ha_cleanup sigint invoked for PID: $$ - exitting
kill -2 $$
exit 1;
}
svt_ha_cleanup_sigterm(){
echo svt_ha_cleanup sigterm invoked for PID: $$ - exitting
kill -15 $$
exit 1;
}
svt_ha_cleanup_sigpipe(){
echo svt_ha_cleanup sigpipe invoked for PID: $$
kill -13 $$
}
svt_ha_cleanup_sigchld(){
#echo svt_ha_cleanup sigchld invoked for PID: $$
kill -17 $$
}
svt_ha_set_traps(){
    trap svt_ha_cleanup_sighup SIGHUP 
    trap svt_ha_cleanup_sigint SIGINT
    trap svt_ha_cleanup_sigterm SIGTERM
    trap svt_ha_cleanup_sigchld SIGCHLD
    trap svt_ha_cleanup_sigpipe SIGPIPE
}
svt_ha_run_single_test(){
    local cmd=${1-""};
    local count=${2-0};
    local total_time
    local start_time
    local end_time
    local ret=0;
    local rc=0;

    if [ "$cmd" == "" ] ;then
        echo "ERROR: no command specified as an input"
        return 1;
    fi
    
    echo "-----------------------------------------------" 
    echo " Started test for PID: $$ count = $count       "
    echo "-----------------------------------------------"

    svt_ha_set_traps

    get_server_time 

    #synchronize_server_time

    if exist_server_core ; then 
        echo "WARNING: Core files detected on one or more appliances. Please stop and debug"
        return 1;  
    fi

    echo "-----------------------------------------------"
    echo "`date` Running test $count: $cmd  as PID: $$   " 
    echo "-----------------------------------------------" 
    echo "`date` Verify that HA Pair is in a stable state" 
    echo "-----------------------------------------------" 
    ha_pair_up 
    rc=$?
    let ret=$ret+$rc
    echo "-----------------------------------------------" 
    if [ "$rc" == "0" ] ;then
        echo "`date` SUCCESS: HA Pair: ret = $rc is in a stable state" 
    else
        echo "----------------------------------------------------------"
        echo "`date` ERROR : HA Pair: ret = $rc is NOT in a stable state" 
        echo "----------------------------------------------------------"
        return 99;
    fi
    echo "-----------------------------------------------" 
    echo "Complete - Verified that HA Pair is in a stable state"  
    start_time=`date +%s`
    echo "-----------------------------------------------" 
    echo "`date` Start test. Current test is $cmd        "
    echo "-----------------------------------------------"
    run_command_with_timeout " $cmd  " 86600  
    test_exit=$?
    let ret=$ret+$test_exit

    end_time=`date +%s`
    let total_time=($end_time-$start_time);
    echo "-----------------------------------------------" 
    echo "`date` RESULT: $ret TIME: $total_time TEST: $count $cmd " 
    echo "-----------------------------------------------" 
    echo " Finished test for PID: $$ count= $count       "
    echo "-----------------------------------------------"

    return $ret;
}

svt_ha_run_all_tests(){
    local line;
    local ret;
    local count=0;
    local collect_data=0;

    rm -rf log.ha.*

    echo "-----------------------------------------------" | tee -a log.ha.out 
    echo "`date` Beginning as PID: $$                    " | tee -a log.ha.out
    echo "-----------------------------------------------" | tee -a log.ha.out

    svt_ha_set_traps

    clear_server_core; # start out fresh

    enable_interfaces

    svt_ha_print_tests_loop | while read line; do

        {
            collect_data=0;
            svt_ha_run_single_test "$line" $count 
            ret=$?;
            if [ "$ret" == "99" ] ; then
                echo "-------------------------------------------------" 
                echo " 99 return code found - exit the test loop" 
                echo "-------------------------------------------------" 
            fi
            if [ "$ret" != "0" ] ; then
                echo "Do must gather because ret == $ret"
                collect_data=1;
            fi
            if exist_server_core ; then
                echo "Do must gather because core files exist on the server." 
                collect_data=1;
            fi
                    
            if [ "$collect_data" == "1" ]  ; then 
                collect_data=0;
                svt_ha_must_gather $ima1
                svt_ha_must_gather $ima2
                clear_server_core; # clean up
            fi
        } | tee -a log.ha.out log.ha.$count


        echo "-----------------------------------------------" | tee -a log.ha.out 
        echo " Completed test for PID: $$ count= $count      " | tee -a log.ha.out 
        echo "-----------------------------------------------" | tee -a log.ha.out

        echo "-----------------------------------------------" | tee -a log.ha.out 
        echo " Sleep for $delay_between_tests ..............." | tee -a log.ha.out 
        echo "-----------------------------------------------" | tee -a log.ha.out
        sleep $delay_between_tests
        echo "-----------------------------------------------" | tee -a log.ha.out 
        echo " continuing after delay...................     " | tee -a log.ha.out 
        echo "-----------------------------------------------" | tee -a log.ha.out
        let count=$count+1; 
    done

    echo "-----------------------------------------------" | tee -a log.ha.out 
    echo " Exitting as PID: $$ count= $count             " | tee -a log.ha.out 
    echo "-----------------------------------------------" | tee -a log.ha.out

}

svt_ha_run_all_tests_2_hour(){
   run_command_with_timeout svt_ha_run_all_tests 7200  
   #run_command_with_timeout svt_ha_run_all_tests 10
}
svt_ha_run_all_tests_8_hour(){
   run_command_with_timeout svt_ha_run_all_tests 28800  
   #run_command_with_timeout svt_ha_run_all_tests 10
}

svt_ha_run_all_tests_36_hour(){
   run_command_with_timeout svt_ha_run_all_tests 129600  
   #run_command_with_timeout svt_ha_run_all_tests 10
}
svt_ha_run_all_tests_48_hour(){
   run_command_with_timeout svt_ha_run_all_tests 172800  
   #run_command_with_timeout svt_ha_run_all_tests 10
}


monitor_disk_usage_loop(){
    local expected_usage=${1-""}
    local primary_ip="";
    local data=""
    local regex1="(DiskUsedPercent = )([0-9]+)(.*)$"

    local lastvalue=0
    local curvalue=0

    if ! [ -n "$expected_usage" ] ; then
        echo "`date` ERROR: monitor_disk_usage_loop : user must supply the expected_usage "
        return 1;
    fi

    echo "---------------------------------------------------------------------------"
    echo "`date` monitor_disk_usage_loop : Monitoring until DiskUsedPercent >= $expected_usage "
    echo "---------------------------------------------------------------------------"

    while((1)); do
        primary_ip=`get_primary`
        if [ "$?" != "0" ] ;then
            #echo "`date` WARNING: Could not detect primary. sleep 60 seconds before continuing."
            echo -n "a" ; # brief indication that there is a WARNING at point a
            sync;
            sleep 60;
            continue;
        fi
        data=`ssh_wrapper -nx  admin@$primary_ip "imaserver stat Store" 2>&1 `:
        if [[ "$data" =~ $regex1 ]] ;then
            curvalue=${BASH_REMATCH[2]}
            if (($curvalue>=$expected_usage)) ;then
                echo " $curvalue - expected value found "; # we have attained disk usage >= curvalue 
                break;
            elif [ "$curvalue" != "$lastvalue" ] ;then
                lastvalue=$curvalue;
                echo -n " $curvalue "; # we have attained disk usage == curvalue but it is less than expected
            else
                echo -n "." 
            fi
        else
            echo -n "b" ; # brief indication that there is a WARNING at point b , probably the primary is not in a state that the command is executing.
            echo " $data "
        fi
        sync;
        sleep 1;
    done

    echo "---------------------------------------------------------------------------"
    echo "`date`monitor_disk_usage_loop : Current usage is: $curvalue"
    echo "---------------------------------------------------------------------------"
        
         
}

SVT_HA_RANDOM_TEST_FILE=.tmp.svt_random_ha.test

svt_ha_gen_random_test () {
    local counter=$1;
    {  
     svt_ha_print_tests |grep -v NETWORK | grep -v SPLITBRAIN |  grep -v SIGSEGV | sort -R | grep PRIMARY | head -5 ;
     svt_ha_print_tests |grep -E 'NETWORK' | sort -R | grep PRIMARY  | head -5
     svt_ha_print_tests |grep -v NETWORK | grep -v SPLITBRAIN |  grep -v SIGSEGV | sort -R | grep STANDBY | head -1 ;
     svt_ha_print_tests |grep -E 'NETWORK' | sort -R | grep STANDBY  | head -1
     #svt_ha_print_tests |grep -v NETWORK | grep -v SPLITBRAIN |  grep -v SIGSEGV | sort -R | grep PRIMARY | head -5 ;
     #svt_ha_print_tests |grep -E 'NETWORK|SPLITBRAIN' | sort -R | grep PRIMARY  | head -5
     #svt_ha_print_tests |grep -v NETWORK | grep -v SPLITBRAIN |  grep -v SIGSEGV | sort -R | grep STANDBY | head -1 ;
     #svt_ha_print_tests |grep -E 'NETWORK|SPLITBRAIN' | sort -R | grep STANDBY  | head -1
     } | sort -R | head -1 | while read a_ha_test; do
        echo "Adding $a_ha_test to $SVT_HA_RANDOM_TEST_FILE.$counter " >> /dev/stderr
        echo "$a_ha_test" > $SVT_HA_RANDOM_TEST_FILE.$counter
    done
    sync;
}

svt_ha_get_random_test () {
    local counter=$1;
    if ! [ -e $SVT_HA_RANDOM_TEST_FILE.$counter ] ;then
        echo "ERROR: $0 : $LINENO - non existent $SVT_HA_RANDOM_TEST_FILE.$counter. File should have already ben generated." >> /dev/stderr ; exit 1;
    fi
    cat $SVT_HA_RANDOM_TEST_FILE.$counter | head -1
}

svt_ha_run_random_test () {
    local counter=$1
    local command=`svt_ha_get_random_test $counter`
    svt_ha_run_single_test "$command" $counter
    return $?
}
svt_ha_clr_random_test () {
    rm -rf $SVT_HA_RANDOM_TEST_FILE.*
}

