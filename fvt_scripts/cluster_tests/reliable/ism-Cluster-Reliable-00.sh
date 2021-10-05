#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster Reliable Messaging Tests" 

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
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - publish 20 messages with maxmessages 10"
component[1]=sync,m1
component[2]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Subscribe
component[3]=wsDriver,m1,reliable/${xml[${n}]}.xml,server2Publish
component[4]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 -
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Simple QoS 2 message flow with no oddities"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Subscribe
component[2]=wsDriver,m1,reliable/${xml[${n}]}.xml,server2Subscribe
component[3]=wsDriver,m1,reliable/${xml[${n}]}.xml,server3Subscribe
component[4]=sleep,3
component[5]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Publish
component[6]=wsDriver,m1,reliable/${xml[${n}]}.xml,server2Publish
component[7]=wsDriver,m1,reliable/${xml[${n}]}.xml,server3Publish
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - Crash clustered server during publishing
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_003"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Crash clustered server during publishing"
component[1]=sync,m1
component[2]=wsDriver,m1,reliable/${xml[${n}]}.xml,server2Subscribe
component[3]=sleep,5
component[4]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Publish
component[5]=cAppDriverLogEnd,m1,"-e|./compare.sh","-o|-a|${xml[${n}]}.xml-server1Publish-M1.log|-b|${xml[${n}]}.xml-server2Subscribe-M1.log|-m|Publishing|-n|ReceiveMessageAction.*complete"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - Crash clustered server during receiving
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_004"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Crash clustered server during receiving"
component[1]=sync,m1
component[2]=wsDriver,m1,reliable/${xml[${n}]}.xml,server1Subscribe
component[3]=sleep,5
component[4]=wsDriver,m1,reliable/${xml[${n}]}.xml,server2Publish
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#component[5]=cAppDriverLogEnd,m1,"-e|./compare.sh","-o|-a|${xml[${n}]}.xml-server1Publish-M1.log|-b|${xml[${n}]}.xml-server2Subscribe-M1.log|-m|Publishing|-n|ReceiveMessageAction.*complete"
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - Publish to server 1 and 2 while server 3 down
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_005"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Publish to server 1 and 2 while server 3 down"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - Publish to server 1 and 2 and stop server 3 inbetween
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_006"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Publish to server 1 and 2 and stop server 3 inbetween"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - Stop server 3 then Publish to server 1 then stop server 1 and start both
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_007"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Stop server 3 then Publish to server 1 then stop server 1 and start both"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - Subscribe to 20 topics on each of 3 servers. Publish to all the topics from 2 servers
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_008"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Subscribe to 20 topics on each of 3 servers. Publish to all the topics from 2 servers"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - Subscribe to many topics on each of 3 servers. Publish to subset of topics on each server
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterReliable_009"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Subscribe to many topics on each of 3 servers. Publish to subset of topics on each server"
component[1]=wsDriver,m1,reliable/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
