#!/bin/bash

source cluster.cfg

i=$1


command() {
  echo ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace.log
  ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace.log
  echo ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace_prev.log
  ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace_prev.log
  echo ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace*.log.gz
  ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imatrace*.log.gz
  echo ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imaserver-connection.log
  ssh ${SERVER[$i]}  rm /var/messagesight/${CLUSTER[$i]}/diag/logs/imaserver-connection.log
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

