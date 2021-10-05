#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT MsgBatching via WSTestDriver"

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
# Test Case 0 - testmqtt_msgbatch_setup
#----------------------------------------------------
xml[${n}]="testmqtt_msgbatch_setup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test Endpoint BatchMessages Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|msgbatching/policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - testmqtt_donotbatch
#----------------------------------------------------
xml[${n}]="testmqtt_donotbatch"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test Endpoint BatchMessages False [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m2,"-e|./msgbatching/busybox_strace.sh","-o|-n|${xml[${n}]}"
#component[2]=searchLogsEnd,m2,msgbatching/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case x - testmqtt_msgbatch_enableBatch
#----------------------------------------------------
xml[${n}]="testmqtt_msgbatch_enableBatch"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test Endpoint BatchMessages Enable Batch"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enableBatch|-c|msgbatching/policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - testmqtt_batch
#----------------------------------------------------
xml[${n}]="testmqtt_batch"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test Endpoint BatchMessages True [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m2,"-e|./msgbatching/busybox_strace.sh","-o|-n|${xml[${n}]}"
#component[2]=searchLogsEnd,m2,msgbatching/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case x - testmqtt_msgbatch_cleanup
#----------------------------------------------------
xml[${n}]="testmqtt_msgbatch_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test Endpoint BatchMessages Cleanup"
# Seem to need to give time for the clients to go away after they were killed....
component[1]=sleep,10
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|msgbatching/policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
