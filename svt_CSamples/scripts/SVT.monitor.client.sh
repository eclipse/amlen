#!/bin/bash

. /niagara/test/svt_cmqtt/svt_test.sh

LOGFILE_ADDN=${1-""}
thishost=`hostname -i`
LOGFILE="log.client.memory.${LOGFILE_ADDN}.${thishost}"

#-----------------------------------------
# 
#-----------------------------------------
{ 
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
done | add_log_time "${thishost}" | tee -a $LOGFILE
} &
