#!/bin/bash
ismserver=${1-"10.10.3.22:16901"}

for publish_client in 1 2 3 5 10 100 250 590; do
for publish_connections_per_client in  1 2 3 5 10 100 250 590; do
for publish_count in 1 2 3 5 10 100 1000 10000 100000 ; do
for publish_rate in 0 1 10 100 ; do
for publish_qos in 0 1 2; do
for publish_topic in /MyTopic ;do
subscribe_topic=$publish_topic
if (($publish_qos>=1)) ; then
    if ((publish_count>100)); then
        # throttle for higher level of qos
        publish_count=10;
        break; # don't even try it.
    fi
fi
for subscribe_qos in 0 1 2; do
for subscribe_rate in 0 ; do
for subscribe_client in  1 ; do
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
    echo ./fan_in.sh $publish_connections_per_client $publish_client $publish_count $publish_rate $publish_qos $publish_topic $subscribe_connections_per_client $subscribe_client $subscribe_total $subscribe_rate $subscribe_qos $subscribe_topic $ismserver
    echo "TODO:analyze data"
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

