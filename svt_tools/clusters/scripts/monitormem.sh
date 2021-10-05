#!/bin/bash

#SERVER1=10.113.50.53
#SERVER2=10.113.50.15

SERVER[4]=10.113.50.31
SERVER[5]=10.113.50.35
SERVER[6]=10.113.50.44
SERVER[7]=10.113.50.23
SERVER[8]=10.113.50.33
SERVER[9]=10.113.50.39

PORT=5003

> monitormem.out

while [ true ]; do
  for i in {4..9}; do
    date 
    echo curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Memory |tee -a monitormem.out
    curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Memory |tee -a monitormem.out
    echo |tee -a monitormem.out
  done
  echo |tee -a monitormem.out
  sleep 2s
done


