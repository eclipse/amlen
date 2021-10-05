#!/bin/bash 

let fail_count=0;

. /niagara/test/svt_cmqtt/svt_test.sh


#--------------------------------------------------
# log prefix (LOGP) the prefix for all log files.
#--------------------------------------------------
LOGP=$(do_get_log_prefix)


if [ -n "$AF_AUTOMATION_TEST" ] ;then 

    svt_automation_env
    echo "-----------------------------------------"
    echo "`date` Start processing input args on ${THIS} for SVT Automation test: $@"
    echo "-----------------------------------------"

    set -x
    workload=${1-"throughput"}
    if [ -n "$SVT_AT_VARIATION_WORKLOAD" ] ;then
        echo "-----------------------------------------"
        echo " SVT AUTOMATION OVERIDE: Will use overide setting for WORKLOAD : SVT_AT_VARIATION_WORKLOAD = $SVT_AT_VARIATION_WORKLOAD"
        echo "-----------------------------------------"
        workload=${SVT_AT_VARIATION_WORKLOAD}
    else
        #-- Export this variable if it was not already provided as it may be used by other scripts.
        export SVT_AT_VARIATION_WORKLOAD="$workload"
    fi

    set +x
    echo "-----------------------------------------"
    echo "`date` Check for failed transaction, and abort test if needed"
    echo "-----------------------------------------"
    svt_running_transaction
    if [ "$?" == "0" ] ; then # if success we are inside a test transaction
        svt_check_transaction # check that the test transaction has not had any failures.
        if [ "$?" != "0" ] ; then 
            # There have been failures, unless we are trying to start/end the transaction, abort the test
            if ! echo "$workload" | grep "test_transaction_" > /dev/null ; then
                if [ "$AF_AUTOMATION_TEST_N" == "0" ] ; then
                    echo "-----------------------------------------"
                    echo "`date` Warning : detected failing transaction... however.... "
                    echo "`date` This is test n == 0 . It is not currently supported to have"
                    echo "`date` test transactions that span multiple test files." 
                    echo "`date` if that is desired in the future then extra support will be needed"
                    echo "`date` Clearing stale transaction info"
                    echo "-----------------------------------------"
                    svt_clear_transaction        
                elif [ "$SVT_AT_VARIATION_RUN_ONLY_ON_TRANSACTION_FAILURE" == "true" ]; then
                    echo "-----------------------------------------"
                    echo "`date` This test would have been aborted, but the SVT_AT_VARIATION_RUN_ONLY_ON_TRANSACTION_FAILURE flag was set."
                    echo "`date` continuing test."
                    echo "-----------------------------------------"
                else
                    echo "-----------------------------------------"
                    echo "`date` ABORT Test: This test was part of failed \"test transaction\", therefore the execution for the rest of the test will be skipped"
                    echo "-----------------------------------------"
                    svt_exit_with_result 1 "$AF_AUTOMATION_TEST" ; 
                fi
            fi
        elif [ "$SVT_AT_VARIATION_RUN_ONLY_ON_TRANSACTION_FAILURE" == "true" ]; then
            echo "-----------------------------------------"
            echo "`date` Skip Test: This test is only supposed to run when the transaction is failing."
            echo "-----------------------------------------"
            svt_exit_with_result 0 "$AF_AUTOMATION_TEST" ; 
        fi
    fi

    echo "-----------------------------------------"
    echo "`date` Continuing..."
    echo "-----------------------------------------"
    
    set -x
    

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

    if [ "${A1_HOST_OS:0:4}" != "CCI_" ] ;then
        insecure_port=${15-17772}
        secure_port=${16-17773}
        client_certificate_port=${17-17774}
        ldap_secure_port=${18-17775}
        ltpa_secure_port=${19-17776}
        client_certificate_CN_fanout_port=${20-17777}
        dynamic_port=${21-17778}
    else
        insecure_port=${15-22100}
        secure_port=${16-22200}
        client_certificate_port=${17-22300}
        ldap_secure_port=${18-22400}
        ltpa_secure_port=${19-17776}
        client_certificate_CN_fanout_port=${20-22500}
        dynamic_port=${21-17778}
    fi
    set +x

    echo "-----------------------------------------"
    echo "`date` AUTOMATION overrides which may further affect behavior env:"
    echo "-----------------------------------------"

    env | grep -E 'AF_AUTOMATION_TEST|SVT_AT|AF_'

    if [ -n "$SVT_AT_VARIATION_HA" ] ;then
        echo "-----------------------------------------"
        echo " SVT AUTOMATION OVERIDE: Will use overide setting for HA : SVT_AT_VARIATION_HA = $SVT_AT_VARIATION_HA"
        echo "-----------------------------------------"
        ima2=${SVT_AT_VARIATION_HA}
    fi
    if [ -n "$SVT_AT_VARIATION_HA2" ] ;then # alternate form to set ha appliance if you don't want automatic testing to happen
        echo "-----------------------------------------"
        echo " SVT AUTOMATION OVERIDE: Will use overide setting for HA : SVT_AT_VARIATION_HA2 = $SVT_AT_VARIATION_HA2"
        echo "-----------------------------------------"
        ima2=${SVT_AT_VARIATION_HA2}
    fi

    if [ -n "$SVT_AT_VARIATION_ALL_M" ] ;then
        echo "-----------------------------------------"
        echo " SVT AUTOMATION OVERIDE: Will use overide setting : SVT_AT_VARIATION_ALL_M = $SVT_AT_VARIATION_ALL_M"
        echo "-----------------------------------------"
        env_vars=` env | grep -E 'AF_AUTOMATION_TEST|SVT_AT|AF_' | grep -v SVT_AT_VARIATION_ALL_M= | awk '{ printf(" %s ", $0);}';`
        my_pid=""
        outlog=${LOGP}.workload.${workload}.remote.start;
        for v in `svt_automation_systems_list` ; do
            echo "-----------------------------------------"
            echo "SVT_AT_VARIATION_ALL_M: Starting command on all Ms and will wait for it to complete on $v: ssh cd `pwd`; $env_vars $0 $@" ;
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
        for v in `svt_automation_systems_list `; do
            #------------------------------------------------
            # - note below because searchLogs is expecting only one message of type SVT_TEST_RESULT:, i must not repeat all
            # the logs with that result, instead i do the find replace on the COPIED results to prevent searchLogs count failures.
            #------------------------------------------------
            echo -en "\nSVT Copied Autommation Result for $v : " ; ssh $v "cd `pwd`; cat $outlog |grep \"SVT_TEST_RESULT: \" " | sed "s/SVT_TEST_RESULT:/COPY_OF_RESULT:/g"
        done
        } > .tmp.$outlog.results
        echo "---------------------------------------"
        echo " `date` Results are : "
        echo "---------------------------------------"
        cat .tmp.$outlog.results 
        
        fail_count=`cat .tmp.$outlog.results | grep "Failure"  | wc -l`
        rm -rf .tmp.$outlog.results

        svt_exit_with_result $fail_count "$AF_AUTOMATION_TEST" ; 

    fi

else
    echo "-----------------------------------------"
    echo "`date` Start processing input args for manual test"
    echo "-----------------------------------------"
    workload=${1-"throughput"}
    workload_subname=${2-""}
    num_iterations=${3-1000}
    ima1=${4-10.10.1.99}
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
    insecure_port=${15-17772}
    secure_port=${16-17773}
    client_certificate_port=${17-17774}
    ldap_secure_port=${18-17775}
    ltpa_secure_port=${19-17776}
    client_certificate_CN_fanout_port=${20-17777}
    dynamic_port=${21-17778}
    #current_port=$insecure_port
fi

if ! [ -n "$SVT_AT_VARIATION_WORKLOAD" ] ;then
    #-- Export this variable if it was not already exported because it may be used by other scripts (even for manual).
    export SVT_AT_VARIATION_WORKLOAD="$workload"
fi


if [ -n "$SVT_AT_VARIATION_SECURITY" -a "$THIS" == "M1" ] ;then 
  if (($SVT_AT_VARIATION_SECURITY<12)); then
    let mycounter=0;
    for MinimumProtocolMethod in SSLv3 TLSv1 TLSv1.1 TLSv1.2; do
   
        for Ciphers in Best Medium Fast; do
            if [ "$mycounter" == "$SVT_AT_VARIATION_SECURITY" ] ; then
                echo "-----------------------------------------"
                echo " $0 - SVT_AT_VARIATION_SECURITY: $SVT_AT_VARIATION_SECURITY:  Changing configuration of imaserver update SecurityProfile Name=SVTcmqttDynamic MinimumProtocolMethod=${MinimumProtocolMethod} UseClientCertificate=False UsePasswordAuthentication=False Ciphers=${Ciphers} CertificateProfile=SVTcmqtt UseClientCipher=False LTPAProfile="
                echo "-----------------------------------------"
                ssh admin@$ima1 "imaserver update SecurityProfile \"Name=SVTcmqttDynamic\" \"MinimumProtocolMethod=${MinimumProtocolMethod}\" \"UseClientCertificate=False\" \"UsePasswordAuthentication=False\" \"Ciphers=${Ciphers}\" \"CertificateProfile=SVTcmqtt\" \"UseClientCipher=False\" \"LTPAProfile=\""
                echo RC=$? 
                echo "-----------------------------------------"
                echo " $0 - Display changes "
                echo "-----------------------------------------"
                ssh admin@$ima1 "imaserver show SecurityProfile \"Name=SVTcmqttDynamic\""
                echo RC=$? 
                break;
            fi
            let mycounter=$mycounter+1;
        done
        if [ "$mycounter" == "$SVT_AT_VARIATION_SECURITY" ] ; then 
            break;
        fi
    done
  else
    echo "ERROR: Testcase error. SVT_AT_VARIATION_SECURITY must be a value between 0 - 11 (4*3) possible values" 
    exit 1;
  fi
fi

if [ -n "$SVT_AT_VARIATION_PORT" ] ;then
    echo "-----------------------------------------"
    echo " $0 - Will use user override SVT_AT_VARIATION_PORT for port setting = $SVT_AT_VARIATION_PORT"
    echo "-----------------------------------------"
    current_port=$SVT_AT_VARIATION_PORT
elif echo "$workload" | grep ltpa_ssl > /dev/null ; then
    echo "-----------------------------------------"
    echo " $0 - Will use ltpa_ssl setting for current_port=$ltpa_secure_port (ltpa_secure_port) "
    echo "-----------------------------------------"
    current_port=$ltpa_secure_port
