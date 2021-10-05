#!/bin/bash

#  ./tracelevel.sh 1 5,util:9

source cluster.cfg

i=$1
l=$2


command() {
  echo curl -X POST -d '{ "TraceLevel": "'$l'" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  curl -X POST -d '{ "TraceLevel": "'$l'" }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  echo curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceLevel
  curl -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/configuration/TraceLevel
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


