#!/bin/bash 

TEST_APP=mqttsample_array

#------------------------------------------------------
# Complete Fan-in test (with SEGFAULT workaround publish_wait)
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
        w=`grep "WARNING" $file_pattern |wc -l`
        e=`grep "ERROR" $file_pattern |wc -l`
        c=`ps -ef | grep $TEST_APP | grep -v grep  | wc -l`
        k=`grep "$condition" $file_pattern |wc -l`
        echo "$iteration: Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
        sleep 2;
    done
    echo "Success."
    return 0;
}

let publish_connections_per_client=${1-590};
publish_client=${2-50};
publish_count=${3-1000};  # Each publisher publishes this many messages (all on same topic)
publish_rate=${4-0};   # The desired rate (message/second) to publish messages
publish_qos=${5-0};   # The desired QoS
publish_topic=${6-"/MQTTCSample/"};   # The desired topic

let publish_total=($publish_connections_per_client*$publish_client)
let subscribe_total=($publish_total*$publish_count)

let subscribe_connections_per_client=${7-1};
subscribe_client=${8-1};
subscribe_count=${9-$subscribe_total};  # Each subscribeer subscribees this many messages (all on same topic)
subscribe_rate=${10-0};   # The desired rate (message/second) to subscribe messages
subscribe_qos=${11-0};   # The desired QoS
subscribe_topic=${12-"/MQTTCSample/"};   # The desired topic

#ismserver=10.10.1.214:16102
#ismserver=10.10.10.10:16102

ismserver=${13-"10.10.3.22:16102"}
#ismserver=${13-"ssl://10.10.10.10:16111"}
#ismserver_addnl=" -S trustStore=CAfile.pem "

automation="true"

if [ "$automation" == "false" ];   then 
    killall -9 $TEST_APP;
    ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}
fi

let j=0;

#while((1)) ; do 
    #echo "===================================="
    #echo "Iteration:$j"
    #echo "===================================="
    #ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}
    #killall -9 $TEST_APP;

rm -rf log.*
let x=1;
while (($x<=$publish_client)) ; do
    #if ! (($x%50)); then
        #valgrind  ./$TEST_APP -o log.p.$x -t $publish_topic -a publish_wait -s $ismserver $ismserver_addnl -n $publish_count -w $publish_rate -q $publish_qos -z $publish_connections_per_client &
    #else
        ./$TEST_APP -o log.p.$x -t $publish_topic -a publish_wait -s $ismserver $ismserver_addnl -n $publish_count -w $publish_rate -q $publish_qos -z $publish_connections_per_client &
        #./$TEST_APP -o log.p.$x -t $publish_topic  -a publish -s $ismserver $ismserver_addnl -n $publish_count -w $publish_rate -q $publish_qos -z  $publish_connections_per_client &
    #fi
    #if [ "$x" == "100" ] ; then
    if ! (($x%1000)); then
        let tmp_total=($publish_connections_per_client*$x);
        echo "Start intermediate verification."
        verify_condition "Connected to $ismserver" $tmp_total "log.p.*" $j
    fi
    let x=$x+1;
done
verify_condition "Connected to $ismserver" $publish_total "log.p.*" $j
verify_condition "All Clients Connected" $publish_client "log.p.*" $j
echo ./$TEST_APP -o log.s.1 -a subscribe -t $subscribe_topic -w $subscribe_rate -s $ismserver $ismserver_addnl -n $subscribe_count -q $subscribe_qos -z $subscribe_connections_per_client &
./$TEST_APP -o log.s.1 -a subscribe -t $subscribe_topic -w $subscribe_rate -s $ismserver $ismserver_addnl -n $subscribe_count -q $subscribe_qos -z $subscribe_connections_per_client &
verify_condition "All Clients Subscribed" 1 "log.s.*" $j
killall -12 $TEST_APP ; # signal to publishers that they may start publishing
verify_condition "All Clients Published All" $publish_client "log.p.*" $j
verify_condition "TEST_COMPLETE" $publish_client "log.p.*" $j
verify_condition "TEST_COMPLETE" 1 "log.s.*" $j

    #echo "===================================="
    #echo "Iteration:$j : Completed with Success."
    #echo "===================================="
#date
#sleep 60;
#date
    #let j=$j+1
#done 




