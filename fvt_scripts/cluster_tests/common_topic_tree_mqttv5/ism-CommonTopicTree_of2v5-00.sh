#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Simple tests to verify Basic Cluster functionality " 

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
# Scenario 1 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTTv5_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on Server 2 , receive Messages on Server 1. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTTv5_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on Server 1, receive Messages on Server 2. (reverse of prior test) "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTTv5_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on both servers, receive all Messages on both. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree_mqttv5/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,common_topic_tree_mqttv5/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree_mqttv5/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))
