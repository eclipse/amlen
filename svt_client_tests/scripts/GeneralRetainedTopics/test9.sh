#!/bin/bash
#$1 single server string
echo "Using server $1"

nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralRetainedTopics $1 $1 null 1000000000 true 2>&1 > test9.out &
