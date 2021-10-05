#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS to MQTT - 00"

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
# Scenario 1 - jms_mqtt_security_001 mqtt -> jms
#----------------------------------------------------
#xml[${n}]="jms_mqtt_security_001"
#timeouts[${n}]=20
#scenario[${n}]="jms_mqtt_security_001 [ ${xml[${n}]}.xml ]"
#component[1]=sync,m1
#component[2]=jmsDriver,m1,jms_policy_validation.xml,rmdr
#component[3]=wsDriver,m2,mqtt_policy_validation.xml,rmdt
#components[${n}]="${component[1]} ${component[2]} ${component[3]}"
#((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_mqtt_security_001 jms -> mqtt
#----------------------------------------------------
#xml[${n}]="jms_mqtt_security_001"
#timeouts[${n}]=20
#scenario[${n}]="jms_mqtt_security_001 [ ${xml[${n}]}.xml ]"
#component[1]=sync,m1
#component[2]=wsDriver,m1,mqtt_policy_validation.xml,rmdr
#component[3]=jmsDriver,m2,jms_policy_validation.xml,rmdt
#components[${n}]="${component[1]} ${component[2]} ${component[3]}"
#((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_mqtt_security_001 jms -> jms
#----------------------------------------------------
#xml[${n}]="jms_mqtt_security_001"
#timeouts[${n}]=20
#scenario[${n}]="jms_mqtt_security_001 [ ${xml[${n}]}.xml ]"
#component[1]=sync,m1
#component[2]=jmsDriver,m1,jms_policy_validation.xml,rmdr
#component[3]=jmsDriver,m2,jms_policy_validation.xml,rmdt
#components[${n}]="${component[1]} ${component[2]} ${component[3]}"
#((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_mqtt_security_001 mqtt -> mqtt
#----------------------------------------------------
#xml[${n}]="jms_mqtt_security_001"
#timeouts[${n}]=20
#scenario[${n}]="jms_mqtt_security_001 [ ${xml[${n}]}.xml ]"
#component[1]=sync,m1
#component[2]=wsDriver,m1,mqtt_policy_validation.xml,rmdr
#component[3]=wsDriver,m2,mqtt_policy_validation.xml,rmdt
#components[${n}]="${component[1]} ${component[2]} ${component[3]}"
#((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_mqtt_001
#----------------------------------------------------
xml[${n}]="jms_mqtt_001"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_001 JMS to MQTT - Test 1 with Unicode Topic Name and RETAINED. [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqttV5.xml,ALL
component[4]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_mqtt_002
#----------------------------------------------------
xml[${n}]="jms_mqtt_002"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_002 MQTT to JMS - Test 2 with Unicode Topic Name and RETAINED.[ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario 7a - jms_mqtt_003
#----------------------------------------------------
xml[${n}]="jms_mqtt_003"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_003 MQTT to/from JMS - Test 3 [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7b - jms_mqttV5_003
#----------------------------------------------------
xml[${n}]="jms_mqttV5_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} MQTTv5 exchange User Properties to/from JMS - Test 3 [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))



#----------------------------------------------------
# Scenario 7c - jms_mqttV5_err01
#----------------------------------------------------
xml[${n}]="jms_mqttV5_err01"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} MQTTv5 exchange User Properties to/from JMS - Test Exceptions and Errors [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}.jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}.mqtt.xml,ALL,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))



#----------------------------------------------------
# Scenario 8 - jms_mqtt_004
#----------------------------------------------------
xml[${n}]="jms_mqtt_004"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_004 MQTT to/from JMS - Test 4 With Unicode topic structure. [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
component[3]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_mqtt_005
#----------------------------------------------------
xml[${n}]="jms_mqtt_005"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_005 MQTT to/from JMS - Test 5 [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
component[4]=runCommand,m1,../common/serverRestart.sh,1,jms_mqtt_005
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_mqtt_006
#----------------------------------------------------
xml[${n}]="jms_mqtt_006"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_006 MQTT to JMS - Test 6, ConvertMessageType=bytes .[ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms/mqtt_DS_003
#----------------------------------------------------
xml[${n}]="jms_mqtt_DS_003"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test 6.1 - Durable Subscription (JMS to MQTT), messages sent while subscriber is inactive are received when it becomes active. (Two hosts)"
component[1]=sync,m1
component[2]=wsDriver,m2,${xml[${n}]}mqtt.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m1,${xml[${n}]}jms.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms/mqtt_DS_004
# 
# Special note:  If the server and client clocks are not sync'd, 
# this test will fail. 
#----------------------------------------------------
xml[${n}]="jms_mqtt_DS_004"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Test 6.2 - Durable Subscription (JMS to MQTT), message expiration and messages sent while disconnected are received when reconnected. (Two hosts)"
component[1]=sync,m1
component[2]=wsDriver,m2,${xml[${n}]}mqtt.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m1,${xml[${n}]}jms.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms/mqtt_DS_003
#----------------------------------------------------
xml[${n}]="jms_mqtt_DS_003"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test 7.1 - Durable Subscription (MQTT to JMS), messages sent while subscriber is inactive are received when it becomes active. (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,${xml[${n}]}jms.xml,rmdr,-o_-l_9
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms/mqtt_DS_004
#----------------------------------------------------
xml[${n}]="jms_mqtt_DS_004"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test 7.2 - Durable Subscription (MQTT to JMS), messages sent while disconnected are received when reconnected. (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,${xml[${n}]}jms.xml,rmdr,-o_-l_9
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario - mqtt_clearRetained
#     Some of the prior tests created retained messages. 
#     This simple MQTT xml will remove those before
#     we do any Wildcard testing which would find them and fail., 
#----------------------------------------------------
xml[${n}]="mqtt_clearRetained"
timeouts[${n}]=60
scenario[${n}]="Clear any retained messages before running JMS Wildcard tests [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_mqtt_disconnect
#----------------------------------------------------
xml[${n}]="jms_mqtt_disconnect"
timeouts[${n}]=60
scenario[${n}]="jms_mqtt_disconnect JMS and MQTT - Test client disconnect on disable endpoint. [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m1,${xml[${n}]}jms.xml,ALL
component[3]=wsDriver,m1,${xml[${n}]}mqtt.xml,ALL
##components[${n}]="${component[2]}"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

