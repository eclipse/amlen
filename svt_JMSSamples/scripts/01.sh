#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Sample App"

typeset -i n=0

declare -r demo_ep=tcp://${A1_IPv4_1}:16102
declare -r internet_ep=tcps://${A1_IPv4_1}:16111
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
# xml[${n}]="jms_01_00"
# test_template_initialize_test "${xml[${n}]}"
# scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.jms.JMSSample 1 million msgs with Topic Filter Subscription"
# timeouts[${n}]=600
#
# Set up the components for the test in the order they should start
# component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|publish|-t|/APP/1/CAR/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000|-w|3000"
# test_template_compare_string[1]="Published 1000000 messages to topic /APP/1/CAR/1"
# component[2]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/1/CAR/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
# component[3]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/1/CAR/+|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
# component[4]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/+/CAR/1|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
# component[5]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/#|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
# component[6]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
# component[7]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
# component[8]=sleep,10
#
# for v in {2..5}; do
#    test_template_compare_string[$v]="Received 1000000 messages."
# done
#
# test_template_runorder="3 2 4 5 8 1 6 7"
# test_template_finalize_test
# ((n+=1))
#
#----------------------------------------------------
# Test Case 1 -
#----------------------------------------------------

if [ "${A1_TYPE}" == "ESX" ]; then
  declare MESSAGES=25000
  timeouts[$n]=1500
elif [ "${A1_TYPE}" == "Bare_Metal" ]; then
  declare MESSAGES=25000
  timeouts[$n]=1500
elif [ "${A1_TYPE}" == "DOCKER" ]; then
  declare MESSAGES=25000
  timeouts[$n]=1500
else
  declare MESSAGES=1000000
  timeouts[$n]=720
fi

xml[${n}]="jms_01_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - svt.jms.ism.JMSSample 1 to 4 fan-out of ${MESSAGES} msgs with Topic Filter, order verification, rate control"


# Set up the components for the test in the order they should start
component[1]=javaAppDriverWait,m1,"-e|svt.jms.ism.JMSSample,-o|-i|c1|-a|subscribe|-t|/APP/1/CAR/1|-s|${intranet_ep}|-n|0|-b|-O|-u|a01|-p|imasvtest|-v"
component[2]=javaAppDriverWait,m1,"-e|svt.jms.ism.JMSSample,-o|-i|c2|-a|subscribe|-t|/APP/1/CAR/+|-s|${intranet_ep}|-n|0|-b|-O|-u|a02|-p|imasvtest|-v"
component[3]=javaAppDriverWait,m1,"-e|svt.jms.ism.JMSSample,-o|-i|c3|-a|subscribe|-t|/APP/+/CAR/1|-s|${intranet_ep}|-n|0|-b|-O|-u|a03|-p|imasvtest|-v"
component[4]=javaAppDriverWait,m1,"-e|svt.jms.ism.JMSSample,-o|-i|c4|-a|subscribe|-t|/APP/#|-s|${intranet_ep}|-n|0|-b|-O|-u|a04|-p|imasvtest|-v"
component[5]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|c1|-a|subscribe|-t|/APP/1/CAR/1|-s|${intranet_ep}|-n|${MESSAGES}|-b|-O|-u|a01|-p|imasvtest|-v"
component[6]=javaAppDriver,m2,"-e|svt.jms.ism.JMSSample,-o|-i|c2|-a|subscribe|-t|/APP/1/CAR/+|-s|${intranet_ep}|-n|${MESSAGES}|-b|-O|-u|a02|-p|imasvtest|-v"
component[7]=javaAppDriver,m3,"-e|svt.jms.ism.JMSSample,-o|-i|c3|-a|subscribe|-t|/APP/+/CAR/1|-s|${intranet_ep}|-n|${MESSAGES}|-b|-O|-u|a03|-p|imasvtest|-v"
component[8]=javaAppDriver,m4,"-e|svt.jms.ism.JMSSample,-o|-i|c4|-a|subscribe|-t|/APP/#|-s|${intranet_ep}|-n|${MESSAGES}|-b|-O|-u|a04|-p|imasvtest|-v"
component[9]=sleep,10
component[10]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|p1|-a|publish|-t|/APP/1/CAR/1|-s|${internet_ep}|-n|${MESSAGES}|-w|5000|-r|-O|-u|c0000001|-p|imasvtest|-v,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[11]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[12]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[13]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[14]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9

for v in {5..8}; do
    test_template_compare_string[$v]="Message Order Pass"
done
test_template_compare_string[10]="sent ${MESSAGES} messages to topic /APP/1/CAR/1"
test_template_finalize_test





