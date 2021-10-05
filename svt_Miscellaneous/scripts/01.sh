#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Misc SVT Scenarios 01"
source ../scripts/commonFunctions.sh

test_template_set_prefix "misc_01_"

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

#----------------------------------------------------
# Test Case
#----------------------------------------------------
#
#  This testcase creates a durable subscription and then publishes messages into the subscription until the appliance
#  will not accept anymore messages.  It then removes all messages and repeats.
#
#----------------------------------------------------

#  command=$(eval echo \$\{${THIS}_TESTROOT\})/svt_misc/runpubsub.sh
#  a_command="-e|${command},-o|\$\$"
#  a_timeout=1800
#  test_template_add_test_all_M_concurrent "Fill and empty memory test" "${a_command}" "${a_timeout}" "" "SUCCESS"

#----------------------------------------------------
# Test Case
#----------------------------------------------------
#
#  This testcase creates as many durable subscriptions as possible until timeout.
#
#----------------------------------------------------

# command=$(eval echo \$\{${THIS}_TESTROOT\})/svt_misc/nvdimm.sh
# a_command="-e|${command},-o|\$\$"
# a_timeout=$((3*3600))
# test_template_add_test_all_M_concurrent "durable subscriptions test" "${a_command}" "${a_timeout}" "" "SUCCESS"

source template1.sh

#----------------------------------------------------
# Test Case 0
#----------------------------------------------------
#
#  This testcase sets up HA
#
#----------------------------------------------------


#  test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


#if false; then
 xml[${n}]="msgex_HA_setup"
 test_template_initialize_test "${xml[${n}]}"
 scenario[${n}]="${xml[${n}]} - Configure HA"
 timeouts[${n}]=400

 component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|240"
 component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|240"

 test_template_finalize_test
 ((n+=1))

#fi


#----------------------------------------------------
# Test Case 0 -  clean any outstanding subscriptions - when other tests don't clean up after themselves, it can break this test.
# Thus  , we will delete any outstanding durable subcriptions that are still active on the appliance before executing this test.
#----------------------------------------------------
xml[${n}]="misc_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for svt_misc tests"
timeouts[${n}]=60

#component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
#component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"


test_template_finalize_test

((n+=1))

#----------------------------------------------------
# Test Case 1
#----------------------------------------------------
#
#  This testcase stores 2,000,000 messages in store, fails over primary, verifies messages
#
#----------------------------------------------------
if [ "${A1_TYPE}" == "ESX" ]; then
  declare pub=30000
  declare TIMEOUT=3000
elif [ "${A1_TYPE}" == "Bare_Metal" ]; then
  declare pub=30000
  declare TIMEOUT=3000
elif [ "${A1_TYPE}" == "RPM" ]; then
  declare pub=2000
  declare TIMEOUT=1500
elif [ "${A1_TYPE}" == "DOCKER" ]; then
  declare pub=30000
  declare TIMEOUT=3000
else
  declare pub=500000
  declare TIMEOUT=600
fi


  printf -v tname "misc_01_%02d" ${n}
  declare -li sub=$((${pub}*4))
  declare -li x=0

  ((x=0)); client[$x]=jms; count[$x]=1;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=true ;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
  ((x++)); client[$x]=jms; count[$x]=4;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=true ;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'sent ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;

  test_template_define "testcase=${tname}" "description=Store ${pub} jms messages in store, fail over primary, verify messages" "timeout=${TIMEOUT}" "order=true" "fill=true" "failover=1" "verify=true" "userid=true" "wait=before"  "${client[*]}" "${count[*]}" "${action[*]}" "${id[*]}" "${qos[*]}" "${durable[*]}" "${persist[*]}" "${topic[*]}" "${port[*]}" "${messages[*]}" "$( IFS=$'|'; echo "${message[*]}" )" "${rate[*]}" "${ssl[*]}"

((n+=1))




#----------------------------------------------------

xml[${n}]="misc_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete existing subscriptions"
timeouts[${n}]=60

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[3]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

test_template_finalize_test

((n+=1))


#----------------------------
# Test Case 2
#----------------------------


unset component
unset test_template_compare_string

MESSAGES=60000
TOTAL=$((${MESSAGES}*6))
MAXWAIT=4500

echo MESSAGES:  $MESSAGES
echo TOTAL:     $TOTAL
echo MAXWAIT:   $MAXWAIT

