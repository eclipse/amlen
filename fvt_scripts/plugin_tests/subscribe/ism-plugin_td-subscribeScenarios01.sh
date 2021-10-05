
#--------------------------------------------------------------
#  This script defines the scenarios for the JSON Plugin tests.
#--------------------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM JSON Plugin subscribe"

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
# Test Case 0 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe01"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to '#'"
timeouts[${n}]=140
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_8
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe02"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe03"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/#'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe04"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/+/c'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe05"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 50 level topic"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 5 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe06"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=1"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 6 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
# Won't work until subscribers requesting QoS 0 get all messages returned QoS 0, or until QoS 1 is supported
xml[${n}]="testplugin_subscribe07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 7 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe08"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' and 'a/b/d'"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 8 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe09"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' with QoS=1 and 'a/b/d' with QoS=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 9 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe10"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' and 'a/b/+', make sure a/b/c messages are not delivered twice"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 10 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe11"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' QoS=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 12 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe13"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to 'a/b/c' QoS=0 publish QoS=1 and Qos=2"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 13 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe14"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to long topic (<32767 characters)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 14 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe15"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe to long topic (<32767 characters)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace9|-c|subscribe/policy_config.cli"
component[2]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Test Case 15 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe16"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket subscribe send 1600 messages on each of two topics"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace5|-c|subscribe/policy_config.cli"
component[2]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Test Case 16 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe17"
scenario[${n}]="${xml[${n}]} - Test MQTT 1)Subscribe to same specific topic twice, should only receive messages once"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 19 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe20"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe to +"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 20 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe21"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe, close, subscribe"
timeouts[${n}]=420
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,subscribe/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,subscribe/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 21 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe22"
scenario[${n}]="${xml[${n}]} - Test MQTT test subscribe to /a/c/+"
timeouts[${n}]=40
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 22 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testplugin_subscribe23"
scenario[${n}]="${xml[${n}]} - Test publish and receive with 0 length message"
timeouts[${n}]=40
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,subscribe/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

