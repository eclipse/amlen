#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Web Sockets Sample App"

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
# Test Case 0 - 
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="cmqtt_05_03"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - QoS=2 Publishers-Subscribers, 10M msgs,"
#timeouts[${n}]=1000
timeouts[${n}]=630

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m3,"-e|mqttsample,-o|-a|publish|-q|2|-t|/topic/QoS2/C|-s|${A1_IPv4_1}:16102|-n|100"
test_template_compare_string[1]="Published 100 messages to topic: /topic/QoS2/C";
component[2]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|publish|-t|/topic/QoS2/JMS|-s|tcp://${A1_IPv4_1}:16102|-n|100|-w|2000"
test_template_compare_string[2]="sent 100 messages to topic /topic/QoS2/JMS";
component[3]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|publish|-q|2|-t|/topic/QoS2/Java|-s|tcp://${A1_IPv4_1}:16102|-n|100|-w|2000"
test_template_compare_string[3]="Published 100 messages to topic /topic/QoS2/Java";
component[4]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9

# C Sample Subscribers
component[5]=cAppDriver,m3,"-e|mqttsample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/C|-s|${A1_IPv4_1}:16102|-n|100"
test_template_compare_string[5]="Received 100 messages.";
component[6]=cAppDriver,m3,"-e|mqttsample,-o|-a|subscribe|-q|2|-t|/topic/+/C|-s|${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[6]="Received 300 messages.";
component[7]=cAppDriver,m3,"-e|mqttsample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/+|-s|${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[7]="Received 300 messages.";
component[8]=cAppDriver,m3,"-e|mqttsample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/#|-s|${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[8]="Received 300 messages.";
component[9]=cAppDriver,m3,"-e|mqttsample,-o|-a|subscribe|-q|2|-t|/topic/#|-s|${A1_IPv4_1}:16102|-n|900"
test_template_compare_string[9]="Received 900 messages.";

# JMS Sample Subscribers
component[10]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/QoS2/JMS|-s|tcp://${A1_IPv4_1}:16102|-n|100"
test_template_compare_string[10]="received 100 messages.";
component[11]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/+/JMS|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[11]="received 300 messages.";
component[12]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/QoS2/+|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[12]="received 300 messages.";
component[13]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/QoS2/#|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[13]="received 300 messages.";
component[14]=javaAppDriver,m3,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/#|-s|tcp://${A1_IPv4_1}:16102|-n|900"
test_template_compare_string[14]="received 900 messages.";

# Java Sample Subscribers
component[15]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/Java|-s|tcp://${A1_IPv4_1}:16102|-n|100"
test_template_compare_string[15]="Received 100 messages.";
component[16]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|2|-t|/topic/+/Java|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[16]="Received 300 messages.";
component[17]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/+|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[17]="Received 300 messages.";
component[18]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|2|-t|/topic/QoS2/#|-s|tcp://${A1_IPv4_1}:16102|-n|300"
test_template_compare_string[18]="Received 300 messages.";
component[19]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|2|-t|/topic/#|-s|tcp://${A1_IPv4_1}:16102|-n|900"
test_template_compare_string[19]="Received 900 messages.";

#-----------------------------------------------
# RTC task 18464, publishers are publishing approximately 15 seconds
# before subscribers are ready adding a 2 minute sleep buffer
#2012-10-07 19:30:34.843160 Published 100 messages to topic: /topic/QoS0/C
#2012-10-07 19:30:45.199017 Published 100 messages to topic: /topic/QoS1/C
#2012-10-07 19:31:47.081573 Published 100 messages to topic: /topic/QoS2/C
#2012-10-07 19:30:49.630388 Subscribed - Ready
#-----------------------------------------------
component[20]=sleep,120

# Start Order
test_template_runorder="5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 4 20 1 2 3"
test_template_finalize_test



##--------------------------------------------------------------------------------------------##
## This is a subcontroller script there will only ever be one test defined for subcontroller.
##--------------------------------------------------------------------------------------------##
