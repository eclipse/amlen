#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#		03/05/2012 - LAW - Initial Creation
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTT BVT via WSTestDriver"

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
# Test Case 1 - testmqtt_connect09
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect09"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage and Qos=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive
component[4]=killComponent,m1,2,1,testmqtt_connect09
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - testmqtt_connect13
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect13"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with cleanSession=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,Receive1,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,Receive2,-o_-l_9
component[4]=wsDriver,m1,connect/${xml[${n}]}.xml,Transmit,-o_-l_9
component[5]=killComponent,m1,2,1,testmqtt_connect13
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - testmqtt_subscribe03
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_subscribe03"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/#'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - testmqtt_subscribe07
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
# Won't work until subscribers requesting QoS 0 get all messages returned QoS 0, or until QoS 1 is supported
xml[${n}]="testmqtt_subscribe07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))
