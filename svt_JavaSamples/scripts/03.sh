#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


scenario_set_name="JMQTT Scenarios 03"
source ../scripts/commonFunctions.sh

test_template_set_prefix "jmqtt_03_"

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

source template2.sh


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------
# setup ha
#----------------------------
# test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
  test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|600" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "Test result is Success!"

#----------------------------------------------------
# Test Case 1 -  clean any outstanding subscriptions - when other tests don't clean up after themselves, it can break this test.
# Thus  , we will delete any outstanding durable subcriptions that are still active on the appliance before executing this test.
#----------------------------------------------------

xml[${n}]="jmqtt_03_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for svt_jmqtt tests"
timeouts[${n}]=60

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

test_template_finalize_test

((n+=1))


#----------------------------
# testcase 2 -- Test $GroupID in message policies
#----------------------------

if [ "${A1_TYPE}" == "Bare_Metal" ]; then
  declare MESSAGES=10000
  timeout=1800
elif [ "${A1_TYPE}" == "ESX" ]; then
  declare MESSAGES=10000
  timeout=1800
else
  declare MESSAGES=20000
  timeout=600
fi

printf -v tname "jmqtt_03_%02d" ${n}
test_template_define testcase=${tname} description=mqtt_31_311_HA \
                     timeout=${timeout} qos=2 order=true stats=false listen=false \
                     mqtt_subscribers=1 sub=10 subtopic=/svtGroup0/chat subport=16212 sub_messages=$((${M_COUNT}*${MESSAGES})) \
                     mqtt_publishers=${M_COUNT} pub=15 pubtopic=/svtGroup0/chat pubport=16211 pub_messages=${MESSAGES} 
((++n))

#----------------------------
# disable ha
#----------------------------
# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"



