#!/bin/bash

source cluster.cfg

i=$1


command() {
  echo ssh ${SERVER[$i]} docker exec ${CLUSTER[$i]} rm /var/messagesight/diag/logs/imatrace.log
  ssh ${SERVER[$i]} docker exec ${CLUSTER[$i]} rm /var/messagesight/diag/logs/imatrace.log
  echo ssh ${SERVER[$i]} docker exec ${CLUSTER[$i]} rm /var/messagesight/diag/logs/imatrace_prev.log
  ssh ${SERVER[$i]} docker exec ${CLUSTER[$i]} rm /var/messagesight/diag/logs/imatrace_prev.log
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

