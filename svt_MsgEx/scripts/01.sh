#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh
source template6.sh

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


scenario_set_name="MsgExpry Scenarios 01"

test_template_set_prefix "msgex_01_"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#               component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or    component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#       where:
#   <SubControllerName>
#               SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#               m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController
#               The configuration file to drive the test case using this controller.
#       <OTHER_OPTS>    is used when configuration file may be over kill,
#                       parameters are passed as is and are processed by the subController.
#                       However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#       component[x]=sync,<machineNumber_ismSetup>,
#       where:
#               <m1>                    is the machine 1
#               <m2>                    is the machine 2
#
#   Sleep:
#       component[x]=sleep,<seconds>
#       where:
#               <seconds>       is the number of additional seconds to wait before the next component is started.
#
#



#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------
# setup ha
#----------------------------
xml[${n}]="msgex_HA_setup"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Configure HA"
timeouts[${n}]=400

component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|300"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|300"

test_template_finalize_test
((n+=1))



#----------------------------
# delete existing data
#----------------------------
printf -v tname "msgex_01_%02d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete all subscriptions and mqtt clients"
timeouts[${n}]=60

test_template6 init
test_template6 m1 cAppDriverWait ../scripts/svt-deleteAllSubscriptions.py
test_template6 m1 cAppDriverWait ../scripts/svt-deleteAllMqttClients.py

test_template_finalize_test
test_template6 ShowSummary

((++n))
#----------------------------
#----------------------------
#----------------------------

printf -v tname "msgex_01_%02d" ${n}
xml[${n}]="${tname}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Test 0 - Policy Setup for message expiry scenario"
timeouts[${n}]=180

test_template6 init
test_template6 m1 cAppDriverLogWait ../scripts/run-cli.sh -s cleanup -c msgex_policy.cli
test_template6 m1 cAppDriverLogWait ../scripts/run-cli.sh -s setup -c msgex_policy.cli

test_template_finalize_test
test_template6 ShowSummary
((n++))

#----------------------------
#-
#----------------------------


declare -li pub=20000
declare -li sub=${pub}
declare -li subport=18918
declare -li pubport=18919
declare subserver=tcp://${A1_IPv4_1}:${subport}+tcp://${A2_IPv4_1}:${subport}
declare pubserver=tcp://${A1_IPv4_1}:${pubport}+tcp://${A2_IPv4_1}:${pubport}

printf -v tname "msgex_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Message Expiry for Queues - 3000 messages"
timeouts[${n}]=180

test_template6 init

test_template6 m1 javaAppDriverWait svt.jms.ism.JMSSample -i u0000012 -a subscribe -q SVTMsgEx_Queue -s ${subserver} -n 0 -u u0000012 -p imasvtest -v
test_template6 m1 javaAppDriver svt.jms.ism.JMSSample -i u0000022 -a publish -q SVTMsgEx_Queue -s ${pubserver} -n 1000 -w 200 -O -u u0000022 -p imasvtest -v -ttl 2
test_template6 m1 SearchString "sent 1000 messages to queue SVTMsgEx_Queue"

test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000023 -a publish -q SVTMsgEx_Queue -s ${pubserver} -n 1000 -w 200 -O -u u0000023 -p imasvtest -v -ttl 3
test_template6 SearchString "sent 1000 messages to queue SVTMsgEx_Queue"

test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000024 -a publish -q SVTMsgEx_Queue -s ${pubserver} -n 1000 -w 200 -O -u u0000024 -p imasvtest -v -ttl 5
test_template6 SearchString "sent 1000 messages to queue SVTMsgEx_Queue"

test_template6 m1 cAppDriverWait ./waitWhileQueue.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -f "Producers" -c ne -v 0 -w 10 -m 120
test_template6 m1 cAppDriverWait ./waitWhileQueue.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -f "BufferedMsgs" -c ne -v 0 -w 10 -m 120

test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ProducedMsgs -c eq -v 3000
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ConsumedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k RejectedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k BufferedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"

test_template6 FinalizeSearch

test_template_finalize_test
test_template6 ShowSummary

((++n))

#----------------------------------------------------
# Test Case 2
#----------------------------------------------------
printf -v tname "msgex_01_%02d" ${n}

declare -li pub=20000
declare -li sub=${pub}
declare -li subport=18918
declare -li pubport=18919
declare subserver=tcp://${A1_IPv4_1}:${subport}+tcp://${A2_IPv4_1}:${subport}
declare pubserver=tcp://${A1_IPv4_1}:${pubport}+tcp://${A2_IPv4_1}:${pubport}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Message Expiry for Queues - 5 minutes"
timeouts[${n}]=360

test_template6 init

test_template6 m1 javaAppDriverWait svt.jms.ism.JMSSample -i u0000012 -a subscribe -q SVTMsgEx_Queue -s ${subserver} -n 0 -u u0000012 -p imasvtest -v

test_template6 m1 javaAppDriver svt.jms.ism.JMSSample -i u0000021 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 2000 -O -u u0000021 -p imasvtest -v -ttl 2
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000022 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 1500 -O -u u0000022 -p imasvtest -v -ttl 3
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000023 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 1000 -O -u u0000023 -p imasvtest -v -ttl 5
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000024 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 500 -O -u u0000024 -p imasvtest -v -ttl 7
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000025 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 300 -O -u u0000025 -p imasvtest -v -ttl 11

#test_template6 m1 cAppDriverWait ./waitq.sh SVTMsgEx_Queue "Producers" -eq 0
#test_template6 m1 cAppDriverWait ./waitq.sh SVTMsgEx_Queue "BufferedMsgs" -eq 0

