#!/bin/bash

rm -rf log.client.memory

while((1)); do
    dmsgdata=`dmesg -c`
    if echo "$dmsgdata" | grep "Out of memory" ; then
        echo "ERROR: Detected Out of memory condition in dmesg"
    fi
    data=`cat /proc/meminfo`
    echo "$data" |grep MemFree
    date;
    sleep 1;
done | tee -a log.client.memory
