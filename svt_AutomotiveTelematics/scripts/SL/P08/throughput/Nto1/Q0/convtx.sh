#!/bin/bash

#########################################################################################################
#
# clientid|cleansession|username|password|txtopic|rxtopic|destaddress|destport|security|websockets|
#
# cleansession      0 or 1
# txtopic,rxtopic   qos:retained:topic
# destaddress       orgid.message...lon02-1.test.internet...
# destport          8883 or 1883 or 443
# security          0 or 1
# websockets        0 or 1
#
# also...
#    topic can contain iot-dev-${COUNT:0-99}
#    which equates to iot-dev-0 ...
#                     iot-dev-99
#########################################################################################################

file=$1

if [ -z $2 ]; then
  qos=0
else
  qos=$2
fi

cat $file | while read line; do
  IFS=$'|' read -r -a array <<< "$line"
  clientid=${array[0]}
  topic=${array[1]}
  username=${array[2]}
  password=${array[3]}
  cleansession=1
  retained=0

  echo ${clientid}'|'$cleansession'|'${username}'|'${password}'|'$qos':'$retained':'$topic'||'
done

