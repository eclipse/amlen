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

let publish_connections_per_client=100;
publish_client=95;
let publish_total=($publish_connections_per_client*$publish_client)
publish_count=100000000;  # Each publisher publishes this many messages (all on same topic)
let subscribe_total=($publish_total*$publish_count)
publish_rate=1;   # The desired rate (message/second) to publish messages
#ismserver=10.10.1.214:16102
#ismserver=10.10.3.22:16901
ismserver=10.10.10.10:16102

#killall -9 $TEST_APP;
#ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}



let j=0;
let x=0;
while [ -e log.p.$x ] ; do 
let x=x+1;
done

let starting_point=$x
echo "Calculated starting point as $x"
let endpoint=($starting_point+$publish_client);
echo "Calculated endpoint as $endpoint"
let publish_total=($publish_connections_per_client*$endpoint);
echo "Calculated publish_total as $publish_total"

while (($x<$endpoint)) ; do
    #if ! (($x%50)); then
        #valgrind  ./$TEST_APP -o log.p.$x -a publish_wait -s $ismserver -n $publish_count -w $publish_rate -q 0 -z $publish_connections_per_client &
    #else
        ./$TEST_APP -o log.p.$x -a publish_wait -s $ismserver -n $publish_count -w $publish_rate -q 0 -z  $publish_connections_per_client &
        #./$TEST_APP -o log.p.$x -a publish -s $ismserver -n $publish_count -w $publish_rate -q 0 -z  $publish_connections_per_client &
    #fi
    #if [ "$x" == "100" ] ; then
    if ! (($x%150)); then
        let tmp_total=($publish_connections_per_client*$x);
        echo "Start intermediate verification."
        verify_condition "Connected to $ismserver" $tmp_total "log.p.*" $j
    fi
    let x=$x+1;
done
verify_condition "Connected to $ismserver" $publish_total "log.p.*" $j
verify_condition "All Clients Connected" $endpoint "log.p.*" $j
