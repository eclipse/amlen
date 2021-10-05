#!/bin/bash

source cluster.cfg

i=$1


command() {
  echo time curl -X GET http://${SERVERPUB[$i]}:${PORT}/ima/v1/service/status
  time curl -s -X GET  http://${SERVERPUB[$i]}:${PORT}/ima/v1/service/status
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


