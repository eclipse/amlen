#!/bin/bash

#--------------------------------
# Test script being used to recreate a defect
# - Start 15,000 MQTT C Clients
#--------------------------------

verify_condition() {
    local condition=$1
    local count=$2
    local file_pattern=$3
    local iteration=$4
    local myregex="[0-9]+"
    

    if ! [[ "$count" =~ $myregex ]] ; then
        echo "ERROR: count non-numeric"
        return 1;
    fi
    k=0;
    while (($k<$count)); do
        w=`grep "WARNING" $file_pattern |wc -l`
        e=`grep "ERROR" $file_pattern |wc -l`
        c=`ps -ef | grep mqttsample_svt | grep -v grep  | wc -l`
        k=`grep "$condition" $file_pattern |wc -l`
        echo "$iteration: Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
        sleep 2;
    done
    echo "Success."
    return 0;
}

cd /niagara/test/xlinux/bin64

. ~/unit.include
publish_client=15000;     # The number of publisher clients
publish_count=100000000;  # Each publisher publishes this many messages (all on same topic)
publish_rate=1;   # The desired rate (message/second) to publish messages
ismserver=10.10.1.214;
#ismserver=10.10.1.59;

killall -9 mqttsample_svt;
rm -rf log.*
let x=1;
while (($x<=$publish_client)) ; do ./mqttsample_svt -o log.p.$x -a publish -s $ismserver:16102 -n $publish_count -w $publish_rate -m "message from $x" -q 1 & let x=$x+1; done
verify_condition "Connected to $ismserver" $publish_client "log.p." 0


