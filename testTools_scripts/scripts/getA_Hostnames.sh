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
# Get SSH parameter to connect to server
    A_USER="A${appliance}_USER"
    A_USERVal=$(eval echo \$${A_USER})
    A_HOST="A${appliance}_HOST"
    A_HOSTVal=$(eval echo \$${A_HOST})
# get the varaible names to Export	
    A_HOSTNAME_OS_SHORT="A${appliance}_HOSTNAME_OS_SHORT"
    A_HOSTNAME_OS="A${appliance}_HOSTNAME_OS"
    A_SERVERNAME="A${appliance}_SERVERNAME"
    ssh_value=`ssh $A_USERVal@$A_HOSTVal "hostname -s"`

    export $A_HOSTNAME_OS_SHORT=`ssh $A_USERVal@$A_HOSTVal "hostname -s"`
    export $A_HOSTNAME_OS=`ssh $A_USERVal@$A_HOSTVal "hostname -f"`
    export $A_SERVERNAME=`ssh $A_USERVal@$A_HOSTVal "uname -n"`
    echo "A${appliance}_HOSTNAME_OS is $(eval echo \$${A_HOSTNAME_OS}), while A${appliance}_HOSTNAME_OS_SHORT is $(eval echo \$${A_HOSTNAME_OS_SHORT}) and A${appliance}_SERVERNAME is $(eval echo \$${A_SERVERNAME})"
	 
	((appliance+=1))    
done


