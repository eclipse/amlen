#!/bin/bash


SERVER[1]=10.142.70.168
SERVER[2]=10.142.70.142
SERVER[3]=10.142.70.152
SERVER[4]=10.142.70.154
SERVER[5]=10.142.70.135
SERVER[6]=10.142.70.181


PORT=5003


for i in {1..6}; do
  echo ssh ${SERVER[$i]} systemctl start tunehost
  ssh ${SERVER[$i]} systemctl start tunehost
done

