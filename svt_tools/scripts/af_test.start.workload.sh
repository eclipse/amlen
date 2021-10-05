#!/bin/bash 

let fail_count=0;

. /niagara/test/scripts/af_test.sh

sleep 1

if [ -n "$AF_AUTOMATION_TEST" ] ;then 

    af_automation_env
	echo "-----------------------------------------"
	echo "`date` Start processing input args on ${THIS} for AF Automation test: $@"
	echo "-----------------------------------------"

    set -x
    workload=${1-"throughput"}
    if [ -n "$AF_AT_VARIATION_WORKLOAD" ] ;then
        echo "-----------------------------------------"
        echo " AF AUTOMATION OVERIDE: Will use overide setting for WORKLOAD : AF_AT_VARIATION_WORKLOAD = $AF_AT_VARIATION_WORKLOAD"
        echo "-----------------------------------------"
        workload=${AF_AT_VARIATION_WORKLOAD}
    else
        #-- Export this variable if it was not already provided as it may be used by other scripts.
        export AF_AT_VARIATION_WORKLOAD="$workload"
    fi

	echo "-----------------------------------------"
	echo "`date` Check for failed transaction, and abort test if needed"
	echo "-----------------------------------------"
    af_running_transaction
    if [ "$?" == "0" ] ; then # if success we are inside a test transaction
        af_check_transaction # check that the test transaction has not had any failures.
        if [ "$?" != "0" ] ; then 
            # There have been failures, unless we are trying to start/end the transaction, abort the test
            if ! echo "$workload" | grep "test_transaction_" > /dev/null ; then
	            echo "-----------------------------------------"
	            echo "`date` ABORT Test: This test was part of failed \"test transaction\", therefore the execution for the rest of the test will be skipped"
	            echo "-----------------------------------------"
                af_exit_with_result 1 "$AF_AUTOMATION_TEST" ; 
            fi
        fi
    fi

	echo "-----------------------------------------"
	echo "`date` Continuing..."
	echo "-----------------------------------------"

    workload_subname=${2-""}
    num_iterations=${3-1}
    ima1=${4-""}
    ima2=${5-""}
    unused_6=${6-""}
    unused_7=${7-""}
    unused_8=${8-""}
    unused_9=${9-""}
    unused_11=${10-""}
    unused_11=${11-""}
    unused_12=${12-""}
    unused_13=${13-""}
    unused_14=${14-""}
    unused_15=${15-17772}
    set +x

	echo "-----------------------------------------"
	echo "`date` AUTOMATION overrides which may further affect behavior env:"
	echo "-----------------------------------------"

    env | grep -E 'AF_AUTOMATION_TEST|AF_AT'

    if [ -n "$AF_AT_VARIATION_HA" ] ;then
        echo "-----------------------------------------"
        echo " AF AUTOMATION OVERIDE: Will use overide setting for HA : AF_AT_VARIATION_HA = $AF_AT_VARIATION_HA"
        echo "-----------------------------------------"
        ima2=${AF_AT_VARIATION_HA}
    fi
    if [ -n "$AF_AT_VARIATION_HA2" ] ;then # alternate form to set ha appliance if you don't want automatic testing to happen
        echo "-----------------------------------------"
        echo " AF AUTOMATION OVERIDE: Will use overide setting for HA : AF_AT_VARIATION_HA2 = $AF_AT_VARIATION_HA2"
        echo "-----------------------------------------"
        ima2=${AF_AT_VARIATION_HA2}
    fi

    if [ -n "$AF_AT_VARIATION_ALL_M" ] ;then
        echo "-----------------------------------------"
        echo " AF AUTOMATION OVERIDE: Will use overide setting : AF_AT_VARIATION_ALL_M = $AF_AT_VARIATION_ALL_M"
        echo "-----------------------------------------"
        env_vars=` env | grep -E 'AF_AUTOMATION_TEST|AF_AT' | grep -v AF_AT_VARIATION_ALL_M= | awk '{ printf(" %s ", $0);}';`
        my_pid=""
        outlog=log.workload.${workload}.remote.start;
        for v in `af_automation_systems_list` ; do
            echo "-----------------------------------------"
            echo "AF_AT_VARIATION_ALL_M: Starting command on all Ms and will wait for it to complete on $v: ssh cd `pwd`; $env_vars $0 $@" ;
            ssh $v "cd `pwd`; $env_vars $0 $@ 1>> $outlog 2>>$outlog" &
            my_pid+="$! "
            echo "-----------------------------------------"
        done
    
        echo "---------------------------------------"
        echo " `date` Wait for clients to complete: $my_pid "
        echo "---------------------------------------"

        wait $my_pid

        echo "---------------------------------------"
        echo " `date` Done. All clients completed. Check results. "
        echo "---------------------------------------"

        {
        for v in `af_automation_systems_list `; do
            #------------------------------------------------
            # - note below because searchLogs is expecting only one message of type AF_TEST_RESULT:, i must not repeat all
            # the logs with that result, instead i do the find replace on the COPIED results to prevent searchLogs count failures.
            #------------------------------------------------
            echo -en "\nAF Copied Autommation Result for $v : " ; ssh $v "cd `pwd`; cat $outlog |grep \"AF_TEST_RESULT: \" " | sed "s/AF_TEST_RESULT:/COPY_OF_RESULT:/g"
        done
        } > .tmp.$outlog.results
        echo "---------------------------------------"
        echo " `date` Results are : "
        echo "---------------------------------------"
        cat .tmp.$outlog.results 
        
        fail_count=`cat .tmp.$outlog.results | grep "Failure"  | wc -l`
        rm -rf .tmp.$outlog.results

        af_exit_with_result $fail_count "$AF_AUTOMATION_TEST" ; 

    fi