elif echo "$workload" | grep ldap_ssl > /dev/null ; then
    echo "-----------------------------------------"
    echo " $0 - Will use ldap_secure setting for current_port=$ldap_secure_port (ldap_secure_port) "
    echo "-----------------------------------------"
    current_port=$ldap_secure_port
elif echo "$workload" | grep ssl > /dev/null ; then
    echo "-----------------------------------------"
    echo " $0 - Will use secure setting for current_port=$secure_port (secure_port) "
    echo "-----------------------------------------"
    current_port=$secure_port
elif echo "$workload" | grep client_certificate > /dev/null ; then
    if [ "$workload"  == "connections_client_certificate_CN_fanout" ] ; then
        echo "-----------------------------------------"
        echo " $0 - Will use client certificate CN fanout setting for current_port=$client_certificate_CN_fanout_port (client_certificate_CN_fanout_port) "
        echo "-----------------------------------------"
        current_port=$client_certificate_CN_fanout_port
    else
        echo "-----------------------------------------"
        echo " $0 - Will use client certificate setting for current_port=$client_certificate_port (client_certificate_port) "
        echo "-----------------------------------------"
        current_port=$client_certificate_port
    fi
else
    echo "-----------------------------------------"
    echo " $0 - Defaulting to insecure port for all further work"
    echo "-----------------------------------------"
    current_port=$insecure_port
fi

DELAY=5

if [ -n "$ima1" -a -n "$ima2" ] ;then
    ima_security="${ima1}:${secure_port} ${ima2}:${secure_port}"
    ima_client_certificate="${ima1}:${client_certificate_port} ${ima2}:${client_certificate_port}"
    ima_default="${ima1}:${insecure_port} ${ima2}:${insecure_port} "
    ima_default_v2=" -s ${ima1}:${insecure_port} -x haURI=${ima2}:${insecure_port} "
    ima_25776="$ima_default"
    ima_30626="${ima1}:${insecure_port} "
    ima_many="${ima1} ${ima2}"  
    ima_many2="${ima2} ${ima1}" 
else
    ima_security="${ima1}:${secure_port} \"\" "
    ima_client_certificate="${ima1}:${client_certificate_port} \"\" "
    ima_default="${ima1}:${insecure_port} \"\" "
    ima_default_v2=" -s ${ima1}:${insecure_port} "
    ima_25776="$ima_default"
    ima_30626="${ima1}:${insecure_port} "
    ima_many="${ima1} NULL " # NULL Means no HA appliance present
    ima_many2=""
fi

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

    outlog="${LOGP}.workload.${workload}"
    rm -rf $outlog

    {

    #echo "------------------"
    #echo "`date` start - record netstat -tnp before starting"
    #echo "------------------"
    #data=`netstat -tnp`
    #pre_net_stat_count=`echo "$data" |wc -l`
    #echo "------------------"
    #echo "`date` complete - record netstat -tnp before starting"
    #echo "------------------"
    
    echo "------------------"
    echo " Start workload: $workload : test_process_name: ${test_process_name} : ${workload_parms} "
    echo " ${test_process_name} ${workload_parms} 2>>$outlog 1>>$outlog "
    echo "------------------"
    ${test_process_name} ${workload_parms} &
    cur_pid=$!
    my_pid+="$cur_pid "
    substat="${LOGP}.s."
    pubstat="${LOGP}.p."

    wait $my_pid
    my_exit=$?

    #while((1)) ; do    
        #data=`netsat -tnp`
        #post_net_stat_count=`echo "$data" |wc -l`
        #if (($post_
    #done

    echo "`date` end iteration $x - exit code was ${my_exit}" 

    if [ -n "$SVT_AT_VARIATION_QUICK_EXIT" -a "$SVT_AT_VARIATION_QUICK_EXIT" == "true" ] ;then
        echo "---------------------------------"
        echo "`date` $0 - Skipping evaluation of messaging results at this time - Quick Exit"
        echo "---------------------------------"
    elif [ -n "$outlog" ] ;then
        if [ "$do_stats" == "1" ] ;then
            for v in "*" 0 1 2 ; do
            {
            echo "-----------------------------"
            echo "Subscriber statistics - ${substat}${v}.${workload}.*[0-9]"
            echo "-----------------------------"
            ./monitor.c.client.sh "${substat}${v}.${workload}.*[0-9]"
            echo "-----------------------------"
            echo "Publisher statistics - ${pubstat}${v}.${workload}*[0-9]"
            echo "-----------------------------"
            ./monitor.c.client.sh  "${pubstat}${v}.${workload}.*[0-9]"
            } >> $outlog
            done
        fi
        if [ -n "$SVT_PF_CRITERIA_ORDERING" ] ;then
            echo "---------------------------------"
            echo "`date` $0 - Evaluating pass fail (PF) criteria SVT_PF_CRITERIA_ORDERING on files $SVT_PF_CRITERIA_ORDERING"
            echo "---------------------------------"
            count=`grep "SVT_TEST_CRITERIA: Failed av.testCriteriaOrderMsg" $SVT_PF_CRITERIA_ORDERING`
            if (($count>0)); then
                echo "---------------------------------"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_ORDERING on files $SVT_PF_CRITERIA_ORDERING"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_ORDERING: count:$count errors detected"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_ORDERING: Detailed list of failures below"
                echo "---------------------------------"
                grep "SVT_TEST_CRITERIA: Failed av.testCriteriaOrderMsg" $SVT_PF_CRITERIA_ORDERING
                pfcount++;
            fi
        fi

        if [ -n "$SVT_PF_CRITERIA_MSGCOUNT" ] ;then
            echo "---------------------------------"
            echo "`date` $0 - Evaluating pass fail (PF) criteria SVT_PF_CRITERIA_MSGCOUNT on files $SVT_PF_CRITERIA_MSGCOUNT"
            echo "---------------------------------"
            count=`grep "SVT_TEST_CRITERIA: Failed av.testCriteriaMsgCount" $SVT_PF_CRITERIA_MSGCOUNT`
            if (($count>0)); then
                echo "---------------------------------"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_MSGCOUNT on files $SVT_PF_CRITERIA_MSGCOUNT"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_MSGCOUNT: count:$count errors detected"
                echo "`date` $0 - FAILED pass fail (PF) criteria SVT_PF_CRITERIA_MSGCOUNT: Detailed list of failures below"
                echo "---------------------------------"
                grep "SVT_TEST_CRITERIA: Failed av.testCriteriaMsgCount" $SVT_PF_CRITERIA_MSGCOUNT
                pfcount++;   
            fi

        fi
    fi

    } 2>>$outlog 1>>$outlog
    
    if [ "$my_exit" == "0" -a "$pfcount" == "0" ] ; then
        return 0;
    else
        return 1;
    fi 
}


do_rtc_43965_MQTT_async_sanity_test(){
    local app=${1-MQTTV3SSAMPLE}
    local qos=${2-0}
    local flag=${3-""}
    local parms

    #MQTTV3SSAMPLE -a subscibe -t /MQTTV3ASAMPLE -b 10.10.10.10 -p 17773 -s 1 -v 1 -r ssl/trustStore.pem -m "Houston we have a problem"
    #MQTTV3SSAMPLE -a publish -t /MQTTV3ASAMPLE -b 10.10.10.10 -p 17773 -s 1 -v 1 -r ssl/trustStore.pem -m "Houston we have a problem"

    local now=`date +%s`

    if [ "$app" == "MQTTV3SSAMPLE" -o "$app" == "MQTTV3AsyncSSAMPLE" ] ;then
        parms=" -p 17773 -v 1 -r ssl/trustStore.pem "
    else
        parms=" -p 17772 "
    fi
    
    if [ "$flag" == "clientcertificate" ] ;then
        if [ "$app" == "MQTTV3SSAMPLE" -o "$app" == "MQTTV3AsyncSSAMPLE" ] ;then
            parms=" -p 17774 -v 1 -r ssl/trustStore.pem -k ssl/imaclient-keystore.pem "
        else
            echo "Warning: client certificate auth not supported for $app"
            return 0;
        fi
    fi
    
    echo $app -a subscribe -t /$app -b $ima1 -s $qos $parms > ${LOGP}.s.$qos.$app.$now 
    $app -a subscribe -t /$app -b $ima1 -s $qos $parms >> ${LOGP}.s.$qos.$app.$now &
    svt_verify_condition2 "Connected to " 1 ${LOGP}.s.$qos.$app.$now $now $qos $app
    svt_verify_condition2 "Subscribing to topic" 1 ${LOGP}.s.$qos.$app.$now $now $qos $app
    echo $app -a publish -t /$app -b $ima1 -s $qos $parms -m "Houston we have a problem" > ${LOGP}.p.$qos.$app.$now 
    $app -a publish -t /$app -b $ima1 -s $qos $parms -m "Houston we have a problem" >> ${LOGP}.p.$qos.$app.$now &
    svt_verify_condition2 "Connected to " 1 ${LOGP}.p.$qos.$app.$now $now $qos $app
    svt_verify_condition2 "Publishing to topic" 1 ${LOGP}.p.$qos.$app.$now $now $qos $app
    svt_verify_condition2 "Houston we have a problem" 1 ${LOGP}.s.$qos.$app.$now $now $qos $app
    
    killall -2 $app # this sample must be killed with CTRL-C when done
    echo "Successfully completed test"
    return 0;

}




declare -a inout_47726_array=(
#--------------|--------------
# Input topic  | Output Topic
#--------------|---------------
#              |
#              + Note: In between the input and output topic is an MQ queue or topic
#                that creates a circular messaging path as specified by the 
#                destination mapping rule for each pair. The DMRs are specified in 
#                svt_tools/common_src/SVT_IMA_environment.cli .
#------------------------------
/LOOP_${M1_HOST}/MSt1a /LOOP_${M1_HOST}/MSt1b
/LOOP_${M1_HOST}/MSt2a /LOOP_${M1_HOST}/MSt2b
/LOOP_${M1_HOST}/MST3a /LOOP_${M1_HOST}/MSt3b
/LOOP_${M1_HOST}/MST4a /LOOP_${M1_HOST}/MST4b/#
/LOOP_${M1_HOST}/MST5a /LOOP_${M1_HOST}/MSt5b
/LOOP_${M1_HOST}/MST6a /LOOP_${M1_HOST}/MSt6b
/LOOP_${M1_HOST}/MST7a /LOOP_${M1_HOST}/MST7b/#
NULL NULL
NULL NULL
NULL NULL
NULL NULL
NULL NULL
);

