#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios
#to debug include source ../scripts/commonFunctions.sh
source ../scripts/commonFunctions.sh

# need to add ip to java args so need to retrive from correct script
if [[ -f "../testEnv.sh" ]]
then
  source ../testEnv.sh
else
  source ../scripts/ISMsetup.sh
fi

scenario_set_name="SVT Regression"

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

#
#Need to setup where each testcase will be executed on which client
#Represented by m1,m2... etc
#
#These variables are setup so that components do not need to be hand modified
#when wanting to execute a testcase on a specific client
#
#Therefore, only modify the list below to modify where a testcase will be
#executed.
#
GeneralConnections_M=m1
GeneralQueueRegression_M=m1
GeneralDeleteMqttClients_M=m1
GeneralRetainedTopics_M=m1
GeneralDeleteSubscriptions_M=m1
GeneralDeleteRetainedMessages_M=m1
GeneralDiscardMessages_M=m1
GeneralDisconnectClients_M=m1
GeneralIntegrityCheck_M=m1
GeneralSubscriptionClients_M=m1
GeneralTopicRegression_M=m1
GeneralFailoverRegression_M=m1

#
# Set Timeout here which is used multiple times below.
# This Timeout determines how long this test should run. By default
# it set for 48hrs or 172800 seconds.
#
SVTTIMEOUT=172800

#
# Sleep timeout should be a little less so automation will not fail
# due to components not ending
#
SLEEPTIMEOUT=${SVTTIMEOUT-180}

xml[${n}]="svt_regression_${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - SVT regression Standalone"
#set timeout to 48hrs
timeouts[${n}]=${SVTTIMEOUT}


# Set up the components for the test in the order they should start
component[1]=javaAppDriver,${GeneralConnections_M},"-e|com.ibm.ima.svt.regression.GeneralConnections,-o|${A1_IPv4_1}|10000000000|SVTCONN|500|0|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[2]=javaAppDriver,${GeneralQueueRegression_M},"-e|com.ibm.ima.svt.regression.GeneralQueueRegression,-o|${A1_IPv4_1}|${A1_HOST}|null|true|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[3]=javaAppDriver,${GeneralDeleteMqttClients_M},"-e|com.ibm.ima.svt.regression.GeneralDeleteMqttClients,-o|${A1_HOST}|null|true|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[4]=javaAppDriver,${GeneralRetainedTopics_M},"-e|com.ibm.ima.svt.regression.GeneralRetainedTopics,-o|${A1_IPv4_1}|${A1_HOST}|null|4000000|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[5]=javaAppDriver,${GeneralDeleteSubscriptions_M},"-e|com.ibm.ima.svt.regression.GeneralDeleteSubscriptions,-o|${A1_HOST}|null|SVTSUB-TOPIC|true|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[6]=javaAppDriver,${GeneralDeleteRetainedMessages_M},"-e|com.ibm.ima.svt.regression.GeneralDeleteRetainedMessages,-o|${A1_IPv4_1}|SVTRET-/chat/|0|10000|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[7]=javaAppDriver,${GeneralDiscardMessages_M},"-e|com.ibm.ima.svt.regression.GeneralDiscardMessages,-o|${A1_IPv4_1}|${A1_HOST}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[8]=javaAppDriver,${GeneralDisconnectClients_M},"-e|com.ibm.ima.svt.regression.GeneralDisconnectClients,-o|${A1_IPv4_1}|${A1_HOST}|null|true|400000|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[9]=javaAppDriver,${GeneralIntegrityCheck_M},"-e|com.ibm.ima.svt.regression.GeneralIntegrityCheck,-o|${A1_IPv4_1}|${A1_HOST}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[10]=javaAppDriver,${GeneralSubscriptionClients_M},"-e|com.ibm.ima.svt.regression.GeneralSubscriptionClients,-o|${A1_IPv4_1}|${A1_HOST}|null|true|4000000|false,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[11]=javaAppDriver,${GeneralTopicRegression_M},"-e|com.ibm.ima.svt.regression.GeneralTopicRegression,-o|${A1_IPv4_1}|${A1_HOST}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[12]=javaAppDriver,${GeneralFailoverRegression_M},"-e|com.ibm.ima.svt.regression.GeneralFailoverRegression,-o|${A1_IPv4_1}|${A1_HOST}|null|1200000|360000|mixed|isFailover|true|100000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true|-Xmx4096m"

