#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#		03/05/2012 - LAW - Initial Creation
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM MQTT via WSTestDriver"
scenario_directory="subscribe"
scenario_file="ism-proxy_td-subscribeScenarios01.sh"
scenario_at=" "+${scenario_directory}+"/"+${scenario_file}

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
# Test Case 0 - start_proxy
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="start_proxy"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port and delete retained messages in proxy_tests subscribe bucket"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./startProxy.sh"
# and removed any leftover RETAINed messages
component[2]=wsDriver,m1,../mqtt_td_tests/misc/deleteAllRetained0.xml,ALL,-o_-l_7
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - testproxy_subscribe01
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe01"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to '#'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - testproxy_subscribe02
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe02"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - testproxy_subscribe03
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe03"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/#'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - testproxy_subscribe04
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe04"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/+/c'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 5 - testproxy_subscribe05
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe05"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 50 level topic"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 6 - testproxy_subscribe06
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe06"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=1"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 7 - testproxy_subscribe07
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
# Won't work until subscribers requesting QoS 0 get all messages returned QoS 0, or until QoS 1 is supported
xml[${n}]="testproxy_subscribe07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 8 - testproxy_subscribe08
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe08"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' and 'a/b/d'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 9 - testproxy_subscribe09
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe09"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=1 and 'a/b/d' with QoS=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 10 - testproxy_subscribe10
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe10"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' and 'a/b/+', make sure a/b/c messages are not delivered twice"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 11 - testproxy_subscribe11
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe11"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' QoS=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 12 - testproxy_subscribe12 - CLIENT NO LONGER ALLOWS QOS=3 SUBSCRIBE
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#xml[${n}]="testproxy_subscribe12"
#scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' QoS=3"
#timeouts[${n}]=60
# Set up the components for the test in the order they should start
#component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
#components[${n}]="${component[1]} "

#((n+=1))

#----------------------------------------------------
# Test Case 13 - testproxy_subscribe13
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe13"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' QoS=0 publish QoS=1 and Qos=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 14 - testproxy_subscribe14
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe14"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to long topic (<32767 characters)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 15 - testproxy_subscribe15
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe15"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to long topic (<32767 characters)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 16 - testproxy_subscribe16
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe16"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe send 1600 messages on each of two topics"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 17 - testproxy_subscribe17
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe17"
scenario[${n}]="${xml[${n}]} - Test MQTT 1)Subscribe to same specific topic twice, should only receive messages once"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 20 - testproxy_subscribe20
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe20"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe to +"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 21 - testproxy_subscribe21
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe21"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe, close, subscribe"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,subscribe/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,subscribe/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 22 - testproxy_subscribe22
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe22"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe to /a/c/+"
timeouts[${n}]=40
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 23 - testproxy_subscribe23
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_subscribe23"
scenario[${n}]="${xml[${n}]} - Test publish and receive with 0 length message"
timeouts[${n}]=40
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case cleanup
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup"
scenario[${n}]="${xml[${n}]} - Cleanup ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} "

((n+=1))