do_47726_test_dmr_check(){
    local data
    local server
    local errcount=0

    for server in $ima1 $ima2 ; do 
        data=`ssh admin@$server imaserver stat DestinationMappingRule "Name=CMQTTLoop*"`
        #The IBM MessageSight server status is "Standby". Getting statistics of "server" is not allowed.
        if echo "$data" | grep "The IBM MessageSight server status is \"Standby\". Getting statistics" > /dev/null; then
            continue;
        else
            echo TODO not implemented
        fi

    done


}

do_47726_test_wait(){
    local quantity=${1-0}
    local sublogs="log.*.s.*"
    local line
    local actual
    local expected=0
    local data
    local x

    let x=0
    while [ "${inout_47726_array[$x]}" != "NULL" ] ; do
        let expected=$expected+1;
        let x=$x+2
    done

    while((1)); do
        data=`
        {
        find $sublogs | xargs -i echo "tac {} | grep -oE 'SVT_MQTT_C_STATUS,.*'  | head -1" |sh | 
            awk --field-separator=',' '{print $4}' | while read line ; do
                echo "read line: $line"
                if (($line>=$quantity)); then 
                        echo yes; 
                else 
                        echo no; 
                fi ; 
            done 
        }`
        echo "data is $data"
        actual=`echo "$data" |grep yes |wc -l`
        echo -n "`date` :  actual is $actual, expected is $expected"
        if [ "$actual" == "$expected" ] ; then
            echo " - Success . Completed wait"
            return 0 ;
        else
            echo " - Pending . Still waiting for expected results"
        fi
        sleep 1;
    done
    
    return 0;
}


do_47726_test_dmr_control(){
    local flag=${1-"ENABLE"} ; # ENABLE or DISABLE
    local cmd=""
    declare -a dmr_47726_array=(
CMQTTLoop1a
CMQTTLoop1b
CMQTTLoop2a
CMQTTLoop2b
CMQTTLoop3a
CMQTTLoop3b
CMQTTLoop4a
CMQTTLoop4b
CMQTTLoop5a
CMQTTLoop5b
CMQTTLoop6a
CMQTTLoop6b
CMQTTLoop7a
CMQTTLoop7b
CMQTTLoop8a
CMQTTLoop8b
CMQTTLoop9a
CMQTTLoop9b
CMQTTLoop10a
CMQTTLoop10b
NULL NULL
NULL NULL
NULL NULL
NULL NULL
NULL NULL
);
    local x;
    local eflag="False"
    
    if [ "$flag" == "ENABLE" ] ;then eflag="True" ; fi

    let x=0
    while [ "${dmr_47726_array[$x]}" != "NULL" ] ; do
        cmd+="imaserver update DestinationMappingRule Name=${dmr_47726_array[$x]} Enabled=${eflag} ; "
        let x=$x+1
    done
   
    echo "TODO / WARNING: no error checking for this operation"
    echo "Cmd is : $cmd" 
    ssh admin@$ima1 "$cmd"
    ssh admin@$ima2 "$cmd"

    return 0;
}

