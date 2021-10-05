#!/bin/bash

# WHY: ssh -nx  
#  rsh and/or ssh can break a while read loop due to it using stdin. 
#  Put a -n into the ssh line which stops it from trying to use stdin and it fixed the problem.

# echo "ifconfig" | ssh -nx  admin@10.10.10.10 "busybox"
# let iteration=0; while((1)); do ./ha_fail.sh; ((iteration++)); echo current iteration is $iteration; date; done

ima1=${1-10.10.10.10}
ima2=${2-10.10.10.10}

toggle_server_interface (){ 
    local server=$1
    local action=$2
    local interface=$3
    echo ssh -nx admin@$server $action ethernet-interface $interface
    ssh -nx admin@$server $action ethernet-interface $interface
    echo rc=$?
    if [ "$action" == "disable" ] ; then
        echo "Warning: Do not interrupt until interface is reenabled"
    fi
    echo "Return code rc= $?"
}
segfault_server() {
    local server=$1;
    echo "Inject segfault on imaserver on $server"
    echo "killall -11 imaserver" | ssh -nx  admin@$server busybox
    echo rc=$?
    echo "Return code rc= $?"
}
device_restart_server() {
    local server=$1;
    echo "Inject device restart on $server"
    ssh -nx admin@$server device restart
    rc=$?
    echo "Return code rc= $?"
   
    echo "Wait for server to stop pinging..." 
    while ping -c 1 $server -w 10; do echo "`date` Server has not yet restarted...";  sleep 1; done

}
kill_server() {
    local server=$1;
    echo ssh  -nx admin@$server imaserver stop force
    ssh  -nx admin@$server imaserver stop force
    rc=$?
    echo "Return code rc= $?"
}
kill_servers() {
    kill_server $ima1 
    kill_server $ima2 
}

X_server_core()  {
    local operation=${1-"CLEAR"}
    local dir=${2-""}
    local line
    local data
    local var
    local rc
    for server in $ima1 $ima2 ; do
        data=`ssh -nx  admin@$server advanced-pd-options list`
        for var in $data ;do
            line=`echo "$var" | grep -oE 'bt.*' |awk '{print $1}' `
            if echo "$line" |grep bt\. >/dev/null ; then
                if [ "$operation" == "CLEAR" ] ; then
                    echo ssh  -nx admin@$server advanced-pd-options delete $line
                    ssh  -nx admin@$server advanced-pd-options delete $line
                    rc=$?
                else if [ "$operation" == "DOWNLOAD" ] ; then
                    echo "download the core files" 
                else
                    echo "ERROR: invalid operation . Internal code bug - please fix."
                fi 
                echo rc is $rc
            fi
        done
    done
}

clear_server_core()  {
    X_server_core CLEAR
}
download_server_core () {
    local todir=${1-""}
    X_server_core DOWNLOAD $todir
}

exist_server_core()  {
    local data
    local regex="bt.*"
    data=` ssh -nx  admin@$ima1 advanced-pd-options list`
    data+=$'\n'
    data+=` ssh -nx  admin@$ima2 advanced-pd-options list`
    if echo "$data" |grep bt\. > /dev/null ;then
        echo "core data found is : $data" >> /dev/stderr
    fi
    if [[ $data =~ $regex ]] ; then
        return 0; 
    fi
    return 1;
}
get_server_time()  {
    echo ssh -nx  admin@$ima1 datetime get
    ssh  -nx admin@$ima1 datetime get
    echo "";
    echo ssh  -nx admin@$ima2 datetime get 
    ssh -nx  admin@$ima2 datetime get
}

synchronize_server_time()  {
 local setdate=`date "+%Y-%m-%d %H:%M:%S"`
    echo ssh -nx  admin@$ima1 datetime set $setdate
    ssh  -nx admin@$ima1 datetime set $setdate
    echo ssh  -nx admin@$ima2 datetime set $setdate
    ssh -nx  admin@$ima2 datetime set $setdate
}

get_harole(){
    local server=$1;
    local rc;
    local data;

    data=`run_command_with_timeout "ssh -nx  admin@$server imaserver harole " 5`
    if [ "$rc" == "0" ] ;then
        echo "$data"
        return 0;
    else
        echo "WARNING: harole command failed: ssh -nx  admin@$server imaserver harole after 5 second timeout"
        return 1;
    fi
    return 0;
}

start_server() {
    local server=$1;
    echo ssh  -nx admin@$server imaserver start
    ssh  -nx admin@$server imaserver start
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
}

