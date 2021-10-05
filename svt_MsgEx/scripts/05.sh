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

scenario_set_name="MsgEx Scenarios 05"
source ../scripts/commonFunctions.sh

declare PREFIX=msgex_05_
test_template_set_prefix "${PREFIX}"

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

declare QOS=2
declare lc=0
declare SUBSERVER=tcp://${A1_IPv4_1}:18911+tcp://${A2_IPv4_1}:18911
declare PUBSERVER=tcp://${A1_IPv4_1}:18914+tcp://${A2_IPv4_1}:18914



#----------------------------
# setup ha
#----------------------------
# test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


 printf -v xml[$n] "${PREFIX}%02d" $n
 test_template_initialize_test "${xml[${n}]}"
 scenario[${n}]="${xml[${n}]} - Configure HA"
 timeouts[${n}]=400
 
 component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|300"
 component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|300"
 
 test_template_finalize_test
 ((++n))


#----------------------------
# delete existing data
#----------------------------

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[${n}]}"
scenario[$n]="${xml[${n}]} - delete subscriptions and mqtt clients"
timeouts[$n]=60

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

test_template_finalize_test
test_template6 ShowSummary

((++n))


#----------------------------
#----------------------------
#----------------------------

printf -v xml[$n] "${PREFIX}%02d" $n
xml[${n}]="${tname}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Test 0 - Policy Setup for message expiry scenario"
timeouts[${n}]=180

test_template6 init
test_template6 m1 cAppDriverLogWait ../scripts/run-cli.sh -s cleanup -c msgex_policy2.cli
test_template6 m1 cAppDriverLogWait ../scripts/run-cli.sh -s setup -c msgex_policy2.cli

test_template_finalize_test
test_template6 ShowSummary
((++n))





printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - create durable QoS ${QOS} subscriptions"
timeouts[$n]=60
lc=0


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"


component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))


printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - publish 10000 QoS ${QOS} messages"
timeouts[$n]=300
lc=0

component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000021|-u|u0000021|-p|imasvtest|-O|-w|2000|-n|2500"
test_template_compare_string[${lc}]="Published 2500 messages"
component[$((++lc))]="javaAppDriver,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|2000|-n|2500"
test_template_compare_string[${lc}]="Published 2500 messages"
component[$((++lc))]="javaAppDriver,m3,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000023|-u|u0000022|-p|imasvtest|-O|-w|2000|-n|2500"
test_template_compare_string[${lc}]="Published 2500 messages"
component[$((++lc))]="javaAppDriver,m4,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000024|-u|u0000024|-p|imasvtest|-O|-w|2000|-n|2500"
test_template_compare_string[${lc}]="Published 2500 messages"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitUntilSubscription.sh,-o|u0000010|BufferedMsgs|-eq|10000|120"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|10000|-w|5|-m|120|-r|true"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
component[$((++lc))]="searchLogsEnd,m2,${xml[$n]}_m2.comparetest,9"
component[$((++lc))]="searchLogsEnd,m3,${xml[$n]}_m3.comparetest,9"
component[$((++lc))]="searchLogsEnd,m4,${xml[$n]}_m4.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))

#test 05

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - attempt to publish more QoS ${QOS} messages"
timeouts[$n]=180
lc=0

component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000021|-u|u0000021|-p|imasvtest|-O|-w|10|-Ns|10"
component[$((++lc))]="javaAppDriver,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|10|-Ns|10"
component[$((++lc))]="javaAppDriver,m3,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000023|-u|u0000023|-p|imasvtest|-O|-w|10|-Ns|10"
component[$((++lc))]="javaAppDriver,m4,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000024|-u|u0000024|-p|imasvtest|-O|-w|10|-Ns|10"

component[$((++lc))]="sleep,30"

#component[$((++lc))]="cAppDriverLogEnd,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000010"
#component[$((++lc))]="cAppDriverLogEnd,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
#component[$((++lc))]="cAppDriverLogEnd,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"

