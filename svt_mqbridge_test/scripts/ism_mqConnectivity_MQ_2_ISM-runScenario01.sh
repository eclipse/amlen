#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It is for testing the ISM-MQ Connectivity
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Testing MQ_Connectivity ISM to MQ"

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
xml[${n}]="ISM_MQTT_JMS_SAMPLE_1"
scenario[${n}]="${xml[${n}]} - com.ibm.ism.mqbridge.test.AJ_Sample 1 million msgs with Topic Filter Subscription"
timeouts[${n}]=400

# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ism.mqbridge.test.AJ_Sample,-o|aj|publish|-t|/topic/A/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#xml[${n}]="ISM_MQTT_JMS_SAMPLE_2"
#scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.jms.JMSSample"
#
## Set up the components for the test in the order they should start
#component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|publish|-t|/topic/A/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
#component[2]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/A/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
#component[3]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/A/+|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
#component[4]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/+/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
#component[5]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/topic/#|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
#component[6]=searchLogsEnd,m1,ism_MQTT_JMS_Sample_2-runscenarios01.comparetest,9
#components[${n}]="${component[3]} ${component[2]}  ${component[4]} ${component[5]} ${component[1]} ${component[6]} "

