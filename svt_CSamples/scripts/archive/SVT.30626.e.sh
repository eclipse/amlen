#!/bin/bash

#--------------------------------------------
# The purpose of this test is to be run during failover. 
#
# The following checks are done on the results:
# 1. All publishers and subscribers can complete expected messaging after failover.
# 2. 
# 3. orderMsg=2 is used to allow ordering information to be sent without verifying the order is correct 
#           (limitation of test client: not able to verify order for multiple publishers on same topic)
# 4. verifyMsg=1 is used to verify that the contents of the message (after the ordering info) is correct.
#
# Current Test Procedure:
#  a) ./RTC.17942.start.workload.sh 25776 "200 50 x x 3 0.008 2" 10 # 8 minute test cycle - 10 K Qos2 connections, doing 20K msg/sec total 
#  b) do_admin_loop 10.10.10.10
#  c) do_admin_loop 10.10.10.10
#  d) . ./ha_test.sh ; do_run_all_tests # only runing the KILL HA test currently 
#--------------------------------------------

name="30626"
now=${1-`date +%s`}
ima1=${2-10.10.1.39:16102}
ima2=${3-10.10.1.40:16102}
num_clients=${4-100};
num_X_per_client=${5-590};
num_0_pub_messages=${6-0};
let num_0_sub_messages_expected=0
let num_0_sub_messages_actual=0
num_1_pub_messages=${7-0};
let num_1_sub_messages_expected=0
let num_1_sub_messages_actual=0
num_2_pub_messages=${8-135};
let num_2_sub_messages_expected=$num_2_pub_messages
let num_2_sub_messages_actual=$num_2_pub_messages
pub_rate=${9-"-1"}
qos0_under_test=${10-""}
qos1_under_test=${11-""}
qos2_under_test=${12-"2"}

qos_under_test="$qos0_under_test $qos1_under_test $qos2_under_test"

echo "ima1 :$ima1"
echo "ima2 :$ima2"
echo "num_clients $num_clients"
echo "num_X_per_client $num_X_per_client"
echo "QOS under test: $qos_under_test"
echo "QOS 0 pub/sub : $num_0_pub_messages $num_0_sub_messages_expected $num_0_sub_messages_actual"
echo "QOS 1 pub/sub : $num_1_pub_messages $num_1_sub_messages_expected $num_1_sub_messages_actual"
echo "QOS 2 pub/sub : $num_2_pub_messages $num_2_sub_messages_expected $num_2_sub_messages_actual"
echo "pub_rate is $pub_rate"

#exit 0;

#qos_under_test="2"

#echo "---------------------------------"
#echo "Kill any other mqttsample_array processes that are running"
#echo "---------------------------------"
#killall -9 mqttsample_array

get_flags(){
    local qos=$1
    local action=$2
    local flags=" -s ${ima1} -x keepAliveInterval=600 -x connectTimeout=10 -x reconnectWait=1 -x retryConnect=3600 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=5 -x orderMsg=2 "

    #flags+=" -x checkConnection=100 "
    if [ -n "$ima2" ] ;then
        flags+=" -x haURI=${ima2} "
    fi

    if [ "$action" == "SUBSCRIBE" ] ;then
        flags+=" -z $num_X_per_client -x verifyPubComplete=./log.p.${name}.${qos}.complete -x verifyStillActive=60 -x userReceiveTimeout=30000 "
        if [ "$qos" == "0" ] ; then                 
            flags+="-n ${num_0_sub_messages_actual} -x testCriteriaPctMsg=50 " # 50% - when failover is injected many Qos 0 messages may be lost.
        elif [ "$qos" == "1" ] ; then
            flags+="-n ${num_1_sub_messages_actual} -x testCriteriaMsgCount=${num_1_sub_messages_expected} -c false -v -v"
        elif [ "$qos" == "2" ] ; then
            flags+="-n ${num_2_sub_messages_actual} -x testCriteriaMsgCount=${num_2_sub_messages_expected} -c false -v -v"
        else
            echo "---------------------------------" >> /dev/stderr
            echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
            echo "---------------------------------" >> /dev/stderr
        fi
            #flags+=" -n 0 "
    elif [ "$action" == "PUBLISH" ] ;then
        flags+=" -w $pub_rate "
	flags+=" -x msgFile=./BIGFILE "
	#flags+=" -x msgFile=./SMALLFILE "
        if [ "$qos" == "0" ] ; then
            flags+="-n ${num_0_pub_messages} "
        elif [ "$qos" == "1" ] ; then
            flags+="-n ${num_1_pub_messages} -c false -v -v"
        elif [ "$qos" == "2" ] ; then
            flags+="-n ${num_2_pub_messages} -c false -v -v"
        else
            echo "---------------------------------" >> /dev/stderr
            echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
            echo "---------------------------------" >> /dev/stderr
        fi
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi
    

    echo $flags
}

