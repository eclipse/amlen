#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
#  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh
#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 03"

typeset -i n=0
typeset -i svt=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#		m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#		
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController 
#		The configuration file to drive the test case using this controller.
#	<OTHER_OPTS>	is used when configuration file may be over kill,
#			parameters are passed as is and are processed by the subController.
#			However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#	component[x]=sync,<machineNumber_ismSetup>,
#	where:
#		<m1>			is the machine 1
#		<m2>			is the machine 2
#
#   Sleep:
#	component[x]=sleep,<seconds>
#	where:
#		<seconds>	is the number of additional seconds to wait before the next component is started.
#

##----------------------------------------------------
# JMS Test Cases
#----------------------------------------------------

   #-------------------------------------------
   #  Assert that n < 10 or this script is brokent.
   #-------------------------------------------
   if (($n>9)); then
        echo "ERROR: Assertion failed: This script does not support n>9. below atelm_03_0 needs to be fixed."
        exit 1;
   fi

if [ "${A1_TYPE}" == "DOCKER" ]; then
  MESSAGES=40000
  SECONDS=600
  TIMEOUT=1200
elif [ "${A1_TYPE}" == "Bare_Metal" ]; then
  MESSAGES=20000
  SECONDS=1680
  TIMEOUT=1800
elif [ "${A1_TYPE}" == "ESX" ]; then
  MESSAGES=20000
  SECONDS=1680
  TIMEOUT=1800
else
  MESSAGES=100000
  SECONDS=360
  TIMEOUT=450
fi

   xml[${n}]="atelm_03_0${n}"
   test_template_initialize_test "${xml[${n}]}"
   scenario[${n}]="${xml[${n}]} - JMS Vehicles, 6000 Car Run"
   let timeouts[${n}]=${TIMEOUT}

   # Set up the components for the test in the order they should start
   component[1]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-i|a01|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_2}:16102|-n|0|-v"
   component[2]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-i|a01|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_2}:16102|-n|${MESSAGES}|-exact|-Ns|${SECONDS}|-v"

   component[3]=sleep,10

   component[4]=javaAppDriver,m2,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|2000|0|0|$((${SECONDS}/60-1))|1"
   component[5]=javaAppDriver,m3,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|2000|0|0|$((${SECONDS}/60-1))|1"
   component[6]=javaAppDriver,m4,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|2000|0|0|$((${SECONDS}/60-1))|1"

    test_template_compare_string[2]="Received "${MESSAGES}" messages."
    for v in {4..6}; do
        test_template_compare_string[$v]="SVTVehicleScale Success"
    done

    component[7]=searchLogsEnd,m1,atelm_03_0${n}_m1.comparetest,9
    component[8]=searchLogsEnd,m2,atelm_03_0${n}_m2.comparetest,9
    component[9]=searchLogsEnd,m3,atelm_03_0${n}_m3.comparetest,9
    component[10]=searchLogsEnd,m4,atelm_03_0${n}_m4.comparetest,9

    test_template_finalize_test

   ((n+=1))