do_47726_init_mq(){

    if [ -n "$MQSERVER1_IP" ] ;then
        ssh -t -t root@$MQSERVER1_IP "sudo -u mquser /var/mqm/configureMQ.sh"
		return $?
    else
		echo "ERROR: MQSERVER1_IP not defined: $MQSERVER1_IP"
		return 1
    fi

}
do_47726_cycle_mq(){
    #-------------------------------------
    #  The 4 step procedure below seemed to prevent issues that i was seeing
    #  lately where the listener MQ process would be left in a "zombie state
    #-------------------------------------

    let rc=0;
    #-------------------------------------
    # 1.endmqm without -i on MQSERVER1_IP
    #-------------------------------------
    ssh $MQSERVER1_IP "su -l mquser -c '/opt/mqm/bin/endmqm SVTBRIDGE.QUEUE.MANAGER ; '"
    let rc=$rc+$?

    #-------------------------------------
    # 2.Wait for mq processes to die on MQSERVER1_IP
    #-------------------------------------
    ssh $MQSERVER1_IP "let count=0; while((1)); do
        echo Verifying that Websphere MQ QM SVTBRIDGE.QUEUE.MANAGER has stopped;
        if ps -ef  | grep SVTBRIDGE.QUEUE.MANAGER |grep -v grep ; then
             echo Waiting pid containing SVTBRIDGE.QUEUE.MANAGER to die: \$count ;
        else 
             echo Completed waiting for mq processes to die ; 
             break;
        fi;
        if ((\$count>360)); then
           break;
        fi;
        if ((\$count>60)); then
            echo Issuing SIGKILL to mq processes that have not died
            ps -ef  | grep SVTBRIDGE.QUEUE.MANAGER |
                grep -v grep | awk '{print \$2}' |
                xargs -i kill -9 {};
        fi;
        sleep 1;
        let count=\$count+1;
    done "


    #-------------------------------------
    # 3.strmqm on MQSERVER1_IP
    #-------------------------------------
    ssh $MQSERVER1_IP "su -l mquser -c '/opt/mqm/bin/strmqm SVTBRIDGE.QUEUE.MANAGER;'"
    let rc=$rc+$?

    #-------------------------------------
    # 4.The next little monstrosity waits for 15 mq processes to start on MQSERVER1_IP
    #-------------------------------------
	#--------------------------------------------
	# convert the next step to use a better way. TODO
	#--------------------------------------------
	# [mquser@svtmq root]$ /opt/mqm/bin/dspmq
	#QMNAME(SVTBRIDGE.QUEUE.MANAGER)                           STATUS(Running)
	#[mquser@svtmq root]$

    ssh $MQSERVER1_IP "
    data=\`ps -ef | grep SVTBRIDGE.QUEUE.MANAGER |grep -v grep |wc -l\`; 
        echo \$data; 
        while((\$data<13)); do # FIXME there must be some better way to verfy that MQ is ready... TODO
            echo waiting for at least 13 pids with SVTBRIDGE.QUEUE.MANAGER to start; 
            data=\`ps -ef  | grep SVTBRIDGE.QUEUE.MANAGER |grep -v grep |wc -l\`;  
            sleep 1; 
            echo \$data; 
        done ; 
        echo 15 expected processes with SVTBRIDGE.QUEUE.MANAGER started " ;


    return $rc
}



do_47726_test_setup(){
    local flag=${1-"SETUP"} ; # SETUP or CLEANUP
    local x;
    declare -a queue_47726_array=(
    LOOP_${M1_HOST}_MQQ1
    LOOP_${M1_HOST}_MQQ5
    LOOP_${M1_HOST}_MQQ10
    NULL NULL
    NULL NULL
    NULL NULL
    NULL NULL
    );

    do_47726_init_mq
    x=$?
    if [ "$x" != "0" ]; then
		echo "ERROR: do_47726_test_setup do_47726_init_mq"
		return $x;
    fi

    #----------------------------------
    # make sure MQ instance is running.    
    #----------------------------------
    do_47726_cycle_mq
    
    #----------------------------------
    #configure MQ Queues
    #----------------------------------

    let x=0
    while [ "${queue_47726_array[$x]}" != "NULL" ] ; do
        echo "x:$x process ssh root@$MQSERVER1_IP \"su -l mquser -c echo -e \"DELETE QL(${queue_47726_array[$x]}) \n DEFINE QL(${queue_47726_array[$x]}) \" | /opt/mqm/bin/runmqsc SVTBRIDGE.QUEUE.MANAGER \""
        if [ "$flag" == "SETUP" ] ;then
            ssh root@$MQSERVER1_IP "su -l mquser -c 'echo -e \"DELETE QL(${queue_47726_array[$x]}) \n DEFINE QL(${queue_47726_array[$x]}) \" | /opt/mqm/bin/runmqsc SVTBRIDGE.QUEUE.MANAGER' "
        else  # DEFAULT : CLEANUP
            ssh root@$MQSERVER1_IP "su -l mquser -c 'echo -e \"DELETE QL(${queue_47726_array[$x]}) \n \" | /opt/mqm/bin/runmqsc SVTBRIDGE.QUEUE.MANAGER' "
        fi
    
        let x=$x+1
    done

    return 0;

}

do_47726_test(){
    local action=${1-"SUB"} # SUB , PUB, or WAIT
    local nummsg=${2-6000}
    local qos=${3-2}
    local pubrate=${4-10}
    local nummsgextra=$nummsg
    local x
    local now=00000000  # just a string to match on , not a time
    local extra_pubflag=" "
    local name="47726" ; # must match same name as in do_47726_test_wait do_47726_test
    local baseflag=""
    local subflag=""
    local pubflag=""
    local launched=0

    if (($nummsg>0)); then
        let nummsgextra=($nummsg+10)
    fi

    baseflag=" -s $ima1:$insecure_port -n $nummsg -x haURI=$ima2:$insecure_port " 
    baseflag+=" -x reconnectWait=10 -x retryConnect=360 -x verifyMsg=1 -x orderMsg=2 "
    baseflag+=" -q $qos -x keepAliveInterval=10  "
    baseflag+=" -x connectTimeout=10 " 

    if [ "$nummsg" == "0" ] ;then
        baseflag+=" -c true "
        now=`date +%s` ; # we dont want clean up logs to overwrite the real logs
    else
        baseflag+=" -c false "
    fi
    
    let x=0
    while [ "${inout_47726_array[$x]}" != "NULL" ] ; do
        echo " Processing ${inout_47726_array[$x]} ${inout_47726_array[$x+1]}   "  >> /dev/stderr
        if [ "$action" == "SUB" ] ;then
            subflag=" -v -o ${LOGP}.s.${qos}.${name}.${x}.${now} -a subscribe -t ${inout_47726_array[$x+1]} "
            subflag+=" -x subscribeOnConnect=1 $baseflag "
            subflag+="  -i loop.s.$x -x sharedMemoryKey=loop.$x -x statusUpdateInterval=1 " 
            subflag+="  -n $nummsgextra -x verifyPubComplete=log.p.${name}.complete -x verifyStillActive=60 "
            subflag+=" -x testCriteriaMsgCount=$nummsg " 
            mqttsample_array $subflag -m "Hola amigo"  &
            
        elif [ "$action" == "PUB" ] ;then
            if echo ${inout_47726_array[$x]} | grep MST > /dev/null ;then
                extra_pubflag=" -x topicMultiTest=10 "
            fi
            pubflag=" -v -o ${LOGP}.p.${qos}.${name}.${x}.${now} -a publish -t ${inout_47726_array[$x]} "
            pubflag+="  -w $pubrate -x publishDelayOnConnect=30 $baseflag $extra_pubflag "
            pubflag+="  -i loop.p.$x -x sharedMemoryKey=loop.$x -x statusUpdateInterval=1 " 
            pubflag+=" -x sharedMemoryVal=10 "
            pubflag+=" -x sharedMemoryCoupling=1 "
            mqttsample_array $pubflag -m "Hola amigo"  &
        elif [ "$action" == "WAIT" ] ;then
            # No-operation here. Will later wait for the pending publishers/subscribers to complete.
            # as long as SVT_AT_VARIATION_QUICK_EXIT is != true
            echo -n ""
        else
            echo "ERROR: unknown action $action "  >> /dev/stderr
            return 1;
        fi
        let launched=$launched+1
        let x=$x+2
        #break;
    done

    if [ "$action" != "WAIT" ] ;then
        if [ "$action" == "SUB" ] ;then
            svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array $launched
        elif [ "$action" == "PUB" ] ;then
            let x=($launched*2)
            echo "launched is $launched"
            svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array $x
        fi
    fi

    if [ "$action" == "SUB" ] ;then
        svt_verify_condition "Connected to " $launched "${LOGP}.s.${qos}.${name}.*.${now}"
        svt_verify_condition "Subscribed - Ready" $launched "${LOGP}.s.${qos}.${name}.*.${now}" 
        svt_verify_condition "All Clients Subscribed" $launched "${LOGP}.s.${qos}.${name}.*.${now}" 
    elif [ "$action" == "PUB" ] ;then
        svt_verify_condition "All Clients Connected" $launched "${LOGP}.p.${qos}.${name}.*.${now}" 
    fi

    if [ "$action" == "WAIT" ] ;then
        echo "Verifying that the operation completed "
        svt_verify_condition "SVT_TEST_RESULT: " $launched "log.*.p.${qos}.${name}.*.${now}"
        touch log.p.${name}.complete
        svt_verify_condition "SVT_TEST_RESULT: " $launched "log.*.s.${qos}.${name}.*.${now}"
    fi
    
    return 0;
} 

do_69925_cleanup_stragglers() {
    #---------------------------------------------------
    # 
    #---------------------------------------------------
    echo "`date` Read out remaining retained messages"
    tmplog=${AF_AUTOMATION_TEST}.subscribe.to.cleanup.final.retained.messages.query.log
    mqttsample_array -s $ima1:$insecure_port -a subscribe -t "#" -n 1000000000000 -v -x retainedMsgCounted=1 -x statusUpdateInterval=1 -x verifyStillActive=5 -o $tmplog  -x warningWait=1 -x warningWaitDynamic=1  -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=360000 -x keepAliveInterval=300 -x subscribeOnReconnect=1 -x msgTimeoutAfterSubscribe=60000000
    cat $tmplog |grep -oE "received on topic.*"  | sed "s/[':]//g" | awk '{print $4}' | sort -u > TOPICLIST
    count=`cat TOPICLIST |wc -l`
    echo "`date` Done! Read out $count :remaining retained messages. Clear them now"
    tmplog=${AF_AUTOMATION_TEST}.subscribe.to.cleanup.final.retained.messages.cleanout.log
    mqttsample_array  -s $ima1:$insecure_port -a publish -x msgFile=NULLMSG -x topicFile=TOPICLIST -x retainedFlag=1 -v -n $count -o $tmplog -x warningWait=1 -x warningWaitDynamic=1  -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=360000 -x keepAliveInterval=300 
    echo "`date` Done! There should not be any retained messages left on the system now"
}

do_51598_additional_test(){
    local nummsgs=${1-1}
    local maxmsgs=${2-1}
    local limit_testcase_nummsgs=${3-99999} ; # a limit of when this testcase will run in terms of # of messages
    local qos
    
    #------------------------------ 
    # Verify defect 53041 is fixed for all qos (publish exactly max messages, expect to recieve them)
    #------------------------------ 
    if [ "$maxmsgs" == "$nummsgs" ] ;then
        if (($nummsgs<$limit_testcase_nummsgs)) ;then
            for qos in 0 1 2 ; do
                mqttsample_array -s $ima1:$current_port -i sub_53041_$qos -n 0 -c true -a subscribe -v -t /53041_$qos -q $qos -o ${LOGP}.s.53041_$qos.1
                mqttsample_array -s $ima1:$current_port -i sub_53041_$qos -n 0 -c false -a subscribe -v -t /53041_$qos -q $qos -o ${LOGP}.s.53041_$qos.2
                mqttsample_array -s $ima1:$current_port -i pub_53041_$qos -n $nummsgs -c false -a publish -v -t /53041_$qos -q $qos -o ${LOGP}.p.53041_$qos.3
                mqttsample_array -s $ima1:$current_port -i sub_53041_$qos -n $nummsgs -c false -a subscribe -v -t /53041_$qos -q $qos -o ${LOGP}.s.53041_$qos.4 -x unsubscribe=1
            done
        else
            echo "`date` : 53041 verfication test is only executed when nummsgs < $limit_testcase_nummsgs, in the interest of test execution time "
        fi
    else
        echo "`date` : 53041 verfication test is only executed when maxmgs:$maxmsgs == nummsgs:$nummsgs"
    fi

    #------------------------------ 
    # Verify defect 53068 is fixed for all qos 
    #------------------------------ 

    qos=0;
    mqttsample_array -o ${LOGP}.s.${qos}.concurrent.slow.51598 -s $ima1:$current_port -a subscribe -x keepAliveInterval=60 -x connectTimeout=60 -x orderMsg=3 -x reconnectWait=2 -x retryConnect=360 -q $qos -n 1000000000000000 -x statusUpdateInterval=1 -w 1 -x subscribeOnReconnect=1 &
    mqttsample_array -o ${LOGP}.s.${qos}.concurrent.fast.51598 -s $ima1:$current_port -a subscribe -x keepAliveInterval=60 -x connectTimeout=60 -x orderMsg=3 -x reconnectWait=2 -x retryConnect=360 -q $qos -x sharedMemoryKey=concurrent.51598 -n 1000000000000000 -x statusUpdateInterval=1 -x sharedMemoryType=1 -x subscribeOnReconnect=1 &
    svt_verify_condition2 "Connected to " 2 "${LOGP}.s.$qos.concurrent*.51598" 51598 $qos "mqttsample_array"
    svt_verify_condition2 "Subscribed - Ready" 2 "${LOGP}.s.$qos.concurrent*.51598" 51598 $qos "mqttsample_array"
    svt_verify_condition2 "All Clients Subscribed" 2 "${LOGP}.s.$qos.concurrent*.51598" 51598 $qos "mqttsample_array"

    mqttsample_array   -o ${LOGP}.p.${qos}.concurrent.51598 -s $ima1:$current_port -a publish -x keepAliveInterval=60 -x connectTimeout=60  -x orderMsg=3  -x reconnectWait=2 -x retryConnect=360 -q $qos -x sharedMemoryKey=concurrent.51598 -n 1000000000000000 -x statusUpdateInterval=1  -x sharedMemoryType=1  -x sharedMemoryVal=50000 &
    svt_verify_condition2 "All Clients Connected" 1 "${LOGP}.p.$qos.concurrent*.51598" 51598 $qos "mqttsample_array"


    #------------------------------ 
    # 
    #------------------------------ 

         
}

check_haFunctions_rc(){
    local filename=$1
    local action=$2  
    local data
    local rcount
    local scount

    if ! [ -e ${filename} ] ;then
        echo "`date`: ERROR: $filename does not exist"
        return 1;
    fi
           
    data=`cat ${filename}`
    if [ -z "$data" ] ; then
        echo "`date`: ERROR: $filename has no data"
        return 1;
    elif [[ "$data" =~ "Test result is Failure" ]] ; then
        echo "`date`: WARNING: $filename shows failure string: Test result is Failure"
	if [ "$cmd" == "stopPrimary" ] ; then
	 	while((1)); do
    		data=`ssh admin@$ima1 status imaserver; ssh admin@$ima2 status imaserver;`
    		rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
    		scount=`echo "$data" | grep "Status = Standby"  | wc -l`
			echo "`date` waiting for rcount==1 current:$rcount and scount==0, current:$scount"
        	if [ "$rcount" == "1" -a "$scount" == "0" ] ;then
				return 0; // success
			fi
			sleep 1
		done
	elif [ "$cmd" == "startStandby" ] ; then
	 	while((1)); do
    		data=`ssh admin@$ima1 status imaserver; ssh admin@$ima2 status imaserver;`
    		rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
    		scount=`echo "$data" | grep "Status = Standby"  | wc -l`
			echo "`date` waiting for rcount==1 current:$rcount and scount==1, current:$scount"
        	if [ "$rcount" == "1" -a "$scount" == "1" ] ;then
				return 0; // success
			fi
			sleep 1
		done
	else
        	echo "`date`: ERROR: $filename shows failure string: Test result is Failure. No extra handling for this error."
	fi
        return 1;
    fi
    return 0; // success

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
    elif [ "$cmd" == "powerCyclePrimary" -a "${A1_HOST_OS:0:4}" == "CCI_" ] ;then   
        if ! [ "$rcount" == "1" -a "$scount" == "1" ] ;then
            echo "`date`: run_ha_cmd: WARNING: not running powerCyclePrimary for Softlayer because we are not in HA mode."
            return 1;
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
    elif [ "$cmd" == "deviceRestartPrimary"  ] ;then
        ${M1_TESTROOT}/scripts/haFunctions.sh -a $cmd -f ${AF_AUTOMATION_TEST}_${cmd}_haFunctions.log
		while((1)); do
    		data=`ssh -o connectTimeout=5 admin@$ima1 status imaserver; ssh -o connectTimeout=5 admin@$ima2 status imaserver;`
    		rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
    		scount=`echo "$data" | grep "Status = Standby"  | wc -l`
        	if [ "$rcount" == "1" -a "$scount" == "1" ] ;then
        		echo "------------------------------------------------------"
        		echo "`date` : complete. deviceRestartPrimary success"
        		echo "------------------------------------------------------"
			return 0;
			fi
		done
    elif [ "$cmd" == "powerCyclePrimary" -a "${A1_HOST_OS:0:4}" == "CCI_" ] ;then   
        echo "------------------------------------------------------"
        echo "`date` : special handling for softlayer and this command - optimally move this to haFunctions.sh"
        echo "------------------------------------------------------"
        echo "------------------------------------------------------"
       	echo "`date` : Check if A1 is primary: $ima1"
       	echo "------------------------------------------------------"
		id=$A1_SL_ID
		type=$A1_TYPE
    	data=`ssh admin@$ima1 status imaserver;`
    	rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
		if [ "$rcount" != "1" ] ; then	
        	echo "------------------------------------------------------"
        	echo "`date` : Check if A2 is primary: $ima2"
        	echo "------------------------------------------------------"
			id=$A2_SL_ID
			type=$A2_TYPE
    		data=`ssh admin@$ima2 status imaserver;`
    		rcount=`echo "$data" | grep "Status = Running (production)"  | wc -l`
			if [ "$rcount" != "1" ] ; then	
        		echo "------------------------------------------------------"
        		echo "`date` : Error. could not locate primary"
        		echo "------------------------------------------------------"
				return 1;
			fi
		fi
		if  [ "$type" == "SOFTLAYERBM" ] ;then
        	echo "------------------------------------------------------"
	        echo "`date` : Issue power off for bare metal: sl server power-off $id --really"
	        echo "------------------------------------------------------"
			sl server power-off $id --really
			rc=$?
    	    echo "------------------------------------------------------"
	        echo "`date` : Completed power-off for bare metal: sl server power-off $id --really: RC: $rc"
	        echo "------------------------------------------------------"
				
	        echo "------------------------------------------------------"
    	    echo "`date` : Issue power on for bare metal: sl server power-on $id "
	        echo "------------------------------------------------------"
			sl server power-on $id 
			rc=$?
    	    echo "------------------------------------------------------"
	        echo "`date` : Completed power-on for bare metal: sl server power-on $id : RC: $rc"
	        echo "------------------------------------------------------"
			return $rc
		fi

	    echo "------------------------------------------------------"
    	echo "`date` : Error: unsupported type: $type"
	    echo "------------------------------------------------------"
		return 1;

    else
        ${M1_TESTROOT}/scripts/haFunctions.sh -a $cmd -f ${AF_AUTOMATION_TEST}_${cmd}_haFunctions.log
        check_haFunctions_rc ${AF_AUTOMATION_TEST}_${cmd}_haFunctions.log  $cmd
    fi
    return $?
}


let x=0;

echo  "x is $x num_iterations $num_iterations, workload $workload , workload_subname $workload_subname"
while(($x<$num_iterations));do
    echo "`date` start iteration $x" 

    if [ $workload == "security" ] ; then
        if ! [ -n "$workload_subname" ] ; then
            workload_subname="100 195 3 3 3 security 1 0 1 2" ; #default for this workload 19.5 K  connections * 3
        fi
        echo "------------------"
        echo " Start ssl workload : ${workload_subname} *3 connections doing 100 mesages each  on Qos 0 1 2"
        echo "------------------"
        run_standard_workload $workload "./SVT.17942.c.sh" "${ima_security} $workload_subname"
        let fail_count=($fail_count+$?)
    elif [ $workload == "RTC_34564"  -o $workload == "resource_topic_monitor" ] ;then 
        #--------------------------------------
        # Client should be explicitly cleaned up later with kill
        # 1 998000 subscribers,  200 publishers publishing to those (on 200 different topics) - This is already ready.
        # 2. 100 subscribers listening to $SYS/ResourceStatistics/# - 
        # 3. Verify that 1+2 is not much slower than 1 by itself 
        #--------------------------------------
        now=`date +%s`
        mqttsample_array ${ima_default_v2} -a subscribe -x subscribeOnConnect=1 -x statusUpdateInterval=1 -t "\$SYS/ResourceStatistics/#" -n 1000000000000 -z 10  -x warningWaitDynamic=1 -x connectTimeout=300 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300  -o ${LOGP}.appliance.SYS.ResourceStatistics.${now} &
        let fail_count=0;

    elif [ $workload == "throughput" ] ;then
        if ! [ -n "$workload_subname" ] ; then
            workload_subname="8 1 1000000 na na throughput 0 0"
        fi
        #------------------
        # Start messaging workload 8 connections doing 1 M messages each
        #------------------
        run_standard_workload $workload "./SVT.17942.c.sh" "${ima_default} $workload_subname"
        let fail_count=($fail_count+$?)
        
    elif [ $workload == "25776" ] ;then
        if ! [ -n "$workload_subname" ] ; then
            workload_subname="10 10 3000 300 300 -1 0 1 2"
        fi
        run_standard_workload $workload "./SVT.25776.sh" "${ima_25776} $workload_subname"
        let fail_count=($fail_count+$?)
    elif [ $workload == "30626"  ] ;then
        num_iterations=1;  # this is a one iteration only test.
        #------------------------------------------- 
        # The purpose of this workload is to fill the disk up to the value specified in workload_subname
        # Note: This test is not sensitive to failovers. (it is expected that a failover does not occur during this test, if it does, then that is a defect.)
        # Note: This test fills up the disk on the appliance at a rate of approximately 1% per minute.
        #------------------------------------------- 
        if ! [ -n "$workload_subname" ] ; then
            workload_subname=30 ; #default for this workload 30 % disk used.
        fi
        echo "------------------------------------------"
        echo "`date` - This workload should be run on a mar168 class system - a system that has at least that much cpu and memory as mar168."
        echo "`date` - If you have doubts you should hit CTRL-C now (Warning: chance of out of memory condition on other systems)"
        echo ""
        echo "`date` - sleeping 10 seconds. please confirm. Hit CTRL-C if you don't know - now."
        echo "------------------------------------------"
        sleep 10;
        echo "------------------------------------------"
        echo "`date` - Starting workload"
        echo "------------------------------------------"
       
        echo "------------------------------------------"
        echo "`date` - Start durable subscriber expecting no messages"
        echo "------------------------------------------"
        ./mqttsample_array -s $ima_30626 -a subscribe -q 2 -c false -i never_ending_s -n 0  -t /neverending -x verifyMsg=1 -x msgFile=./SMALLFILE  -x orderMsg=1  -v &

        echo "------------------------------------------"
        echo "`date` - Start publishing to durable subscriber without receiving the messages causing buffering to occurr"
        echo "------------------------------------------"
        ./mqttsample_array -s $ima_30626 -o ${LOGP}.p.neverending.30626 -a publish -q 2 -c false -i never_ending_p -n 100000000 -t /neverending -x verifyMsg=1 -x msgFile=./SMALLFILE  -x orderMsg=1 -w 1 -v
        echo "------------------------------------------"
        echo "`date` - Start publishing to to durable subcribers that are then properly cleaned up. This combination with the above should cause the disk to fill up."
        echo "`date` - Note this version of the test does not actually receive the messages, it just cleans up the durable subscribers emptying buffers"
        echo "------------------------------------------"
        { let x=0; while((1)); do  ./SVT.30626.b.sh ${ima_default} 20 100 0 0 10 0.5 2 1>>${LOGP}.$workload.$x 2>>${LOGP}.$workload.$x ; let x=$x+1; done } &
  
        #------------------------------------------- 
        # Wait for disk usage to get to 30.
        #------------------------------------------- 
        . ./ha_test.sh $ima1 $ima2 ; monitor_disk_usage_loop $workload_subname

    elif [ $workload == "test_exit1"  ] ;then
        run_standard_workload $workload  "test -d /dir_that_doesnt_exist" 
        let fail_count=($fail_count+$?)

    #elif [ $workload == "39574_setup"  ] ;then
        #
        #java svt.jms.ism.JMSSample -s tcp://10.10.10.10:16102 -a publish -q MARCOS -n 1000000 -r

    elif [ $workload == "svt_ha_run_random_test"  ] ;then
        run_standard_workload $workload.$workload_subname  "./SVT.ha_test.sh" "$ima1 $ima2 svt_ha_run_random_test $workload_subname" 0
        let fail_count=($fail_count+$?)
    elif [ $workload == "rtc43580_check_zero_buffered_msgs"  ] ;then
        num_iterations=1;  # this is a one iteration only test.
        svt_check_subscription_buffered_msgs $ima1 $workload 0
        let fail_count=($fail_count+$?)
    elif  [ $workload == "svt_monitor_for_85_percent_mem_utilization"  ] ;then
        outlog=log.svt_monitor_for_85_percent_mem_utilization.$ima1 
        { . /niagara/test/svt_cmqtt/svt_test.sh; svt_monitor_for_85_percent_mem_utilization $ima1 >>$outlog 2>>$outlog ; } &
        svt_exit_with_result 0 "$AF_AUTOMATION_TEST"
    elif  [ $workload == "wait_for_85_percent_mem_utilization"  ] ;then
        outlog=`svt_get_85_percent_mem_utilization_reached_file $ima1 `
        echo "`date` Waiting for $outlog file to exist"
        while((1)); do
            if [ -e $outlog ] ;then
                echo "`date` 85% memory utilization reached"
                break;
            fi 
            sleep 5;
        done
        svt_exit_with_result 0 "$AF_AUTOMATION_TEST"
    elif [ $workload == "pattern_file"  ] ;then
        if [ -n "$SVT_AT_VARIATION_FILL_PATTERN" ] ;then
            echo "SVT_AT_VARIATION_FILL_PATTERN set to : $SVT_AT_VARIATION_FILL_PATTERN "
        else
            echo "ERROR: SVT_AT_VARIATION_PATTERN required for workload pattern_file"
            svt_exit_with_result 1 "$AF_AUTOMATION_TEST"
        fi
        if [ -n "$SVT_AT_VARIATION_FILL_PATTERN_SZ" ] ;then
            echo "SVT_AT_VARIATION_FILL_PATTERN_SZ set to : $SVT_AT_VARIATION_FILL_PATTERN_SZ "
        else
            echo "ERROR: SVT_AT_VARIATION_FILL_PATTERN_SZ required for workload pattern_file"
            svt_exit_with_result 1 "$AF_AUTOMATION_TEST"
        fi
        num_iterations=1;  # this is a one iteration only test.
        # arg2 is filename
        # SVT_AT_VARIATION_PATTERN  is pattern in octal
        dd bs=1024 if=/dev/zero of=${2}.tmp count=${SVT_AT_VARIATION_FILL_PATTERN_SZ}
        cat ${2}.tmp | tr '\000' $SVT_AT_VARIATION_FILL_PATTERN  > $2
        rm -rf  ${2}.tmp 

    elif [ $workload == "test_transaction_start"  ] ;then
        svt_start_transaction 

    elif [ $workload == "test_transaction_end"  ] ;then
        svt_end_transaction 
        find trans.log.* -type f | sed "s/trans.log.//g" |  xargs -i echo "mv trans.log.{} $AF_AUTOMATION_TEST.`hostname`.{}.trans.log" | sh -x
        svt_exit_with_result $? "$AF_AUTOMATION_TEST"

    elif [ $workload == "collectlogs"  ] ;then
        num_iterations=1;  
        # this is just a no-op so that the collect log logic can be run below.
    elif [ $workload == "cleanup"  ] ;then
        num_iterations=1;  # this is a one iteration only test.
        run_standard_workload $workload  "./SVT.cleanup.sh"
        # this can be called in automation to sweep up any outstanding logs that may have been missed due to errors.
        # as well as do the standard cleanup of making sure no processes are running.
    elif [ $workload == "email_notifcation"  ] ;then
        if [ -n "$AF_AUTOMATION_TEST" ] ;then
            echo "`date` Send email notification that an automation run has started the svt_cmqtt portion in case I want to monitor it "
            echo -e " `date `\n`hostname`\n`cat /niagara/test/testEnv.sh` "  | mail  -s "svt_cmqtt Automation:${AF_AUTOMATION_TEST} just started on `hostname` " someuser@email.com
        fi

    elif [ $workload == "svt_47726_dmr_control"  ] ;then
        do_47726_test_dmr_control $workload_subname
        let fail_count=($fail_count+$?)
    elif [ $workload == "svt_47726_setup"  ] ;then
        do_47726_test_setup $workload_subname 
        let fail_count=($fail_count+$?)
    elif [ $workload == "svt_47726"  ] ;then
        if ! [ -n "$SVT_AT_VARIATION_47726_NUMMSG" ] ;then
            echo "ERROR: SVT_AT_VARIATION_47726_NUMMSG required for svt_47726 test"
            svt_exit_with_result 1 "$AF_AUTOMATION_TEST"
        fi
        do_47726_test $workload_subname $SVT_AT_VARIATION_47726_NUMMSG
        let fail_count=($fail_count+$?)
    elif [ $workload == "svt_47726_wait_msg_count"  ] ;then
        do_47726_test_wait $workload_subname
        let fail_count=($fail_count+$?)
    elif [ $workload == "svt_47726_verify_result"  ] ;then
        if ! [ -n "$SVT_AT_VARIATION_47726_NUMMSG" ] ;then
            echo "ERROR: SVT_AT_VARIATION_47726_NUMMSG required for svt_47726_verify_result test"
            svt_exit_with_result 1 "$AF_AUTOMATION_TEST"
        fi
        logcount=`find log.*47726* | grep -v .complete | wc -l` 
        logresult=`grep "SVT_TEST_RESULT: SUCCESS" log.*47726* | grep -v .complete | wc -l`
        dupecheck=`grep "SVT_TEST_CRITERIA: Failed av.testCriteriaMsgCount" log.*47726* | \
             grep -oE 'found count.*' |awk '{print $3}' | \
             xargs -i echo "if (({}>$SVT_AT_VARIATION_47726_NUMMSG)); then echo YES ; else echo NO ; fi " | 
             sh | grep YES | wc -l`
        if [ "$dupecheck" != "0" ] ; then
            echo "ERROR: duplicate messages found in $dupecheck out of $logcount subscribers logs"
            let fail_count=($fail_count+1)
        elif [ "$logresult" != "$logcount" ] ; then
            echo "ERROR: logresult:$logresult != logcount:$logcount"
            let fail_count=($fail_count+1)
        else
            echo "SUCCESS: logresult:$logresult == logcount:$logcount"
        fi

    elif [ $workload == "svt_47726_cycle_mq"  ] ;then
        do_47726_cycle_mq
        let fail_count=($fail_count+$?)
    elif [ $workload == "connections_setup"  ] ;then
        num_iterations=1;
        if [ "$workload_subname" == "SERVER" -o "$workload_subname" == "CLIENT" ] ;then
            workload="connections_setup_SERVER"
            echo "------------------"
            echo "Setup for connections workloads - $workload_subname $ima1 "
            echo "------------------"
            run_standard_workload $workload  "./SVT.connections.sh" "$workload_subname $ima1 NULL "
            let fail_count=($fail_count+$?)
        else
            echo "ERROR: invalid workload_subname $workload_subname for the connections_setup workload "
            let fail_count=($fail_count+1)
        fi
    elif echo $workload | grep "connections" > /dev/null ; then
        if ! [ -n "$workload_subname" ] ; then
            workload_subname=833000 ; #default for this workload 833000 connections
        fi
        echo "------------------"
        echo "Start workload: $workload with workload_subname:  $workload_subname"
        echo "------------------"
           run_standard_workload $workload  "./SVT.connections.sh" "LAUNCH ${ima_many} ${workload_subname} 0.2 ${current_port} ${workload} "
        let fail_count=($fail_count+$?)
    elif [ "$workload" == "69925" ] ;then
         if [ "$workload_subname" == "print_store_stats" ] ;then
#           ssh admin@$ima1 "imaserver stat store" 
            echo curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store
            curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store
         elif [ "$workload_subname" == "create_null_msg" ] ;then
            echo -n "" > NULLMSG;
         elif [ "$workload_subname" == "verify_retained" ] ;then
            if [ -z "$SVT_AT_VARIATION_69925_VAL" -o -z "$SVT_AT_VARIATION_69925_OP" ] ;then
                echo "ERROR : SVT_AT_VARIATION_69925_VAL parm is required. given : \"$SVT_AT_VARIATION_69925_VAL\" "
                echo "ERROR : SVT_AT_VARIATION_69925_OP parm is required. given : \"$SVT_AT_VARIATION_69925_OP\" "
                let fail_count=($fail_count+1)
            else    
                while((1)); do
                    date
#                   data=`ssh admin@$ima1 "imaserver stat " `
#                   retaineddata=`echo "$data" | grep "^RetainedMessages = " | awk '{print $3}'`

                    echo curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Server
                    data=`curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Server`
                    retaineddata=`data=${data//*RetainedMessages\":/}; echo ${data//,*/}`

                    echo "Detected RetainedMessages = $retaineddata "
                    if (($retaineddata${SVT_AT_VARIATION_69925_OP}${SVT_AT_VARIATION_69925_VAL})); then
                        echo "Break! RetainedMessages \"$retaineddata\" ${SVT_AT_VARIATION_69925_OP} ${SVT_AT_VARIATION_69925_VAL} "
                        break
                    fi
                    if [ -n "$SVT_AT_VARIATION_69925_CLEAN_STRAGGLERS" ]; then
                        echo "`date` : SVT_AT_VARIATION_69925_CLEAN_STRAGGLERS is set. Do retained msg cleanup. Start"
                        if (($retaineddata>0)); then
                            do_69925_cleanup_stragglers
                        fi
                        echo "`date` : SVT_AT_VARIATION_69925_CLEAN_STRAGGLERS is set. Do retained msg cleanup. Done."
                    fi
                    sleep 30
                done
                    
            fi
         elif [ "$workload_subname" == "java_read_retained" ] ;then
            # C mqtt client fails on subscribe
            let count=1
            while(($count>0)); do
                # TODO: Add topic prepend to make sure all messages start with /svt_cmqtt_17_retain to make sure this test will work under all cases.
                data=`java com.ibm.ima.samples.mqttv3.MqttSample -s tcp://$ima1:$insecure_port -a subscribe -t "#" -n 100000  -v `
                rc=$?
                echo "`date` : RC=$rc for java com.ibm.ima.samples.mqttv3.MqttSample -s tcp://$ima1:$insecure_port -a subscribe -t \"#\" -n 100000  -v"
                if [ "$rc" != "0" ] ;then
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                    echo "`date`: WARNING: java returned non-zero"
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                    echo "---------------------------------------"
                fi
                count=`echo -n "$data" | grep "received on topic"  | grep svt_cmqtt_17_retain | wc -l`
                echo "`date` : Waiting for count of retained messages to reach 0. Current count is $count: data is :"
                echo "$data" | head -n 10
                if (($count==0)); then
                    echo "`date`: Break. Succcess. No retained messages on svt_cmqtt_17_retain"
                    break;
                fi
                sleep 10;
            done
         elif [ "$workload_subname" == "cleanup_stragglers" ] ;then
            do_69925_cleanup_stragglers
         elif [ "$workload_subname" == "verify_stable" ] ;then
            if [ -z "$SVT_AT_VARIATION_69925_VAL"  ] ;then
                echo "ERROR : SVT_AT_VARIATION_69925_VAL parm is required. given : \"$SVT_AT_VARIATION_69925_VAL\" "
                let fail_count=($fail_count+1)
            else    
              # This test could detect memory leaks during test like rtc 71066
              lastpool1=NULL
              lastmem=NULL
              count=0
              while(($count<$SVT_AT_VARIATION_69925_VAL)); do
             
#                   data=`ssh admin@$ima1 "imaserver stat store" `
#                   p1data=`echo "$data" | grep "^Pool1UsedPercent = " | awk '{print $3}'`

                    echo curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store
                    data=`curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store`
                    p1data=`data=${data//*Pool1UsedPercent\":/}; echo ${data//,*/}`

                    if [ "$lastpool1" == "NULL" ] ;then
                        let lastpool1=$p1data+1; # give a 1% buffer zone
                        echo "Set lastpool1 to $lastpool1 with 1 percent buffer allowed"
                    elif (($p1data>$lastpool1)); then
                        echo "ERROR : pool 1 memory usage is still growing $p1data>$lastpool1  "
                        let fail_count=($fail_count+1)
                        break;
                    fi
                    echo "`date` Verify Stability: Pool1UsedPercent = $lastpool1" 
                    #admin@(none)> imaserver stat store
                    #DiskUsedPercent = 0
                    #DiskFreeBytes = 1523500998656
                    #MemoryUsedPercent = 0
                    #MemoryTotalBytes = 8589934080
                    #Pool1TotalBytes = 6012954112
                    #Pool1UsedBytes = 7808
                    #Pool1UsedPercent = 0                                                ####### value should stop growing
                    #Pool1RecordsLimitBytes = 3006477056
                    #Pool1RecordsUsedBytes = 7808
                    #Pool2TotalBytes = 2576979968
                    #Pool2UsedBytes = 44384256
                    #Pool2UsedPercent = 1
                    #admin@(none)> imaserver stat memory
                    #MemoryTotalBytes = 134911709184
                    #MemoryFreeBytes = 121049399296
                    #MemoryFreePercent = 90                                              ####### value should stop decreasing
                    #ServerVirtualMemoryBytes = 29295509504
                    #ServerResidentSetBytes = 8763195392
                    #MessagePayloads = 1048576
                    #PublishSubscribe = 18350080
                    #Destinations = 8921056
                    #CurrentActivity = 6819824
                    #ClientStates = 1048576

                    data=`ssh admin@$ima1 "imaserver stat memory" `
                    memdata=`echo "$data" | grep "^MemoryFreePercent = " | awk '{print $3}'`
                    if [ "$lastmem" == "NULL" ] ;then
                        let lastmem=$memdata-1  # allow a 1% buffer 
                        echo "Set lastmem to $lastmem with 1 percent buffer allowed"
                    elif (($memdata<$lastmem)); then
                        echo "ERROR : MemoryFreePercent is still decreasing: memdata<$lastmem"
                        let fail_count=($fail_count+1)
                        break;
                    fi
        
                    echo "`date` Verify Stability: MemoryFreePercent = $lastmem" 

                    sleep 60;
                    let count=$count+1;
              done
              echo "`date`: SUCCESS values were stable for $count iterations"
            fi
         elif [ "$workload_subname" == "wait_pool1" ] ;then
            if [ -z "$SVT_AT_VARIATION_69925_VAL" -o -z "$SVT_AT_VARIATION_69925_OP" ] ;then
                echo "ERROR : SVT_AT_VARIATION_69925_VAL parm is required. given : \"$SVT_AT_VARIATION_69925_VAL\" "
                echo "ERROR : SVT_AT_VARIATION_69925_OP parm is required. given : \"$SVT_AT_VARIATION_69925_OP\" "
                let fail_count=($fail_count+1)
            else    
              while((1)); do
                date
#               data=`ssh admin@$ima1 "imaserver stat store" `
#               p1data=`echo "$data" | grep "^Pool1UsedPercent = " | awk '{print $3}'`

                echo curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store
                data=`curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Store`
                p1data=`data=${data//*Pool1UsedPercent\":/}; echo ${data//,*/}`

                echo "Detected Pool1UsedPercent = \"$p1data\" "
                if (($p1data${SVT_AT_VARIATION_69925_OP}${SVT_AT_VARIATION_69925_VAL})); then
                    echo "Break in 60! Pool1UsedPercent \"$p1data\" ${SVT_AT_VARIATION_69925_OP} ${SVT_AT_VARIATION_69925_VAL} "
                    sleep 60
                    break
                fi
                appcount=`ps -ef |grep mqttsample_array |grep -v grep |wc -l`
                if (($appcount<1)); then
                    echo "WARNING: Break! No more apps running, so it will not change any more. We did not achieve the target, but will still return success."
                    break;
                fi
                sleep 30
              done
           fi
         elif [ "$workload_subname" == "wait_disk" ] ;then
            if [ -z "$SVT_AT_VARIATION_69925_VAL" -o -z "$SVT_AT_VARIATION_69925_OP" ] ;then
                echo "ERROR : SVT_AT_VARIATION_69925_VAL parm is required. given : \"$SVT_AT_VARIATION_69925_VAL\" "
                echo "ERROR : SVT_AT_VARIATION_69925_OP parm is required. given : \"$SVT_AT_VARIATION_69925_OP\" "
                let fail_count=($fail_count+1)
            else    
              while((1)); do
                date
                echo curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Memory
                data=`curl -X GET http://${ima1}:${A1_PORT}/ima/v1/monitor/Memory`
                p1data=`data=${data//*MemoryFreePercent\":/}; echo ${data//,*/}`

                echo "Detected MemoryFreePercent = \"$p1data\" "
                if (($p1data${SVT_AT_VARIATION_69925_OP}${SVT_AT_VARIATION_69925_VAL})); then
                    echo "Break in 5! MemoryfreePercent \"$p1data\" ${SVT_AT_VARIATION_69925_OP} ${SVT_AT_VARIATION_69925_VAL} "
                    sleep 1
                    break
                fi
                appcount=`ps -ef |grep mqttsample_array |grep -v grep |wc -l`
                if (($appcount<1)); then
                    echo "WARNING: Break! No more apps running, so it will not change any more. We did not achieve the target, but will still return success."
                    break;
                fi
                sleep 5
              done
           fi
         else
            echo "ERROR: invalid workload_subname $workload_subname for this workload"
            let fail_count=($fail_count+1)
         fi
    elif [ "$workload" == "43965" ] ;then
        do_rtc_43965_MQTT_async_sanity_test $workload_subname $SVT_AT_VARIATION_QOS $SVT_AT_VARIATION_43965_CLIENT_CERTIFICATE
    elif echo $workload | grep "monitor_primary_appliance" > /dev/null ; then
           ./SVT.monitor.appliance.sh ${ima1} "monitor.${workload_subname}" > /dev/null 2> /dev/null &
    elif echo $workload | grep "monitor_secondary_appliance" > /dev/null ; then
           ./SVT.monitor.appliance.sh ${ima2} "monitor.${workload_subname}" > /dev/null 2> /dev/null &
    elif echo $workload | grep "monitor_client" > /dev/null ; then
           ./SVT.monitor.client.sh "monitor.${workload_subname}"  > /dev/null 2> /dev/null &
    elif echo $workload | grep "print_stats" > /dev/null ; then
        num_iterations=1;
        cmd=`echo "imaserver stat Store
            imaserver stat Server
            imaserver stat Topic
            imaserver stat Subscription StatType=PublishedMsgsHighest
            imaserver stat Subscription StatType=PublishedMsgsLowest
            imaserver stat Subscription StatType=BufferedMsgsHighest
            imaserver stat Subscription StatType=BufferedMsgsLowest
            imaserver stat Subscription StatType=BufferedPercentHighest
            imaserver stat Subscription StatType=BufferedPercentLowest
            imaserver stat Subscription StatType=BufferedHWMPercentHighest
            imaserver stat Subscription StatType=BufferedHWMPercentLowest
            imaserver stat Subscription StatType=RejectedMsgsHighest
            imaserver stat Subscription StatType=RejectedMsgsLowest
            imaserver stat Server
            advanced-pd-options memorydetail
            advanced-pd-options list
            status imaserver
            status mqconnectivity;
            status webui
            datetime get" | awk '{ printf("echo ============================================================ %s ; %s ; ", $0,$0);}'`
            ssh admin@$ima1 "$cmd"
    elif echo $workload | grep "verify_svt_criteria" > /dev/null ; then
        echo "`date` : verify_svt_criteria: start counting "
        data=`grep "SVT_TEST_CRITERIA:" ${workload_subname}*.log`
        numcriteria=`echo "$data" |wc -l`
        numpass=`echo "$data" | grep "SVT_TEST_CRITERIA: Passed" |wc -l`
        numfail=`echo "$data" | grep "SVT_TEST_CRITERIA: Failed" |wc -l`
        echo "`date` : result : numcriteria:$numcriteria numpass:$numpass numfail:$numfail "
        if [ "$numfail" != "0" ] ;then
            echo "`date` : ERROR: numfail!=0 verify_svt_criteria: result: numcriteria:$numcriteria numpass:$numpass numfail:$numfail "
            let fail_count=($fail_count+1)
        elif [ "$numpass" != "$numcriteria" ] ;then
            echo "`date` : ERROR: numpass != numcriteria verify_svt_criteria: result: numcriteria:$numcriteria numpass:$numpass numfail:$numfail "
            let fail_count=($fail_count+1)
        else
            echo "`date` : SUCCESS: verify_svt_criteria: result: numcriteria:$numcriteria numpass:$numpass numfail:$numfail "
        fi
    elif echo $workload | grep "dynamic_endpoint_17778" > /dev/null ; then
        max_msgs=5000
        if [ -n "$SVT_AT_VARIATION_17778_MAX_MSGS" ] ;then
            max_msgs=$SVT_AT_VARIATION_17778_MAX_MSGS
        fi
        ssh admin@$ima1 imaserver update MessagingPolicy "Name=SVTcmqttDynamic" "Protocol=JMS,MQTT" "MaxMessages=$max_msgs" "DestinationType=Topic" "MaxMessagesBehavior=DiscardOldMessages" "Destination=*" 
        let fail_count=($fail_count+$?)

    elif [ "$workload" == "softlayer_setup" ] ;then
        let fail_count=0

#		data=`ssh admin@$ima1 imaserver list Endpoint`	
        echo curl -X GET http://${ima1}/ima/v1/configuration/Endpoint
        data=`curl -X GET http://${ima1}/ima/v1/configuration/Endpoint`
		count=`echo "$data" | grep -E '(22100|22120|22200|22220|22300|22320|22400|22420|22500|22520)$' | wc -l`
		count2=`echo "$data"  | grep -E '22[12345][012][0-9]$' |wc -l`
		
		if [ "$count" == "10" -a "$count2" == "105" ] ;then	
	    	echo "Softlayer endpoints are already setup"

		else
            for v in {22100..22120}; do
#               ssh admin@$ima1 imaserver show   Endpoint "Name=SVTcmqtt${v}"
                echo curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt${v}
                curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt${v}
                if [ "$?" != "0" ] ;then
#                  	ssh admin@$ima1 imaserver create Endpoint "Name=SVTcmqtt${v}" "Enabled=True" "Port=${v}" "MessageHub=SVTcmqtt" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTcmqtt" "MessagingPolicies=SVTcmqtt" "Description=SVT_unsecured_endpoint_for_svt_cmqtt_port_${v} "
                    echo curl -X POST -d '{"Endpoint":{"SVTcmqtt'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_unsecured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null }}}' http://${ima1}/ima/v1/configuration
                    curl -X POST -d '{"Endpoint":{"SVTcmqtt'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_unsecured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null }}}' http://${ima1}/ima/v1/configuration

                    let fail_count=($fail_count+$?)
                else
                  	echo "Endpoint is already setup"
                fi
            done
	
	    	for v in {22200..22220}; do