rm -rf log.s.${name}.*
for qos in $qos_under_test ; do 
    let i=0;
    while (($i<$num_clients)); do
        #echo "MQTT_C_CLIENT_TRACE= ./mqttsample_array -o log.s.${name}.${qos}.${i}.${now} -r log.s.${name}.${qos}.${i}.${now}.persist -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` &" |sh -x > log.s.${name}.${qos}.${i}.${now}.mqtt 
        #-----------------------------------
        # 4/8/13: Note: it seems like during the -r option was causing "too many open files" erros today so I took it out.
        #-----------------------------------
        #-----------------------------------
        # 4/17/13: doing qos2 testing with eth pulls, it seems like client doesn't reconnect well. add back in -r option to see what happens
        #-----------------------------------
        echo "MQTT_C_CLIENT_TRACE=ON ./mqttsample_array -o log.s.${name}.${qos}.${i}.${now} -r log.s.${name}.${qos}.${i}.${now}.persist -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` &" |sh -x > log.s.${name}.${qos}.${i}.${now}.mqtt 
        let i=$i+1;
    done
done

for qos in $qos_under_test ; do 
    echo "---------------------------------"
    echo "Wait for all qos $qos clients to subscribe" 
    echo "---------------------------------"
    while((1)) ; do
        echo -n "."
        echo "grep \"All Clients Subscribed\" log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l"
        count=`grep "All Clients Subscribed" log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l `
        if (($count==$num_clients)); then
            echo "count is $count"
            break;   
        else
        echo count $count is not == to $num_clients
        fi
        sleep 1;
    done
done

echo "---------------------------------"
echo "Complete. All clients subscribed . Exit."
echo "---------------------------------"

for qos in $qos_under_test ; do 
    for var in subscribers; do 
        echo "---------------------------------"
        echo "Wait for all qos $qos $var to complete" 
        echo "---------------------------------"
        while((1)) ; do
            echo -n "."
            if [ "$var" == "publishers" ] ;then
                pass_count=`grep "SVT_TEST_RESULT: SUCCESS " log.p.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                fail_count=`grep "SVT_TEST_RESULT: FAILURE " log.p.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                if (($fail_count!= 0))  ; then         
                    grep SVT_TEST_RESULT log.p.${name}.${qos}.*${now}
                    mkdir -p logs.failures
                    echo "Archiving failing logs into logs.failures"
                    grep -l "SVT_TEST_RESULT: FAILURE " log.p.${name}.${qos}.*${now} | sed "s/log.p/log.*/g" | xargs -i echo " cp {} logs.failures" |sh -x
                fi
                total=$num_clients;
            elif [ "$var" == "subscribers" ] ;then
                pass_count=`grep "SVT_TEST_RESULT: SUCCESS " log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                fail_count=`grep "SVT_TEST_RESULT: FAILURE " log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                if (($fail_count!= 0))  ; then         
                    grep SVT_TEST_RESULT log.s.${name}.${qos}.*${now}
                    mkdir -p logs.failures
                    echo "Archiving failing logs into logs.failures"
                    grep -l "SVT_TEST_RESULT: FAILURE " log.s.${name}.${qos}.*${now} | sed "s/log.s/log.*/g" | xargs -i echo " cp {} logs.failures" |sh -x
                fi
                total=$num_clients
            else
                echo "---------------------------------"
                echo " ERROR: - internal bug in script unsupported var : $var" 
                echo "---------------------------------"
            fi
            let execution_count=($pass_count+$fail_count); 
            if (($execution_count==$total)); then
                echo "Finished processing : QOS : $qos: Complete - execution_count is $execution_count"
                if (($fail_count== 0))  ; then         
                    echo -e "\nRESULT: PASS : $var : QOS : $qos: Complete - pass_count is $pass_count  fail_count is $fail_count"
                else
                    echo -e "\nRESULT: FAIL : $var : QOS : $qos: Complete - pass_count is $pass_count  fail_count is $fail_count"
                fi
    
                if [ "$var" == "publishers" ] ;then
                    touch ./log.p.${name}.${qos}.complete;  # used with "-x verifyPubComplete" setting to show that all publishing is complete.
                fi
                break;
            else 
                echo "$execution_count != $total"
            fi
            sleep 1;
        done
    done
done

echo ""
echo "------------------------------------------------"
echo "`date` Subscriber logs"
echo "------------------------------------------------"
grep "SVT_TEST_RESULT: " log.s.${name}.${qos}.*${now}
echo ""
echo ""
echo ""
echo "------------------------------------------------"
echo "`date` DETAILED Subscriber logs"
echo "------------------------------------------------"
find log.s.${name}.${qos}.*${now} | xargs -i echo "echo ================================ ; echo Details for {} ; echo ================================ ; cat {} | awk '/SVT_MQTT_C_INFO_START/,/*/ {print \$0}' "   |sh

echo ""

echo "---------------------------------"
echo "Cleanup - set clean to true and number of messages to zero to delete all the durable subscribers"
echo "TODO - monitor status for cleanup"
echo "---------------------------------"
my_pid=""
for qos in $qos_under_test ; do
    let i=0;
    while (($i<$num_clients)); do
        ./mqttsample_array -a subscribe -o log.s.${name}.${qos}.${i}.${now}.cleanup -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` -n 0 -c true > /dev/null 2>/dev/null &
        my_pid+="$! "
        ./mqttsample_array -a publish -o -o log.p.${name}.${qos}.${i}.${now}.cleanup -i ${qos}_${i}_${now}_P `get_flags $qos PUBLISH` -n 0 -c true > /dev/null 2>/dev/null &
        my_pid+="$! "
        let i=$i+1;
    done
    my_pid+="$! "
done

wait $my_pid

echo "---------------------------------"
echo "Done. All actions complete"
echo "---------------------------------"
    
exit 0;
