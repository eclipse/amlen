#!/bin/bash

source cluster.cfg

i=$1


command() {
  echo curl -X GET http://${SERVER[$i]}:${PORT}/ima/v1/service/status
  s=`curl -s -X GET  http://${SERVER[$i]}:${PORT}/ima/v1/service/status`
  echo $s | grep --color '^\|"Status = [^"]*"\|"ConnectedServers":[^,]*\|"V[^,:]*-[^,]*\|"IMA-[^,]*\|"Active"'
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


