#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="WS Samples Tests"

typeset -i n=0


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
# Scenario 0 - MqttToJmsListenerTest 0
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="DefaultServerSetup"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Setup Servers for Testing - default values for test"
#component[1]=cAppDriverLog,m1,"-e|../cli_tests/defaultServerSetup.sh",,-s_TraceLevel=9
#components[${n}]="${component[1]}"
#((n+=1))

#----------------------------------------------------
# Scenario 1 - MqttToJmsListenerTest 1
#----------------------------------------------------

xml[${n}]="MqttToJmsListenerTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - MqttToJmsListenerTest 1000"
component[1]=javaAppDriver,m1,-e_MqttToJmsListenerTest,-o_1000,-s-IMAServer=${A1_IPv4_1}-IMAJmsPort=${IMAJmsPort}-IMAMqttPort=${IMAMqttPort}
component[2]=searchLogsEnd,m1,MqttToJmsListenerTest_1.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - JmsToMqttListenerTest 0
#----------------------------------------------------

#xml[${n}]="JmsToMqttListenerTest0"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JmsToMqttListenerTest default"
#component[1]=javaAppDriver,m1,-e_JmsToMqttListenerTest,,-s-IMAServer=${A1_IPv4_1}-IMAJmsPort=${IMAJmsPort}-IMAMqttPort=${IMAMqttPort}
#component[2]=searchLogsEnd,m1,JmsToMqttListenerTest_0.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario 3 - JmsToMqttListenerTest 1
#----------------------------------------------------

xml[${n}]="JmsToMqttListenerTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JmsToMqttListenerTest 10000 using IPv6 Address"
component[1]=javaAppDriver,m1,-e_JmsToMqttListenerTest,-o_10000,-s-IMAServer=${A1_IPv6_1}-IMAJmsPort=${IMAJmsPort}-IMAMqttPort=${IMAMqttPort}
component[2]=searchLogsEnd,m1,JmsToMqttListenerTest_1.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
