#! /bin/bash

#----------------------------------------------------------------------------
# This script is to stop and restart eth1 on A1. 
# It must be called from the same machine where DriverSync is running. 
#----------------------------------------------------------------------------

function timeout {
echo "ERROR: DriverSync wait request timed out"
}

# Source the ISMsetup file
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

# Initialize the eth1_started variable
java test.llm.TestClient ${SYNCPORT} -n 127.0.0.1 -f sync.log -u '1 eth1_started'

# Wait for the indication from client2 that the interface should be shutdown
RESULT=`java test.llm.TestClient ${SYNCPORT} -n 127.0.0.1 -f sync.log -u '3 client2_ready 1 5000'`
echo ${RESULT}
case ${RESULT} in
  *"Waited: 0"*) timeout;;
esac

# Disable the eth1 interface
ssh admin@A1_IPv4_1 netif set eth1 enabled=false

# Wait for the indication from client2 that the interface should be restarted
RESULT=`java test.llm.TestClient ${SYNCPORT} -n 127.0.0.1 -f sync.log -u '3 client2_disconnected 1 20000'`
echo ${RESULT}
case ${RESULT} in
  *"Waited: 0"*) timeout;;
esac

# Enable the eth1 interface
ssh admin@A1_IPv4_1 netif set eth1 enabled=true

# Let the clients know that the interface is ready
java test.llm.TestClient ${SYNCPORT} -n 127.0.0.1 -f sync.log -u '2 eth1_started 1'

