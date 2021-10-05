#!/bin/bash

source cluster.cfg

i=$1


command() {
# echo ssh ${SERVER[$i]} "if [ ! -f /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg.bak ]; then mv /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg.bak ; fi"
# ssh ${SERVER[$i]} "if [ ! -f /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg.bak ]; then mv /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg /var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg.bak ; fi"

  echo scp ./vmprofile.cfg ${SERVER[$i]}:/var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg
  scp ./vmprofile.cfg ${SERVER[$i]}:/var/messagesight/${CLUSTER[$i]}/data/config/vmprofile.cfg

}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

