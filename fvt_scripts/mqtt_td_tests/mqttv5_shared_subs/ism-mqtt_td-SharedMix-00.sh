#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT SharedMix00 via WSTestDriver"

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

# if [[ "$A1_LDAP" == "FALSE" ]] ; then
#     #----------------------------------------------------
#     # Enable for LDAP on M1 and initilize  
#     #----------------------------------------------------
#     xml[${n}]="mqtt_sharedsub_00_M1_LDAP_setup"
#     scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
#     timeouts[${n}]=40
#     component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
#     component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
#     components[${n}]="${component[1]} ${component[2]}"
#     ((n+=1))
# fi

#----------------------------------------------------
# Test Case 0 - mqtt_sharedsub_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_sharedMix_00_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - set policies for the Mixed Durability SharedSubscriptions bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupSM|-c|mqttv5_shared_subs/policy_sharedmix.cli"
components[${n}]="${component[1]}"
((n+=1))

# #----------------------------------------------------
# Test Case 1 - testmqtt_sharedMix_csF_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_csF_error01"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test basic Errors in or MQTTv5 shared subs using a durable subscriber. "
component[1]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - testmqtt_sharedMix_csF_error02
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_csF_error02"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test connect, disconnect, reconnect, and ClientID theft conditions for MQTTv5 shared subs using a durable subscriber. "
component[1]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case  - testmqtt_sharedMix_csT_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_csT_error01"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test basic Errors in or MQTTv5 shared subs using a non-durable subscriber. "
component[1]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case  - testmqtt_sharedMix_csT_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_csT_error02"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test connect, disconnect,and ClientID theft conditions for MQTTv5 shared subs using a non-durable subscriber. "
component[1]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case  - testmqtt_sharedMix_csT_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_error03"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test creation and distruction of $share subscriptions using cleansession clients in all different orders "
component[1]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 3 - testmqtt_sharedMix_01 - different subs on different topics with same subname
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_01"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - SharedMix - check different subs on different topics with same subname "
component[1]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - testmqtt_sharedMix_02 - Basic subscribing and messaging
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_02"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Basic test of mixed-durability sharedsubs - subscribing and messaging. "
component[1]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
component[3]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_02.xml-Subscribes-M1.log|-t|500|-c|CF1|CF2|-v|2"
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case 3 - testmqtt_sharedMix_03 - Messaging with restart
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_03"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - Clients reconnecting and receiving after server restart. "
component[1]=sync,m1
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Publisher
component[4]=wsDriverWait,m2,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
component[5]=sleep,5
component[6]=cAppDriverLogWait,m2,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_03.xml-Subscribes-M2.log|-t|2500|-c|CF1|CF2|-v|2|-ge|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Test Case 4 - testmqtt_sharedMix_04 - Messaging with restart
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_04"
timeouts[${n}]=280
scenario[${n}]="${xml[${n}]} - Clients reconnecting and receiving after server restart - bigger test with multiple subs and more messages. "
component[1]=sync,m1
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Publisher
component[4]=wsDriverWait,m2,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
component[5]=sleep,5
component[6]=cAppDriverLogWait,m2,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_04.xml-Subscribes-M2.log|-t|8000|-c|CF1|CF2|CF3|CF4|CF5|-v|2|-ge|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Test Case 5 - testmqtt_sharedMix_05 - Client Disconnect
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_05"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - Redelivery of in-flight msg on client disconnect. "
component[1]=sync,m1
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Publisher
component[4]=wsDriverWait,m2,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
component[5]=sleep,5
component[6]=cAppDriverLogWait,m2,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_05.xml-Subscribes-M2.log|-t|2500|-c|CF1|CF2|-v|2|-ge|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Test Case 6 - testmqtt_sharedMix_06 - Wildcard subscriptions
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_06"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - Wildcard subscriptions for share subs. "
component[1]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Subscribes
component[3]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_06.xml-Subscribes-M1.log|-t|600|-c|CF1|CF2|-v|2"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_06.xml-Subscribes-M1.log|-t|1200|-c|CF3|CF4|-v|2"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/sumReceived.py,-o|-l|testmqtt_sharedMix_06.xml-Subscribes-M1.log|-t|600|-c|CF5|CF6|-v|2"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case 7 - testmqtt_sharedMix_07ND - Busy test with logs of subscribes and unsubscribes, nondurable
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_07ND"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - Busy share test with many subscribes-unsubscribes, nondurable. "
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,BusySubs
component[3]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Pubs
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 7 - testmqtt_sharedMix_07D - Busy test with logs of subscribes and unsubscribes, durable
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_07D"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - Busy share test with many subscribes-unsubscribes, nondurable. "
component[1]=sync,m1
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,BusySubs
component[4]=wsDriver,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Pubs
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 9 - testmqtt_sharedMix_09 - New Retained Messages behavior (11-21-16)
#----------------------------------------------------
xml[${n}]="testmqtt_sharedMix_09"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - New Retained Messages behavior - shared durable should not receive RMs when create sub or on reconnect."
component[1]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,CleanSession
component[2]=wsDriverWait,m1,mqttv5_shared_subs/${xml[${n}]}.xml,Test
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_sharedMix_00_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup policies for sharedMix bucket"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupSM|-c|mqttv5_shared_subs/policy_sharedmix.cli"
components[${n}]="${component[1]}"
((n+=1))

# if [[ "$A1_LDAP" == "FALSE" ]] ; then
#     #----------------------------------------------------
#     # Cleanup and disable LDAP on M1 
#     #----------------------------------------------------
#     xml[${n}]="mqtt_sharedsub_00_M1_LDAP_cleanup"
#     scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
#     timeouts[${n}]=10
#     component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
#     component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
#     components[${n}]="${component[1]} ${component[2]}"
#     ((n+=1))
# fi

