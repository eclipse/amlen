#!/bin/bash 

TEST_APP=mqttsample_array

#------------------------------------------------------
# Complete Fan-in test (with SEGFAULT workaround subscribe_wait)
#------------------------------------------------------
verify_condition() {
    local condition=$1
    local count=$2
    local file_pattern=$3
    local iteration=$4
    local myregex="[0-9]+"

    echo "File pattern is $file_pattern"
    

    if ! [[ "$count" =~ $myregex ]] ; then
        echo "ERROR: count non-numeric"
        return 1;
    fi
    k=0;
    while (($k<$count)); do
        w=`grep "WARNING" $file_pattern 2>/dev/null |wc -l`
        e=`grep "ERROR" $file_pattern 2>/dev/null  |wc -l`
        c=`ps -ef | grep $TEST_APP 2>/dev/null | grep -v grep 2>/dev/null  | wc -l`
        k=`grep "$condition" $file_pattern 2>/dev/null |wc -l`
        echo "$iteration: Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
        sleep 2;
    done
    echo "Success."
    return 0;
}

subscribe_topic=${1-""};   # The desired topic
let subscribe_connections_per_client=${2-590};
subscribe_client=${3-100};
let subscribe_total=($subscribe_connections_per_client*$subscribe_client);
subscribe_count=${4-100};  # Each subscribeer subscribees this many messages (all on same topic)
subscribe_rate=${5-0};   # The desired rate (message/second) to subscribe messages
subscribe_qos=${6-2};   # The desired QoS
imaserver=${7-"10.10.1.87:16102"}
logfile_prefix=${8-"log"}
ha_imaserver=${9-""}
pubrate=${10-0.033} # Default 1 message published on each fan out pattern every 30 seconds.
messaging_type=""

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

rm -rf $logfile_prefix*.s.*
rm -rf $logfile_prefix*.p.*
h=`hostname`

let x=1;
while (($x<=$subscribe_client)) ; do

        MQTT_C_CLIENT_TRACE= ./$TEST_APP -c false -o ${logfile_prefix}.s.$x -x topicChangeTest=1 -t $subscribe_topic -a subscribe -s $imaserver $ha_imaserver -n $subscribe_count -w $subscribe_rate -q $subscribe_qos -z $subscribe_connections_per_client -x statusUpdateInterval=1 -x connectTimeout=30 -x retryConnect=3600 -x keepAliveInterval=600 -x cleanupOnConnectFails=1 -i "$h.$subscribe_topic.$x"  -x subscribeOnConnect=1 -x publishDelayOnConnect=300 -x reconnectWait=1 -x verifyStillActive=120 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1 -v & 
    let x=$x+1;
done
verify_condition "Connected to " $subscribe_total "$logfile_prefix.s.*" $j
verify_condition "Subscribed - Ready" $subscribe_total "$logfile_prefix.s.*" $j
verify_condition "All Clients Subscribed" $subscribe_client "$logfile_prefix.s.*" $j

exit 0;

	MQTT_C_CLIENT_TRACE= ./$TEST_APP -a publish -s $imaserver $ha_imaserver -t $subscribe_topic  -o ${logfile_prefix}.p.$subscribe_topic -n $subscribe_count -w $pubrate -v & 

verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "$logfile_prefix.p.*" $j
verify_condition "SVT_TEST_RESULT: SUCCESS" $subscribe_client "$logfile_prefix.s.*" $j
