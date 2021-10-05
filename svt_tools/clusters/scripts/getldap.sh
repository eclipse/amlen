#!/bin/bash

source cluster.cfg

i=$1


command() {
echo curl -X GET http://${SERVER[$i]}:${PORT}/ima/v1/configuration/LDAP
curl -X GET http://${SERVER[$i]}:${PORT}/ima/v1/configuration/LDAP
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


