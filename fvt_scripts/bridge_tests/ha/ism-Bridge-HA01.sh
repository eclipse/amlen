#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="HA-MessageGateway with HA-MQTTBridge"
scenario_directory="bridge_tests/ha"
scenario_file="ism-Bridge-HA01.sh"
scenario_at=" ${scenario_directory}/${scenario_file}"

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







#----------------------------------------------------------
# Test Case  - Setup HA Bridge MS_A1A2 to MS_A3 Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup1_HA"
scenario[${n}]="${xml[${n}]} - Configure A1,A2,A3 Servers for HA Bridge Config ${scenario_at}"
timeouts[${n}]=30
# Configure policies
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|1"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|2"
component[3]=cAppDriverLogWait,m2,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|3"
#Clean any retained before starting
component[4]=wsDriverWait,m1,mqtt_clearRetainedA1A2A3.xml,ALL
# Start Bridge
component[5]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.HA_Px1.cfg"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.HA_Px2.cfg|-s|B2"
# Configure policies
component[7]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"
# Configure policies for Bridge
component[8]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|2|-u|${B2_REST_USER}:${B2_REST_PW}"

#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} "
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}  "

((n+=1))


#----------------------------------------------------
# Test Case - HA Setup
#----------------------------------------------------
xml[${n}]="HASetup_HA"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Configure HA - enableHA A1,A2 ${scenario_at}"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA"

components[${n}]="${component[1]}"
((n+=1))




#----------------------------------------------------------
# Test Case  - Setup HA Bridge MS_A1A2 to MS_A3 Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup2_HA"
scenario[${n}]="${xml[${n}]} - Configure B1,B2 Bridges for HA Bridge Config ${scenario_at}"
timeouts[${n}]=90
# Start Bridge
component[5]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.HA_Px1.cfg"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.HA_Px2.cfg|-s|B2"
# Configure policies
component[7]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"
# Configure policies for Bridge
component[8]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|2|-u|${B2_REST_USER}:${B2_REST_PW}"

components[${n}]=" ${component[5]} ${component[6]} ${component[7]} ${component[8]} "

((n+=1))



#----------------------------------------------------------
# Test Case  - HA Bridge/MS to WIOTP 
#   
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.HAA1A2"
scenario[${n}]="${xml[${n}]} - Test Failover of A1 Server and B1 Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,ha/${xml[${n}]}.xml,A3Subscriber
component[3]=wsDriver,m1,ha/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} "

((n+=1))


#----------------------------------------------------
# Test Case - HA DISABLE
#----------------------------------------------------
xml[${n}]="HADisable_HA"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - DE-Configure HA - disableHA A1,A2 ${scenario_at}"
component[1]=sync,m1
# Start Bridge without any Forwarders or Endpoints
component[2]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.Clean.cfg"
component[3]=cAppDriverLogWait,m2,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.Clean.cfg|-s|B2"
# Disable HA
component[4]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA"
# Defect workaround to clean up
component[5]=wsDriver,m1,ha/bridge.HAA1A2.xml,SanitySubscriptionClean

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]}  ${component[5]} "

((n+=1))



#----------------------------------------------------
# Test Case  - Cleanup and Clear old RETAINed Msgs 
#----------------------------------------------------
xml[${n}]="Clean_up"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Clear UP after @{scenario_at}"
# Clear retained
component[1]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL
# deConfigure policies
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|1"
component[3]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|2"
component[4]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|3"
component[5]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"
component[6]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_B.cli|-b|2|-u|${B2_REST_USER}:${B2_REST_PW}"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}  "
