#!/bin/bash 
TEST_APP=mqttsample_array
file_pattern="log.s.*"
while ((1)); do
    w=`grep "WARNING" $file_pattern 2>/dev/null |wc -l`
    e=`grep "ERROR" $file_pattern 2>/dev/null  |wc -l`
    c=`ps -ef | grep $TEST_APP | grep -v grep  | wc -l`
    echo "`date` Monitor clients warnings($w) errors:($e) clients:($c)"
    sleep 1;
done