else
    echo "ERROR: Must specify AF_AUTOMATION_TEST env variable to use this script"
    exit 1;
fi

if ! [ -n "$AF_AT_VARIATION_WORKLOAD" ] ;then
    #-- Export this variable if it was not already exported because it may be used by other scripts (even for manual).
    export AF_AT_VARIATION_WORKLOAD="$workload"
fi

if [ -n "$AF_AT_VARIATION_PORT" ] ;then
    echo "-----------------------------------------"
    echo " $0 - Will use user override AF_AT_VARIATION_PORT for port setting = $AF_AT_VARIATION_PORT"
    echo "-----------------------------------------"
    current_port=$AF_AT_VARIATION_PORT
else
    echo "-----------------------------------------"
    echo " $0 - Defaulting to insecure demo port for all further work"
    echo "-----------------------------------------"
    current_port=16102
fi

DELAY=5

run_standard_workload () {
    local workload=${1}
    local test_process_name=${2}
    local workload_parms="${3}"
    local do_stats=${4-1}
    local cur_pid;
	local my_pid=""
    local my_exit=0; 
    local outlog=""
    local substat=""
    local pubstat=""
    local pfcount=0
    local count=0
    #local pre_net_stat_count;
    #local data;
    #local post_net_stat_count;

    outlog="log.workload.${workload}"
    rm -rf $outlog

    {

	echo "------------------"
	echo " Start workload: $workload : test_process_name: ${test_process_name} : ${workload_parms} "
    echo " ${test_process_name} ${workload_parms} 2>>$outlog 1>>$outlog "
	echo "------------------"
    ${test_process_name} ${workload_parms} &
    cur_pid=$!
    my_pid+="$cur_pid "
    substat="log.s."
    pubstat="log.p."

	wait $my_pid
	my_exit=$?

	echo "`date` end iteration $x - exit code was ${my_exit}" 

    } 2>>$outlog 1>>$outlog
    
    if [ "$my_exit" == "0" -a "$pfcount" == "0" ] ; then
    	return 0;
    else
    	return 1;
    fi 
}

