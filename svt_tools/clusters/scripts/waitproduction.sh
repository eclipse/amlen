#!/bin/bash 

source cluster.cfg

i=$1


command() {
  p=0
  while [ $p -ne 1 ]; do
    echo curl -X GET http://${SERVER[$i]}:${PORT}/ima/v1/service/status
    s=`curl -X GET http://${SERVER[$i]}:${PORT}/ima/v1/service/status 2> /dev/null`
    p=`echo $s| grep production| wc -l`
    if [ $p -ne 1 ]; then
      echo sleep 5s
      sleep 5s
      echo $s | grep --color '^\|"Status = [^"]*"\|"IMA-[^,]*'
    else
      echo $s | grep --color '^\|"Status = [^"]*"\|"ConnectedServers":[^,]*\|"V[^,:]*-[^,]*\|"IMA-[^,]*\|"Active"'
    fi
  done
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


