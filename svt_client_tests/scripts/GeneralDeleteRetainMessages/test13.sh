#!/bin/bash
#$1 single server string
echo "Using server $1"
nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralDeleteRetainedMessages $1 SVTDELSUB 0 1000000000 true 2>&1 > test13.out &