test_template6 m1 cAppDriverWait ./waitWhileQueue.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -f "Producers" -c ne -v 0 -w 10 -m 240
test_template6 m1 cAppDriverWait ./waitWhileQueue.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -f "BufferedMsgs" -c ne -v 0 -w 10 -m 240

#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ProducedMsgs-gt0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ConsumedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s RejectedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s BufferedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue

test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ProducedMsgs -c gt -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ConsumedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k RejectedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k BufferedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"

test_template6 FinalizeSearch

test_template_finalize_test
test_template6 ShowSummary

((++n))


#----------------------------------------------------
# Test Case 2
#----------------------------------------------------
printf -v tname "msgex_01_%02d" ${n}

declare -li pub=20000
declare -li sub=${pub}
declare -li subport=18918
declare -li pubport=18919
declare subserver=tcp://${A1_IPv4_1}:${subport}+tcp://${A2_IPv4_1}:${subport}
declare pubserver=tcp://${A1_IPv4_1}:${pubport}+tcp://${A2_IPv4_1}:${pubport}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Message Expiry for Queues - 2 failovers"
timeouts[${n}]=360

test_template6 init

test_template6 m1 javaAppDriverWait svt.jms.ism.JMSSample -i u0000012 -a subscribe -q SVTMsgEx_Queue -s ${subserver} -n 0 -u u0000012 -p imasvtest -v

test_template6 m1 javaAppDriver svt.jms.ism.JMSSample -i u0000021 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 2000 -O -u u0000021 -p imasvtest -v -ttl 2
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000022 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 1500 -O -u u0000022 -p imasvtest -v -ttl 3
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000023 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 1000 -O -u u0000023 -p imasvtest -v -ttl 5
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000024 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 500 -O -u u0000024 -p imasvtest -v -ttl 7
test_template6 m? javaAppDriver svt.jms.ism.JMSSample -i u0000025 -a publish -q SVTMsgEx_Queue -s ${pubserver} -Nm 5 -w 300 -O -u u0000025 -p imasvtest -v -ttl 11

test_template6 m1 sleep 60
test_template6 m1 failover
test_template6 m1 sleep 60
test_template6 m1 failover
test_template6 m1 sleep 60
#test_template6 m1 cAppDriverWait ./waitq.sh SVTMsgEx_Queue "BufferedMsgs" -eq 0
test_template6 m1 cAppDriverWait ./waitWhileQueue.py -a ${A1_IPv4_1}:${A1_PORT} -i SVTMsgEx_Queue -f "BufferedMsgs" -c ne -v 0 -w 10 -m 240

#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ProducedMsgs-gt0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ConsumedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s RejectedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s BufferedMsgs-eq0 -c imaserver+stat+Queue+Name=SVTMsgEx_Queue
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ProducedMsgs -c gt -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k ConsumedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k RejectedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"
test_template6 m1 cAppDriverWait ./verifyQueueStat.py -a ${A1_HOST}:${A1_PORT} -i SVTMsgEx_Queue -k BufferedMsgs -c eq -v 0
test_template6 m1 SearchString "Success"

test_template6 FinalizeSearch
test_template_finalize_test
test_template6 ShowSummary

((++n))


#----------------------------------------------------
# Test Case 4
#----------------------------------------------------
#printf -v tname "msgex_01_%02d" ${n}
#
#declare -li pub=20000
#declare -li sub=${pub}
#declare -li subport=18918
#declare -li pubport=18919
#declare subserver=tcp://${A1_IPv4_1}:${subport}+tcp://${A2_IPv4_1}:${subport}
#declare pubserver=tcp://${A1_IPv4_1}:${pubport}+tcp://${A2_IPv4_1}:${pubport}
#
#xml[${n}]=${tname}
#test_template_initialize_test "${xml[${n}]}"
#scenario[${n}]="${xml[${n}]} - Message Expiry for MQTT"
#timeouts[${n}]=360
#
#test_template6 init
#
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 0 -t /MQTT/Expires/2Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 0 -t /MQTT/Expires/3Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 0 -t /MQTT/Expires/5Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 1 -t /MQTT/Expires/2Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 1 -t /MQTT/Expires/3Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 1 -t /MQTT/Expires/5Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 2 -t /MQTT/Expires/2Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 2 -t /MQTT/Expires/3Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#test_template6 m? javaAppDriver svt.mqtt.mq.MqttSample -a publish -q 2 -t /MQTT/Expires/5Sec -s ${pubserver} -n 1000  -w 100 -O -u u0000024 -p imasvtest -v
#
#
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/2Sec "Producers" -eq 0
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/2Sec "BufferedMsgs" -eq 0
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/3Sec "Producers" -eq 0
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/3Sec "BufferedMsgs" -eq 0
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/5Sec "Producers" -eq 0
#test_template6 m1 cAppDriverWait ./waitSubscription.sh /MQTT/Expires/5Sec "BufferedMsgs" -eq 0
#
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ProducedMsgs-eq1000 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/2Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ConsumedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/2Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s RejectedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/2Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s BufferedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/2Sec
#
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ProducedMsgs-eq1000 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/3Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ConsumedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/3Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s RejectedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/3Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s BufferedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/3Sec
#
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ProducedMsgs-eq1000 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/5Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s ConsumedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/5Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s RejectedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/5Sec
#test_template6 m1 cAppDriverLogEnd ../scripts/svt-verifyStat.sh -s BufferedMsgs-eq0 -c imaserver+stat+Subscription+TopicString=/MQTT/Expires/5Sec
#
#test_template6 FinalizeSearch
#test_template_finalize_test
#test_template6 ShowSummary
#
#((++n))


#----------------------------
# disable ha
#----------------------------

# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

xml[${n}]="jmqtt_DisableHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300"
components[${n}]="${component[1]}"
((n+=1))
