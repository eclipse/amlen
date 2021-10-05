#! /bin/bash 
##################################
#  Parameters for this Execution:
RECV_TIMEOUT=30		# Receive Timeout
INSTANCES=10			# Number of Client Instances to run
COUNT=5000			# Number of Messages to Receive
##################################
# DO NOT CHANGE THESE PARAMETERS
PRIMARY=""
SERVERLIST=""
SERVERS=`getserverlist`
# Make the SERVER array into a CSV list and use in java calls
for i in ${SERVERS}
do
    if [[ "${SERVERLIST}" == "" ]]; then
        SERVERLIST=${i}
    else
        SERVERLIST=${SERVERLIST}","${i}
    fi
done
echo "The Server List:  " ${SERVERLIST}

# BUILD CLIENT USERNAME for CLIENT_ID and LDAP if added.
CLIENT=`getlocaladdr`
CLIENT_IP4=` echo ${CLIENT} | cut -d . -f 4`
USERHOST=`printf "%03d" ${CLIENT_IP4}`
#
# Lauch 
for ((x=0; x<=${INSTANCES}; x+=1));
do
   USERCNT=`printf "%04d" ${x}`
   USERNAME=c${USERCNT}${USERHOST}
   # java HADurableSubscriber  serverlist  serverport  topicname  count  [receivetimeout]  [clientID]
   java svt.jms.ism.HADurableSubscriber "${SERVERLIST}" 16102 /topic/A  ${COUNT}  ${RECV_TIMEOUT}  ${USERNAME} &
   sleep 3s
done
echo "===> The Clients are started, waiting for them to complete"
for ((x=0; x<=${INSTANCES}; x+=1));
do
   wait
done
echo "===> The Clients are complete! - round 1"


for i in ${SERVERS}
do
  HAROLE=`ssh -nx admin@${i} "status imaserver" | grep HARole`
#  HAROLE=`ssh -nx admin@${i} "status imaserver" `
  if [[ "${HAROLE}" == *"PRIMARY" ]]; then
    PRIMARY=`echo ${i}`
    break
  fi
done
if [[ "${PRIMARY}" == "" ]]; then
  echo "NO SERVER IN LIST WAS PRIMARY:  "${SERVERS}
  exit 1
fi 


echo ssh -nx admin@${PRIMARY} "advanced-pd-options dumptopictree TopicString=/topic/A"
echo ssh -nx admin@${PRIMARY} "advanced-pd-options dumptopictree TopicString=/topic/A Level=9"

for ((x=0; x<=${INSTANCES}; x+=1));
do
   USERCNT=`printf "%04d" ${x}`
   USERNAME=c${USERCNT}${USERHOST}
   # java HADurableSubscriber  serverlist  serverport  topicname  count  [receivetimeout]  [clientID]
   java svt.jms.ism.HADurableSubscriber "${SERVERLIST}" 16102 /topic/A  ${COUNT}  ${RECV_TIMEOUT}  ${USERNAME} &
   sleep 3s
done
echo "===> The Clients are started, waiting for them to complete..."
for ((x=0; x<=${INSTANCES}; x+=1));
do
   wait
done
echo "===> The Clients are complete!"

exit
