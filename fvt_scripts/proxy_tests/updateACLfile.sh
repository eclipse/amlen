#!/bin/bash +x
# for NOW ACLfile filename is hardcoded to:  WIoTP_ACLfile.cfg
# Inputs:  Server ISMsetup Prefix tag  

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

    aUSER="A${appliance}_USER"
    aUSERVal=$(eval echo \$${aUSER})

    aTYPE="A${appliance}_TYPE"
    aTYPEVal=$(eval echo \$${aTYPE})
    if [ "${aTYPEVal}" == "DOCKER" ]; then
        DIRPATH="/mnt/A${appliance}"
    else
        DIRPATH="/var"
    fi
    
    isPresent=`echo "grep 'ACLfile' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aUSERVal}@${aIPVal} 2>/dev/null`
    if [[ "${isPresent}" =~ "ACLfile" ]] ; then
    	echo "sed -i -e 's/ACLfile.*/ACLfile = /var/messagesight/data/config/WIoTP_ACLfile.cfg/g' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aUSERVal}@${aIPVal} 2>/dev/null
    else
    	echo "echo 'ACLfile = /var/messagesight/data/config/WIoTP_ACLfile.cfg' >> ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aUSERVal}@${aIPVal} 2>/dev/null
    fi
	
	echo "A${appliance} server.cfg (expect to see cfg file):"
	echo "grep 'ACLfile' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aUSERVal}@${aIPVal} 2>/dev/null
	 
	((appliance+=1))    
done


##Handled in proxy.ACLfile.cfg
##proxy=1
##while [ ${proxy} -le ${P_COUNT} ]
##do
##    pIP="P${proxy}_HOST"
##    pIPVal=$(eval echo \$${pIP})
##
##    pUSER="P${proxy}_USER"
##    pUSERVal=$(eval echo \$${pUSER})
##
##    pTYPE="P${proxy}_TYPE"
##    pTYPEVal=$(eval echo \$${pTYPE})
##    if [ "${pTYPEVal}" == "DOCKER" ]; then
##        DIRPATH="/mnt/P${proxy}"
##    else
##        DIRPATH="/var"
##    fi
##    
##    isPresent=`echo "grep 'ACLfile' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${pUSERVal}@${pIPVal} 2>/dev/null`
##    if [[ "${isPresent}" =~ "ACLfile" ]] ; then
##    	echo "sed -i -e 's/ACLfile.*/ACLfile = /var/messagesight/data/config/WIoTP_ACLfile.cfg/g' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${pUSERVal}@${pIPVal} 2>/dev/null
##    else
##    	echo "echo 'ACLfile = /var/messagesight/data/config/WIoTP_ACLfile.cfg' >> ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${pUSERVal}@${pIPVal} 2>/dev/null
##    fi
##	
##	echo "P${proxy} server.cfg (expect to see cfg file):"
##	echo "grep 'ACLfile' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${pUSERVal}@${pIPVal} 2>/dev/null
##	 
##	((proxy+=1))    
##done
