#!/bin/bash

#----------------------------------------------
# If the automation is pushing the client too hard the log files saved here can help to explain that.
#----------------------------------------------

. /niagara/test/scripts/af_test.sh

af_automation_env

LOGFILE_PREFIX=${1-"log.client.memory"}
thishost=`hostname -i`
LOGFILE="${LOGFILE_PREFIX}.${THIS}.${thishost}"


client_ethX=`ifconfig |grep -C2 10.10.[01] |grep -oE 'eth.*|br.*' | awk '{print $1}' | head -1`



#-----------------------------------------
# 
#-----------------------------------------
{ 
rm -rf $LOGFILE
while((1)); do
    dmsgdata=`dmesg -c`
    if echo "$dmsgdata" | grep "Out of memory" ; then
        echo "ERROR: Detected Out of memory condition in dmesg"
    fi
    data=`cat /proc/meminfo`
    echo "$data" |grep MemFree
    ifconfig $client_ethX | grep -E 'RX|TX|collision' | awk '{printf("'$client_ethX' %s\n",$0);}'
    df | grep -v "/mnt" | awk '{printf("df %s\n",$0);}'
    sleep 1;
done | af_add_log_time "${thishost}" | tee -a $LOGFILE
} &
