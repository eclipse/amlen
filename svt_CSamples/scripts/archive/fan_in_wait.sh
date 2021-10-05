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

let publish_connections_per_client=50;
publish_client=1190;
let publish_total=($publish_connections_per_client*$publish_client)
publish_count=10000000;  # Each publisher publishes this many messages (all on same topic)
let subscribe_total=($publish_total*$publish_count)
publish_rate=1;   # The desired rate (message/second) to publish messages
#ismserver=10.10.1.214:16102
ismserver=10.10.3.22:16901

killall -9 $TEST_APP;
ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}

rm -rf log.*
let x=1;
while (($x<=$publish_client)) ; do
    #if ! (($x%50)); then
        #valgrind  ./$TEST_APP -o log.p.$x -a publish_wait -s $ismserver -n $publish_count -w $publish_rate -q 0 -z $publish_connections_per_client &
    #else
        ./$TEST_APP -o log.p.$x -a publish_wait -s $ismserver -n $publish_count -w $publish_rate -q 0 -z  $publish_connections_per_client &
        #./$TEST_APP -o log.p.$x -a publish -s $ismserver -n $publish_count -w $publish_rate -q 0 -z  $publish_connections_per_client &
    #fi
    #if [ "$x" == "100" ] ; then
    if ! (($x%1500)); then
        let tmp_total=($publish_connections_per_client*$x);
        echo "Start intermediate verification."
        verify_condition "Connected to $ismserver" $tmp_total "log.p.*" $j
    fi
    let x=$x+1;
done
verify_condition "Connected to $ismserver" $publish_total "log.p.*" $j
verify_condition "All Clients Connected" $publish_client "log.p.*" $j
./$TEST_APP -o log.s.1 -a subscribe -s $ismserver -n $subscribe_total -q 0 &
verify_condition "All Clients Subscribed" 1 "log.s.*" $j




