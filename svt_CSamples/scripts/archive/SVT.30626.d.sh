#!/bin/bash

#--------------------------------------------
#  This version of the script is just for cleanup 
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
num_2_pub_messages=${8-0};
let num_2_sub_messages_expected=0
let num_2_sub_messages_actual=0
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
            flags+=" -n 0 "
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
