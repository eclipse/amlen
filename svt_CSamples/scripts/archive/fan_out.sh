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
subscribe_qos=${6-0};   # The desired QoS
#imaserver=${7-"10.10.1.87:16102"}
imaserver=${7}
subscribe_logfile_prefix=${8-"log.s"}

if [ "$subscribe_topic" == "" ] ;then
    echo ERROR no topic
    exit 1;
fi

if [ "$imaserver" == "" ] ;then
        echo ERROR no imaserver specified as argument 7
        exit 1;
fi


automation="true"

if [ "$automation" == "false" ];   then 
    killall -9 $TEST_APP;
    ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}
fi

let j=0;

rm -rf $subscribe_logfile_prefix*
let x=1;
while (($x<=$subscribe_client)) ; do
    ./$TEST_APP -o ${subscribe_logfile_prefix}.$x -t $subscribe_topic -a subscribe -s $imaserver -n $subscribe_count -w $subscribe_rate -q $subscribe_qos -z $subscribe_connections_per_client -x noDisconnect=1 -x statusUpdateInterval=1 -x sleepLoop=0 -x connectTimeout=30 -x retryConnect=1000 -x reconnectWait=10 &
    if ! (($x%1000)); then
        let tmp_total=($subscribe_connections_per_client*$x);
        echo "Start intermediate verification."
        verify_condition "Connected to $imaserver" $tmp_total "$subscribe_logfile_prefix.*" $j
    fi
    let x=$x+1;
done
verify_condition "Connected to $imaserver" $subscribe_total "$subscribe_logfile_prefix.*" $j
verify_condition "All Clients Subscribed" $subscribe_client "$subscribe_logfile_prefix.*" $j




