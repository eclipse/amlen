#!/bin/bash

source cluster.cfg

for i in $(eval echo {1..${#SERVER[@]}}); do
  curl  -X POST -d '{"HighAvailability":{ "EnableHA":true }}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  echo curl -X POST -d '{"HighAvailability":{ "EnableHA":true }}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
done