check_haFunctions_rc(){
    local filename=$1
    local count
    local data
    local unexpected=0;

    if ! [ -e ${filename} ] ;then
        echo "`date`: ERROR: $filename does not exist"
        return 1;
    fi

    #-----------------------------------------
    # 2014.04.07 - new sometimes hafuncions.sh is still running
    # even after giving an exit code. I want to print a warning and kill  it if
    # that happens.... this is not a good situation but it should give better
    # clarity to the situation. Further updates may be needed.
    # 2014.04.09 - should be fixed by 58907 - took out workaround - 
    #-----------------------------------------
 
    data=`cat ${filename}`
    if [ -z "$data" ] ; then
        echo "`date`: ERROR: $filename has no data"
        return 1;
    elif [[ "$data" =~ "Test result is Failure" ]] ; then
        echo "`date`: ERROR: $filename shows failure string: Test result is Failure"
        return 1;
    fi
    return 0;

}


run_ha_cmd () {
    local cmd=${1-""}
    local data
    local rcount
    local scount
    local y

    if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
        echo "`date`: run_ha_cmd: A_COUNT is $A_COUNT. Continuing"
    else
        echo "`date`: run_ha_cmd: ERROR: A_COUNT is $A_COUNT. Cannot run HA commands."
        return 1;
    fi

    data=`ssh admin@$ima1 status imaserver; ssh admin@$ima2 status imaserver;`
    echo "`date`: run_ha_cmd: State: $data"
    rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
    scount=`echo "$data" | grep "Status = Standby"  | wc -l`
    if [ "$cmd" == "setupHA"  ] ;then
        if [ "$rcount" == "1" -a "$scount" == "1" ] ;then
            echo "`date`: run_ha_cmd: WARNING: not running setupHA because the HA is already setup."
            return 0;
        fi
    elif [ "$cmd" == "disableHA"  ] ;then
        if (($rcount==2)); then
            echo "`date`: run_ha_cmd: WARNING: not running disableHA because the HA is already disabled."
            return 0;
        fi
    elif [ "$cmd" == "verifyBothRunning"  ] ;then
        if [ "$rcount" == "2" ]] ;then
            echo "`date`: run_ha_cmd: ERROR: Tried to verify Running Standby but we are in standalone mode"
            return 1;
        fi
    else
        echo "`date`: run_ha_cmd: Continuing. No verification that $cmd is valid for this State"
    fi

    if [ "$cmd" == "verifyBothRunning"  ] ;then
        if [ "$rcount" == "2" ]] ;then
            echo "`date`: run_ha_cmd: ERROR: Tried to verify Running Standby but we are in standalone mode"
            return 1;
        fi
    else
        echo "`date`: run_ha_cmd: Continuing. No verification that $cmd is valid for this State"
    fi

    if [ "$cmd" == "verifyBothRunning"  ] ;then
        #-------------------------------------------------------------------
        # This function keeps trying forever. The user must use a timeout on the component to make it die in case of error.
        #-------------------------------------------------------------------
        let y=0;
        ${M1_TESTROOT}/scripts/haFunctions.sh -a verifyBothRunning -f ${AF_AUTOMATION_TEST}_verifyRunning_haFunctions.$y.log
        data=`cat ${AF_AUTOMATION_TEST}_verifyRunning_haFunctions.$y.log`
        #echo "$data"
        while [[ "$data" =~ "Test result is Failure" ]] ; do
            let y=$y+1
            echo "------------------------------------------------------"
            echo "`date` : (retry) haFunctions.sh -a verifyBothRunning: $y, previous rc: $rc"
            echo "------------------------------------------------------"
            ${M1_TESTROOT}/scripts/haFunctions.sh -a verifyBothRunning -f ${AF_AUTOMATION_TEST}_verifyRunning_haFunctions.$y.log
            data=`cat ${AF_AUTOMATION_TEST}_verifyRunning_haFunctions.$y.log`
            #echo "$data"
        done
        echo "------------------------------------------------------"
        echo "`date` : complete. verifyBothRunning is a success"
        echo "------------------------------------------------------"
    else
        ${M1_TESTROOT}/scripts/haFunctions.sh -a $cmd -f ${AF_AUTOMATION_TEST}_${cmd}_haFunctions.log
        check_haFunctions_rc ${AF_AUTOMATION_TEST}_${cmd}_haFunctions.log
    fi
    return $?
}


