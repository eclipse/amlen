#!/bin/bash
## 
## start.sh
## Description: This script will launch JBOSS AppServer
IP=`echo ${WASIP} | cut -d: -f1`

START_COMMAND="nohup ${WASPath}/bin/standalone.sh -c standalone-full.xml &"

echo "START_COMMAND: $START_COMMAND"

(nohup echo $START_COMMAND | ssh ${IP} &) || (true)

sleep 3 # run-scenarios hack

