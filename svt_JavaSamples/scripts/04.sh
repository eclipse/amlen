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
for i in {1..20}; do
  c=$(($c % ${M_COUNT} + 1))
  export M$i="m$c"
done
unset c

scenario_set_name="JMQTT Scenarios 04"
source ../scripts/commonFunctions.sh

test_template_set_prefix "jmqtt_04_"

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

source template4.sh
source template5.sh


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------
# setup ha
#----------------------------
#test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

#----------------------------------------------------
# Scenario 0 - HA setup
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jmqtt_HA_setup"
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
printf -v tname "jmqtt_04_%02d" ${n}

xml[${n}]=${tname}
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

test_template5_BashScriptWait ../scripts/svt-deleteAllSubscriptions.py
test_template5_BashScriptWait ../scripts/svt-deleteAllMqttClients.py

test_template_finalize_test
test_template5_ShowSummary

((++n))

#----------------------------
#-
#----------------------------

#printf -v tname "jmqtt_04_%02d" ${n}
#
#declare -li pub=200000
#declare -li sub=$((${pub}*1))
#declare -li x=0
#
#((x=0)); client[$x]=mqtt;count[$x]=0;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
#((x++)); client[$x]=mqtt;count[$x]=0;action[$x]=subscribe;id[$x]=11;qos[$x]=1;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
#((x++)); client[$x]=mqtt;count[$x]=10;action[$x]=subscribe;id[$x]=12;qos[$x]=2;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
#((x++)); client[$x]=mqtt;count[$x]=0;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'Published ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;
#((x++)); client[$x]=mqtt;count[$x]=0;action[$x]=publish  ;id[$x]=21;qos[$x]=1;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'Published ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;
#((x++)); client[$x]=mqtt;count[$x]=1;action[$x]=publish  ;id[$x]=22;qos[$x]=2;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'Published ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;
#((x++)); client[$x]=jms; count[$x]=0;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]=`eval echo 'Received ${sub} messages'`;rate[$x]=0   ;ssl[$x]=false;
#((x++)); client[$x]=jms; count[$x]=0;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'Published ${pub} messages'`;rate[$x]=1000;ssl[$x]=false;
#((x++)); client[$x]=jms; count[$x]=0;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=true ;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]=`eval echo 'Received ${sub} messages'`;rate[$x]=0   ;ssl[$x]=false;
#((x++)); client[$x]=jms; count[$x]=0;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=true ;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'Published ${pub} messages'`;rate[$x]=1000;ssl[$x]=false;
#
#test_template_define "testcase=${tname}" "description=mqtt_41_311_HA" "timeout=900" "order=true" "${client[*]}" "${count[*]}" "${action[*]}" "${id[*]}" "${qos[*]}" "${durable[*]}" "${persist[*]}" "${topic[*]}" "${port[*]}" "${messages[*]}" "$( IFS=$'|'; echo "${message[*]}" )" "${rate[*]}" "${ssl[*]}"

########################3
# n=2
# xml[2]="jmqtt_04_02"
########################3

if [ "${A1_TYPE}" == "RPM" ]; then
  declare MESSAGES=5000
  declare MINUTES=20
  timeouts[$n]=1260
elif [ "${A1_TYPE}" == "DOCKER" ]; then
  declare MESSAGES=5000
  declare MINUTES=20
  timeouts[$n]=1260
elif [ "${A1_TYPE}" == "ESX" ]; then
  declare MESSAGES=5000
  declare MINUTES=20
  timeouts[$n]=1260
elif [ "${A1_TYPE}" == "Bare_Metal" ]; then
  declare MESSAGES=5000
  declare MINUTES=20
  timeouts[$n]=1260
else
  declare MESSAGES=20000
  declare MINUTES=15
  timeouts[$n]=960
fi

declare lc=0

xml[$n]="jmqtt_04_0$n"
test_template_initialize_test "jmqtt_04_0$n"
scenario[$n]="jmqtt_04_0$n - 1:10 Fan Out of ${MESSAGES} messages"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000013|-u|u0000013|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000014|-u|u0000014|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000015|-u|u0000015|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000016|-u|u0000016|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000017|-u|u0000017|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000018|-u|u0000018|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000019|-u|u0000019|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000020|-u|u0000020|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000021|-u|u0000021|-p|imasvtest|-c|true|-e|false|-n|0"
test_template_compare_string[${lc}]="subscribed to topic"

component[$((++lc))]="javaAppDriver,${M2},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M3},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000013|-u|u0000013|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M4},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000014|-u|u0000014|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M5},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000015|-u|u0000015|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M6},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000016|-u|u0000016|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M2},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000017|-u|u0000017|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M3},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000018|-u|u0000018|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M4},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000019|-u|u0000019|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M5},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000020|-u|u0000020|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"
component[$((++lc))]="javaAppDriver,${M6},-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16212+tcp://${A2_IPv4_1}:16212|-i|u0000021|-u|u0000021|-p|imasvtest|-c|false|-O|-e|true|-n|${MESSAGES}|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"

component[$((++lc))]="javaAppDriver,${M1},-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|2|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:16211+tcp://${A2_IPv4_1}:16211|-i|u0000022|-u|u0000022|-p|imasvtest|-O|-n|${MESSAGES}|-w|500"
test_template_compare_string[${lc}]="Published ${MESSAGES} messages"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-i|u0000021|-f|PublishedMsgs|-c|lt|-v|$((${MESSAGES}/10))|-w|5|-m|60"
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_IPv4_1}:${A2_PORT}|-i|u0000021|-f|PublishedMsgs|-c|lt|-v|$((${MESSAGES}/4))|-w|5|-m|120"
component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"

for i in $(eval echo {1..${M_COUNT}}); do
  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
done

test_template_finalize_test
test_template5_ShowSummary

unset lc
unset MESSAGES
unset MINUTES


((++n))
#----------------------------
# disable ha
#----------------------------
# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


#----------------------------------------------------
# Test Case 0 - HA Cleanup
#----------------------------------------------------
xml[${n}]="jmqtt_DisableHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300"
components[${n}]="${component[1]}"
((n+=1))
