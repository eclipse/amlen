#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT Java Sample App"

typeset -i n=0

declare -r demo_ep=tcp://${A1_IPv4_1}:16102
declare -r internet_ep=ssl://${A1_IPv4_1}:16111
declare -r intranet_ep=tcp://${A1_IPv4_2}:16999

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
xml[${n}]="jmqtt_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.mqttv3.MqttSample QoS 0 msgs [1 Pub; 4 Sub] with Topic wildcard Subscription"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|publish|-q|0|-t|/topic/A/0|-s|${demo_ep}|-n|1000000|-w|3000"
component[2]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/0|-s|${demo_ep}|-n|1000000"
component[3]=javaAppDriver,m2,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/+|-s|${demo_ep}|-n|1000000"
component[4]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/+/0|-s|${demo_ep}|-n|1000000"
component[5]=javaAppDriver,m2,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/#|-s|${demo_ep}|-n|1000000"
component[6]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[7]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9

test_template_compare_string[1]="Published 1000000 messages to topic /topic/A/0"
for v in {2..5}; do
    test_template_compare_string[$v]="Received 1000000 messages."
done

test_template_runorder="3 2 4 5 1 6 7"
test_template_finalize_test



###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver

#------------------------------------
# RTC Task 18578, measurements taken show that this workload
# takes ~ 2 minutes to send/receieve 10,000 messages, thus it is not
# realistic to expect this workload to finish 1 M messages in under 4 minutes.
# The total number of messages sent/received needs to be lowered to 10,000 
# in order to complete in the time allotted to the test.
#------------------------------------
xml[${n}]="jmqtt_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - svt/mqtt/mq/MqttSample QoS 1 msgs [1 Pub; 4 Sub] with Topic wildcard Subscription"
#
## Set up the components for the test in the order they should start
component[1]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|publish|-q|1|-t|/topic/Java/1|-s|${demo_ep}|-n|10000"
component[2]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|1|-t|/topic/Java/1|-s|${demo_ep}|-n|10000"
component[3]=javaAppDriver,m2,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|1|-t|/topic/Java/+|-s|${demo_ep}|-n|10000"
component[4]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|1|-t|/topic/+/1|-s|${demo_ep}|-n|10000"
component[5]=javaAppDriver,m2,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|1|-t|/topic/#|-s|${demo_ep}|-n|10000"
component[6]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[7]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9

test_template_compare_string[1]="Published 10000 messages to topic /topic/Java/1"
for v in {2..5}; do
    test_template_compare_string[$v]="Received 10000 messages."
done

test_template_runorder="3 2 4 5 1 6 7"
test_template_finalize_test



###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
###----------------------------------------------------
### Test Case 2 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#------------------------------------
# RTC Task 18578, measurements taken show that this workload
# takes ~ 2 minutes to send/receieve 10,000 messages, thus it is not
# realistic to expect this workload to finish 1 M messages in under 4 minutes.
# The total number of messages sent/received needs to be lowered to 1,000 
# in order to complete in the time allotted to the test.
#------------------------------------
xml[${n}]="jmqtt_01_0${n}"
scenario[${n}]="${xml[${n}]} - svt/mqtt/mq/MqttSample QoS 2 msgs [3 Pub; 4 Sub] with Topic wildcard Subscription"
#
## Set up the components for the test in the order they should start
component[1]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|publish|-q|2|-t|/topic/Java/2|-s|${demo_ep}|-n|1000"
component[2]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|2|-t|/topic/Java/2|-s|${demo_ep}|-n|1000"
component[3]=javaAppDriver,m2,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|2|-t|/topic/Java/+|-s|${demo_ep}|-n|1000"
component[4]=javaAppDriver,m1,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|2|-t|/topic/+/2|-s|${demo_ep}|-n|1000"
component[5]=javaAppDriver,m2,"-e|svt/mqtt/mq/MqttSample,-o|-a|subscribe|-q|2|-t|/topic/#|-s|${demo_ep}|-n|1000"
component[6]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[7]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9

test_template_compare_string[1]="Published 1000 messages to topic /topic/Java/2"
for v in {2..5}; do
    test_template_compare_string[$v]="Received 1000 messages."
done

test_template_runorder="3 2 4 5 1 6 7"
test_template_finalize_test


