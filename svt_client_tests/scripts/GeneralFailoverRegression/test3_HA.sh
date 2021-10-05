#!/bin/bash
# $1 string of servers
echo "Using servers $1 "

#lets parse server string
OLDIFS=$IFS
IFS=,
servers=($1)
IFS=$OLDIFS
echo "serverA= ${servers[0]}"
echo "serverB= ${servers[1]}"

nohup java -Xmx2048m -classpath "/niagara/test/lib/*:/niagara/sdk/ImaClient/*:/niagara/application/client_ship/lib/*" -DMyTraceFile=stdout -DMyTrace=true com.ibm.ima.svt.regression.GeneralFailoverRegression $1 ${servers[0]} ${servers[1]}  1200000 360000 mixed isFailover true 1000000000 2>&1 >test3_HA.out &
