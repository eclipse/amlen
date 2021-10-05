#!/bin/bash  -x
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

getClientid ${THE_HOSTID} ${AGG_INDEX} ${CLIENT_INDEX};

echo "   ...Cleaning the MQTT Session for PUB client:  ${CLIENT_PUBID}"
set -x
java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102  -a publish  -c true  -t /top${TARGET_IP}  -m "Cleaning Session - `date`"  -i ${CLIENT_PUBID} 
set +x

echo "   ...Cleaning the MQTT Session for SUB client:  ${CLIENT_SUBID}"
set -x 
java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102  -a publish  -c true  -t /top${TARGET_IP}  -m "Cleaning Session - `date`"  -i ${CLIENT_SUBID}  
set +x

echo "   ...Cleaning the MQTT Session for STAT client:  ${CLIENT_STATID}"
set -x
java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102  -a publish  -c true  -t /stat${TARGET_IP} -m "Cleaning Session - `date`"  -i ${CLIENT_STATID} 
set +x

DATE=`date`
echo "   ...Send the Subscriber is READY Message, twice 1-Publisher, 1-StatusMonitor @ ${DATE}"
set -x
SUBPUB_MSG=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a publish -t /stat${TARGET_IP} -n 2 -m "Subscriber is READY! - ${DATE}" -rm -q 2 -i ${CLIENT_SUBID} `
set +x


echo "   ...Subscribe for '${MSG_COUNT}' QOS-${QOS} messages from topic: '/top${TARGET_IP}' from '${TARGET_IP}:16102'."
set -x
SUB_MSG=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a subscribe -e true -c false -t /top${TARGET_IP} -n ${MSG_COUNT} -O -q ${QOS} -receiverTimeout 0:0 -i ${CLIENT_SUBID}`
set +x

DATE=`date`
echo "   ...Send the Subscriber Completed Message @ ${DATE}"
set -x
SUBPUB_MSG=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a publish -t /stat${TARGET_IP} -m "Subscriber Completed! - ${DATE}" -rm -q 2 -i ${CLIENT_SUBID} `
set +x

echo "   ...Checking for Subscriber Success STATUS MSG: 'Message Order Pass'."
printMsg "----------------------------------------------------------------------------------"
printMsg "${SUB_MSG}"
printMsg "----------------------------------------------------------------------------------"
if [[ "${SUB_MSG}" =~ .*\ \ Message\ Order\ Pass.* ]]; then
   RC=0
   echo "PASS:  Successfully completed $0 $@"
else
   RC=99
   echo "FAIL:  Unsuccessfully completed $0 $@"
fi

exit ${RC}