test_template_finalize_test
test_template6 ShowSummary

((++n))


printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - update endpoint messaging policy such that MaxMessages is 11K"
timeouts[$n]=120
lc=0

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub11K{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update1{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub11K\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"




component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[${n}]}"
scenario[$n]="${xml[$n]} - reconnect one of the subscribers"
timeouts[$n]=300
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-c|false|-e|false|-n|10000"
test_template_compare_string[${lc}]="Received 10000 messages."

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

test_template_finalize_test
test_template6 ShowSummary

((++n))



printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[${n}]}"
scenario[$n]="${xml[$n]} - reconnect remaining subscribers"
timeouts[$n]=300
lc=0


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|false|-e|false|-n|10000"
test_template_compare_string[${lc}]="Received 10000 messages."
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-e|false|-n|10000"
test_template_compare_string[${lc}]="Received 10000 messages."

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))
printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - publish 11000 messages"
timeouts[$n]=360
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|2000|-n|11000|-Nm|5"
test_template_compare_string[${lc}]="Published 11000 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))



printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - update MaxMessages to 5K"
timeouts[$n]=120
lc=0

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub11K{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=5000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update2{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub11K\\\":{\\\"MaxMessages\\\":5000}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"



component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))





printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Receive all messages for one subscriber"
timeouts[$n]=360
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-c|false|-e|false|-n|11000"
test_template_compare_string[${lc}]="Received 11000 messages."
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))



printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Receive all messages for remaining subscribers"
timeouts[$n]=360
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|false|-e|false|-n|11000"
test_template_compare_string[${lc}]="Received 11000 messages."
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-e|false|-n|11000"
test_template_compare_string[${lc}]="Received 11000 messages."
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"


component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))



printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Fail over primary"
timeouts[$n]=120

test_template6 init
test_template6 m1 failover
test_template6 FinalizeSearch
test_template_finalize_test
test_template6 ShowSummary

((++n))









printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - verify stats"
timeouts[$n]=60
lc=0

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq0|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"


test_template_finalize_test
test_template6 ShowSummary

((++n))






printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - publish 5000 messages"
timeouts[$n]=360
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|2000|-n|5000|-Nm|5"
test_template_compare_string[${lc}]="Published 5000 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary


((++n))


