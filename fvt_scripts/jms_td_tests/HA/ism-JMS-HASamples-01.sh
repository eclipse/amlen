#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS HA Samples  Failover - 01"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
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
# Scenario 2 - jms_HASamples_001
# The !!COMMA!! will be converted to a comma in the SVT 
# Script.
#----------------------------------------------------
# A1 = Primary
# A2 = Standby


xml[${n}]="jms_HASamples_001"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Kill primary server in middle of sending/receiving messages with HA Customer Samples"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.HATopicPublisher,-o|${A1_IPv4_1}!!COMMA!!${A2_IPv4_1}|16102|durableTopic|800|Message_001|10"
component[2]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.HADurableSubscriber,-o|${A1_IPv4_1}!!COMMA!!${A2_IPv4_1}|16102|durableTopic|600|30"
component[3]=jmsDriver,m2,HA/jms_HASamples_001.xml,killPrimary
component[4]=searchLogsEnd,m1,HA/jms_HASamples_001.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 cleanup - jms_HASamples_001_cleanup
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_jmsSamples_001_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jmsSamples_001"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_HASamples_002
# The !!COMMA!! will be converted to a comma in the SVT 
# Script.
#----------------------------------------------------
# A2 = Primary
# A1 = Standby


xml[${n}]="jms_HASamples_002"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Kill primary server in middle of sending/receiving messages with HA Customer Samples"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.HATopicPublisher,-o|${A1_IPv4_1}!!COMMA!!${A2_IPv4_1}|16102|durableTopic|600|Message_002|10|false|true"
component[2]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.HADurableSubscriber,-o|${A1_IPv4_1}!!COMMA!!${A2_IPv4_1}|16102|durableTopic|800|30"
component[3]=jmsDriver,m2,HA/jms_HASamples_002.xml,killPrimary
component[4]=searchLogsEnd,m1,HA/jms_HASamples_002.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 cleanup - jms_HASamples_002_cleanup
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_jmsSamples_001_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jmsSamples_001"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))