let x=0; while(($x<$num_iterations));do
	echo "`date` start iteration $x" 

	if [ $workload == "test_transaction_start"  ] ;then
        num_iterations=1;
        af_start_transaction 
        rc=$?
        af_exit_with_result $rc "$AF_AUTOMATION_TEST"
	elif [ $workload == "test_transaction_end"  ] ;then
        af_end_transaction 
        rc=$?
	    find trans.log.* -type f | sed "s/trans.log.//g" |  xargs -i echo "mv trans.log.{} $AF_AUTOMATION_TEST.`hostname`.{}.trans.log" | sh -x
        af_exit_with_result $rc "$AF_AUTOMATION_TEST"
	elif [ $workload == "process_logs"  ] ;then # run default post processing on all log.* files.
        fail_count=0;
        num_iterations=1;  
    elif [ "$workload" == "haFunctions.sh" ] ;then
        run_ha_cmd $workload_subname
        let fail_count=($fail_count+$?)
	elif [ $workload == "cleanup"  ] ;then
        num_iterations=1;  # this is a one iteration only test.
        run_standard_workload $workload  "/niagara/test/scripts/af_test.cleanup.sh $workload_subname "
		let fail_count=($fail_count+$?)
        # this can be called in automation to sweep up any outstanding logs that may have been missed due to errors.
        # as well as do the standard cleanup of making sure no processes are running.
    elif [ $workload == "email_notifcation"  ] ;then
        if [ -n "$AF_AUTOMATION_TEST" ] ;then
            if [ -n $workload_subname ] ;then
                echo "`date` Send email notification to $workload_subname that an automation run has started the af_test portion in case I want to monitor it "
                echo -e " `date `\n`hostname`\n`cat /niagara/test/testEnv.sh` "  | mail  -s "af_test Automation:${AF_AUTOMATION_TEST} just started on `hostname` " ${workload_subname}
            else
	            echo "-----------------------------------------"
	            echo "`date` Error: workload_subname is required for this operation"
	            echo "-----------------------------------------"
                let fail_count=$fail_count+1 
            fi
            
        fi
    elif [ $workload == "svt_disable_ha"  ] ;then 
        # this function disables HA if it is enabled, otherwise it doesn't do anything
        if [ -z "$ima1" -o -z "$ima2" ] ;then
            echo "`date` : ERROR: ima1 $ima1 or ima2 $ima2 is not specified"
            let fail_count=$fail_count+1 
            break;
        fi
        num_iterations=1;  # this is a one iteration only test.
        data=`ssh admin@$ima1 "imaserver show HighAvailability" ; ssh admin@$ima2 "imaserver show HighAvailability" ;`
        count=`echo "$data" | grep EnableHA | grep True |wc -l`
        if (($count>=1)); then
            echo "`date` : $count appliances show up in HA mode. Issuing disableHA" 
            ${M1_TESTROOT}/scripts/haFunctions.sh -a disableHA -f ${AF_AUTOMATION_TEST}_haFunctions_disableHA.log
        else
            echo "`date` : no appliances show up in HA mode. No need to disableHA" 
        fi
	elif echo $workload | grep "continuous_monitor_primary_appliance" > /dev/null ; then
        /niagara/test/scripts/af_test.monitor.appliance.sh ${ima1} "continuous.appliance.monitor" "CONTINUOUS" > /dev/null 2> /dev/null &
	elif echo $workload | grep "continuous_monitor_secondary_appliance" > /dev/null ; then
        /niagara/test/scripts/af_test.monitor.appliance.sh ${ima2} "continuous.appliance.monitor" "CONTINUOUS" > /dev/null 2> /dev/null &
	elif echo $workload | grep "continuous_monitor_client" > /dev/null ; then
        /niagara/test/scripts/af_test.monitor.client.sh "continuous.client.monitor" > /dev/null 2> /dev/null &
    else
        echo "ERROR: unrecognized workload $workload"
	fi

	echo "`date` end iteration $x" 
	let x=$x+1; 

	if (($num_iterations>1)); then
		echo "`date` Start - Sleep before next iteration"
        sleep $DELAY
		echo "`date` Done - Sleep before next iteration"
	fi	
    
