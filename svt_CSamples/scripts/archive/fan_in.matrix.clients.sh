#!/bin/bash
rm -rf out.log
let j=0;
ismserver=${1-"10.10.3.22:16901"}

for publish_client in 1 2 3 4 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100; do
for publish_connections_per_client in 1 2 3 4 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100; do
for publish_count in 1000; do
for publish_rate in 0 ; do
for publish_qos in 0 ; do
for publish_topic in /MyTopic ;do
subscribe_topic=$publish_topic
if (($publish_qos>=1)) ; then
    if ((publish_count>100)); then
        # throttle for higher level of qos
        publish_count=10;
        break; # don't even try it.
    fi
fi
for subscribe_qos in 1; do
for subscribe_rate in 0 ; do
for subscribe_client in 1 2 ; do
for subscribe_connections_per_client in 1;  do
    let publish_total=($publish_connections_per_client*$publish_client)
    let subscribe_total=($publish_total*$publish_count)
    if ((subscribe_total>1000)); then
        if (($publish_qos>=1)) ; then
            break; #it will take too long
        fi
    fi
    if ((publish_total>59000)); then
            break; #not supported
    fi
    ./fan_in.sh $publish_connections_per_client $publish_client $publish_count $publish_rate $publish_qos $publish_topic $subscribe_connections_per_client $subscribe_client $subscribe_total $subscribe_rate $subscribe_qos $subscribe_topic $ismserver
    echo "START  Test $j: =========================================================" >> out.log
    date >> out.log
    echo -n "STATS Test $j: " >> out.log
    echo -n " Inputs: ./fan_in.sh $publish_connections_per_client $publish_client $publish_count $publish_rate $publish_qos $publish_topic $subscribe_connections_per_client $subscribe_client $subscribe_total $subscribe_rate $subscribe_qos $subscribe_topic $ismserver " >> out.log
    echo -n " Results: " >> out.log
    /home/developer/monitor.c.client.sh >> out.log
    echo -e "\nEND  Test $j: =========================================================" >> out.log
    date >> out.log
    let j=j+1;
done
done
done
done
done
done
done
done
done
done

