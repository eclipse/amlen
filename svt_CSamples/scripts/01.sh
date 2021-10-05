#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#   11/26/12 - Refactored ism-MQTT_ALL_Sample-runScenario01.sh into 01.sh
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


# Testing the JMSSampleAdmin. It is used to create JNDI that is used by the JMSSample program below.
xml[${n}]="jms_JMSSampleAdmin"
test_template_initialize_test "${xml[${n}]}"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - Use JMSSampleAdmin to create JNDI"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSampleAdmin,-o|-c|file:.|../common/JMSAdminSample.config"
component[2]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSampleAdmin,-o|-c|file:.|../common/JMSAdminSample.config"
test_template_compare_string[1]="The bind of the config file was successful";
component[3]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest
test_template_runorder="1 2 3"
test_template_finalize_test
((n+=1))

# Testing the JMSSampleAdmin. It is used to create JNDI that is used by the JMSSample program below.
xml[${n}]="jms_JMSSampleAdmin_list"
test_template_initialize_test "${xml[${n}]}"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - Use JMSSampleAdmin to list JNDI"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSampleAdmin,-o|-c|file:.|-l"
test_template_compare_string[1]="connection SampleconnFactory1";
test_template_compare_string[1]+=$'\ndestination topicA2'
test_template_compare_string[1]+=$'\ndestination topicB1'
test_template_compare_string[1]+=$'\ndestination topicPlus'
test_template_compare_string[1]+=$'\ndestination topicPound'
component[2]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest
test_template_runorder="1 2"
test_template_finalize_test
((n+=1))


for qos in 0 1 2 ; do

# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="cmqtt_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - JMS, MQTTv3 C and Java Samples 29 million QoS=0 msgs with Wildcard Subscriptions"

if [ "$qos" == "0" ] ; then
    cmsgs=1000000
    jmsmsgs=1000000
    jmsgs=1000000
    timeouts[${n}]=800
else
    #----------------------------------------------------------
    # RTC Task 18576 - C client cannot send more than 100 messages per second  
    # for this test, the number of messages must be lowered to 50000 for the c client
    #----------------------------------------------------------
    cmsgs=50000
    jmsmsgs=1000000
    jmsgs=1000000
    timeouts[${n}]=1600
fi

# MQTTv3 C Client
component[1]=cAppDriver,m1,"-e|mqttsample_array,-o|-a|publish|-q|$qos|-t|/topic/A/1|-s|${A1_IPv4_1}:16102|-n|${cmsgs}|-w|2000|-x|86WaitForCompletion=1"
test_template_compare_string[1]="Published ${cmsgs} messages";
component[2]=cAppDriver,m1,"-e|mqttsample,-o|-a|subscribe|-q|$qos|-t|/topic/A/2|-s|${A1_IPv4_1}:16102|-n|${jmsmsgs}"
test_template_compare_string[2]="Received ${jmsmsgs} messages.";
component[3]=cAppDriver,m2,"-e|mqttsample,-o|-a|subscribe|-q|$qos|-t|/topic/A/+|-s|${A1_IPv4_1}:16102|-n|$((${jmsmsgs}+${cmsgs}))"
test_template_compare_string[3]="Received $((${jmsmsgs}+${cmsgs})) messages.";
component[4]=cAppDriver,m1,"-e|mqttsample,-o|-a|subscribe|-q|$qos|-t|/topic/+/1|-s|${A1_IPv4_1}:16102|-n|$((${cmsgs}+${jmsgs}))"
test_template_compare_string[4]="Received $((${cmsgs}+${jmsgs})) messages.";
component[5]=cAppDriver,m2,"-e|mqttsample,-o|-a|subscribe|-q|$qos|-t|/topic/#|-s|${A1_IPv4_1}:16102|-n|$((${cmsgs}+${jmsmsgs}+${jmsgs}))"
test_template_compare_string[5]="Received $((${cmsgs}+${jmsmsgs}+${jmsgs})) messages.";

# JMS Client
component[6]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|publish|-t|topicA2|-jc|file:.|-jx|com.sun.jndi.fscontext.RefFSContextFactory|-jcf|SampleconnFactory1|-n|${jmsmsgs}|-w|2000"
test_template_compare_string[6]="sent ${jmsmsgs} messages to topic topicA2(/topic/A/2)";
component[7]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|topicB1|-jc|file:.|-jx|com.sun.jndi.fscontext.RefFSContextFactory|-jcf|SampleconnFactory1|-n|${jmsgs}"
test_template_compare_string[7]="received ${jmsgs} messages.";
component[8]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|topicBPlus|-jc|file:.|-jx|com.sun.jndi.fscontext.RefFSContextFactory|-jcf|SampleconnFactory1|-n|${jmsgs}"
test_template_compare_string[8]="received ${jmsgs} messages.";
component[9]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|topicPlus|-jc|file:.|-jx|com.sun.jndi.fscontext.RefFSContextFactory|-jcf|SampleconnFactory1|-n|$((${cmsgs}+${jmsgs}))"
test_template_compare_string[9]="received $((${cmsgs}+${jmsgs})) messages.";
component[10]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|topicPound|-jc|file:.|-jx|com.sun.jndi.fscontext.RefFSContextFactory|-jcf|SampleconnFactory1|-n|$((${cmsgs}+${jmsmsgs}+${jmsgs}))"
test_template_compare_string[10]="received $((${cmsgs}+${jmsmsgs}+${jmsgs})) messages.";

# MQTTv3 Java Client
component[11]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|publish|-q|$qos|-t|/topic/B/1|-s|tcp://${A1_IPv4_1}:16102|-n|${jmsgs}|-w|2000"
test_template_compare_string[11]="Published ${jmsgs} messages to topic /topic/B/1";
component[12]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|$qos|-t|/topic/A/2|-s|tcp://${A1_IPv4_1}:16102|-n|${jmsmsgs}"
test_template_compare_string[12]="Received ${jmsmsgs} messages.";
component[13]=javaAppDriver,m2,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|$qos|-t|/topic/A/+|-s|tcp://${A1_IPv4_1}:16102|-n|$((${jmsmsgs}+${cmsgs}))"
test_template_compare_string[13]="Received $((${jmsmsgs}+${cmsgs})) messages.";
msgs=$((${cmsgs}+${jmsgs}))
component[14]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|$qos|-t|/topic/+/1|-s|tcp://${A1_IPv4_1}:16102|-n|${msgs}"
test_template_compare_string[14]="Received ${msgs} messages.";
msgs=$((${cmsgs}+${jmsmsgs}+${jmsgs}))
component[15]=javaAppDriver,m2,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|$qos|-t|/topic/#|-s|tcp://${A1_IPv4_1}:16102|-n|${msgs}"
test_template_compare_string[15]="Received ${msgs} messages.";

component[16]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[17]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[18]=sleep,1
test_template_runorder="18 2 3 4 5 7 8 9 10 12 13 14 15 16 17 1 6 11"
test_template_finalize_test
((n+=1))

done

