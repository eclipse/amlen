#!/bin/bash

#----------------------------------------------------------------------------
# Source the ISMsetup file to get access to information for the remote machine
#----------------------------------------------------------------------------
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

appliance=1
while [ ${appliance} -le ${A_COUNT} ]
do
    aIP="A${appliance}_IPv4_1"
    aIPVal=$(eval echo \$${aIP})
    
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > server${appliance}.xml
    echo "<ActionParameter name=\"ip\">${aIPVal}</ActionParameter>" >> server${appliance}.xml
    
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > server${appliance}failover.xml
    echo "<ActionParameter name=\"servers\">${aIPVal};${aIPVal}</ActionParameter>" >> server${appliance}failover.xml

    echo "Created server${appliance}.xml and server${appliance}failover.xml for server IP: ${aIPVal}"
    ((appliance+=1))
done

scp server*.xml ${M2_USER}@${M2_HOST}:${M2_TESTROOT}/cluster_tests/
