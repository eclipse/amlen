#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Java Script MQTT Sample Application - 3.1.1 test of IMA shared subscription feature"

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
#NOTE: As of ima14a sprint 5, only QoS 0 is supported for shared subscriptions
xml[${n}]="jsws_06_01"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - JavaScript MQTT 2 Pub, 2Sub, 400 msgs, QoS 0 - 311 test IMA shared sub feature"
# Set up the components for the test in the order they should start

# If this log file is created outside of RunScenarios (such as by php) then
# specify a different filename than the one RunScenarios generates.
# .log will be automatically concatenated, so do not specify it here
test_template_search_file[1]=${xml[${n}]}-cAppDriver-firefox-M1-1js.screenout

component[1]=cAppDriver,m1,"-e|firefox","-o|http://${M1_HOST}${M1_TESTROOT}/svt_jsws/web/svtStockTickerScale.html?logfile=${test_template_search_file[1]}&clientID=M1&mqttVersion=4&sharedSub=svtShared&subQoS=0&winClose=true","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest
test_template_compare_string[1]="Pub Client PUBM1_0 completed. Published 400 msgs."
test_template_compare_string[1]+=$'\nPub Client PUBM1_1 completed. Published 400 msgs.'
test_template_compare_string[1]+=$'\nSub Client $SharedSubscription/svtShared/SUBM1_0 Completed!  Expected Msgs: 800  Received Msgs: 800'
test_template_compare_string[1]+=$'\nSub Client $SharedSubscription/svtShared/SUBM1_1 Completed!  Expected Msgs: 800  Received Msgs: 800'
test_template_compare_string[1]+=$'\nTest Successful!'

test_template_finalize_test

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#NOTE: As of ima14a sprint 5, only QoS 0 is supported for shared subscriptions
xml[${n}]="jsws_06_02"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - 3 StockTicker @ JavaScript MQTT 6 Pub, 6Sub, 400 msgs, QoS 0 - 311 test IMA shared sub feature"
timeouts[${n}]=200
#
test_template_search_file[1]=${xml[${n}]}-cAppDriver-firefox-M1-1js.screenout
test_template_search_file[2]=${xml[${n}]}-cAppDriver-firefox-M2-2js.screenout
test_template_search_file[3]=${xml[${n}]}-cAppDriver-firefox-M3-3js.screenout

component[1]=cAppDriver,m1,"-e|firefox","-o|http://${M1_HOST}${M1_TESTROOT}/svt_jsws/web/svtStockTickerScale.html?logfile=${test_template_search_file[1]}&clientID=M1&mqttVersion=4&sharedSub=svtShared&subQoS=0&winClose=true","-s-DISPLAY=localhost:1"
component[2]=cAppDriver,m2,"-e|firefox","-o|http://${M1_HOST}${M1_TESTROOT}/svt_jsws/web/svtStockTickerScale.html?logfile=${test_template_search_file[2]}&clientID=M2&mqttVersion=4&sharedSub=svtShared&subQoS=0&winClose=true","-s-DISPLAY=localhost:1"
component[3]=cAppDriver,m3,"-e|firefox","-o|http://${M1_HOST}${M1_TESTROOT}/svt_jsws/web/svtStockTickerScale.html?logfile=${test_template_search_file[3]}&clientID=M3&mqttVersion=4&sharedSub=svtShared&subQoS=0&winClose=true","-s-DISPLAY=localhost:1"

#--------------------------------------
# New 1.3.2013: Support for multiple compare strings for a single component. Big caveat is that only a single 
# compare count can be used across multiple strings (in this case the default 1 count is used)
#--------------------------------------
test_template_compare_string[1]='Pub Client PUBM1_0 completed. Published 400 msgs.'
test_template_compare_string[1]+=$'\nPub Client PUBM1_1 completed. Published 400 msgs.'
test_template_compare_string[1]+=$'\nSub Client $SharedSubscription/svtShared/SUBM1_0 Completed!  Expected Msgs: 800  Received Msgs: 800'
test_template_compare_string[1]+=$'\nSub Client $SharedSubscription/svtShared/SUBM1_1 Completed!  Expected Msgs: 800  Received Msgs: 800' 
test_template_compare_string[1]+=$'\nTest Successful!'

test_template_compare_string[2]='Pub Client PUBM2_0 completed. Published 400 msgs.'
test_template_compare_string[2]+=$'\nPub Client PUBM2_1 completed. Published 400 msgs.'
test_template_compare_string[2]+=$'\nSub Client $SharedSubscription/svtShared/SUBM2_0 Completed!  Expected Msgs: 800  Received Msgs: 800'
test_template_compare_string[2]+=$'\nSub Client $SharedSubscription/svtShared/SUBM2_1 Completed!  Expected Msgs: 800  Received Msgs: 800'  
test_template_compare_string[2]+=$'\nTest Successful!'

test_template_compare_string[3]='Pub Client PUBM3_0 completed. Published 400 msgs.'
test_template_compare_string[3]+=$'\nPub Client PUBM3_0 completed. Published 400 msgs.'
test_template_compare_string[3]+=$'\nPub Client PUBM3_1 completed. Published 400 msgs.'
test_template_compare_string[3]+=$'\nSub Client $SharedSubscription/svtShared/SUBM3_0 Completed!  Expected Msgs: 800  Received Msgs: 800'
test_template_compare_string[3]+=$'\nSub Client $SharedSubscription/svtShared/SUBM3_1 Completed!  Expected Msgs: 800  Received Msgs: 800'  
test_template_compare_string[3]+=$'\nTest Successful!'

component[4]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest
component[5]=searchLogsEnd,m1,${xml[${n}]}_m2.comparetest
component[6]=searchLogsEnd,m1,${xml[${n}]}_m3.comparetest
test_template_finalize_test


