#!/bin/bash
#$1 single server string
echo "Using server $1"
nohup java -Xmx2048m -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralFailoverRegression $1 $1 null 1200000 360000 mixed isFailover true 1000000000 2>&1 >test3.out &
