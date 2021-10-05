#!/bin/bash
#$1 single server string
echo "Using server $1"
nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralSubscriptionClients $1 $1 null true 4000000 true 2>&1 > test8.out &