#               ssh admin@$ima1 imaserver show   Endpoint   "Name=SVTcmqtt_secure${v}"
                echo curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure${v}
                curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure${v}
                if [ "$?" != "0" ] ;then
#                   ssh admin@$ima1 imaserver create Endpoint   "Name=SVTcmqtt_secure${v}" "Enabled=True" "Port=${v}" "MessageHub=SVTcmqtt" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTcmqtt" "MessagingPolicies=SVTcmqtt" "SecurityProfile=SVTcmqtt"
                    echo curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqtt" }}}' http://${ima1}/ima/v1/configuration
                    curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqtt" }}}' http://${ima1}/ima/v1/configuration
        			let fail_count=($fail_count+$?)
                else
                       echo "Endpoint is already setup"
                fi
	    	done
	
	    	for v in {22300..22320}; do
#           	ssh admin@$ima1 imaserver show   Endpoint   "Name=SVTcmqtt_secure_CC${v}"
                echo curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC${v}
                curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC${v}
            	if [ "$?" != "0" ] ;then
#                   ssh admin@$ima1 imaserver create Endpoint   "Name=SVTcmqtt_secure_CC${v}" "Enabled=True" "Port=${v}" "MessageHub=SVTcmqtt" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTcmqtt" "MessagingPolicies=SVTcmqtt" "SecurityProfile=SVTcmqttCC"
                    echo curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_CC'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttCC" }}}' http://${ima1}/ima/v1/configuration
                    curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_CC'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttCC" }}}' http://${ima1}/ima/v1/configuration
        			let fail_count=($fail_count+$?)
            	else
                	echo "Endpoint is already setup"
            	fi
	    	done
	
	    	for v in {22400..22420}; do
