#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 04"

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

#----------------------------------------------------
# Java MQTT Test Cases
#----------------------------------------------------
#
if [[ "${A1_TYPE}" != "ESX" ]] ; then
  let MINUTES=10
  let timeouts[${n}]=720
  let MESSAGES=20000
else
  let minutes=5
  let TIMEOUTS[${n}]=420
  let MESSAGES=60000
fi


    xml[${n}]="atelm_04_0${n}"
    test_template_initialize_test "${xml[${n}]}"
    scenario[${n}]="${xml[${n}]} - Java MQTT Vehicles, 9000 Car Run"

    # Set up the components for the test in the order they should start



    component[1]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-i|a01|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_2}:16102|-n|0|-v"
    component[2]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-i|a01|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_2}:16102|-n|${MESSAGES}|-Nm|$((${MINUTES}+1))|-v"

    component[3]=sleep,10

    component[4]=javaAppDriver,m2,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|100|0|${MINUTES}|1|false|0|false|false|c0000000|imasvtest"
    component[5]=javaAppDriver,m3,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|100|0|${MINUTES}|1|false|0|false|false|c0001000|imasvtest"
    component[6]=javaAppDriver,m4,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|100|0|${MINUTES}|1|false|0|false|false|c0002000|imasvtest"
   
    #----------------------------------------
    # Specify compare test, and metrics for components 3-6
    #----------------------------------------
    test_template_compare_string[2]="Received ${MESSAGES} messages."
    for v in {4..6}; do
        test_template_compare_string[$v]="SVTVehicleScale Success"
    done

    component[7]=searchLogsEnd,m1,atelm_04_0${n}_m1.comparetest,9
    component[8]=searchLogsEnd,m2,atelm_04_0${n}_m2.comparetest,9
    component[9]=searchLogsEnd,m3,atelm_04_0${n}_m3.comparetest,9
    component[10]=searchLogsEnd,m4,atelm_04_0${n}_m4.comparetest,9

    test_template_finalize_test
    ((n+=1))


