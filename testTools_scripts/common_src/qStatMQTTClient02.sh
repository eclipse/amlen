#! /bin/bash

cmd="curl -X GET -s -S http://A1_HOST:A1_PORT/ima/v1/monitor/MQTTClient?StatType=LastConnectedTimeOldest"

rc1=`${cmd}\&ResultCount=10 | grep -o ClientID | wc -l`
rc2=`${cmd}\&ResultCount=25 | grep -o ClientID | wc -l`
rc3=`${cmd}\&ResultCount=50 | grep -o ClientID | wc -l`
rc4=`${cmd}\&ResultCount=100 | grep -o ClientID | wc -l` 

rc=0

if [[ $rc1 -ne 10 ]] ; then
  echo "Expecting ten lines when ResultCount=10, found $rc1" 
  ${cmd}\&ResultCount=10
  echo done
  rc=`expr $rc + 1`
else
  echo "Ten lines passed"
fi

if [[ $rc2 -ne 25 ]] ; then
  echo "Expecting 25 lines when ResultCount=25, found $rc2" 
  ${cmd}\&ResultCount=25
  echo done
  rc=`expr $rc + 2`
else
  echo "25 lines passed"  
fi

if [[ $rc3 -ne 50 ]] ; then
  echo "Expecting 50 lines when ResultCount=50, found $rc3"
  ${cmd}\&ResultCount=50
  echo done 
  rc=`expr $rc + 4`
else
  echo "50 lines passed"  
fi

if [[ $rc4 -ne 100 ]] ; then
  echo "Expecting 100 lines when ResultCount=100, found $rc4"
  ${cmd}\&ResultCount=100
  echo done 
  rc=`expr $rc + 8`
else
  echo "100 lines passed"  
fi

addrc=0
rc5=`${cmd}\&ResultCount=1 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"1\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=1
  ${cmd}\&ResultCount=1
  addrc=16
else
  echo "ResultCount=1 failed as expected"  
fi

rc5=`${cmd}\&ResultCount=9 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"9\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=9
  ${cmd}\&ResultCount=9
  addrc=16
else
  echo "ResultCount=9 failed as expected"    
fi

rc5=`${cmd}\&ResultCount=11 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"11\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=11
  ${cmd}\&ResultCount=11
  addrc=16
else
  echo "ResultCount=11 failed as expected"    
fi

rc5=`${cmd}\&ResultCount=24 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"24\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=24
  ${cmd}\&ResultCount=24
  addrc=16
else
  echo "ResultCount=24 failed as expected"    
fi

rc5=`${cmd}\&ResultCount=26 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"26\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=26
  ${cmd}\&ResultCount=26
  addrc=16
else
  echo "ResultCount=26 failed as expected"    
fi

rc5=`${cmd}\&ResultCount=49 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"49\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=49
  ${cmd}\&ResultCount=49
  addrc=16
fi

rc5=`${cmd}\&ResultCount=51 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"51\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=51
  ${cmd}\&ResultCount=51
  addrc=16
else
  echo "ResultCount=51 failed as expected"    
fi

rc5=`${cmd}\&ResultCount=99 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"99\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=99
  ${cmd}\&ResultCount=99
  addrc=16
fi

rc5=`${cmd}\&ResultCount=101 2>&1 | grep 'The property value is not valid: Property: ResultCount Value: \\\\\"101\\\\\".' | wc -l`
if [[ $rc5 -ne 1 ]] ; then
  echo Unexpected result from command: ${cmd}\&ResultCount=101
  ${cmd}\&ResultCount=101
  addrc=16
else
  echo "ResultCount=101 failed as expected"    
fi

rc=`expr $rc + $addrc`


if [[ $rc -ne 0 ]] ; then
  ${cmd:0:${#cmd}-1}
fi

exit $rc