#           	ssh admin@$ima1 imaserver show   Endpoint   "Name=SVTcmqtt_secure_UP${v}"
                echo curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_UP${v}
                curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_UP${v}
            	if [ "$?" != "0" ] ;then
#                   ssh admin@$ima1 imaserver create Endpoint   "Name=SVTcmqtt_secure_UP${v}" "Enabled=True" "Port=${v}" "MessageHub=SVTcmqtt" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTcmqtt" "MessagingPolicies=SVTcmqtt" "SecurityProfile=SVTcmqttUP"
                    echo curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_UP'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttUP" }}}' http://${ima1}/ima/v1/configuration
                    curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_UP'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt", "ConnectionPolicies": "SVTcmqtt", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttUP" }}}' http://${ima1}/ima/v1/configuration
        			let fail_count=($fail_count+$?)
       	     	else
         	       echo "Endpoint is already setup"
          	  fi
	    	done
	
	    	for v in {22500..22520}; do
#           	ssh admin@$ima1 imaserver show   Endpoint   "Name=SVTcmqtt_secure_CC_CN_topic_CI_eq_CN${v}"
                echo curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC_CN_topic_CI_eq_CN${v}
                curl -f -X GET http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC_CN_topic_CI_eq_CN${v}
   	         	if [ "$?" != "0" ] ;then
