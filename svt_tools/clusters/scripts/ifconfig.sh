#!/bin/bash

source cluster.cfg

i=$1

command() {
  echo ssh ${SERVER[$i]} ifconfig
  ssh ${SERVER[$i]} ifconfig
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


