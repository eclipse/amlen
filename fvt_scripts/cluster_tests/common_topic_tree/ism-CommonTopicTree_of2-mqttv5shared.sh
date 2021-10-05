#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Simple tests to verify mixed durability shared subs functionality in cluster " 

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case runningon the target machine.
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

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi


#----------------------------------------------------
# Scenario 17 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_017_sharedM"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - sharedMix subs. Messages should not be forwarded. Pub on server 1"
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 17b - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_017b_sharedM"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - sharedMix subs. Messages should not be forwarded. Pub on server 2"
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 18 -
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_018_sharedM"
timeouts[${n}]=160
scenario[${n}]="${xml[${n}]} - sharedMix subs - redelivery of in-flight msg on client disconnect"
component[1]=sync,m1
component[2]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,Publisher
component[4]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,Subscribes
component[5]=sleep,5
component[6]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_clusterCTT_018_sharedM.xml-Subscribes-M1.log|-t|2500|-c|CF1|CF2|-v|2|-ge|1"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_clusterCTT_018_sharedM.xml-Subscribes-M1.log|-t|2500|-c|CF3|CF4|-v|2|-ge|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))
