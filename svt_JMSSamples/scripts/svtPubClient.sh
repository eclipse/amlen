#! /bin/bash -x
#
# pubClient [true|false] [DEBUG]
#
MSG_COUNT=99999999
MSG_RATE=100000
PORT=16102
declare PRIMARY=""
declare SERVERLIST=""

# Get the Server List to test against.
SERVERS=`getserverlist`
## Make the SERVER array into a CSV list and use in java calls
for i in ${SERVERS[@]}
do
    if [[ "${SERVERLIST}" == "" ]]; then
        SERVERLIST=${i}
    else
        SERVERLIST=${SERVERLIST}","${i}
    fi
done

# Manually override 'getserverlist'
#SERVERLIST="10.2.1.140,10.2.1.131"
SERVERLIST="10.10.1.135,10.10.1.138"

echo "The Server List:  " ${SERVERLIST}

#Create Durable Subscription (subscription1) before Publishing for clientid (HASubscriber)
if [[ $# == 0 ]]; then
   DURABLE=false
else
   DURABLE=$1
fi

# Enable Client Trace
#
if [[ $2 == "DEBUG" ]]; then
   java -DIMATraceLevel=9 -DIMATraceFile=/niagara/Pubtrace.log svt.jms.ism.HATopicPublisher ${SERVERLIST} ${PORT} /topic/A ${MSG_COUNT} "${SERVERLIST} JMS ${HOSTNAME}" ${MSG_RATE} ${DURABLE} true  | tee svtPubClient.log &
else
   java svt.jms.ism.HATopicPublisher ${SERVERLIST} ${PORT} /topic/A ${MSG_COUNT} "${SERVERLIST} JMS ${HOSTNAME}" ${MSG_RATE} ${DURABLE} false   | tee svtPubClient.log &
fi
