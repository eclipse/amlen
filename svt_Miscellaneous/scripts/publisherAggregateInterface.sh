#!/bin/bash -x 
. ./commonAggregateInterface.sh 
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
do_loops () {
   printMsg  "  ENTER: do_loops()  ........."


}

###  MAIN   #############################################################
DEBUG=1
set +x
if [ -z "${A1_HOST}" ]; then
   printMsg "SOURCE ISMsetup.sh to get A1_HOST's value, this is unexpected!"
   . /niagara/test/scripts/ISMsetup.sh
fi

if [ $# -ne 1 ]; then
   THE_HOST=${A1_HOST}
   THE_HOSTID="A1"
else
   THE_HOST=$(eval echo \$${1}_HOST)
   THE_HOSTID=${1}
fi 

declare -i AGG_INDEX=1
if [ $# -ge 2 ]; then
   AGG_INDEX=${2}
fi

declare -i CLIENT_INDEX=1
if [ $# -ge 3 ]; then
   CLIENT_INDEX=${3}
fi


if [ -z "${THE_HOST}" ];then
   echo "ERROR:  Failed to get the Appliance ID, A1_HOST or A2_HOST. ABORTING! "
   exit 69
else
   AGG_IPV4_NETMASK=$(eval echo \$${THE_HOSTID}_AGG${AGG_INDEX}_IPV4)
   AGG_IPV4=`echo ${AGG_IPV4_NETMASK} | cut -d '/' -f 1`
   echo "Publish to the ${AGG_IPV4} Aggregate Interfaces on ${THE_HOST} Appliance."
fi


getAggregateDefinition ${THE_HOSTID} ${AGG_INDEX}

echo "${THE_HOST}, ${AGG_ETH_LIST[*]} - `date`" 
echo "AGG_ETH_LIST Count= ${#AGG_ETH_LIST[*]} Elements: ${AGG_ETH_LIST[*]}" 

##------------------------------------------------------
# remove old subscriptions and retained msgs

getClientid ${THE_HOSTID} ${AGG_INDEX} ${CLIENT_INDEX};

WAIT_ON_SUB_READY=true

while ( ${WAIT_ON_SUB_READY} )
do
   echo "   ...Checking if Subscriber is ready..."
   PUBSTAT=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a subscribe -e true -t /stat${TARGET_IP} -n 1 -receiverTimeout 1:1 -i ${CLIENT_PUBID} -v`
   printMsg "----------------------------------------------------------------------------------"
   printMsg "${PUBSTAT}"
   printMsg "----------------------------------------------------------------------------------"
   if [[ "${PUBSTAT}" =~ .*\ \ MSG:Subscriber\ is\ READY.* ]]; then
      WAIT_ON_SUB_READY=false
      echo " TIME TO START PUBLISHING!!! YEA!!!"
   else
      echo "DID NOT FIND EXPECTED TEXT: 'Subscriber is READY'.  Try again, turn on DEBUG to dump PUBSTAT"
      DEBUG=1
      sleep 2
   fi
done 


echo "   ...Publish '${MSG_COUNT}' QOS-${QOS} messages to topic: '/top${TARGET_IP}' at '${TARGET_IP}:16102'."
set -x
DATE=`date`
#PUB_MSG=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a publish -c false -t /top${TARGET_IP} -n ${MSG_COUNT} -m " \"d\":{\"date\":\"${DATE}\", \"qos\":${QOS}, \"total_sent\":${MSG_COUNT} } " -OJSON -w 100 -q ${QOS}  -i ${CLIENT_PUBID}`
PUB_MSG=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a publish -c false -t /top${TARGET_IP} -n ${MSG_COUNT} -m " \"d\":{\"date\":\"${DATE}\", \"client\":\"${CLIENT_PUBID}\", \"topic\":\"/top${TARGET_IP}\", \"qos\":${QOS}, \"count\":${MSG_COUNT} } " -OJSON -w 100 -q ${QOS}  -i ${CLIENT_PUBID}`
set +x

echo "   ...Checking For Publisher Success STATUS msg: ' Published ${MSG_COUNT} messages to topic /top${TARGET_IP}'."
printMsg "----------------------------------------------------------------------------------"
printMsg "${PUB_MSG}"
printMsg "----------------------------------------------------------------------------------"

if [[ "${PUB_MSG}" =~  .*\ Published\ ${MSG_COUNT}\ messages\ to\ topic\ /top${TARGET_IP}.* ]]; then
   RC=0
   echo "PASS:  Successfully completed $0 $@"
else
   RC=99
   echo "FAIL:  Unsuccessfully completed $0 $@"
fi

exit ${RC}
