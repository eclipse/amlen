
#!/bin/bash

source cluster.cfg

i=$1

command() {
# curl -X POST -d '{"ClusterMembership":{ "EnableClusterMembership":true, "UseMulticastDiscovery": false }}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  curl -X POST -d '{"ClusterMembership":{ "EnableClusterMembership":true }}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
  echo curl -X POST -d '{"ClusterMembership":{ "EnableClusterMembership":true }}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi

