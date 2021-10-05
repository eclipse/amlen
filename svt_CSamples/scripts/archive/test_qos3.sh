#!/bin/bash 
#------------------------------------------------------
# TEST:
#------------------------------------------------------

TEST_APP=mqttsample_svt

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


subscriber_clients=2;

let rate=50000;
while (($rate<300000)); do
    rm -rf log.*
    echo "Start test for rate $rate" > out.qos.0.$rate
    #./mqttsample_svt -s 10.10.3.22:16102 -n 1000000 -a subscribe -x verifyStillActive=30 -o log.s.1 &
    #verify_condition "Subscribed - Ready" 1 "log.s.*" $rate 
    let pub_rate=($rate/10);
    let pub_count=(1000000/10);
    let sub_count=(1000000/$subscriber_clients);
    let i=0;
    while (($i<$subscriber_clients)); do
        echo "./mqttsample_svt -s 10.10.3.22:16102 -n sub_count  -t $i -a subscribe -o log.s.$i -x verifyStillActive=30  &"
        ./mqttsample_svt -s 10.10.3.22:16102 -n $sub_count -t $i -a subscribe -o log.s.$i -x verifyStillActive=30  &
        let i=$i+1;
    done
    verify_condition "Subscribed - Ready" $subscriber_clients "log.s.*" $rate 
    let i=0;
    while (($i<10)); do
        if (($i>=5)) ; then
            mytopic="1";
        else
            mytopic="0";
        fi
        echo "./mqttsample_svt -s 10.10.3.22:16102 -t $mytopic -n $pub_count -a publish -w $pub_rate -o log.p.$i &"
        ./mqttsample_svt -s 10.10.3.22:16102 -t $mytopic -n $pub_count -a publish -w $pub_rate -o log.p.$i &
        let i=$i+1;
    done
    verify_condition "SVT_MQTT_C_INFO_END" 10 "log.p.*" $rate 
    verify_condition "SVT_MQTT_C_INFO_END" $subscriber_clients "log.s.*" $rate 
    /home/developer/monitor.c.client.sh >> out.qos.0.$rate
    cat ./log.s.* >>  out.qos.0.$rate
    cat ./log.p.* >>  out.qos.0.$rate
    let rate=$rate+5000;
done


