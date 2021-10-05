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

if [ -z "$1" ] ; then
  echo must specify 4..9
  exit
fi
i=$1

OUT=monmem$1.out
> ${OUT}

while [ true ]; do
  date |tee -a ${OUT}
  echo curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Memory |tee -a ${OUT}
  curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Memory |tee -a ${OUT}
  echo |tee -a ${OUT}
  echo |tee -a ${OUT}
  date |tee -a ${OUT}
  echo curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Store |tee -a ${OUT}
  curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Store |tee -a ${OUT}
  echo |tee -a ${OUT}
  echo |tee -a ${OUT}
  date |tee -a ${OUT}
  echo curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Subscription |tee -a ${OUT}
  curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/monitor/Subscription |tee -a ${OUT}
  echo |tee -a ${OUT}
  echo |tee -a ${OUT}
  date |tee -a ${OUT}
  echo curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/service/status |tee -a ${OUT}
  curl -s -X GET http://${SERVER[$i]}:${PORT}/ima/v1/service/status |tee -a ${OUT}
  echo |tee -a ${OUT}
  echo |tee -a ${OUT}
  sleep 2s
done


