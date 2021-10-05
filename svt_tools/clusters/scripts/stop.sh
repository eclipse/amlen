#!/bin/bash

source cluster.cfg
i=$1

command() {
  echo curl -X POST -d '{"Service":"Server"}' http://${SERVER[$i]}:${PORT}/ima/v1/service/stop
  curl -X POST -d '{"Service":"Server"}' http://${SERVER[$i]}:${PORT}/ima/v1/service/stop
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


