#!/bin/bash

source cluster.cfg

i=$1

if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    echo ssh ${SERVER[$i]} docker images -a
    ssh ${SERVER[$i]} docker images -a
  done
else 
  echo ssh ${SERVER[$i]} docker images -a
  ssh ${SERVER[$i]} docker images -a
fi
