#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Misc SVT Scenarios 02"
source ../scripts/commonFunctions.sh

test_template_set_prefix "misc_02_"

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

source template1.sh

#----------------------------------------------------
# Test Case 0
#----------------------------------------------------
#
#  This testcase sets up HA
#
#----------------------------------------------------


# test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


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
xml[${n}]="misc_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for svt_misc tests"
timeouts[${n}]=60

#component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
#component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"

component[1]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllSubscriptions.py,"
component[2]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-deleteAllMqttClients.py,"

test_template_finalize_test

((n+=1))


##----------------------------------------------------
#
#  This testcase stores 10,000,000 messages in store, fails over primary, verifies messages
#
#----------------------------------------------------
  printf -v tname "misc_02_%02d" ${n}

if [ "${A1_TYPE}" == "ESX" ]; then
  declare pub=200000
  declare TIMEOUT=600
elif [ "${A1_TYPE}" == "Bare_Metal" ]; then
  declare pub=36000
  declare TIMEOUT=3600
elif [ "${A1_TYPE}" == "RPM" ]; then
  declare pub=2500
  declare TIMEOUT=3000
elif [ "${A1_TYPE}" == "DOCKER" ]; then
  declare pub=36000
  declare TIMEOUT=3600
else
  declare pub=200000
  declare TIMEOUT=600
fi

  declare sub=$((${pub}*4))
  declare x=0

  ((x=0)); client[$x]=jms; count[$x]=2;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=true ;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16212;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
  ((x++)); client[$x]=jms; count[$x]=4;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=true ;topic[$x]=/svtGroup0/chat;port[$x]=16211;messages[$x]=${pub};message[$x]=`eval echo 'sent ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;

  test_template_define "testcase=${tname}" description=`eval echo 'Store_$((${pub}*8))_messages_in_store_verify_messages'` "timeout=${TIMEOUT}" "order=true" "fill=true" "failover=1" "verify=true" "userid=true" "wait=before" "${client[*]}" "${count[*]}" "${action[*]}" "${id[*]}" "${qos[*]}" "${durable[*]}" "${persist[*]}" "${topic[*]}" "${port[*]}" "${messages[*]}" "$( IFS=$'|'; echo "${message[*]}" )" "${rate[*]}" "${ssl[*]}"

  ((++n))

#----------------------------
# Test Case 2
#----------------------------
#
# This testcase disables ha
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
