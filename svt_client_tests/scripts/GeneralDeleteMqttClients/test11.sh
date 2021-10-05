#!/bin/bash
#$1 single server string
echo "Using server $1"
nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralDeleteMqttClients $1 null true 1000000000 2>&1 > test11.out &
