#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ima Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_imaSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_imaSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
#   <machineNumber_imaSetup>
#		m1 is the machine 1 in imaSetup.sh, m2 is the machine 2 and so on...
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
#	component[x]=sync,<machineNumber_imaSetup>,
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
# Test Case 1 - ISM to ISM
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="MQ_CONNECTIVITY_1"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj 1500 message is published and subscribed by a durable subscriber [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60

# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.sample.test.aj.SimpleJMSSyncTest,-o|${A1_IPv4_1}|16102|${MQKEY}/FOOTBALL/ARSENAL|1500"
component[2]=sleep,5
component[3]=searchLogsEnd,m1,ima_test_connectivity-runscenarios01_m1.comparetest,
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------

xml[${n}]="MQ_CONNECTIVITY_2"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj 1 message is published  to ISM and subscribed by MQ [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.sample.test.aj.ImaToMq,-o|${A1_IPv4_1}|${MQSERVER1_IP}|16102|1415|QM_MQJMS|${MQKEY}/FOOTBALL/ARSENAL|MQCLIENTS|100"
component[3]=searchLogsEnd,m1,ima_test_connectivity-runscenarios02_m1.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_setup_mqconnectivity|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------

xml[${n}]="MQ_CONNECTIVITY_MULTI_PROD_SETUP"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj.ImaToMqMultiProd Setup"
timeouts[${n}]=15
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------

if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
#slow disk persistence, longer time needed to send messages to IMA Server
#fewer messages are sent.
    export TIMEOUTMULTIPLIER=1

	xml[${n}]="MQ_CONNECTIVITY_MULTI_PROD"
	scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj.ImaToMqMultiProd multiple producers publishing multiple messages to ISM and subscribed by MQ [Many Pub; 1 Sub] with Topic Subscription"
	timeouts[${n}]=240
	# Set up the components for the test in the order they should start
	component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.sample.test.aj.ImaToMqMultiProd,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|${MQKEY}/FOOTBALL/ARSENAL|MQCLIENTS|1000"
	component[2]=searchLogsEnd,m1,ima_test_connectivity-runscenarios03_m1.comparetest
	components[${n}]="${component[1]} ${component[2]} "
	((n+=1))

else 
	xml[${n}]="MQ_CONNECTIVITY_MULTI_PROD"
	scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj.ImaToMqMultiProd multiple producers publishing multiple messages to ISM and subscribed by MQ [Many Pub; 1 Sub] with Topic Subscription"
	timeouts[${n}]=240
	# Set up the components for the test in the order they should start
	component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.sample.test.aj.ImaToMqMultiProd,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|${MQKEY}/FOOTBALL/ARSENAL|MQCLIENTS|10000"
	component[2]=searchLogsEnd,m1,ima_test_connectivity-runscenarios03_m1.comparetest
	components[${n}]="${component[1]} ${component[2]} "
	((n+=1))

fi

#----------------------------------------------------

xml[${n}]="MQ_CONNECTIVITY_MULTI_PROD_TEARDOWN"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.test.aj.ImaToMqMultiProd Teardown"
timeouts[${n}]=15
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_setup_mqconnectivity|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))























