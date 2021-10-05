#!/bin/bash

#  ./tracelevel.sh 1 5,util:9

source cluster.cfg

i=$1
l=$2


command() {
  echo curl -X POST -d '{"Endpoint":{"SVT-CarsInternetEndpoint": { "Port": 5004, "Enabled": true, "TopicPolicies": "SVTPubMsgPol-app,SVTSubMsgPol-car", "ConnectionPolicies": "SVTSecureCarsConnectPolicy", "MessageHub": "SVTAutoTeleHub", "Interface": "All", "MaxMessageSize": "256MB", "SecurityProfile": "", "Protocol": "JMS,MQTT", "Description": "", "QueuePolicies": null, "BatchMessages": true, "InterfaceName": "All", "MessagingPolicies": null, "MaxSendSize": 16384, "SubscriptionPolicies": null }}}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration/Endpoint
}

if [ -z $l ]; then
  l=$i
  i=
fi

if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command 
  done
else 
    command 
fi


