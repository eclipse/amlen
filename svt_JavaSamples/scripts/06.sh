#!/bin/bash 

if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


scenario_set_name="JMQTT Scenarios 06"
source ../scripts/commonFunctions.sh

test_template_set_prefix "jmqtt_06_"

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

source template5.sh


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------
# setup ha
#----------------------------
# test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

#----------------------------
# delete existing data
#----------------------------
printf -v tname "jmqtt_06_%02d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

test_template5_init
test_template5_BashScriptWait ../scripts/svt-deleteAllSubscriptions.py
test_template5_BashScriptWait ../scripts/svt-deleteAllMqttClients.py

test_template_finalize_test
test_template5_ShowSummary

((++n))

#----------------------------
#-
#----------------------------

printf -v tname "jmqtt_06_%02d" ${n}

declare -li pub=20000
declare -li sub=$((${pub}*1))
declare -li pubport=16211
declare -li shareddiscardsubport=16215
declare -li gatherpubport=16218
declare -li gathersubport=16217
declare subserver=tcp://${A1_IPv4_1}:${shareddiscardsubport}+tcp://${A2_IPv4_1}:${shareddiscardsubport}
declare pubserver=tcp://${A1_IPv4_1}:${pubport}+tcp://${A2_IPv4_1}:${pubport}
declare gathersubserver=tcp://${A1_IPv4_1}:${gathersubport}+tcp://${A2_IPv4_1}:${gathersubport}
declare gatherpubserver=tcp://${A1_IPv4_1}:${gatherpubport}+tcp://${A2_IPv4_1}:${gatherpubport}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Shared Subscribers"
timeouts[${n}]=300

test_template5_init

test_template5_MQTTClientWait -a subscribe -mqtt 3.1.1 -t /svtGroup0/chat/gather -s ${gathersubserver} -i u0000010 -v -q 2 -c true -u u0000010 -p imasvtest -n 0 -gather -O -sharedsubscribers 2 -debug 
test_template5_MQTTClientWait -vv -debug -a subscribe -mqtt 3.1.1 -q 2 -t '\$SharedSubscription/dog//svtGroup0/chat' -s ${subserver} -i u0000011 -u u0000011 -p imasvtest -v -c true  -n 0 -resendURI ${gatherpubserver} -resendID u0000021 -resendTopic /svtGroup0/chat/gather 
test_template5_MQTTClientWait -vv -debug -a subscribe -mqtt 3.1.1 -q 2 -t '\$SharedSubscription/dog//svtGroup0/chat' -s ${subserver} -i u0000014 -u u0000014 -p imasvtest -v -c true  -n 0 -resendURI ${gatherpubserver} -resendID u0000024 -resendTopic /svtGroup0/chat/gather 

test_template5_MQTTClient -a subscribe -mqtt 3.1.1 -t /svtGroup0/chat/gather -s ${gathersubserver} -i u0000010 -v -q 2 -c true -u u0000010 -p imasvtest -n 4000 -gather -O -sharedsubscribers 2  -debug -discard -buffersize 50
test_template5_SearchString "Order check SUCCESS"

test_template5_MQTTClient -vv -debug -a subscribe -mqtt 3.1.1 -q 2 -t '\$SharedSubscription/dog//svtGroup0/chat' -s ${subserver} -i u0000011 -u u0000011 -p imasvtest -v -c false  -Nm 4 -resendURI ${gatherpubserver} -resendID u0000021 -resendTopic /svtGroup0/chat/gather 
test_template5_MQTTClient -vv -debug -a subscribe -mqtt 3.1.1 -q 2 -t '\$SharedSubscription/dog//svtGroup0/chat' -s ${subserver} -i u0000014 -u u0000014 -p imasvtest -v -c false  -Nm 4 -resendURI ${gatherpubserver} -resendID u0000024 -resendTopic /svtGroup0/chat/gather 
# test_template5_JMSClient -a subscribe -t "/svtGroup0/chat" -s ${subserver} -v -b -u u0000014 -p imasvtest -Nm 4 -resendURI ${gatherpubserver} -resendID u0000024 -debug -resendTopic /svtGroup0/chat/gather -shared dog 

test_template5_MQTTClient -a publish -mqtt 3.1.1 -q 2 -t /svtGroup0/chat -s ${pubserver} -i u0000020 -u u0000020 -p imasvtest -O -w 4000 -n 2000 -c false -v -vv  
test_template5_JMSClient -a publish -t "/svtGroup0/chat" -s ${pubserver} -v -i u0000022 -u u0000022 -p imasvtest -n 2000 -O -w 4000 -debug 

test_template5_FinalizeSearch

test_template_finalize_test

test_template5_ShowSummary

((++n))

#----------------------------
# delete existing data
#----------------------------
printf -v tname "jmqtt_06_%02d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

test_template5_init
test_template5_BashScriptWait ../scripts/svt-deleteAllSubscriptions.py
test_template5_BashScriptWait ../scripts/svt-deleteAllMqttClients.py

test_template_finalize_test
test_template5_ShowSummary

((++n))


#----------------------------
# disable ha
#----------------------------
# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