#16

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - update MaxMessages to 6000"
timeouts[$n]=360
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|2|-i|update16{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub11K\\\":{\\\"MaxMessages\\\":6000}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq5000|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|5000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-w|500|-n|1000"
test_template_compare_string[${lc}]="Published 1000 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|10"
test_template_compare_string[${lc}]="Published 0 messages"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary


((++n))


#17

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - update MaxMessages to 11000"
timeouts[$n]=60
lc=0

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub11K{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=11000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|2|-i|update17{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub11K\\\":{\\\"MaxMessages\\\":11000}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A2_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"


component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary


((++n))

#18

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Fail over primary"
timeouts[$n]=120

test_template6 init
test_template6 m2 failover
test_template6 FinalizeSearch
test_template_finalize_test
test_template6 ShowSummary

((++n))


#19

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - verify stats"
timeouts[$n]=60
lc=0

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-eq6000|-c|imaserver+stat+Subscription+ClientID=u0000012"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq11000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|eq|-v|6000"
test_template_compare_string[${lc}]="Success"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|11000"
test_template_compare_string[${lc}]="Success"

test_template_finalize_test
test_template6 ShowSummary

((++n))

#20

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Receive all messages for subscribers"
timeouts[$n]=360
lc=0

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000011|-p|imasvtest|-c|false|-e|false|-n|6000"
test_template_compare_string[${lc}]="Received 6000 messages."
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|false|-e|false|-n|6000"
test_template_compare_string[${lc}]="Received 6000 messages."
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-e|false|-n|6000"
test_template_compare_string[${lc}]="Received 6000 messages."

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
test_template_finalize_test
test_template6 ShowSummary

((++n))

#21

#----------------------------
# delete existing data
#----------------------------

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[${n}]}"
scenario[$n]="${xml[${n}]} - delete subscriptions and mqtt clients"
timeouts[$n]=60

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

test_template_finalize_test
test_template6 ShowSummary

((++n))


#22


printf -v xml[$n] "${PREFIX}%02d" $n
unset test_template_compare_string
unset component
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Update policy while active pub/sub"
timeouts[$n]=600
lc=0

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update22{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"MaxMessages\\\":25000}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update22{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"

#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq25000|-c|imaserver+stat+Subscription+ClientID=u0000010"
#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq25000|-c|imaserver+stat+Subscription+ClientID=u0000011"
#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq25000|-c|imaserver+stat+Subscription+ClientID=u0000012"


component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|25000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|25000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|25000"
test_template_compare_string[${lc}]="Success"




component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|3|-v"

component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-v|-w|700"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|3|-v|-w|800"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-v|-w|800"
test_template_compare_string[${lc}]="Client u0000012 received"
 
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|24500|120|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|24500|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"



component[$((++lc))]="sleep,5"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-le25000|-c|imaserver+stat+Subscription+ClientID=u0000010"


component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|le|-v|25000"
test_template_compare_string[${lc}]="Success"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=35000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
 
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update22{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"MaxMessages\\\":35000}}},"
test_template_compare_string[${lc}]="RC compare Passed"


#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq35000|-c|imaserver+stat+Subscription+ClientID=u0000010"
#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq35000|-c|imaserver+stat+Subscription+ClientID=u0000011"
#component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq35000|-c|imaserver+stat+Subscription+ClientID=u0000012"


component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|35000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|35000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|35000"
test_template_compare_string[${lc}]="Success"

 
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|34500|120|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|34500|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"


component[$((++lc))]="sleep,5"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-le35000|-c|imaserver+stat+Subscription+ClientID=u0000010"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|le|-v|35000"
test_template_compare_string[${lc}]="Success"



# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=10000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
 
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update22{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"MaxMessages\\\":10000}}},"
test_template_compare_string[${lc}]="RC compare Passed"


# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq10000|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MaxMessages|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MaxMessages|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MaxMessages|-c|eq|-v|10000"
test_template_compare_string[${lc}]="Success"
  

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-gt|10000|120|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|BufferedMsgs|-gt|10000|60|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|BufferedMsgs|-gt|10000|60|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"


component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"


component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))




#23



printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - Increase msg expiry while active pub/sub"
timeouts[$n]=300
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update23{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"MaxMessages\\\":25000}}},"
test_template_compare_string[${lc}]="RC compare Passed"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update23{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"MaxMessageTimeToLive\\\":\\\"2\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"




# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Publish{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessageTimeToLive=2{QUOTE}"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE}"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"

component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-v|-w|400"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|3|-v|-w|450"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-v|-w|450"
test_template_compare_string[${lc}]="Client u0000012 received"

component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|3|-w|2000"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|24500|60|5"
# test_template_compare_string[${lc}]="TIMEOUT FAILURE!!!"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|24500|-w|5|-m|60"
test_template_compare_string[${lc}]="maxwait exceeded"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Publish{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessageTimeToLive=200{QUOTE}"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update23{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"MaxMessageTimeToLive\\\":\\\"200\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|24500|120|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
 
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|24500|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))




#24

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - New policy for temp disconnected client"
timeouts[$n]=300
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSubDsc{QUOTE},"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSubDsc,"

 
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Name\\\":\\\"SVTMsgEx_USERSub\\\"{COMMA}\\\"Destination\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"DestinationType\\\":\\\"Topic\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":\\\"25000\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


  
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

 
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Publish{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Publish\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"}}},"


#{QUOTE}Enabled{QUOTE}:true{COMMA}{QUOTE}Port{QUOTE}:18914{COMMA}{QUOTE}Interface{QUOTE}:{QUOTE}All{QUOTE}{COMMA}{QUOTE}MaxMessageSize{QUOTE}:{QUOTE}256MB{QUOTE}{COMMA}{QUOTE}ConnectionPolicies{QUOTE}:{QUOTE}SVTMsgEx_USER{QUOTE}{COMMA}{QUOTE}TopicPolicies{QUOTE}:{QUOTE}SVTMsgEx_USERPub{QUOTE}{COMMA}{QUOTE}MaxMessageTimeToLive{QUOTE}:{QUOTE}unlimited{QUOTE}}}},"
test_template_compare_string[${lc}]="RC compare Passed"

 
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE}"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERPub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


# component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{{QUOTE}TopicPolicy{QUOTE}:{{QUOTE}SVTMsgEx_USERSub{QUOTE}:{{QUOTE}MaxMessages{QUOTE}:25000}}},"
#test_template_compare_string[${lc}]="RC compare Passed"
#component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update24{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{{QUOTE}TopicPolicy{QUOTE}:{{QUOTE}SVTMsgEx_USERPub{QUOTE}:{{QUOTE}MaxMessageTimeToLive{QUOTE}:{QUOTE}unlimited{QUOTE}}}},"
#test_template_compare_string[${lc}]="RC compare Passed"


test_template_finalize_test
test_template6 ShowSummary

((++n))




#25


printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - temp disconnected client"
timeouts[$n]=300
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}x{SPACE}DELETE{SPACE}configuration/Endpoint/SVTMsgEx_USERSub,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub,"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Name\\\":\\\"SVTMsgEx_USERSub\\\"{COMMA}\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000}}},"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18911{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"

component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|2|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000012 received"

component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|2|-w|2000"

component[$((++lc))]="sleep,10"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000012"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"



# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSubDsc{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000011{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=DiscardOldMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}x{SPACE}DELETE{SPACE}configuration/Endpoint/SVTMsgEx_USERSub,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSubDsc,"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSubDsc\\\":{\\\"Name\\\":\\\"SVTMsgEx_USERSubDsc\\\"{COMMA}\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000011\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"DiscardOldMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSubDsc{COMMA}SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18911{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSubDsc{COMMA}SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"


component[$((++lc))]="sleep,10"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}close{SPACE}Connection{SPACE}{QUOTE}ClientID=u0000011{QUOTE},"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|close{SPACE}0{SPACE}POST{SPACE}service/close/connection{SPACE}{\\\"ClientID\\\":\\\"u0000011\\\"},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|0|30|5"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|0|-w|5|-m|30|-r|true"
test_template_compare_string[${lc}]="SUCCESS"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSubDsc|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000012"


component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSubDsc"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"




# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|1|60|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|Consumers|-eq|1|60|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|Consumers|-eq|1|60|5"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|1|-w|5|-m|180"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|Consumers|-c|eq|-v|1|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|Consumers|-c|eq|-v|1|-w|5|-m|180"
test_template_compare_string[${lc}]="SUCCESS"


# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-lt1000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-gt10000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-lt1000|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|1000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|gt|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|lt|-v|1000"
test_template_compare_string[${lc}]="Success"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|Consumers-eq0|-c|imaserver+stat+Subscription+ClientID=u0000011"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|0"
test_template_compare_string[${lc}]="Success"


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSubDsc{QUOTE},"
# test_template_compare_string[${lc}]="The messaging policy is in pending delete state."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update25{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSubDsc,"


component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|1|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000011 received"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub|-c|imaserver+stat+Subscription+ClientID=u0000012"

component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub"
test_template_compare_string[${lc}]="Success"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|Consumers|-eq|1|120|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|1|120|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|Consumers|-eq|1|120|5"


component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|Consumers|-c|eq|-v|1|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|1|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|Consumers|-c|eq|-v|1|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"


component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5|-e"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}show{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSubDsc{QUOTE},"
# test_template_compare_string[${lc}]="not found."

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))



#26


printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - temp disconnected client"
timeouts[$n]=300
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

####################################################
# create new policies for u10, u11, u12
#
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u10{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000010{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=RejectNewMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub_u10\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000010\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"RejectNewMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u11{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000011{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=RejectNewMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub_u11\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000011\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"RejectNewMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u12{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000012{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=RejectNewMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub_u12\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000012\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"RejectNewMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub_u10{COMMA}SVTMsgEx_USERSub_u11{COMMA}SVTMsgEx_USERSub_u12{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
####################################################

####################################################
# update endpoint with temporary policies
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub_u10{COMMA}SVTMsgEx_USERSub_u11{COMMA}SVTMsgEx_USERSub_u12\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"
####################################################


####################################################
# create subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"
####################################################

####################################################
# connect subscribers
#
component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|1|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000012 received"
####################################################

####################################################
# start publisher
#
component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Ns|100|-w|2000"
####################################################

component[$((++lc))]="sleep,10"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u10|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u11|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u12|-c|imaserver+stat+Subscription+ClientID=u0000012"


####################################################
# verify subscription topic policy
#
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u10"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u11"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u12"
test_template_compare_string[${lc}]="Success"
####################################################


# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|1|120|5"

####################################################
# wait until u11 is finished
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|1|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
####################################################

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u11{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000011{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=DiscardOldMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

####################################################
# update policy for u11 to discard old messages
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub_u11\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000011\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"DiscardOldMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"
####################################################

component[$((++lc))]="sleep,20"

# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-gt|200|120|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|BufferedMsgs|-gt|200|120|5"

####################################################
# wait for u10 and u12 buffers to fall below 200
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|gt|-v|200|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|gt|-v|200|-w|5|-m|120"
test_template_compare_string[${lc}]="SUCCESS"
####################################################


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u11{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=25000{QUOTE}{SPACE}{QUOTE}ClientID=u0000011{QUOTE}{SPACE}{QUOTE}MaxMessagesBehavior=RejectNewMessages{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

####################################################
# update topic policy for u11 to reject new messages
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub_u11\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":25000{COMMA}\\\"ClientID\\\":\\\"u0000011\\\"{COMMA}\\\"MaxMessagesBehavior\\\":\\\"RejectNewMessages\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"
####################################################


####################################################
# start u11 subscriber
#
component[$((++lc))]="javaAppDriverWait,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|1|-v|-w|2000"
test_template_compare_string[${lc}]="Client u0000011 received"
####################################################

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u10|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u11|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|MessagingPolicy==SVTMsgEx_USERSub_u12|-c|imaserver+stat+Subscription+ClientID=u0000012"

####################################################
# verify subscription topic policies
#
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u10"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u11"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|MessagingPolicy|-c|==|-v|SVTMsgEx_USERSub_u12"
test_template_compare_string[${lc}]="Success"
####################################################

####################################################
# delete subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5|-e"
####################################################

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

####################################################
# restore endpoint to original policy
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"
####################################################

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u10{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u11{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub_u12{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


####################################################
# delete temporary policies
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub_u10,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub_u11,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update26{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub_u12,"
####################################################


component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))



#rka03
#27


printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - temp disconnected client"
timeouts[$n]=300
lc=0

########################################
# reset policies etc
#
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update27{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update27{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000}}},"
test_template_compare_string[${lc}]="RC compare Passed"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update27{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"
test_template_compare_string[${lc}]="RC compare Passed"
########################################

########################################
# create durable JMS subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"
########################################

########################################
# begin publishing (non_persistent/DUPS_OK_ACK)
#
component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|3|-w|4000"
########################################

########################################
# reconnect subscribers at slow pace
#
component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-v|-w|20"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|3|-v|-w|20"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-v|-w|20"
test_template_compare_string[${lc}]="Client u0000012 received"
########################################

#   QOS=0
#   component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-c|true|-e|false|-Ns|5"
#   component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|true|-e|false|-Ns|5"
#   component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-Ns|5"
#   
#   component[$((++lc))]="javaAppDriver,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-c|true|-e|false|-Nm|3"
#   component[$((++lc))]="javaAppDriver,m3,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-c|true|-e|false|-Nm|3"
#   component[$((++lc))]="javaAppDriver,m4,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-Nm|3"
#   
#   component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|5|-w|4000"


component[$((++lc))]="sleep,10"



########################################
# wait until buffered msgs reaches 20K
#
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|20000|90|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|BufferedMsgs|-lt|20000|90|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|BufferedMsgs|-lt|20000|90|5"
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|20000|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|lt|-v|20000|-w|5|-m|90|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|lt|-v|20000|-w|5|-m|60|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
########################################




########################################
# set TopicPolicy MaxMessages to 10000
#
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=10000{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update27{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":10000}}},"
########################################

component[$((++lc))]="sleep,10"


########################################
# failover
#
# component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
# component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
########################################


component[$((++lc))]="sleep,10"

########################################
# wait for consumers to reconnect
#
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|Consumers|-eq|0|30|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|0|30|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|Consumers|-eq|0|30|5"
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|Consumers|-c|eq|-v|0|-w|5|-m|120|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|0|-w|5|-m|30|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|Consumers|-c|eq|-v|0|-w|5|-m|30|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
########################################

component[$((++lc))]="sleep,10"


########################################
# wait for buffered msgs to fall to 10K
#
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-gt|10000|90|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|BufferedMsgs|-gt|10000|90|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|BufferedMsgs|-gt|10000|90|5"
# test_template_compare_string[${lc}]="SUCCESS!!!"
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|gt|-v|10000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
########################################


########################################
# wait for consumers to finish
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|Consumers|-c|gt|-v|0|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|Consumers|-c|gt|-v|0|-w|5|-m|60"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|Consumers|-c|gt|-v|0|-w|5|-m|30"
test_template_compare_string[${lc}]="SUCCESS"
########################################

########################################
# delete subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5|-e"
########################################


########################################
# set TopicPolicy MaxMessages back to 20000000
#
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=20000000{QUOTE},"
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|2|-i|update27{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000}}},"
########################################

##############################################
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
##############################################

component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))


#28

printf -v xml[$n] "${PREFIX}%02d" $n
test_template_initialize_test "${xml[$n]}"
scenario[$n]="${xml[$n]} - temp disconnected client"
timeouts[$n]=300
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

#  component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Subscribe{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=20000000{QUOTE},"
#  test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERSub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18911{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERSub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}x{SPACE}DELETE{SPACE}configuration/Endpoint/SVTMsgEx_USERSub,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERSub,"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Name\\\":\\\"SVTMsgEx_USERSub\\\"{COMMA}\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Subscribe\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000}}},"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERSub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18911{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERSub\\\"}}},"



# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Publish{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=20000000{QUOTE}{SPACE}{QUOTE}MaxMessageTimeToLive=1{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."
# 
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}x{SPACE}DELETE{SPACE}configuration/Endpoint/SVTMsgEx_USERPub,"
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_USERPub,"

# setup 0 create TopicPolicy "Name=SVTMsgEx_USERPub" "Topic=/\${GroupID}\/chat" "ActionList=Publish" "Protocol=JMS,MQTT" "MaxMessages=20000000"

##############################################
# policy with max time to live to 1
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Publish\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000{COMMA}\\\"MaxMessageTimeToLive\\\":\\\"1\\\"}}},"
##############################################

# setup 0 create Endpoint "Name=SVTMsgEx_USERPub" "Enabled=True" "Port=18914" "MessageHub=SVTMsgEx_HUB" "Interface=*" "MaxMessageSize=256MB" "ConnectionPolicies=SVTMsgEx_USER" "TopicPolicies=SVTMsgEx_USERPub"

component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18914{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERPub\\\"}}},"

##############################################
# create subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000010 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000011 received 0 messages"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5"
test_template_compare_string[${lc}]="Client u0000012 received 0 messages"
##############################################

##############################################
# connect subscribers
#
component[$((++lc))]="javaAppDriver,m2,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Nm|3|-w|500"
test_template_compare_string[${lc}]="Client u0000010 received"
component[$((++lc))]="javaAppDriver,m3,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Nm|3|-w|500"
test_template_compare_string[${lc}]="Client u0000011 received"
component[$((++lc))]="javaAppDriver,m4,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Nm|3|-w|500"
test_template_compare_string[${lc}]="Client u0000012 received"
##############################################

##############################################
# start publishers
#
component[$((++lc))]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|/svtGroup0/chat|-s|${PUBSERVER}|-i|u0000022|-u|u0000022|-p|imasvtest|-Nm|3|-w|1000"
##############################################

component[$((++lc))]="sleep,30"

# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-lt5000|-c|imaserver+stat+Subscription+ClientID=u0000010"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-lt5000|-c|imaserver+stat+Subscription+ClientID=u0000011"
# component[$((++lc))]="cAppDriverLogWait,m1,-e|../scripts/svt-verifyStat.sh,-o|-s|BufferedMsgs-lt5000|-c|imaserver+stat+Subscription+ClientID=u0000012"



##############################################
# verify buffered msgs are expiring
#
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
##############################################

component[$((++lc))]="sleep,30"

##############################################
# verify buffered msgs are expiring
#
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000011|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000012|-k|BufferedMsgs|-c|lt|-v|10000"
test_template_compare_string[${lc}]="Success"
##############################################



# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}create{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_TempPub{QUOTE}{SPACE}{QUOTE}Destination=/{DOLLAR}{GroupID}/chat{QUOTE}{SPACE}{QUOTE}DestinationType=Topic{QUOTE}{SPACE}{QUOTE}ActionList=Publish{QUOTE}{SPACE}{QUOTE}Protocol=JMS{COMMA}MQTT{QUOTE}{SPACE}{QUOTE}MaxMessages=20000000{QUOTE}{SPACE}{QUOTE}MaxMessageTimeToLive=200{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

##############################################
# temp policy with max time to live to 200
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"TopicPolicy\\\":{\\\"SVTMsgEx_TempPub\\\":{\\\"Topic\\\":\\\"/\\\${GroupID}/chat\\\"{COMMA}\\\"ActionList\\\":\\\"Publish\\\"{COMMA}\\\"Protocol\\\":\\\"JMS{COMMA}MQTT\\\"{COMMA}\\\"MaxMessages\\\":20000000{COMMA}\\\"MaxMessageTimeToLive\\\":\\\"200\\\"}}},"
##############################################


# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_TempPub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


##############################################
# set max time to live to 200
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|1|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18914{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_TempPub\\\"}}},"
##############################################



component[$((++lc))]="sleep,10"

#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"

##############################################
# fail over primary
#
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
##############################################


# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|Consumers|-eq|0|150|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|0|150|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|Consumers|-eq|0|150|5"


##############################################
# wait until subscribers reconnect
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|Consumers|-c|eq|-v|0|-w|5|-m|150|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|0|-w|5|-m|150|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|Consumers|-c|eq|-v|0|-w|5|-m|150|-r|true"
test_template_compare_string[${lc}]="SUCCESS"
##############################################


# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|BufferedMsgs|-lt|25000|90|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|BufferedMsgs|-lt|25000|90|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|BufferedMsgs|-lt|25000|90|5"


##############################################
# wait until subscriptions fill to 25K
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|BufferedMsgs|-c|lt|-v|25000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|BufferedMsgs|-c|lt|-v|25000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|BufferedMsgs|-c|lt|-v|25000|-w|5|-m|90"
test_template_compare_string[${lc}]="SUCCESS"
##############################################



# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000010|Consumers|-eq|1|150|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000011|Consumers|-eq|1|150|5"
# component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.sh,-o|u0000012|Consumers|-eq|1|150|5"


##############################################
# wait until subscribers finish
#
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000010|-k|Consumers|-c|eq|-v|1|-w|5|-m|150"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000011|-k|Consumers|-c|eq|-v|1|-w|5|-m|150"
test_template_compare_string[${lc}]="SUCCESS"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000012|-k|Consumers|-c|eq|-v|1|-w|5|-m|150"
test_template_compare_string[${lc}]="SUCCESS"
##############################################


##############################################
# delete subscriptions
#
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000010|-u|u0000010|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000011|-u|u0000011|-p|imasvtest|-b|-Ns|5|-e"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/svtGroup0/chat|-s|${SUBSERVER}|-i|u0000012|-u|u0000012|-p|imasvtest|-b|-Ns|5|-e"
##############################################

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}update{SPACE}Endpoint{SPACE}{QUOTE}Name=SVTMsgEx_USERPub{QUOTE}{SPACE}{QUOTE}Enabled=True{QUOTE}{SPACE}{QUOTE}Port=18914{QUOTE}{SPACE}{QUOTE}Interface=*{QUOTE}{SPACE}{QUOTE}MaxMessageSize=256MB{QUOTE}{SPACE}{QUOTE}ConnectionPolicies=SVTMsgEx_USER{QUOTE}{SPACE}{QUOTE}MessagingPolicies=SVTMsgEx_USERPub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."


##############################################
# restore topic policy on endpoint
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|2|-i|update28{SPACE}0{SPACE}POST{SPACE}configuration{SPACE}{\\\"Endpoint\\\":{\\\"SVTMsgEx_USERPub\\\":{\\\"Enabled\\\":true{COMMA}\\\"Port\\\":18914{COMMA}\\\"MessageHub\\\":\\\"SVTMsgEx_HUB\\\"{COMMA}\\\"Interface\\\":\\\"*\\\"{COMMA}\\\"MaxMessageSize\\\":\\\"256MB\\\"{COMMA}\\\"ConnectionPolicies\\\":\\\"SVTMsgEx_USER\\\"{COMMA}\\\"TopicPolicies\\\":\\\"SVTMsgEx_USERPub\\\"}}},"
##############################################

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{SPACE}|-q|{QUOTE}|-c|{COMMA}|-d|{DOLLAR}|-e|imaserver{SPACE}delete{SPACE}MessagingPolicy{SPACE}{QUOTE}Name=SVTMsgEx_TempPub{QUOTE},"
# test_template_compare_string[${lc}]="The requested configuration change has completed successfully."

##############################################
# delete temp topic policy
#
component[$((++lc))]="cAppDriverWait,m1,-e|../scripts/run-cli.sh,-o|-a|2|-i|update28{SPACE}x{SPACE}DELETE{SPACE}configuration/TopicPolicy/SVTMsgEx_TempPub,"
##############################################


##############################################
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
##############################################



component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
component[$((++lc))]="searchLogsEnd,m2,${xml[$n]}_m2.comparetest,9"
component[$((++lc))]="searchLogsEnd,m3,${xml[$n]}_m3.comparetest,9"
component[$((++lc))]="searchLogsEnd,m4,${xml[$n]}_m4.comparetest,9"

test_template_finalize_test
test_template6 ShowSummary

((++n))


#----------------------------
#----------------------------

#----------------------------
#----------------------------
# disable ha
#----------------------------

# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"



printf -v xml[$n] "${PREFIX}%02d" $n
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300"
components[${n}]="${component[1]}"
((++n))


unset QOS
unset lc
unset SUBSERVER
unset PUBSERVER
unset PREFIX

