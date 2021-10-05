#!/bin/bash
# $1 string of servers
echo "Using servers $1"

#lets parse server string
OLDIFS=$IFS
IFS=,
servers=($1)
IFS=$OLDIFS
echo "serverA= ${servers[0]}"
echo "serverB= ${servers[1]}"

nohup java -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralQueueRegression $1 ${servers[0]} ${servers[1]} true false 1000000000 2>&1 >test1_HA.out &
