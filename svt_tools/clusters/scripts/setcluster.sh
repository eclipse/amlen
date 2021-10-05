
#!/bin/bash

source cluster.cfg

i=$1

command() {
  echo curl -X POST -d ' { "Version": "v1", "ClusterMembership": { "EnableClusterMembership": true, "UseMulticastDiscovery": true, "MulticastDiscoveryTTL": 16, "ControlPort": 5001, "MessagingPort": 5002, "MessagingUseTLS": false, "DiscoveryPort": 5104, "DiscoveryTime": 30, "MessagingAddress": "10.142.70.173", "ClusterName": "PURPLE01", "ControlAddress": "10.142.70.173", "ControlExternalAddress": null, "ControlExternalPort": null, "MessagingExternalPort": null, "MessagingExternalAddress": null, "DiscoveryServerList": "10.142.70.173:5001,10.120.16.122:5001" } } http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  curl -X POST -d ' { "Version": "v1", "ClusterMembership": { "EnableClusterMembership": true, "UseMulticastDiscovery": true, "MulticastDiscoveryTTL": 16, "ControlPort": 5001, "MessagingPort": 5002, "MessagingUseTLS": false, "DiscoveryPort": 5104, "DiscoveryTime": 30, "MessagingAddress": "10.142.70.173", "ClusterName": "PURPLE01", "ControlAddress": "10.142.70.173", "ControlExternalAddress": null, "ControlExternalPort": null, "MessagingExternalPort": null, "MessagingExternalAddress": null, "DiscoveryServerList": "10.142.70.173:5001,10.120.16.122:5001" } } http://${SERVER[$i]}:${PORT}/ima/v1/configuration
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