printf -v tname "misc_01_%02d" ${n}
xml[$n]=${tname}
test_template_initialize_test ${tname}
scenario[$n]=${tname}
timeouts[$n]=$MAXWAIT

component[1]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000010|-u|u0000010|-p|imasvtest|-c|true|-e|false|-n|0"
component[2]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000011|-u|u0000011|-p|imasvtest|-c|true|-e|false|-n|0"
component[3]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0"
component[4]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000013|-u|u0000013|-p|imasvtest|-c|true|-e|false|-n|0"
component[5]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000014|-u|u0000014|-p|imasvtest|-c|true|-e|false|-n|0"
component[6]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000015|-u|u0000015|-p|imasvtest|-c|true|-e|false|-n|0"

component[7]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000022|-u|u0000022|-p|imasvtest|-n|${MESSAGES}|-O"
component[8]="javaAppDriver,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000023|-u|u0000023|-p|imasvtest|-n|${MESSAGES}|-O"
component[9]="javaAppDriver,m3,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000024|-u|u0000024|-p|imasvtest|-n|${MESSAGES}|-O"
component[10]="javaAppDriver,m4,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000025|-u|u0000025|-p|imasvtest|-n|${MESSAGES}|-O"
component[11]="javaAppDriver,m5,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000026|-u|u0000026|-p|imasvtest|-n|${MESSAGES}|-O"
component[12]="javaAppDriverWait,m6,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:18914+tcp://${A2_HOST}:18914|-i|u0000027|-u|u0000027|-p|imasvtest|-n|${MESSAGES}|-O"

component[13]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000010"
component[14]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000011"
component[15]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000012"
component[16]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000013"
component[17]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000014"
component[18]=cAppDriverWait,m1,"-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|/svtGroup0/chat|-k|BufferedMsgs|-c|lt|-v|${TOTAL}|-w|10|-m|${MAXWAIT}|-r|true|-i|u0000015"

component[19]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|stopBoth|-t|300"
component[20]=sleep,30
component[21]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startBoth|-t|300"

component[22]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000010|-u|u0000010|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"
component[23]="javaAppDriver,m2,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000011|-u|u0000011|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"
component[24]="javaAppDriver,m3,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"
component[25]="javaAppDriver,m4,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000013|-u|u0000013|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"
component[26]="javaAppDriver,m5,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000014|-u|u0000014|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"
component[27]="javaAppDriverWait,m6,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_HOST}:16212+tcp://${A2_HOST}:16212|-i|u0000015|-u|u0000015|-p|imasvtest|-c|false|-e|true|-n|${TOTAL}|-O"

component[28]=searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9
component[29]=searchLogsEnd,m2,${xml[$n]}_m2.comparetest,9
component[30]=searchLogsEnd,m3,${xml[$n]}_m3.comparetest,9
component[31]=searchLogsEnd,m4,${xml[$n]}_m4.comparetest,9
component[32]=searchLogsEnd,m5,${xml[$n]}_m5.comparetest,9
component[33]=searchLogsEnd,m6,${xml[$n]}_m6.comparetest,9

test_template_compare_string[7]="Published ${MESSAGES} messages"
test_template_compare_string[8]="Published ${MESSAGES} messages"
test_template_compare_string[9]="Published ${MESSAGES} messages"
test_template_compare_string[10]="Published ${MESSAGES} messages"
test_template_compare_string[11]="Published ${MESSAGES} messages"
test_template_compare_string[12]="Published ${MESSAGES} messages"

test_template_compare_string[22]="Message Order Pass"
test_template_compare_string[23]="Message Order Pass"
test_template_compare_string[24]="Message Order Pass"
test_template_compare_string[25]="Message Order Pass"
test_template_compare_string[26]="Message Order Pass"
test_template_compare_string[27]="Message Order Pass"

test_template_finalize_test
((n+=1))



#----------------------------
# Test Case 3
#----------------------------
#
# This testcase disables HA
#
#----------------------------

# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


#if false; then


 xml[${n}]="msgex_HA_disable"
 test_template_initialize_test "${xml[${n}]}"
 scenario[${n}]="${xml[${n}]} - Disable HA"
 timeouts[${n}]=400

 component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|240"

 test_template_finalize_test
 ((n+=1))

#fi
