#!/bin/bash

source cluster.cfg

i=$1

if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    echo ssh ${SERVER[$i]} docker start ${CLUSTER[$i]}
    ssh ${SERVER[$i]} docker start ${CLUSTER[$i]}
  done
else 
  echo ssh ${SERVER[$i]} docker start ${CLUSTER[$i]}
  ssh ${SERVER[$i]} docker start ${CLUSTER[$i]}
fi