set_environment () {
    local ima_ip=${1-""}
    local ima_status=${2-""}
    local ima_harole=${3-""}
    local rc=0;

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


get_status () {
    local needed_status_1=${1-""}
    local needed_status_count_1=${2-0}
    local needed_status_2=${3-""}
    local needed_status_count_2=${4-0}
    local timeout_cycles=${5-3600};
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
        h1=`ssh -nx  admin@$ima1 imaserver harole`
        msg+="$h1"
        msg+=$'\nstatus of ima1 is : '
        s1=`ssh -nx  admin@$ima1 status imaserver`
        msg+="$s1"
        if [[ "$s1" =~ "$needed_status_1" ]] ;then
            ((total_status_1++));
        elif [[ "$s1" =~ "$needed_status_2" ]] ;then
            ((total_status_2++));
        fi

        set_environment $ima1 `echo $s1 | head -1 | awk '{printf("%s", $3);}'` `echo $h1 | grep ^NewRole | head -1 | awk '{printf("%s", $3);}'`

        msg+=$'\n'
        msg+=$'\nharole of ima2 is : '
        h2=`ssh -nx  admin@$ima2 imaserver harole`
        msg+="$h2"
        msg+=$'\nstatus of ima2 is : '
        s2=`ssh -nx  admin@$ima2 status imaserver`
        msg+="$s2"
        if [[ "$s2" =~ "$needed_status_1" ]] ;then
            ((total_status_1++));
        elif [[ "$s2" =~ "$needed_status_2" ]] ;then
            ((total_status_2++));
        fi

        set_environment $ima2 `echo $s2 | head -1 | awk '{printf("%s", $3);}'` `echo $h2 | grep ^NewRole | head -1 | awk '{printf("%s", $3);}'`
    
        msg+=$'\n'
        msg+="total_status_1 = $total_status_1 , total_status_2 = $total_status_2"
        msg+=$'\n'
        msg+="IMA_HA_PRIMARY|$IMA_HA_PRIMARY|IMA_HA_PRIMARY_ROLE|$IMA_HA_PRIMARY_ROLE|IMA_HA_STANDBY|$IMA_HA_STANDBY|IMA_HA_STANDBY_ROLE|$IMA_HA_STANDBY_ROLE"
        msg+=$'\n'
            
        if (($needed_status_count_1>0)); then
            if (($total_status_1==$needed_status_count_1)); then
                if (($needed_status_count_2>0)); then
                    if (($total_status_2==$needed_status_count_2)); then
                        msg+="Expected status reached on both servers. $ima1 $ima2"
                        echo "$msg"
                        break;
                    fi
                elif (($needed_status_count_2==0)); then
                    msg+="Expected status reached on one server. $ima1 "
                    echo needed_status_count_2 is $needed_status_count_2
                    echo "$msg"
                    break;
                fi
            fi 
        else
            msg+="Expected status reached on zero servers. "
            echo "$msg"
            break;
        fi 
        if [ "$msg" != "$prevmsg" ] ;then
            echo "$msg"
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
    echo "running $command" >> /dev/stderr
    $command &
    running_test=$!
    while(($timeout>0)); do
        if ! [ -e /proc/$running_test/cmdline  ] ;then
            break;
        fi
        let timeout=$timeout-1;
        #echo "Timeout is $timeout"
        sleep 1;
    done
    if (($timeout<=0)); then
        echo "TIMEOUT  running command $command, kill -9 $running_test" >> /dev/stderr
        kill -9 $running_test;
        return 1;
    fi
    wait $running_test
    test_exit=$?
echo 2 - test exit is $test_exit for $running_test >> /dev/stderr
    return $test_exit;
}

#----------------------------------------
# Call this function to force the ha pair into a known good state.
#----------------------------------------
ha_pair_up() {
    if run_command_with_timeout "get_status "Running" 1 "Standby" 1 5 " 10 ; then
        echo "HA Pair is already up at Running and Standby"
        return 0;
    else
        echo "HA pair - force into known good state"
    fi
    date
    kill_servers
    get_status "Stopped" 2 
    start_servers
    get_status "Running" 1 "Standby" 1
}

get_primary(){
    get_status "Running" 1  > /dev/null
    echo $IMA_HA_PRIMARY
}

get_standby(){
    get_status "Standby" 1 > /dev/null
    echo $IMA_HA_STANDBY
}


do_test_X_disrupt_server_on_Y_check_result(){

    local old_primary_ip=${1}  ; # Y
    local target=${2}  ; # Y
    local tmp_ip;

    tmp_ip=`get_primary`
    if [ "$target" == "PRIMARY" ] ;then
        if [ "$tmp_ip" == "$old_primary_ip" ]; then 
            echo "ERROR: Primary ip after test did not change: $tmp_ip == $old_primary_ip"
            echo "FAILURE: Test failed."
            return 1;
        fi
    elif [ "$target" == "STANDBY" ] ;then
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
do_test_X_disrupt_server_on_Y_finish_test_v2(){
#used for SIGSEGV, DEVICE to finish the test
    local primary_ip=${1}  
    local target=${2}  ; 
    local method=${3}  ; 
    local ret;

    echo "---------------------------"
    echo "step 2.1a for method $method, expect state to return to Running Standby"
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1
    let ret=$ret+$?
    echo "step 2.2 for method: $method, verfiy final result."
    echo "---------------------------"
    do_test_X_disrupt_server_on_Y_check_result $primary_ip $target
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
do_test_X_disrupt_server_on_Y(){
    local method=${1-"KILL"}     ; # X
    local target=${2-"PRIMARY"}  ; # Y
    local network_interface=${3-""}    ; # Required only if method is "NETWORK"
    local network_outage_time=${4-60}    
    local outage_time=${3-60}    
    local outage_time=${3-60}    
    local ret=0;
     
    
    echo "---------------------------"
    echo "step 1"
    echo "---------------------------"
    local standby_ip=`get_standby`
    echo "---------------------------"
    echo "step 1 b continued..."
    echo "---------------------------"
    local primary_ip=`get_primary`
    local tmp_ip;
    local target_ip;
    if [ "$target" == "PRIMARY" ] ;then
        target_ip=$primary_ip;
    elif [ "$target" == "STANDBY" ] ;then
        target_ip=$standby_ip;
    else       
        echo "ERROR: unsupported target $target"
        return 1; 
    fi

    echo "---------------------------"
    echo "step 1 c continued.."
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1
    echo "---------------------------"
    echo "step 2"
    echo "---------------------------"
    if [ "$method" == "KILL" ] ;then
        kill_server $target_ip
    elif [ "$method" == "SIGSEGV" ] ;then
        segfault_server $target_ip
        do_test_X_disrupt_server_on_Y_finish_test_v2 $primary_ip $target $method
        return $?
    elif [ "$method" == "DEVICE" ] ;then
        device_restart_server $target_ip
        do_test_X_disrupt_server_on_Y_finish_test_v2 $primary_ip $target $method
        return $?
    elif [ "$method" == "NETWORK" ] ;then
        if [ "$network_interface" == "" ] ; then
            echo "ERROR: network_interface $network_interface"
            return 1; 
        fi
        toggle_server_interface $target_ip disable $network_interface
        sleep $network_outage_time
        toggle_server_interface $target_ip enable $network_interface
        echo "---------------------------"
        echo "step 2 b. network outage must reach Running, Maintenance, as long as the outage was longer than the discovery timeout."
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
        echo "step 2 c. stop backup "
        echo "---------------------------"
        #kill_server $target_ip
        kill_server $standby_ip
        echo "---------------------------"
        echo "step 2 d. start backup "
        echo "---------------------------"
        start_server $standby_ip
        echo "---------------------------"
        echo "step 2 e. get Running Standby status "
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
    echo "step 3"
    echo "---------------------------"
    get_status "Running" 1 "Stopped" 1
    let ret=$ret+$?
    echo "---------------------------"
    echo "step 4"
    echo "---------------------------"
    start_server $target_ip
    echo "---------------------------"
    echo "step 5"
    echo "---------------------------"
    get_status "Running" 1 "Standby" 1
    let ret=$ret+$?
    echo "---------------------------"
    echo "step 6"
    echo "---------------------------"
    do_test_X_disrupt_server_on_Y_check_result $primary_ip $target
    let ret=$ret+$?
    return $ret;
}



do_test_1_disrupt_server_on_standby(){
    do_test_X_disrupt_server_on_Y "KILL" "STANDBY"
    return $?
}

do_test_2_disrupt_server_on_primary(){
    do_test_X_disrupt_server_on_Y "KILL" "PRIMARY"
    return $?
}

do_test_3_disrupt_server_on_primary(){
    do_test_X_disrupt_server_on_Y "SIGSEGV" "PRIMARY"      ; # defect 23786
    return $?
}
do_test_4_disrupt_server_on_primary(){
    do_test_X_disrupt_server_on_Y "NETWORK" "STANDBY" ha1 60
    return $?
}


old_print_tests(){
local x;
let x=0;
while(($x<400)); do
echo do_test_X_disrupt_server_on_Y KILL PRIMARY
echo do_test_X_disrupt_server_on_Y KILL STANDBY
echo do_test_X_disrupt_server_on_Y KILL PRIMARY
echo do_test_X_disrupt_server_on_Y KILL STANDBY
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 1
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 1
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 10
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 10
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 30
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 30
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 60
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 60
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 100
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 100
echo do_test_X_disrupt_server_on_Y NETWORK PRIMARY mgt1 1000
echo do_test_X_disrupt_server_on_Y NETWORK STANDBY mgt1 1000
let x=$x+1;
done
}

print_tests(){
    for method in KILL SIGSEGV DEVICE ; do 
        for target in PRIMARY STANDBY ; do
            if [ "$method" == "NETWORK" ] ; then
                echo do_test_X_disrupt_server_on_Y $method $target $network_interface $network_outage
            else
                echo do_test_X_disrupt_server_on_Y $method $target 
            fi
        done
    done
    for network_outage in 1 10 30 60 100 1000 ; do 
        #for network_interface in ha1 ha2 ; do 
        for network_interface in ha1 eth7 ; do 
    #        for method in KILL SIGSEGV NETWORK ; do     # currently SIGSEGV has defect 23786
    #        for method in KILL SIGSEGV NETWORK DEVICE POWER ; do     # currently SIGSEGV has defect 23786
            #for method in KILL SIGSEGV NETWORK DEVICE WER ; do     # currently SIGSEGV has defect 23786
            #for method in KILL NETWORK ; do 
            for method in NETWORK ; do 
                for target in PRIMARY STANDBY ; do
                    if [ "$method" == "NETWORK" ] ; then
                        echo do_test_X_disrupt_server_on_Y $method $target $network_interface $network_outage
                    else
                        echo do_test_X_disrupt_server_on_Y $method $target 
                    fi
                done
            done
        done
    done
}

do_cleanup_sighup(){
echo do_cleanup sighup invoked for PID: $$
}
do_cleanup_sigint(){
echo do_cleanup sigint invoked for PID: $$ - exitting
exit 1;
}
do_cleanup_sigterm(){
echo do_cleanup sigterm invoked for PID: $$ - exitting
exit 1;
}
do_cleanup_sigpipe(){
echo do_cleanup sigpipe invoked for PID: $$
}
do_cleanup_sigchld(){
echo do_cleanup sigchld invoked for PID: $$
}
do_set_traps(){
    trap do_cleanup_sighup SIGHUP 
    trap do_cleanup_sigint SIGINT
    trap do_cleanup_sigterm SIGTERM
    trap do_cleanup_sigchld SIGCHLD
    trap do_cleanup_sigpipe SIGPIPE
}
do_run_all_tests(){
    local line;
    local timeout;
    local count=0;
    local total_time
    local start_time
    local end_time


    rm -rf log.ha.*

    echo "-----------------------------------------------" | tee -a log.ha.out 
    echo "`date` Beginning as PID: $$                    " | tee -a log.ha.out
    echo "-----------------------------------------------" | tee -a log.ha.out

    do_set_traps

    clear_server_core; # start out fresh

    print_tests | sort -R | while read line; do
{
    do_set_traps

    get_server_time | tee -a log.ha.out log.ha.$count
    synchronize_server_time

    if exist_server_core ; then 
        echo "WARNING: Core files detected on one or more appliances. Please stop and debug"
        return 1;  
    fi

    echo "-----------------------------------------------" | tee -a log.ha.out 
    echo "`date` Running test count=$count as PID: $$    " | tee -a log.ha.out
    echo "-----------------------------------------------" | tee -a log.ha.out
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count
        echo "`date` Verify that HA Pair is in a stable state" | tee -a log.ha.out log.ha.$count
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count
        ha_pair_up   | tee -a log.ha.out log.ha.$count
        echo "Complete - Verified that HA Pair is in a stable state"  | tee -a log.ha.out log.ha.$count
        start_time=`date +%s`
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count
        echo "`date` Start test. Current test is $line       " | tee -a log.ha.out log.ha.$count
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count
        run_command_with_timeout " $line " 3600  | tee -a  log.ha.$count
        test_exit=$?

        end_time=`date +%s`
        let total_time=($end_time-$start_time);
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count
        echo "`date` RESULT: $test_exit TIME: $total_time TEST: $count $line" | tee -a log.ha.out log.ha.$count
        echo "-----------------------------------------------" | tee -a log.ha.out log.ha.$count ; 
}
        echo "-----------------------------------------------" | tee -a log.ha.out 
        echo " Finished test for PID: $$ count= $count       " | tee -a log.ha.out 
        echo "-----------------------------------------------" | tee -a log.ha.out

        let count=$count+1; 

    done
        echo "-----------------------------------------------" | tee -a log.ha.out 
        echo " Exitting as PID: $$ count= $count             " | tee -a log.ha.out 
        echo "-----------------------------------------------" | tee -a log.ha.out

}



