#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="mqtt HAScenarios02"

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case runningon the target machine.
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
# Test Case 0 - mqtt_HA02
#----------------------------------------------------
xml[${n}]="testmqtt_HA02"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - Test HA failover with actively transmitting and receiving clients [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,receive,-o_-l_7
component[4]=runCommand,m1,../common/serverKill.sh,1,testmqtt_HA02
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 script to re-ready the killed server - temporary
#----------------------------------------------------
xml[${n}]="testmqtt_HA02Cleanup"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Reset HA to preferred primary as primary [ disableHA ]}.xml ]"
component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-f|testmqtt_HA02disableHA.log"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 script to re-ready the killed server - temporary
#----------------------------------------------------
xml[${n}]="testmqtt_HA02Cleanup1"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Reset HA to preferred primary as primary [ setupHA ]}.xml ]"
component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA|-f|testmqtt_HA02setupHA.log"
components[${n}]="${component[1]}"
((n+=1))