#Lets sleep for slightly less than timeout (ie 48hrs run)
component[13]=sleep,${SVTTIMEOUT}

#kill all testcases before verifing
component[14]=cAppDriverWait,m1,"-e|/niagara/test/scripts/svt-runCmdAt.sh,-o|1s|killall|java"

#Invoke check scripts for each testcase
component[15]=cAppDriverWait,${GeneralConnections_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralConnections.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralConnections-${GeneralConnections_M/m/M}-1.screenout.log"

component[16]=cAppDriverWait,${GeneralQueueRegression_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralQueueRegression.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralQueueRegression-${GeneralQueueRegression_M/m/M}-2.screenout.log"

component[17]=cAppDriverWait,${GeneralDeleteMqttClients_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralDeleteMqttClients.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralDeleteMqttClients-${GeneralDeleteMqttClients_M/m/M}-3.screenout.log"

component[18]=cAppDriverWait,${GeneralRetainedTopics_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralRetainedTopics.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralRetainedTopics-${GeneralRetainedTopics_M/m/M}-4.screenout.log"

component[19]=cAppDriverWait,${GeneralDeleteSubscriptions_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralDeleteSubscriptions.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralDeleteSubscriptions-${GeneralDeleteSubscriptions_M/m/M}-5.screenout.log"

component[20]=cAppDriverWait,${GeneralDeleteRetainedMessages_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralDeleteRetainedMessages.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralDeleteRetainedMessages-${GeneralDeleteRetainedMessages_M/m/M}-6.screenout.log"

component[21]=cAppDriverWait,${GeneralDiscardMessages_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralDiscardMessages.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralDiscardMessages-${GeneralDiscardMessages_M/m/M}-7.screenout.log"

component[22]=cAppDriverWait,${GeneralDisconnectClients_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralDisconnectClients.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralDisconnectClients-${GeneralDisconnectClients_M/m/M}-8.screenout.log"

component[23]=cAppDriverWait,${GeneralIntegrityCheck_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralIntegrityCheck.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralIntegrityCheck-${GeneralIntegrityCheck_M/m/M}-9.screenout.log"

component[24]=cAppDriverWait,${GeneralSubscriptionClients_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralSubscriptionClients.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralSubscriptionClients-${GeneralSubscriptionClients_M/m/M}-10.screenout.log"

component[25]=cAppDriverWait,${GeneralTopicRegression_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralTopicRegression.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralTopicRegression-${GeneralTopicRegression_M/m/M}-11.screenout.log"

component[26]=cAppDriverWait,${GeneralFailoverRegression_M},"-e|${M1_TESTROOT}/svt_client_tests/Validate_GeneralFailoverRegression.sh,-o|STD|${M1_TESTROOT}/svt_client_tests/${xml[$n]}-javaAppDriver-com.ibm.ima.svt.regression.GeneralFailoverRegression-${GeneralFailoverRegression_M/m/M}-12.screenout.log"


# searchlogs end is required for each client used above
component[27]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

test_template_compare_string[15]="GeneralConnections Success"
test_template_compare_string[16]="GeneralQueueRegression Success"
test_template_compare_string[17]="GeneralDeleteMqttClients Success"
test_template_compare_string[18]="GeneralRetainedTopics Success"
test_template_compare_string[19]="GeneralDeleteSubscriptions Success"
test_template_compare_string[20]="GeneralDeleteRetainedMessages Success"
test_template_compare_string[21]="GeneralDiscardMessages Success"
test_template_compare_string[22]="GeneralDisconnectClients Success"
test_template_compare_string[23]="GeneralIntegrityCheck Success"
test_template_compare_string[24]="GeneralSubscriptionClients Success"
test_template_compare_string[25]="GeneralTopicRegression Success"
test_template_compare_string[26]="GeneralFailoverRegression Success"

test_template_runorder="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27"
test_template_finalize_test

