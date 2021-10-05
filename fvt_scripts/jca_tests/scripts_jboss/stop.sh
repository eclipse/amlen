#!/bin/bash
## 
## stop.sh
## Description: This script will launch JBOSS AppServer
IP=`echo ${WASIP} | cut -d: -f1`

STOP_COMMAND="kill -s SIGKILL \`ps -ef | grep org.jboss | grep -v grep | awk '{print \$2}'\`"

echo "STOP_COMMAND: $STOP_COMMAND"

(echo $STOP_COMMAND | ssh ${IP}) || (true)

sleep 3 # run-scenarios hack
