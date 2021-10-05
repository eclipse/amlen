#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTTV5 Reason Codes"
##===================================================================================##
## Test notes:  Intent is to have all REASON CODE tested here, exceptions are
#
#   0,1,2 - SUBACK QoS 
#     4 - Disconnect with Will Message -- This is a Client to Server ONLY path.
#   142 - is tested in mqttv5_cleanstart/ client steal test cases
#
##===================================================================================##

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
# Test Case 0 - mqtt_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - set policies for the mqttv5 reasoncodes bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupv5|-c|mqttv5_cleanstart/policy_mqttv5.cli"
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 MaxPacketSz exceeded with ReasonCode 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_MPSwRC"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 MaxPacketSz exceeded with ReasonCode"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,connackMPSNoRC,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc016 No matching Subscribers
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc016"
scenario[${n}]="${xml[${n}]} - Test rc016 No matching Subscribers"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc016,-o_-l_9
component[2]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,WSrc016,-o_-l_9
components[${n}]="${component[1]} ${component[2]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc017 Unsubscribe without Subscribe
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc017"
scenario[${n}]="${xml[${n}]} - Test rc017 No Subscription Existed"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc017,-o_-l_9
# Not Be Supported/Fixed #component[2]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,WSrc017,-o_-l_9
components[${n}]="${component[1]}  "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc130 Protocol Error
# Removed since we were getting a race condition and the
# same code is tested in Proxy, where things are more reliable
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
#xml[${n}]="testmqttV5_rc130"
#scenario[${n}]="${xml[${n}]} - Test rc130 Protocol Error"
#timeouts[${n}]=180
# Set up the components for the test in the order they should start
#component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc130
#components[${n}]="${component[1]} "

#((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc133 Client Identified not valid
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc133"
scenario[${n}]="${xml[${n}]} - Test rc133 Client Identified not valid"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
#component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc133,-o_-l_9   PAHO CLIENT is having GVT ISSUES
component[2]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,WSrc133,-o_-l_9
#components[${n}]="${component[1]} ${component[2]} "
components[${n}]="                 ${component[2]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc135 Not Authorized
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc135"
scenario[${n}]="${xml[${n}]} - Test rc135 Not Authorized"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc135connack,-o_-l_9
component[2]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc135puback,-o_-l_9
component[3]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc135pubrec,-o_-l_9
component[4]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc135suback,-o_-l_9
component[5]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc135disconn,-o_-l_9
components[${n}]=" ${component[1]}  ${component[2]}  ${component[3]}  ${component[4]} ${component[5]}  "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc136 Server Unavailable
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc136"
scenario[${n}]="${xml[${n}]} - Test rc136 Server Unavailable"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc136,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc139 Server Stopping
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc139"
scenario[${n}]="${xml[${n}]} - Test rc139 Server Stopping"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc139,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc141 KeepAlive Timeout
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc141"
scenario[${n}]="${xml[${n}]} - Test rc139 Server Stopping"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc141,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc143 TopicFilter Subscribe
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc143"
scenario[${n}]="${xml[${n}]} - Test rc143 TopicFilter"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc143TopicFilter,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV3_rc143"
scenario[${n}]="${xml[${n}]} - Test rc143 TopicFilter"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc143TopicFilter,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


# The prefix of the XML configuration file for the driver
xml[${n}]="testWSmqttV5_rc143"
scenario[${n}]="${xml[${n}]} - Test rc143 TopicFilter"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc143TopicFilter,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 rc144 TopicFilter Publish
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqttV5_rc144"
scenario[${n}]="${xml[${n}]} - Test rc144 TopicFilter"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc144puback
component[2]=wsDriver,m1,mqttv5_reasoncodes/${xml[${n}]}.xml,rc144pubrec
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup and gather proxy logs.  
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup policies for v5 reasoncodes bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupv5|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupv5exp|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[3]=cAppDriver,m1,"-e|../common/rmScratch.sh"
components[${n}]="${component[1]}                 ${component[3]} "
#components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))
