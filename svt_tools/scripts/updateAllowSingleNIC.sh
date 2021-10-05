#!/bin/bash

if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

appliance=1
while [ ${appliance} -le ${A_COUNT} ]
do
    aIP="A${appliance}_HOST"
    aIPVal=$(eval echo \$${aIP})

    isPresent=`echo "grep 'HA.AllowSingleNIC' /mnt/A${appliance}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null`
    if [[ "${isPresent}" =~ "HA.AllowSingleNIC" ]] ; then
        echo "sed -i -e 's/HA.AllowSingleNIC.*/HA.AllowSingleNIC = 1/g' /mnt/A${appliance}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null
    else
        echo "echo 'HA.AllowSingleNIC = 1' >> /mnt/A${appliance}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null
    fi

        echo "A${appliance} server.cfg:"
        echo "grep 'HA.AllowSingleNIC' /mnt/A${appliance}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null

        ((appliance+=1))
done
