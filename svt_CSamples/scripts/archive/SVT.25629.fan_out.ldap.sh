#!/bin/bash 

. ./svt_test.sh

TEST_APP=mqttsample_array

subscribe_topic=${1-""};   # The desired topic
let subscribe_connections_per_client=${2-590};
subscribe_client=${3-100};
let subscribe_total=($subscribe_connections_per_client*$subscribe_client);
subscribe_count=${4-100};  # Each subscribeer subscribees this many messages (all on same topic)
subscribe_rate=${5-0};   # The desired rate (message/second) to subscribe messages
qos=${6-0};   # The desired QoS
imaserver=${7-"10.10.1.87:16102"}
name=${8-"log"}
ha_imaserver=${9-""}
pubrate=${10-0.033} # Default 1 message published on each fan out pattern every 30 seconds.
messaging_type=""
now=`date +%s.%N`

if [ "$subscribe_topic" == "" ] ;then
    echo ERROR no topic
    exit 1;
fi

if [ "$imaserver" == "" ] ;then
        echo ERROR no imaserver specified as argument 7
        exit 1;
fi

if [ -n "$ha_imaserver" ] ;then
    if [ "$messaging_type" == "CLIENT_CERTIFICATE" ] ;then
	ha_imaserver="-x haURI=ssl://$ha_imaserver"
    else	
	ha_imaserver="-x haURI=$ha_imaserver"
    fi	
    echo "set ha_imaserver to \"$ha_imaserver\""
	
fi

automation="true"

if [ "$automation" == "false" ];   then 
    killall -9 $TEST_APP;
    ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}
fi

let j=0;

rm -rf log.s.${name}.${qos}*
rm -rf log.p.${name}.${qos}*
h=`hostname`

let x=1;
while (($x<=$subscribe_client)) ; do
    let ui=($x*$subscribe_connections_per_client);	
    MQTT_C_CLIENT_TRACE= ./$TEST_APP -o log.s.${name}.${qos}.${x}.${now} -x userIncrementer=$ui -u u -p imasvtest  -t $subscribe_topic -a subscribe -s $imaserver $ha_imaserver -n $subscribe_count -w $subscribe_rate -q ${qos} -z $subscribe_connections_per_client -x statusUpdateInterval=1 -x connectTimeout=10 -x retryConnect=3600 -x keepAliveInterval=1200 -x cleanupOnConnectFails=1 -i "$h.$subscribe_topic.$x" -x verifyMsg=1 -x msgFile=./SMALLFILE  -x subscribeOnConnect=1 -x publishDelayOnConnect=120 -x subscribeOnConnect=1 -x reconnectWait=1 -x verifyStillActive=300   & 
    let x=$x+1;
done
svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${subscribe_client}
svt_verify_condition "Connected to " $subscribe_total "log.s.${name}.${qos}.*${now}" $subscribe_client
svt_verify_condition "Subscribed - Ready" $subscribe_total "log.s.${name}.${qos}.*${now}" $subscribe_client
svt_verify_condition "All Clients Subscribed" $subscribe_client "log.s.${name}.${qos}.*${now}" $subscribe_client
MQTT_C_CLIENT_TRACE= ./$TEST_APP -a publish -s $imaserver $ha_imaserver -t $subscribe_topic  -o log.p.${name}.${qos}.1.${now} -n $subscribe_count -w $pubrate -v -x msgFile=./SMALLFILE & 
svt_verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "log.p.${name}.${qos}.1.${now}" 
svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $subscribe_client "log.s.${name}.${qos}.*${now}" 

exit 0;
