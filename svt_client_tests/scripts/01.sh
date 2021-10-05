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
xml[${n}]="svt_regression_${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - SVT regression standalone"
#timeouts[${n}]=600
#set timeout to 48hrs
timeouts[${n}]=172800

# Set up the components for the test in the order they should start
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralConnections,-o|${A1_IPv4_1}|10000000000|SVTCONN|500|0|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[2]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralQueueRegression,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|true|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[3]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralDeleteMqttClients,-o|${A1_IPv4_1}|null|true|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[4]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralRetainedTopics,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|4000000|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[5]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralDeleteSubscriptions,-o|${A1_IPv4_1}|null|SVTSUB-TOPIC|true|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[6]=javaAppDriver,m2,"-e|com.ibm.ima.svt.regression.GeneralDeleteRetainedMessages,-o|${A1_IPv4_1}+${A2_IPv4_1}|SVTRET-/chat/|0|10000|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[7]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralDiscardMessages,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[8]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralDisconnectClients,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|true|400000|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[9]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralIntegrityCheck,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[10]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralRetainedTopics,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|4000000|true,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[11]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralSubscriptionClients,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|true|4000000|false,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[12]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralTopicRegression,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|false|10000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true"
component[13]=javaAppDriver,m1,"-e|com.ibm.ima.svt.regression.GeneralFailoverRegression,-o|${A1_IPv4_1}|${A1_IPv4_1}|null|1200000|360000|mixed|isFailover|true|100000000000,-s|JVM_ARGS=-DMyTraceFile=stdout|-DMyTrace=true|-Xmx4096m"
component[14]=cAppDriverWait,m1,"-e|/niagara/test/scripts/svt-runCmdAt.sh,-o|172700s|killall|java"

test_template_compare_string[1]="GeneralConnections started"
test_template_compare_string[2]="GeneralQueueRegression started"
test_template_compare_string[3]="GeneralDeleteMqttClients started"
test_template_compare_string[4]="GeneralRetainedTopics started"
test_template_compare_string[5]="GeneralDeleteSubscriptions started"
test_template_compare_string[6]="GeneralDeleteRetainedMessages started"
test_template_compare_string[7]="GeneralDiscardMessages started"
test_template_compare_string[8]="GeneralDisconnectClients started"
test_template_compare_string[9]="GeneralIntegrityCheck started"
test_template_compare_string[10]="GeneralRetainedTopics started"
test_template_compare_string[11]="GeneralSubscriptionClients started"
test_template_compare_string[12]="GeneralTopicRegression started"
test_template_compare_string[13]="GeneralFailoverRegression started"

test_template_runorder="1 2 3 4 5 6 7 8 9 10 11 12 13 14"
test_template_finalize_test





