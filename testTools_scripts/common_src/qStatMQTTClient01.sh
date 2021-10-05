#! /bin/bash

cmd="curl -X GET -s -S http://A1_HOST:A1_PORT/ima/v1/monitor/MQTTClient?StatType=LastConnectedTimeOldest"  

rc1=`$cmd | grep stat01.receive1 | wc -l`
rc2=`$cmd | grep stat01.receive2 | wc -l`
rc3=`$cmd | grep stat01.receive3 | wc -l`
rc4=`$cmd | wc -l`

rc=0
    
if [[ $rc1 -ne 1 ]] ; then
  echo "Expecting one line containing stat01.receive1, found $rc1" 
  rc=`expr $rc + 1`
fi

if [[ $rc2 -ne 0 ]] ; then
  echo "Expecting no lines containing stat01.receive2, found $rc2" 
  rc=`expr $rc + 2`
fi

if [[ $rc3 -ne 0 ]] ; then
  echo "Expecting no lines containing stat01.receive3, found $rc3" 
  rc=`expr $rc + 4`
fi

# depending on run order, some other tests seem to be leaving
#   other disconnected clients. Cannot count total lines 
#if [[ $rc4 -ne 3 ]] ; then
#  echo "Expecting three lines total in response, found $rc4" 
#  rc=`expr $rc + 8`
#fi

if [[ $rc -ne 0 ]] ; then
  $cmd
fi

exit $rc
