#!/bin/bash

#  ./tracelevel.sh 1 5,util:9

source cluster.cfg

COUNT=100

command() {
  echo curl -X POST -d '{ "TraceBackup": "1" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  curl -X POST -d '{ "TraceBackup": "1" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  echo curl -X POST -d '{ "TraceBackupCount": "'${COUNT}'" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  curl -X POST -d '{ "TraceBackupCount": "'${COUNT}'" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  echo curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceBackup
  curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceBackup
  echo curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceBackupCount
  curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceBackupCount
}

if [ -z $l ]; then
  l=$i
  i=
fi

if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command 
  done
else 
    command 
fi