done

if [ -n "$AF_AT_VARIATION_QUICK_EXIT" ] ;then
    echo "-----------------------------------------"
    echo "`date` AF AUTOMATION OVERIDE: Do \"quick exit\" before doing any tars on logs etc... "
    echo "`date` other testing will be done while this workload continues to run, and messaging results will be evaluated later"
    echo "-----------------------------------------"

    echo "---------------------------------"
    echo "`date` $0 - Done! Successfully completed all expected processing - Quick Exit"
    echo "---------------------------------"

    af_exit_with_result $fail_count "$AF_AUTOMATION_TEST"

fi

if [ -n "$AF_AUTOMATION_TEST_PP_PREFIX" ]; then # override default log processing with custom user supplied prefix
	echo "-----------------------------------------"
	echo "`date` Start post processing for AF Automation test"
	echo "-----------------------------------------"
   
    find ${AF_AUTOMATION_TEST_PP_PREFIX}.* -type f | sed "s/${AF_AUTOMATION_TEST_PP_PREFIX}.//g" |  xargs -i echo "mv ${AF_AUTOMATION_TEST_PP_PREFIX}.{} $AF_AUTOMATION_TEST.`hostname`.{}.${AF_AUTOMATION_TEST_PP_PREFIX}.log "  | sed "s/.log.log$/.log/" | sh -x

elif [ -n "$AF_AUTOMATION_TEST" ] ;then 
	echo "-----------------------------------------"
	echo "`date` Start post processing for AF Automation test"
	echo "-----------------------------------------"
   
    find log.* -type f | sed "s/log.//g" |  xargs -i echo "mv log.{} $AF_AUTOMATION_TEST.`hostname`.{}.log" | sh -x
fi

if [ -n "$AF_AUTOMATION_TEST_LOG_TAR" ] ;then 
    if [ -n "$AF_AUTOMATION_TEST_PP_PREFIX" ]; then # override default log processing with custom user supplied prefix
	    echo "-----------------------------------------"
	    echo "`date` Tar up all the logs w/ specified custom user file pattern $AF_AUTOMATION_TEST_PP_PREFIX as *${AF_AUTOMATION_TEST_PP_PREFIX}*.log"
	    echo "-----------------------------------------"
	    tar -czvf ${AF_AUTOMATION_TEST_LOG_TAR}.`hostname`.tgz.log *${AF_AUTOMATION_TEST_PP_PREFIX}*.log
	    rm -rf *${AF_AUTOMATION_TEST_PP_PREFIX}*.log
    elif [ -n $workload_subname ] ;then
	    echo "-----------------------------------------"
	    echo "`date` Tar up all the logs w/ specified file pattern $workload_subname as *${workload_subname}*.log"
	    echo "-----------------------------------------"
	    tar -czvf ${AF_AUTOMATION_TEST_LOG_TAR}.`hostname`.tgz.log *${workload_subname}*.log
	    rm -rf *${workload_subname}*.log
    else
	    echo "-----------------------------------------"
	    echo "`date` Error: workload_subname is required for this operation"
	    echo "-----------------------------------------"
        let fail_count=$fail_count+1 
    fi
	
fi

af_exit_with_result $fail_count "$AF_AUTOMATION_TEST"

