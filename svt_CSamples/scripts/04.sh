#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#   11/26/12 - Refactored ism-MQTT_C_Sample-runScenarios02.sh into 04.sh
# 	Note: Found that this testcase is not in use currently. Delete?
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
xml[${n}]="cmqtt_04_00"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - mqttsample 69 messages with Topic Filter Subscription"
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|mqttsample,-o|-a|publish|-q|0|-t|/topic/A/1|-s|${A1_IPv4_2}:16102|-n|69"
test_template_compare_string[$v]="Published 69 messages to topic: /topic/A/1";
component[2]=cAppDriver,m1,"-e|mqttsample,-o|-a|subscribe|-q|0|-t|/topic/A/1|-s|${A1_IPv4_2}:16102|-n|69"
component[3]=cAppDriver,m2,"-e|mqttsample,-o|-a|subscribe|-q|0|-t|/topic/A/+|-s|${A1_IPv4_2}:16102|-n|69"
component[4]=cAppDriver,m1,"-e|mqttsample,-o|-a|subscribe|-q|0|-t|/topic/+/1|-s|${A1_IPv4_2}:16102|-n|69"
component[5]=cAppDriver,m2,"-e|mqttsample,-o|-a|subscribe|-q|0|-t|/topic/#|-s|${A1_IPv4_2}:16102|-n|69"
component[6]=sleep,5
#----------------------------------------
# Specify compare test, and metrics for components 2-5 
#----------------------------------------
for v in {2..5}; do
    test_template_compare_string[$v]="Received 69 messages."
done
component[7]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
test_template_runorder="2 3 4 5 6 1 7"
test_template_finalize_test

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
xml[${n}]="cmqtt_04_01"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - mqttsample 1 million msgs with Topic Filter Subscription"
#timeouts[${n}]=1200
#
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|mqttsample_array,-o|-a|publish|-q|0|-t|/topic/A/1|-s|${A1_IPv4_2}:16102|-n|1000000|-x|testCriteriaPctMsg=50|-x|verifyStillActive=30"
component[2]=cAppDriver,m1,"-e|mqttsample_array,-o|-a|subscribe|-q|0|-t|/topic/A/1|-s|${A1_IPv4_2}:16102|-n|1000000|-x|testCriteriaPctMsg=50|-x|verifyStillActive=30"
component[3]=cAppDriver,m2,"-e|mqttsample_array,-o|-a|subscribe|-q|0|-t|/topic/A/+|-s|${A1_IPv4_2}:16102|-n|1000000|-x|testCriteriaPctMsg=50|-x|verifyStillActive=30"
component[4]=cAppDriver,m1,"-e|mqttsample_array,-o|-a|subscribe|-q|0|-t|/topic/+/1|-s|${A1_IPv4_2}:16102|-n|1000000|-x|testCriteriaPctMsg=50|-x|verifyStillActive=30"
component[5]=cAppDriver,m2,"-e|mqttsample_array,-o|-a|subscribe|-q|0|-t|/topic/#|-s|${A1_IPv4_2}:16102|-n|1000000|-x|testCriteriaPctMsg=50|-x|verifyStillActive=30"
component[6]=sleep,5
#----------------------------------------
# Specify compare test, and metrics for components 1-5 
#----------------------------------------
for v in {1..5}; do
    test_template_compare_string[$v]="SVT_TEST_RESULT: SUCCESS"
done

component[7]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
test_template_runorder="2 3 4 5 6 1 7"
test_template_finalize_test

