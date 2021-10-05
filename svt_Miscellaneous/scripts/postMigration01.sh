#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Misc Post Scenarios 01"
source ../scripts/commonFunctions.sh

test_template_set_prefix "miscPostMig_01_"

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
# Test Case 0 
#----------------------------------------------------
#
#  This testcase creates multiple generations in the store and then fails over the HA pair and verifies store.
#
#----------------------------------------------------

 source template1.sh

 printf -v tname "miscPostMig_%02d" ${n}

 declare -li pub=500000
 declare -li sub=$((${pub}*4))
 declare -li x=0

 ((x=0)); client[$x]=jms; count[$x]=1;action[$x]=subscribe;id[$x]=10;qos[$x]=0;durable[$x]=true ;persist[$x]=false;topic[$x]=/svtGroup0/chat;port[$x]=16102;messages[$x]=${sub};message[$x]='Message Order Pass';rate[$x]=0   ;ssl[$x]=false;
 ((x++)); client[$x]=jms; count[$x]=4;action[$x]=publish  ;id[$x]=20;qos[$x]=0;durable[$x]=false;persist[$x]=true ;topic[$x]=/svtGroup0/chat;port[$x]=16102;messages[$x]=${pub};message[$x]=`eval echo 'sent ${pub} messages'`;rate[$x]=4000;ssl[$x]=false;

 test_template_define "testcase=${tname}" "description=mqtt_41_311_HA" "timeout=600" "order=true" "fill=false" "failover=0" "verify=true" "userid=false" "${client[*]}" "${count[*]}" "${action[*]}" "${id[*]}" "${qos[*]}" "${durable[*]}" "${persist[*]}" "${topic[*]}" "${port[*]}" "${messages[*]}" "$( IFS=$'|'; echo "${message[*]}" )" "${rate[*]}" "${ssl[*]}"

#----------------------------
