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

killall -12 $TEST_APP ; # signal to publishers that they may start publishing
verify_condition "All Clients Published All" $publish_client "log.p.*" $j
verify_condition "TEST_COMPLETE" $publish_client "log.p.*" $j
verify_condition "TEST_COMPLETE" 1 "log.s.*" $j