#                   ssh admin@$ima1  imaserver create Endpoint   "Name=SVTcmqtt_secure_CC_CN_topic_CI_eq_CN${v}" "Enabled=True" "Port=${v}" "MessageHub=SVTcmqtt" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTcmqtt_CN_CI" "MessagingPolicies=SVTcmqtt_CN_topic,SVTcmqtt_Publish" "SecurityProfile=SVTcmqttCC"
                    echo curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_CC_CN_topic_CI_eq_CN'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt_CN_topic,SVTcmqtt_Publish", "ConnectionPolicies": "SVTcmqtt_CN_CI", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttCC" }}}' http://${ima1}/ima/v1/configuration
                    curl -X POST -d '{"Endpoint":{"SVTcmqtt_secure_CC_CN_topic_CI_eq_CN'${v}'": { "Port": '${v}', "Enabled": true, "TopicPolicies": "SVTcmqtt_CN_topic,SVTcmqtt_Publish", "ConnectionPolicies": "SVTcmqtt_CN_CI", "MessageHub": "SVTcmqtt", "Interface": "All", "MaxMessageSize": "256MB", "Protocol": "All", "Description": "SVT_secured_endpoint_for_svt_cmqtt_port_'${v}'", "InterfaceName": "All", "SubscriptionPolicies": null, "SecurityProfile":"SVTcmqttCC" }}}' http://${ima1}/ima/v1/configuration
        			let fail_count=($fail_count+$?)
            	else
        	        echo "Endpoint is already setup"
    	        fi
		    done
		fi

    elif [ "$workload" == "softlayer_cleanup" ] ;then
        let fail_count=0

        for v in {22100..22120}; do
           echo curl -X DELETE  http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt$v
           curl -X DELETE  http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt$v
        done

        for v in {22200..22220}; do
           echo curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure$v
           curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure$v
        done

        for v in {22300..22320}; do
           echo curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC$v
           curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC$v
        done

        for v in {22400..22420}; do
           echo curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_UP$v
           curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_UP$v
        done

        for v in {22500..22520}; do
           echo curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC_CN_topic_CI_eq_CN$v
           curl -X DELETE http://${ima1}/ima/v1/configuration/Endpoint/SVTcmqtt_secure_CC_CN_topic_CI_eq_CN$v
        done

    elif [ "$workload" == "51598_additional_test" ] ;then
        if ! [ -n "$SVT_AT_VARIATION_MSGCNT" ] ;then
            echo "-----------------------------------------"
            echo " `date` ERROR: SVT_AT_VARIATION_MSGCNT must be specified for this test."
            echo "-----------------------------------------"
            let fail_count=($fail_count+1)
        else
            do_51598_additional_test $SVT_AT_VARIATION_MSGCNT
            let fail_count=($fail_count+$?)
        fi
    elif [ "$workload" == "haFunctions.sh" ] ;then
        run_ha_cmd $workload_subname 
        let fail_count=($fail_count+$?)
    elif [ $workload == "client_certificate" -o  $workload == "all" ] ;then
        if ! [ -n "$workload_subname" ] ; then
            workload_subname="100 195 3 3 3 client_certificate 1 0 1 2" ; 
        fi
        echo "------------------"
        echo " Start client certificate workload: $workload_subname * 3 "
        echo " connections doing 3 mesages each on Qos 0 1 2, 1 msg/second "
        echo "------------------"
            run_standard_workload $workload "./SVT.17942.c.sh" "${ima_client_certificate} $workload_subname"
        let fail_count=($fail_count+$?)
    else
        echo "------------------"
        echo " ERROR: could not start workload : $workload"
        echo "------------------"
        let fail_count=($fail_count+1)
    fi

    echo "`date` end iteration $x" 
    let x=$x+1; 

    if (($num_iterations>1)); then
        echo "`date` Start - Sleep before next iteration"
            sleep $DELAY
        echo "`date` Done - Sleep before next iteration"
    fi    
    
