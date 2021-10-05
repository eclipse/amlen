#!/bin/bash
rm -rf log.messaging
let iteration=0;
{
while((1)); do  
    echo "`date` : INFO: Starting messaging iteration $iteration" 
    ./RTC.20899.d.sh
    rc=$?
    echo "`date` : INFO: Done messaging iteration $iteration" 
    echo "`date` : INFO: Messaging was active during these times for iteration $iteration " 
    grep -h SVT_MQTT_C_STATUS log.s* log.p* | awk '{print $2}' | cut -b-8 |sort -u | awk '{printf( "Active Messaging|%s\n", $0);}'
    echo "`date` : RESULT : $rc ITERATION: $iteration : Messaging"
    let iteration=$iteration+1;
done
} 2>&1 | tee -a log.messaging
