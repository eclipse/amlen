#!/bin/bash

source cluster.cfg

i=$1


command() {
  echo curl -X POST -d '{"Service":"Server","CleanStore":false}' http://${SERVER[$i]}:${PORT}/ima/v1/service/restart
  curl -X POST -d '{"Service":"Server","CleanStore":false}' http://${SERVER[$i]}:${PORT}/ima/v1/service/restart
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