done

if [ "$x" == "0" ] ;then
    echo "-----------------------------------------"
    echo "`date` ERROR: no iterations run. This could not be right. Setting a failure."
    echo "-----------------------------------------"
    let fail_count=($fail_count+1)
fi

if [ -n "$SVT_AT_VARIATION_QUICK_EXIT" -a "$SVT_AT_VARIATION_QUICK_EXIT" == "true" ] ;then
    echo "-----------------------------------------"
    echo "`date` SVT AUTOMATION OVERIDE: Do \"quick exit\" before doing any tars on logs etc... "
    echo "`date` other testing will be done while this workload continues to run, and messaging results will be evaluated later"
    echo "-----------------------------------------"

    echo "---------------------------------"
    echo "`date` $0 - Done! Successfully completed all expected processing - Quick Exit"
    echo "---------------------------------"

    svt_exit_with_result $fail_count "$AF_AUTOMATION_TEST"

fi

if [ -n "$SVT_AT_VARIATION_SKIP_LOG_PROCESSING" -a "$SVT_AT_VARIATION_SKIP_LOG_PROCESSING" == "true" ] ;then
    echo "-----------------------------------------"
    echo "`date` SVT AUTOMATION OVERIDE: Do \"SVT_AT_VARIATION_SKIP_LOG_PROCESSING\" before doing any tars on logs etc... "
    echo "`date` other testing will be done while this workload continues to run, and messaging results will be evaluated later"
    echo "-----------------------------------------"

    echo "---------------------------------"
    echo "`date` $0 - Done! Successfully completed all expected processing - SVT_AT_VARIATION_SKIP_LOG_PROCESSING"
    echo "---------------------------------"

    svt_exit_with_result $fail_count "$AF_AUTOMATION_TEST"
fi


if [ -n "$AF_AUTOMATION_TEST" ] ;then
    if [ -n "$SVT_AT_VARIATION_TT_PREFIX" ] ; then
        echo "-----------------------------------------"
        echo "`date` SVT AUTOMATION OVERIDE: User passed in SVT_AT_VARIATION_TT_PREFIX: $SVT_AT_VARIATION_TT_PREFIX"
        echo -n "`date` Start post processing for SVT Automation test : phase 0 : number of files to process: "
        find log.${SVT_AT_VARIATION_TT_PREFIX}*.`hostname`.*[0-9] -type f |wc
        echo "-----------------------------------------"
        find log.${SVT_AT_VARIATION_TT_PREFIX}*.`hostname`.*[0-9] -type f | sed "s/log.//g"  |  xargs -i echo "mv log.{} {}.log" | sh -x
    fi

    echo "-----------------------------------------"
    echo -n " `date` Process current running test logs first: Number of files to process: "
    find log.${AF_AUTOMATION_TEST}.*.`hostname`.*[0-9] -type f | wc -l
    echo "-----------------------------------------"
    find log.${AF_AUTOMATION_TEST}.*.`hostname`.*[0-9] -type f | sed "s/log.//g"  |  xargs -i echo "mv log.{} {}.log" | sh -x

    echo "-----------------------------------------"
    echo -n "`date` Start post processing for SVT Automation test : phase 1 : number of files to process: "
    find log.* -type f | wc -l
    echo "-----------------------------------------"
    find log.* -type f | sed "s/log.//g" |  xargs -i echo "mv log.{} $AF_AUTOMATION_TEST.`hostname`.{}.log" | sh -x

fi


if [ -n "$SVT_AUTOMATION_TEST_LOG_TAR" ] ;then 

    if [ -n $workload_subname ] ;then
        echo "-----------------------------------------"
        echo "`date` Tar up all the logs w/ specified file pattern $workload_subname as *${workload_subname}*.log"
        echo "-----------------------------------------"
        tar -czvf ${SVT_AUTOMATION_TEST_LOG_TAR}.`hostname`.tgz.log *${workload_subname}*.log
        rm -rf *${workload_subname}*.log
    else
        echo "-----------------------------------------"
        echo "`date` Tar up all the logs w/ DEFAULT file pattern cmqtt*.log"
        echo "-----------------------------------------"
        tar -czvf ${SVT_AUTOMATION_TEST_LOG_TAR}.`hostname`.tgz.log cmqtt*.log
        rm -rf cmqtt*.log
    fi
    
fi

svt_exit_with_result $fail_count "$AF_AUTOMATION_TEST"

