#!/bin/bash

#--------------------------------------------------------
# af_test.sh : helper functions for af_test.* scripts
# 
# af_test scripts are a set of extensions that can be used on top
# of the the automation framework to support some additional features
# such of grouping of tests into a "test transaction", simplified 
# test execution with common syntax, standardized cleanup ,
# standardized log collection, etc...
#
# Examples:
#
#   ... 
#   ... 
#   ... 
#
# Changelog:
#
#--------------------------------------------------------

. /niagara/test/scripts/commonFunctions.sh


af_run_command_with_timeout() {
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


af_admin_loop (){
    local server=$1
    local usercmd=${2-""}
    local outlog=${3-"log.admin.${server}"}
    rm -rf $outlog
    af_run_command_with_timeout "ssh -nx  admin@$server imaserver set AcceptLicense "
    af_run_command_with_timeout "ssh -nx  admin@$server imaserver update Endpoint Name=DemoEndpoint Enabled=true"
    while ((1)) ; do
        if [ -n "$usercmd" ] ; then
            af_run_command_with_timeout "ssh -nx  admin@$server $usercmd"
        else
            af_run_command_with_timeout "ssh -nx  admin@$server imaserver harole "
            af_run_command_with_timeout "ssh -nx  admin@$server imaserver stat "
            af_run_command_with_timeout "ssh -nx  admin@$server datetime get "
            af_run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options list "
            af_run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options memorydetail "
    
            for var in Memory Store Server Connection; do 
                af_run_command_with_timeout "ssh -nx  admin@$server imaserver stat $var "
            done
    
            for var in battery ethernet-interface flash imaserver memory mqconnectivity nvdimm temperature uptime vlan-interface voltage volume webui ; do
                af_run_command_with_timeout "ssh -nx  admin@$server status $var"
            done
        fi
        date
    done  | af_add_log_time "${server}" | tee -a $outlog


}

af_add_log_time () {
    local additional_msg=${1-""}
    if [ -n "$additional_msg" ] ;then
        msg=" ${additional_msg} "
    else
        msg=" ";
    fi
    while read line; do
        echo "`date +"%Y-%m-%d %H:%M:%S.%U" `${msg}${line}"
    done
}

af_automation_master () {
	af_automation_systems_list 2> /dev/null | head -1 
}

af_automation_cleanup () {
    local flag=$1
	local v
    local waitpid

    waitpid=""
	for v in `af_automation_systems_list` ; do ssh $v "/niagara/test/scripts/af.cleanup.sh $flag " & waitpid+="$! "; done
    echo "-----------------------------------"
    echo "Start -Waiting for cleanup processes"
    echo "-----------------------------------"
    wait $waitpid
    echo "-----------------------------------"
    echo "Done. Waiting for cleanup processes"
    echo "-----------------------------------"
    
}
af_automation_cleanup_force () {
    local waitpid

    af_automation_cleanup force

    
}

af_automation_run_command_and_master() {
    for v in `af_automation_systems_list ` ; do
        echo "======================== Run command on $v =========================== ";
        ssh $v $@
    done;
}
af_automation_run_command() {
    local master=`af_automation_master`;
    echo "args are $@"
    for v in `af_automation_systems_list | grep -v $master`; do
        echo "======================== Run command on $v =========================== ";
        ssh $v $@
    done;
}

af_automation_env(){
    test_template_env
    #if [[ -f "/niagara/test/testEnv.sh" ]]
    #then
        ## Official ISM Automation machine assumed
        #source /niagara/test/testEnv.sh
    #else
        ## Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
        #source ../scripts/ISMsetup.sh
        ##../scripts/prepareTestEnvParameters.sh
        ##source ../scripts/ISMsetup.sh
    #fi
} 
af_automation_systems_list () {
    test_template_systems_list
    #local v=1
    #af_automation_env
    #{ while (($v<=${M_COUNT})); do
        #echo "M${v}_HOST"
        #let v=$v+1;
    #done ; }  |  xargs -i echo "echo \${}" |sh
}

af_automation_ping(){
    let x=0;
    af_automation_env
    for v in `af_automation_systems_list` ; do            
        echo  "================================================ pinging $v " ; 
        ping -c 1 -w 1  $v  ;   
        let x=$x+$?
    done
    echo ""
    echo "Done. RC of $x (number of ping failures) for ${M_COUNT} systems"
    echo ""
}
    
af_exit_with_result(){
    local fail_count=${1-0};
    local test_name=${2-""};
    if (($fail_count>0)); then
        echo "-----------------------------------------"
        echo "`date` Done - AF_TEST_RESULT: Failure. Completed all expected processing w/ $fail_count failures. Exit with 1. "
        echo "-----------------------------------------"
        af_continue_transaction $test_name 1
        exit 1;
    else
        echo "-----------------------------------------"
        echo "`date` Done - AF_TEST_RESULT: SUCCESS . Completed all expected processing w/ $fail_count failures . Exit with 0."
        echo "-----------------------------------------"
        af_continue_transaction $test_name 0
        exit 0;
    fi
}

#--------------------------------------------------------------------
# The test transaction functions below do handle the run-scenario TIMEOUT, and direct failures. 
# But the other "soft" errors like core file detected or imaserver stopped running during test are not handled.
#--------------------------------------------------------------------
af_start_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local last_runscenario_rc_file="./.svt.af.runscenario.last.rc"
    export AUTOMATION_FRAMEWORK_SAVE_RC=$last_runscenario_rc_file
    rm -rf ${transaction_file}.failed
    echo "0" > ./.svt.af.runscenario.last.rc
    echo "`date` " > $transaction_file
    echo "0" > ${transaction_file}.last.exit
}
af_continue_transaction(){
    local testname=${1-""}
    local exitcode=${2-""}
    local transaction_file="./trans.log.svt.test.transaction"

    if [ -e $transaction_file ] ; then
        echo "`date` $testname $exitcode" >> $transaction_file
        echo "$exitcode" >  ${transaction_file}.last.exit
    fi
}
af_running_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    if [ -e $transaction_file ] ;then
        return 0; # yes we are running a transactiton
    else
        return 1; # no we are not running a transaction
    fi  
    return 1; # no we are not running a transaction
}  
af_check_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local last_runscenario_rc_file="./.svt.af.runscenario.last.rc" # especially used to detect TIMEOUT s
    local var=""
    local var2=""
    if [ -e $transaction_file -a -e $last_runscenario_rc_file ] ;then
        var=`cat ${transaction_file}.last.exit`
        var2=`cat ${last_runscenario_rc_file}`
        if [ -e ${transaction_file}.failed ] ; then
            echo "`date` Error in transaction: Transaction has failed due to existence of ${transaction_file}.failed  "
            return 2; # transaction has failed already
        elif [ "$var" != "0" -o "$var2" != "0" ]; then 
            echo "`date` Error in transaction: v:$var v2:$var2 "
            echo "`date` Error in transaction: v:$var v2:$var2 " > ${transaction_file}.failed
            return 1; # transaction is failing.
        else
            return 0; # transaction is passing
        fi
        return 1; # transaction is failing.
    fi
    return 1 ; # Failure because we should be inside a transaction
}
af_end_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local rc
    cat $transaction_file
    if af_check_transaction ; then 
        echo "AF Test Transaction passed"
        rc=0;
    else
        echo "AF Test Transaction failed "
        rc=1;
    fi
    find ${transaction_file}* | xargs -i mv {} {}.complete
    #rm -rf ${transaction_file}
    #rm -rf ${transaction_file}.*
    sync
    return $rc
}

