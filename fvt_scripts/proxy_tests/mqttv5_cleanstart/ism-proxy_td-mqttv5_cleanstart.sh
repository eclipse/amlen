#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv5 tests Proxy CleanStart and Sessions"

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
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

#----------------------------------------------------
# Test Case 0 - mqtt_policyconfig_setup and Proxy Start
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - set policies for the mqttv5 bucket"
# Start Proxy
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|../common/proxy.mqttV5.cfg"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupv5|-c|mqttv5_cleanstart/policy_mqttv5.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


#----------------------------------------------------
# Test Case 1 - connect cleanStart & ConnAck SessionPresent
#----------------------------------------------------
xml[${n}]="testproxy_mqttv5_cleanstart_01"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - mqttv5 connect cleanStart - ConnAck SessionPresent"
component[1]=wsDriverWait,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSessions
component[2]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,testCleanStart
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


#----------------------------------------------------
# Test Case 2 - Pub Sub Receive - NonDurable
#----------------------------------------------------
xml[${n}]="testproxy_mqttv5_pubsub_02_nonDurable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - mqttv5 Pub Sub Receive "
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,SubscribeReceive
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,Publish
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))


#----------------------------------------------------
# Test Case 3 - Pub Sub Receive - DurableSession
#----------------------------------------------------
xml[${n}]="testproxy_mqttv5_pubsub_03_durableSession"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - mqttv5 Pub Sub Receive "
component[1]=wsDriverWait,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSessions
component[2]=wsDriverWait,m1,mqttv5_cleanstart/${xml[${n}]}.xml,Subscribe
component[3]=wsDriverWait,m1,mqttv5_cleanstart/${xml[${n}]}.xml,Publish
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,Receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))



#----------------------------------------------------
# Test Case 4 - ClientId Steal - nonDurableSession  ==>   with Version v3-v5-v3
#----------------------------------------------------
xml[${n}]="testproxy_mqttv4_clientSteal_04_nonDurable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - PAHO mqttv4 Pub Sub Receive with ClientId Steal"
component[1]=sync,m1
component[2]=wsDriverWait,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSession
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[6]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))



#----------------------------------------------------
# Test Case 4 - ClientId Steal - DurableSession  ==>   with Version v3-v5-v3
#----------------------------------------------------
xml[${n}]="testproxy_mqttv4_clientSteal_05_Durable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - PAHO mqttv4 Pub Sub Receive with ClientId Steal"
component[1]=sync,m1
component[2]=wsDriverWait,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSession
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[6]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))



#----------------------------------------------------
# Test Case 4 - ClientId Steal - nonDurableSession  ==>   as WS_MQTT-bin
#----------------------------------------------------
xml[${n}]="testproxy_WSmqttv5_clientSteal_04_nonDurable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - WEBSOCKET mqttv5 Pub Sub Receive with ClientId Steal "
component[1]=sync,m1
component[2]=wsDriverWait,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSession
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[6]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))



#----------------------------------------------------
# Test Case 4 - ClientId Steal - nonDurableSession
#----------------------------------------------------
xml[${n}]="testproxy_mqttv5_clientSteal_04_nonDurable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - PAHO mqttv5 Pub Sub Receive with ClientId Steal "
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSession
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "
((n+=1))



#----------------------------------------------------
# Test Case 5 - ClientId Steal - DurableSession
#----------------------------------------------------
xml[${n}]="testproxy_mqttv5_clientSteal_05_durable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - mqttv5 Pub Sub Receive PAHO MQTTv5"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSessions
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5-1
component[6]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5-2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))



#----------------------------------------------------
# Test Case 5 - ClientId Steal - DurableSession
#----------------------------------------------------
xml[${n}]="testproxy_WSmqttv5_clientSteal_05_durable"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - mqttv5 Pub Sub Receive WebSocket MQTTv5"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,setupSessions
component[3]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,thief
component[4]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV3
component[5]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5-1
component[6]=wsDriver,m1,mqttv5_cleanstart/${xml[${n}]}.xml,receiveV5-2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))


#----------------------------------------------------
# Finally - mqtt_policyconfig_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup policies for v5 connect bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupv5|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupv5exp|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[3]=cAppDriver,m1,"-e|../common/rmScratch.sh"
component[4]=cAppDriver,m1,"-e|./killProxy.sh"
#components[${n}]="${component[1]} ${component[2]} ${component[3]}  ${component[4]}"
components[${n}]="${component[1]}                  ${component[3]}  ${component[4]}"
((n+=1))


