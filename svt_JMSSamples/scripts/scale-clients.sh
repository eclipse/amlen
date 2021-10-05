#! /bin/bash  
##################################
#  Parameters for this Execution:
RECV_TIMEOUT=60	# Receive Timeout for FIRST msg
MSGWAIT_TIMEOUT=120	# Receive Timeout for Subsequent msgs
FORCE_TIMEOUT=180	# Receive Timeout force exit, giving up on receiveing
CONNECT_TIMEOUT=240	# How long to keep retrying for connection to MessageSight
INSTANCES=10		# Number of Client Instances to run
COUNT=99999999	# Number of Messages to Receive
TOPIC_NAME="/topic/#"	# Topic Name
TRACELEVEL=7
##################################
# DO NOT CHANGE THESE PARAMETERS
PRIMARY=""
SERVERLIST="10.6.1.212,10.6.1.87"
SERVERS=`getserverlist`
## Make the SERVER array into a CSV list and use in java calls
#for i in ${SERVERS}
#do
#    if [[ "${SERVERLIST}" == "" ]]; then
#        SERVERLIST=${i}
#    else
#        SERVERLIST=${SERVERLIST}","${i}
#    fi
#done
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
   # java HADurableSubscriber  serverlist  serverport  topicname  count  [receivetimeout]  [msgwaittimeout]  [forcetimeout]  [connecttimeout]  [clientID]
   java -DIMATraceLevel=${TRACELEVEL}  -DIMATraceFile=stdout  svt.jms.ism.HADurableSubscriber "${SERVERLIST}" 16102  ${TOPIC_NAME}  ${COUNT}  ${RECV_TIMEOUT} ${MSGWAIT_TIMEOUT}  ${FORCE_TIMEOUT} ${CONNECT_TIMEOUT}  ${USERNAME}  > ${USERNAME}.log 2>&1  &
#   java   svt.jms.ism.HADurableSubscriber "${SERVERLIST}" 16102  ${TOPIC_NAME}  ${COUNT}  ${RECV_TIMEOUT} ${MSGWAIT_TIMEOUT}  ${FORCE_TIMEOUT} ${CONNECT_TIMEOUT} ${USERNAME}  &
   sleep 3s
done

echo "===> The Clients are started, waiting for them to complete"
for ((x=0; x<=${INSTANCES}; x+=1));
do
   wait
done
echo "===> The Clients are complete!"

# Determine the ${PRIMARY} server
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
ssh -nx admin@${PRIMARY} "imaserver stat Subscription"
## Delete the Subscription?
#for ((x=0; x<=${INSTANCES}; x+=1));
#do
#   USERCNT=`printf "%04d" ${x}`
#   USERNAME=c${USERCNT}${USERHOST}
#   ssh -nx admin@${PRIMARY} "imaserver delete Subscription SubscriptionName=subscription1 ClientID=${USERNAME}"
#done


exit 0
