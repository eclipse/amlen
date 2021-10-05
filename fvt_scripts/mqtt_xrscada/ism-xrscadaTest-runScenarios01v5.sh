#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQ Telemetry XRSCADA Tests"

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
# Scenario 0 
#----------------------------------------------------
# All of these tests the Socket timeout is set to 30000 so our timeout should be greater
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.xml[${n}]="MqttXrscadaTest0"
xml[${n}]="MqttXrscadaTest0"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_MESSAGES with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_MESSAGES,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_0.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


#----------------------------------------------------
# Scenario 12
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest12"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_SUBSCRIBE with MQTTv5"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_SUBSCRIBE,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[3]=searchLogsEnd,m1,mqttXrscadaTest_12.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest1"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_QOS0 using IPv6 Address with MQTTv5"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_QOS0,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[3]=searchLogsEnd,m1,mqttXrscadaTest_1.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest2"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_QOS01 with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_QOS1,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_2.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest4"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS0 with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS0,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_4.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 5
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest5"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS1 using IPv6 Address with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS1,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_5.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest6"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS2 with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS2,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_6.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest7"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS0RETAIN using IPv6 Address with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS0RETAIN,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_7.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 8
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest8"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS1RETAIN with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS1RETAIN,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_8.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 9
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest9"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILLQOS2RETAIN using IPv6 Address with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILLQOS2RETAIN,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_9.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest10"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_RETAIN with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_RETAIN,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_10.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest11"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_KEEPALIVE using IPv6 Address with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_KEEPALIVE,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_11.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 13
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest13"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_NETWORK using IPv6 Address with MQTTv5"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace7|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_NETWORK,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[3]=searchLogsEnd,m1,mqttXrscadaTest_13.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 14
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest14"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_ERRORS with MQTTv5"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace5|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_ERRORS,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[3]=searchLogsEnd,m1,mqttXrscadaTest_14.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario 3 
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest3"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_QOS02 using IPv6 Address with MQTTv5"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace5|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_QOS2,-o|-h|${A1_IPv6_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[3]=searchLogsEnd,m1,mqttXrscadaTest_3.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 15
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest15"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_PACKED with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_PACKED,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_15.comparetest
components[${n}]="${component[1]} ${component[2]} "
((n+=1))

#----------------------------------------------------
# Scenario 16
#----------------------------------------------------
xml[${n}]="MqttXrscadaTest16"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - SDP_PROTOCOL_WILL0LENGTHRETAIN with MQTTv5"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.mqst.mqxr.scada.SDP_PROTOCOL_WILL0LENGTHRETAIN,-o|-h|${A1_IPv4_1}|-p|${ISMMqttPort}|-v|${MQTTVERSION}"
component[2]=searchLogsEnd,m1,mqttXrscadaTest_16.comparetest
components[${n}]="${component[1]} ${component[2]} "
((n+=1))


#----------------------------------------------------
# Scenario clear all retained 
#----------------------------------------------------
xml[${n}]="mqtt_clearRetained.xml"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Clear all retained messages with MQTTv5"
component[1]=wsDriver,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
