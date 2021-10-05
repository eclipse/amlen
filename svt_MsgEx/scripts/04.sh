#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

c=0
#for i in $(eval echo {1..${M_COUNT}}); do
for i in {1..100}; do
  c=$(($c % ${M_COUNT} + 1))
  echo export M$i="m$c"
  export M$i="m$c"
done
unset c

scenario_set_name="MsgEx Scenarios 04"
source ../scripts/commonFunctions.sh

test_template_set_prefix "msgex_04_"

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

source template6.sh


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

declare -a DELETELIST1=( \
'imaserver delete Endpoint "Name=SVTMsgEx_USERPub"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_USERSub"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_QueueSub"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_QueuePub"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_USERPub_2sec"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_USERPub_3sec"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_USERPub_5sec"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_Sub"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_Pub2sec" ' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_Pub3sec"' '' \
'imaserver delete Endpoint "Name=SVTMsgEx_Pub5sec"' '' \
)

declare -a DELETELIST2=( \
'imaserver delete MessagingPolicy "Name=SVTMsgEx_USERPub"' '' \
'imaserver delete MessagingPolicy "Name=SVTMsgEx_USERSub"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_QueueSub"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_QueuePub"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_Pub2sec"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_Pub3sec"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_Pub5sec"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_Sub"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_USERPub_2sec"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_USERPub_3sec"' '' \
'imaserver delete MessagingPolicy  "Name=SVTMsgEx_USERPub_5sec"' '' \
'imaserver delete ConnectionPolicy  "Name=SVTMsgEx_USER"' '' \
)

declare -a POLICYLIST=( \
'imaserver create MessageHub "Name=SVTMsgEx_HUB" "Description=SVT MsgExpiry Hub"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy "Name=SVTMsgEx_USERPub" "Destination=/{DOLLAR}{GroupID}/chat" "DestinationType=Topic" "ActionList=Publish" "Protocol=JMS,MQTT" "MaxMessages=20000000"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy "Name=SVTMsgEx_USERSub" "Destination=/{DOLLAR}{GroupID}/chat" "DestinationType=Topic" "ActionList=Subscribe" "Protocol=JMS,MQTT" "MaxMessages=20000000"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_QueueSub" "Destination=SVTMsgEx_Queue" "DestinationType=Queue" "ActionList=Browse,Receive"  "Protocol=JMS"  "Description=SVT messaging policy for svt_msgex Queue testing"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_QueuePub" "Destination=SVTMsgEx_Queue" "DestinationType=Queue" "ActionList=Send,Browse"  "Protocol=JMS"  "Description=SVT messaging policy for svt_msgex Queue testing"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_Pub2sec" "Destination=/MQTT/Expires/2Sec" "DestinationType=Topic" "ActionList=Publish" "Protocol=MQTT" "MaxMessageTimeToLive=2"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_Pub3sec" "Destination=/MQTT/Expires/3Sec" "DestinationType=Topic" "ActionList=Publish" "Protocol=MQTT" "MaxMessageTimeToLive=3"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_Pub5sec" "Destination=/MQTT/Expires/5Sec" "DestinationType=Topic" "ActionList=Publish" "Protocol=MQTT" "MaxMessageTimeToLive=5"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_Sub" "Destination=/MQTT/Expires/*" "DestinationType=Topic" "ActionList=Subscribe" "Protocol=MQTT" "MaxMessageTimeToLive=5"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_USERPub_2sec" "Destination=/{DOLLAR}{GroupID}/chat" "DestinationType=Topic" "ActionList=Publish" "Protocol=JMS,MQTT" "MaxMessageTimeToLive=2"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_USERPub_3sec" "Destination=/{DOLLAR}{GroupID}/chat" "DestinationType=Topic" "ActionList=Publish" "Protocol=JMS,MQTT" "MaxMessageTimeToLive=3"' \
'The requested configuration change has completed successfully.' \
'imaserver create MessagingPolicy  "Name=SVTMsgEx_USERPub_5sec" "Destination=/{DOLLAR}{GroupID}/chat" "DestinationType=Topic" "ActionList=Publish" "Protocol=JMS,MQTT" "MaxMessageTimeToLive=5"' \
'The requested configuration change has completed successfully.' \
'imaserver create ConnectionPolicy "Name=SVTMsgEx_USER" "Protocol=JMS,MQTT" "Description=SVT MsgExpr USER ConnectPolicy" "ExpectedMessageRate=Max"' \
'The requested configuration change has completed successfully.' \
)

declare -a ENDPOINTLIST=( \
'imaserver create Endpoint "Name=SVTMsgEx_USERPub" "Enabled=True" "Port=18914" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_USERPub"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_USERSub" "Enabled=True" "Port=18911" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_USERSub"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint   "Name=SVTMsgEx_QueueSub" "Enabled=True" "Port=18918" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_QueueSub" "Description=SVT unsecured endpoint for testing Msg Expiry with Queues"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint   "Name=SVTMsgEx_QueuePub" "Enabled=True" "Port=18919" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_QueuePub" "Description=SVT unsecured endpoint for testing Msg Expiry with Queues"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_USERPub_2sec" "Enabled=True" "Port=18922" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_USERPub_2sec"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_USERPub_3sec" "Enabled=True" "Port=18923" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_USERPub_3sec"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_USERPub_5sec" "Enabled=True" "Port=18925" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_USERPub_5sec"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_Sub" "Enabled=True" "Port=18930" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_Sub"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_Pub2sec" "Enabled=True" "Port=18932" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_Pub2sec"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_Pub3sec" "Enabled=True" "Port=18933" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_Pub3sec"' \
'The requested configuration change has completed successfully.' \
'imaserver create Endpoint "Name=SVTMsgEx_Pub5sec" "Enabled=True" "Port=18935" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "MessagingPolicies=SVTMsgEx_Pub5sec"' \
'The requested configuration change has completed successfully.' \
)

declare -a COMMANDS=
declare -a RESULTS=
# for i in "${!COMMANDLIST[@]}"; do 

# for COMMAND in "${COMMANDS[@]}"
# do
#   echo ${COMMAND}
# done


#----------------------------
# setup ha
#----------------------------
test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

#----------------------------
# delete existing data
#----------------------------

xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.sh,"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.sh,"
component[3]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteHUB.sh,-o|-h|SVTMsgEx_HUB,"

test_template_finalize_test
test_template6 ShowSummary

((++n))
xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - create policies"
timeouts[${n}]=60

declare -i lc=1
declare -i c=1
declare -i i=0
for (( i=0 ; i<${#DELETELIST1[@]} ; i++ )); do
  component[${lc}]="cAppDriver,m$c,-e|$(eval echo \$\{M${c}_TESTROOT\})/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|$(sed -e's/ /{SPACE}/g; s/,/{COMMA}/g; s/\$/{DOLLAR}/g'; s/\"/{QUOTE}/g' <<< ${DELETELIST1[$i]}),"
  if [ "${DELETELIST1[$((++i))]}" != "" ]; then
    test_template_compare_string[${lc}]="${DELETELIST1[$i]}"
  fi
  c=$((${lc} % ${M_COUNT} + 1))
  ((lc++))
done

# component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
#for i in $(eval echo {1..${M_COUNT}}); do
#  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
#done

test_template_finalize_test
test_template6 ShowSummary
((++n))


xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - create policies"
timeouts[${n}]=60

declare -i lc=1
declare -i c=1
declare -i i=0
for (( i=0 ; i<${#DELETELIST2[@]} ; i++ )); do
  component[${lc}]="cAppDriver,m$c,-e|$(eval echo \$\{M${c}_TESTROOT\})/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|$(sed -e's/ /{SPACE}/g; s/,/{COMMA}/g; s/\$/{DOLLAR}/g'; s/\"/{QUOTE}/g' <<< ${DELETELIST2[$i]}),"
  if [ "${DELETELIST2[$((++i))]}" != "" ]; then
    test_template_compare_string[${lc}]="${DELETELIST2[$i]}"
  fi
  c=$((${lc} % ${M_COUNT} + 1))
  ((lc++))
done

# component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
#for i in $(eval echo {1..${M_COUNT}}); do
#  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
#done

test_template_finalize_test
test_template6 ShowSummary
((++n))




xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - create policies"
timeouts[${n}]=60

lc=1
c=1
for (( i=0 ; i<${#POLICYLIST[@]} ; i++ )); do
  component[${lc}]="cAppDriver,m$c,-e|$(eval echo \$\{M${c}_TESTROOT\})/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|$(sed -e's/ /{SPACE}/g; s/,/{COMMA}/g; s/\$/{DOLLAR}/g'; s/\"/{QUOTE}/g' <<< ${POLICYLIST[$i]}),"
  if [ "${POLICYLIST[$((++i))]}" != "" ]; then
    test_template_compare_string[${lc}]="${POLICYLIST[$i]}"
  fi
  c=$((${lc} % ${M_COUNT} + 1))
  ((lc++))
done

# component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
for i in $(eval echo {1..${M_COUNT}}); do
  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
done

test_template_finalize_test
test_template6 ShowSummary
((++n))

xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

lc=1
c=1
for (( i=0 ; i<${#ENDPOINTLIST[@]} ; i++ )); do
  component[${lc}]="cAppDriver,m$c,-e|$(eval echo \$\{M${c}_TESTROOT\})/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|$(sed -e's/ /{SPACE}/g; s/,/{COMMA}/g; s/\$/{DOLLAR}/g'; s/\"/{QUOTE}/g' <<< ${ENDPOINTLIST[$i]}),"
  if [ "${POLICYLIST[$((++i))]}" != "" ]; then
    test_template_compare_string[${lc}]="${ENDPOINTLIST[$i]}"
  fi
  c=$((${lc} % ${M_COUNT} + 1))
  ((lc++))
done

# component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
for i in $(eval echo {1..${M_COUNT}}); do
  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
done


test_template_finalize_test
test_template6 ShowSummary

((++n))


if [ 1 -eq 0 ]; then
########################
# n=2
# xml[2]="jmqtt_04_02"
########################

declare MESSAGES=5000
declare MINUTES=2
declare lc=0
declare SUBSERVER=tcp://${A1_IPv4_1}:18911+tcp://${A2_IPv4_1}:18911
declare PUBSERVER=tcp://${A1_IPv4_1}:18914+tcp://${A2_IPv4_1}:18914

xml[$n]="msgex_04_0$n"
test_template_initialize_test "${xml[${n}]}"
scenario[$n]="msgex_04_0$n - 1:10 Fan Out of ${MESSAGES} messages"
timeouts[$n]=90

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE},"
test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}MessageHub=SVTMsgEx_HUB{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE},"
test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

# if [ 0 -eq 1 ]; then 
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000013|-u|u0000013|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000014|-u|u0000014|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000015|-u|u0000015|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000016|-u|u0000016|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000017|-u|u0000017|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000018|-u|u0000018|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000019|-u|u0000019|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000020|-u|u0000020|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000021|-u|u0000021|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"

component[$((++lc))]="javaAppDriver,${M1},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M2},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000013|-u|u0000013|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M3},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000014|-u|u0000014|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M4},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000015|-u|u0000015|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M5},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000016|-u|u0000016|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M6},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000017|-u|u0000017|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M7},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000018|-u|u0000018|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M8},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000019|-u|u0000019|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M9},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000020|-u|u0000020|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M10},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000021|-u|u0000021|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}"
test_template_compare_string[${lc}]="Message Order Pass"

component[$((++lc))]="javaAppDriver,${M11},-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|4000|-n|${MESSAGES}"
test_template_compare_string[${lc}]="Published ${MESSAGES} messages"

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE},"
test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

# component[$((++lc))]="sleep,30"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-d|+|-c|status+imaserver,"
# test_template_compare_string[${lc}]="status = running"
# component[$((++lc))]="sleep,30"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-d|+|-c|status+imaserver,"
# test_template_compare_string[${lc}]="status = running"
# component[$((++lc))]="sleep,30"
#  component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-q|:|-s|+|-e|imaserver+delete+Endpoint+:Name=SVTMsgEx_USERPub:,"
#  test_template_compare_string[${lc}]="the requested configuration change has completed successfully."

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="sleep,60"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"

for i in $(eval echo {1..${M_COUNT}}); do
  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
done
#fi

test_template_finalize_test
test_template6 ShowSummary

unset lc
unset MESSAGES
unset MINUTES

fi
((++n))
#----------------------------
# disable ha
#----------------------------
test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"




