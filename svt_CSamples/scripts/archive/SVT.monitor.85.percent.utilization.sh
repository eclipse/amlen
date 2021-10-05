#!/bin/bash

rm -rf /niagara/test/svt_cmqtt/log.mem.utilization.85.percent.$server

smallest_mem_utilization=100

while((1)); do
    data=`ssh admin@mar425 "advanced-pd-options memorydetail" | grep MemoryFreePercent | awk '{print $3}'`

    if echo "$data" | grep [0-9] > /dev/null ; then
        if [ -n "$data" ] ;then
            if (($data<=15)); then
                echo "Reached 85% message utilization. Break." >> /niagara/test/svt_cmqtt/log.mem.utilization.85.percent.$server
                break;
            elif (($data<$smallest_mem_utilization)); then
                smallest_mem_utilization=$data
            fi
        fi
    fi

    data=`ps -ef | grep mqttsample_array | grep -v grep |wc -l`
    if echo "$data" | grep [0-9] > /dev/null ; then
        if [ -n "$data" ] ;then
            if (($data<=0)); then
                echo "No mqttsample_array processes are running. Break." >> /niagara/test/svt_cmqtt/log.mem.utilization.85.percent.$server
                break;
            fi
        fi
    
    fi

    #data=`grep "WARNING" /niagara/test/svt_cmqtt/log.p.* |wc -l`
    #if echo "$data" | grep [0-9] > /dev/null ; then
        #if [ -n "$data" ] ;then
            #if (($data>=1000)); then
                #if (($smallest_mem_utilization<25)); then 
                    #echo "Warning: Greater than 1000 warning messages and Mem Utililization > 75. Break." 
                    #break;
                #else
                    #echo "Greater than 1000 warning messages, but smallest_mem_utilization:$smallest_mem_utilization > 25"
                #fi
            #fi
        #fi
    #fi

    echo -n "."
    sleep 1;
done

echo "----------------------------------------------"
echo "`date` Stop publishers by injecting stop file."
echo "----------------------------------------------"
echo "`date` stop publishers" >> /niagara/test/svt_cmqtt/log.mem.utilization.85.percent.$server

echo "----------------------------------------------"
echo "`date` Record published messages"
echo "----------------------------------------------"
data=`grep -h "actual_msgs:" log.p.*  | awk 'BEGIN { x=0; } x=x+$2; END {print sum}' |  grep -h "actual_msgs:" log.p.*  | awk 'BEGIN { x=0; } {x=x+$2;} END {print x}' `
echo "$data" >> /niagara/test/svt_cmqtt/log.mem.utilization.85.percent.$server

echo "----------------------------------------------"
echo "`date` Total messages published: $data"
echo "----------------------------------------------"

return 0;
