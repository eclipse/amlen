#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   1/23/14 - Created to do sniff test MQTT C Asynchronous apps
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv3 Asynchronous Samples sniff test"
source ./svt_test.sh

#------------------------------------------
# Next line is required if you want to run "test transactions"
#------------------------------------------
export AUTOMATION_FRAMEWORK_SAVE_RC="/niagara/test/svt_cmqtt/.svt.af.runscenario.last.rc"

test_template_set_prefix "cmqtt_06_"

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

#----------------------------------------------
# The next for loops generate the 24 svt tests named cmqtt_06_00 - cmqtt_06_23
# Success is implied by test finishing without timing out in 30 second window. 
# If test timed out there was an error. These tests comprise a basic sniff test
# for MQTT Async samples provided in the M2M messaging pack. The entire test should
# not take more than 3 minutes for good path.
#----------------------------------------------
for qos in 0 1 2 ; do  
    for app in MQTTV3AsyncSSAMPLE MQTTV3SSAMPLE MQTTV3SAMPLE MQTTV3ASAMPLE; do 
        test_template_add_test_single "MQTTv3 Asynchronous C Sample Sanity Test - app:$app qos:$qos" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|43965|$app|1|${A1_IPv4_1}|${A2_IPv4_1}" 30 "SVT_AT_VARIATION_QOS=$qos" ""
        test_template_add_test_single "MQTTv3 Asynchronous C Sample Sanity Test - app:$app qos:$qos client certificate" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|43965|$app|1|${A1_IPv4_1}|${A2_IPv4_1}" 30 "SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_43965_CLIENT_CERTIFCATE=clientcertificate" ""
    done
done
