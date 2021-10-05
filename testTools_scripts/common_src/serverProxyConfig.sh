#! /bin/bash  +x
# Set the Protocol.AllowMqttProxyProtocol in MessageSight server.cfg.  
#  This script now supports setting the property in ALL A# servers of ISMsetup.sh/testEnv.sh ($A_COUNT)
# INPUTS:  [on|off]
#  

AX=1;
AX_LAST=A_COUNT   ## TEMP FIX until this script is moved from common_src to ../scripts  ${A_COUNT} gets PrepareTestEnv'ed to ${2} and there is only $1
                  ## WHEN MOVED change AX_LAST back to A1_COUNT and remove AX_LAST
while [ "${AX}" -le ${AX_LAST} ]; do
#set -x
	AX_TEMP=A${AX}_USER
	AX_USER=$(eval echo \$${AX_TEMP})
	AX_TEMP=A${AX}_HOST
	AX_HOST=$(eval echo \$${AX_TEMP})

	offcmd="ssh ${AX_USER}@${AX_HOST} sed -i \"/Protocol.AllowMqttProxyProtocol/d\" /mnt/A${AX}/messagesight/data/config/server.cfg "
	oncmd="ssh ${AX_USER}@${AX_HOST} echo  'Protocol.AllowMqttProxyProtocol = 2'  >>/mnt/A${AX}/messagesight/data/config/server.cfg "
#set +x
	if [ "$1" = "on" ]; then
		$oncmd
		../scripts/serverControl.sh -a restartServer -i ${AX} -s production
	else 
		if [ "$1" = "off" ]; then
		$offcmd
			../scripts/serverControl.sh -a restartServer -i ${AX} -s production
		else
			echo "Usage is: $0 [on|off]"
			AX=${AX_LAST}    # FORCE EXIT!
		fi
	fi 
	let AX+=1
#	echo "AX is on server#: ${AX} of ${AX_LAST}"

done
