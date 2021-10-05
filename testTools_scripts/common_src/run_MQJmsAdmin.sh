#!/bin/bash
# This script is used to generate input for creating JNDI objects needed
# for foreign message tests that use MQ JMS as the foreign provider.
#

host=$1

# MQ Topic Connection Factory
echo "def tcf(FVT_M1_HOST_M2_HOST_jmsTCF) QMGR(QM_MQJMS) TRAN(CLIENT) HOST(MQSERVER1_IP) PORT(1415) CHAN(CHNLJMS)" > mqjndi.in

# MQ Queue Connection Factory
echo "def qcf(FVT_M1_HOST_M2_HOST_jmsQCF) QMGR(QM_MQJMS) TRAN(CLIENT) HOST(MQSERVER1_IP) PORT(1415) CHAN(CHNLJMS)" >> mqjndi.in

# MQ Topic Destinations

# jms foreign message topic
echo "def t(FVT_M1_HOST_M2_HOST_jmsT) topic(jmsT_$1)" >> mqjndi.in

# JCA Topics
echo "def t(FVT_M1_HOST_M2_HOST_sendTopic) topic($host/send/Topic)" >> mqjndi.in
echo "def t(FVT_M1_HOST_M2_HOST_replyTopic) topic($host/reply/Topic)" >> mqjndi.in

# MQ Topic -> ISM Topic
echo "def t(FVT_M1_HOST_M2_HOST_mqTopicToIsmTopic) topic($host/MQTopic/To/ISMTopic)" >> mqjndi.in

# MQ Topic -> ISM Queue
echo "def t(FVT_M1_HOST_M2_HOST_mqTopicToIsmQueue) topic($host/MQTopic/To/ISMQueue)" >> mqjndi.in

# ISM Topic -> MQ Topic
echo "def t(FVT_M1_HOST_M2_HOST_ismTopicToMQTopic) topic($host/ISMTopic/To/MQTopic)" >> mqjndi.in

# ISM Queue -> MQ Topic
echo "def t(FVT_M1_HOST_M2_HOST_ismQueueToMQTopic) topic($host/ISMQueue/To/MQTopic)" >> mqjndi.in

# MQ Queue Destinations

# JCA Queues
echo "def q(FVT_M1_HOST_M2_HOST_sendQueue) QMGR(QM_MQJMS) qu($host.send.Queue)" >> mqjndi.in
echo "def q(FVT_M1_HOST_M2_HOST_replyQueue) QMGR(QM_MQJMS) qu($host.reply.Queue)" >> mqjndi.in

# ISM Topic -> MQ Queue
echo "def q(FVT_M1_HOST_M2_HOST_ismTopicToMQQueue) QMGR(QM_MQJMS) qu($host.ISMTopic.To.MQQueue)" >> mqjndi.in

# MQ Queue -> ISM Topic
echo "def q(FVT_M1_HOST_M2_HOST_mqQueueToIsmTopic) QMGR(QM_MQJMS) qu($host.MQQueue.To.ISMTopic)" >> mqjndi.in

# ISM Queue -> MQ Queue
echo "def q(FVT_M1_HOST_M2_HOST_ismQueueToMQQueue) QMGR(QM_MQJMS) qu($host.ISMQueue.To.MQQueue)" >> mqjndi.in

# MQ Queue -> ISM Queue
echo "def q(FVT_M1_HOST_M2_HOST_mqQueueToIsmQueue) QMGR(QM_MQJMS) qu($host.MQQueue.To.ISMQueue)" >> mqjndi.in

echo "end" >> mqjndi.in

java com.ibm.mq.jms.admin.JMSAdmin -cfg ../common/JMSAdmin.config < mqjndi.in
