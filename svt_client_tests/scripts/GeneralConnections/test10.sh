#!/bin/bash
# $1 single server string
echo "Using server $1"
nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralConnections $1 10000000000 SVTCONN 500 0 true 2>&1 > test10.out &
