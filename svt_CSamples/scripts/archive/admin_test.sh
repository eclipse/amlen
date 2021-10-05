#!/bin/bash

# WHY: ssh -nx  
#  rsh and/or ssh can break a while read loop due to it using stdin. 
#  Put a -n into the ssh line which stops it from trying to use stdin and it fixed the problem.

# echo "ifconfig" | ssh  admin@10.10.10.10 "busybox"
# let iteration=0; while((1)); do ./ha_fail.sh; ((iteration++)); echo current iteration is $iteration; date; done

. ./svt_test.sh

clear_server_core()  {
    local server=$1
    local line
    local data
    local var
    local rc
    if [ -n "$server" ] ; then
        data=`ssh -nx  admin@$server advanced-pd-options list | awk '{print $1}'`
        for var in $data ;do
            #line=`echo "$var" | grep -oE 'bt.*|.*_core.*' |awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*|.*_core.*' >/dev/null ; then
            #line=`echo "$var" | grep -oE 'bt.*' |awk '{print $1}' `
            line=`echo "$var" | awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*' >/dev/null ; then
                echo ssh  -nx admin@$server advanced-pd-options delete $line
                ssh  -nx admin@$server advanced-pd-options delete $line
                rc=$?
                echo rc is $rc
            #fi
        done
    else
        echo "ERROR: did not specifify server argument"
    fi
}
exist_server_core()  {
    local server=$1;
    local data
    local regex="bt.*"
    data=` ssh -nx  admin@$server advanced-pd-options list`
    #if echo "$data" | grep -E 'bt.*|.*_core.*'  > /dev/null ;then
    if echo "$data" | grep -E 'bt.*'  > /dev/null ;then
        echo "==========================================================="
        echo "==========================================================="
        echo "==========================================================="
        echo ""
        echo ""
        echo "WARNING: core data found is : $data" >> /dev/stderr
        echo ""
        echo ""
        echo ""
        echo "==========================================================="
        echo "==========================================================="
        echo "==========================================================="
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

run_command_with_timeout() {
    local command="$1"
    local timeout=${2-60}
    local running_test;
    local test_exit;
    local starttime=`date +%s`
    $command &
    running_test=$!
    echo -e "==========================================================================="
    echo -e "Running $command with timeout $timeout, as pid: running_test=$running_test\n"
    while(($timeout>0)); do
        if ! [ -e /proc/$running_test/cmdline  ] ;then
            echo "INFO: /proc/$running_test/cmdline does not exist anymore" 
            break;
        fi
        let timeout=$timeout-1;
        sleep 1;
    done
    local endtime=`date +%s`
    local totaltime
    let totaltime=($endtime-$starttime)
    
    echo -e "\nINFO: response time: $totaltime $command"
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


do_admin_loop (){
    local server=$1
    local usercmd=${2-""}
    local outlog="log.admin.$server"
    rm -rf $outlog
    run_command_with_timeout "ssh -nx  admin@$server imaserver set AcceptLicense "
    run_command_with_timeout "ssh -nx  admin@$server imaserver update Endpoint Name=DemoEndpoint Enabled=true"
    while ((1)) ; do
        if [ -n "$usercmd" ] ; then
            run_command_with_timeout "ssh -nx  admin@$server $usercmd"
        else
            run_command_with_timeout "ssh -nx  admin@$server imaserver harole "
            run_command_with_timeout "ssh -nx  admin@$server imaserver stat "
            run_command_with_timeout "ssh -nx  admin@$server datetime get "
            run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options list "
            run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options memorydetail "
    
            for var in Memory Store Server Connection; do 
                run_command_with_timeout "ssh -nx  admin@$server imaserver stat $var "
            done
    
            for var in battery ethernet-interface flash imaserver memory mqconnectivity nvdimm temperature uptime vlan-interface voltage volume webui ; do
                run_command_with_timeout "ssh -nx  admin@$server status $var"
            done
        fi
        date
    done  | tee -a $outlog


}
