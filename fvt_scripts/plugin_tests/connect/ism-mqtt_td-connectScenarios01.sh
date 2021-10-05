#! /bin/bash

#--------------------------------------------------------------
#  This script defines the scenarios for the JSON Plugin tests.
#--------------------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM JSON Plugin connect"

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

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="mqtt_AdminDynamic_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect01"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|connect/policy_config.cli"
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect03"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect valid user/password"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect04"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect invalid user/password"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 6 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage, publish close normally"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 7 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect08"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage and Qos=1"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,plugin_testmqtt_connect08
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 8 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect09"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage and Qos=2"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive
component[4]=killComponent,m1,2,1,plugin_testmqtt_connect09
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 9 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect10"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, message and RETAIN"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,plugin_testmqtt_connect10
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 10 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect11"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with same clientId as already connected client"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,Connect1
component[3]=wsDriver,m2,connect/${xml[${n}]}.xml,Connect2
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 11 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect12"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, message (zero length)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,plugin_testmqtt_connect12
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 12 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect13"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with cleanSession=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,Receive1,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,Receive2,-o_-l_9
component[4]=wsDriver,m1,connect/${xml[${n}]}.xml,Transmit,-o_-l_9
component[5]=killComponent,m1,2,1,plugin_testmqtt_connect13
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "

((n+=1))

#----------------------------------------------------
# Test Case 13 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect14"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, connect second client same clientId"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=wsDriver,m2,connect/${xml[${n}]}.xml,publish2,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 14 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect15"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect will topic longer than 60 characters"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,plugin_testmqtt_connect15
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))

#----------------------------------------------------
# Test Case 15 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect16"
scenario[${n}]="${xml[${n}]} - Test MQTT connect without cleanSession, receive stored msg"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 16 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_connect17"
scenario[${n}]="${xml[${n}]} - Test MQTT connect with cleanSession, don't receive stored msg"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 20 - testmqtt_connect21
#----------------------------------------------------
xml[${n}]="testmqtt_connect21"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test that message in store are available after server fail/restart [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,plugin_testmqtt_connect21
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 21 - testmqtt_connect22
#----------------------------------------------------
xml[${n}]="testmqtt_connect22"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test that RETAINed messages are available after server fail/restart [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,plugin_testmqtt_connect22
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 22 - testmqtt_connect23
#----------------------------------------------------
xml[${n}]="testmqtt_connect23"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - More RETAINed messages tests [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,plugin_testmqtt_connect23
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|connect/policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/rmScratch.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1  
    #----------------------------------------------------
    xml[${n}]="mqtt_AdminDynamic_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
