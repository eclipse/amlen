#/bin/bash
# SOURCE this file

# copied PROXY build from BuildServer.
# (local) yum install /niagara/proxy/server_ship/statsd-C-client/statsd-c-client-2.0.1-1.fc23.x86_64.rpm  and  statsd-c-client-debuginfo-2.0.1-1.fc23.x86_64.rpm


export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/niagara/proxy/server_ship/lib:/niagara/proxy/client_ship/lib
export PATH=${PATH}:/niagara/proxy/server_ship/bin
export PROFILEREAD=FALSE

# use startProxy.sh -> imaproxy  -d proxy.cfg
