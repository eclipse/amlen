#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#		03/05/2012 - LAW - Initial Creation
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTT Connect via WSTestDriver"

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
xml[${n}]="deleteAllRetained0"
scenario[${n}]="${xml[${n}]} - Delete any RETAINed messages, error if any exist"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="mqtt_connect_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 1 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect01"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|connect/policy_config.cli"
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} "
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_connectAD01
#
#  NOTE:  This must run at least 30 seconds before
#		mqtt_connectAD02 runs, because of ldap 
#		caching times for Groups.  
#
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connectAD01"
scenario[${n}]="${xml[${n}]} - Test MQTT Policies with AllowDurable parameter"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect03"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect valid user/password"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect04"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect invalid user/password"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case  - Assigned Client ID
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect_AssignCID"
scenario[${n}]="${xml[${n}]} - Test MQTTV5 Assigns clientID when zero length ID passed"
timeouts[${n}]=60
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive-NDS
component[4]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive-DS
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))


#----------------------------------------------------
# Test Case 7 - mqtt_maxpacketsize
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect_MPS"
scenario[${n}]="${xml[${n}]} - Test MQTTV5 that we don't exceed Maximum Packet Size specified by client"
timeouts[${n}]=60
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive-NDS
component[4]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive-DS
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 7 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage, publish close normally"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case 8 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect08"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage and Qos=1"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,testmqtt_v5connect08
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 9 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect09"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topicmessage and Qos=2"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive
component[4]=killComponent,m1,2,1,testmqtt_v5connect09
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 10 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect10"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, message and RETAIN"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,testmqtt_v5connect10
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 11 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect11"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with same clientId as already connected client"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,Connect1
component[3]=wsDriver,m2,mqttv5_connect/${xml[${n}]}.xml,Connect2
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case 12 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect12"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, message (zero length)"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,testmqtt_v5connect12
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 13 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect13"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with cleanSession=0"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,Receive1,-o_-l_9
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,Receive2,-o_-l_9
component[4]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,Transmit,-o_-l_9
component[5]=killComponent,m1,2,1,testmqtt_v5connect13
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "
((n+=1))

#----------------------------------------------------
# Test Case 14 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect14"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect with Will topic, connect second client same clientId"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=wsDriver,m2,mqttv5_connect/${xml[${n}]}.xml,publish2,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 15 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect15"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect will topic longer than 60 characters"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriverNocheck,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_9
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
component[4]=killComponent,m1,2,1,testmqtt_v5connect15
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 16 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect16"
scenario[${n}]="${xml[${n}]} - Test MQTT connect without cleanSession, receive stored msg"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case 17 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connect17"
scenario[${n}]="${xml[${n}]} - Test MQTT connect with cleanSession, don't receive stored msg"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))


#----------------------------------------------------
# Test Case 21 - testmqtt_connect21
#----------------------------------------------------
xml[${n}]="testmqtt_v5connect21"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test that message in store are available after server fail/restart [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,testmqtt_v5connect21
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 22 - testmqtt_connect22
#----------------------------------------------------
xml[${n}]="testmqtt_v5connect22"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test that RETAINed messages are available after server fail/restart [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,testmqtt_v5connect22
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 23 - testmqtt_connect23
#----------------------------------------------------
xml[${n}]="testmqtt_v5connect23"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - More RETAINed messages tests [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,receive,-o_-l_7,"-s|BITFLAG=-Djava.util.logging.config.file=./jsr47tofile.properties"
component[4]=runCommand,m1,../common/serverKillRestart.sh,1,testmqtt_v5connect23
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


#----------------------------------------------------
# Test Case  - mqtt_connectAD02
#
#  NOTE:   mqtt_connectAD01 must run at least 30 seconds before
#		mqtt_connectAD02 runs, because of ldap 
#		caching times for Groups.  
#
#  		mqtt_connectAD01 runs at the start of this 
#		test bucket, to assure that timing gap.
#
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connectAD02"
scenario[${n}]="${xml[${n}]} - Test MQTT ConnectionPolicies with AllowDurable parameter is applied. "
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_connectAD03
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_v5connectAD03"
scenario[${n}]="${xml[${n}]} - Test MQTT ConnectionPolicy with AllowDurable being updated on ConnectionPolicy."
timeouts[${n}]=60
component[1]=wsDriver,m1,mqttv5_connect/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="mqtt_connect_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup for the mqtt connect test bucket."
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|connect/policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="mqtt_connect_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
