#!/bin/bash

source cluster.cfg

which=$1

command() {
  echo ssh ${SERVER[$i]} docker rmi ${which}
  ssh ${SERVER[$i]} docker rmi ${which}
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


