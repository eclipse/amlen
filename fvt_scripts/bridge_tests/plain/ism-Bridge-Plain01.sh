#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MessageGateway and MQTTBridge"
scenario_directory="bridge_tests/plain"
scenario_file="ism-Bridge-Plain01.sh"
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
# Test Case  - Setup Simple Bridge MA_A1 to MS_A2 Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_Simple"
scenario[${n}]="${xml[${n}]} - Configure Simple Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Configure policies
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|1"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|3"
#Clean any retained before starting
component[4]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL
# Start Bridge
component[5]=cAppDriverLogWait,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.A1A2.cfg"
# Configure policies for Bridge
component[6]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "

((n+=1))

#----------------------------------------------------------
# Test Case  - Simple MS to WIOTP 
#   (MUST CLEAN RETAIN BEFORE NEXT TC)
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.SimpleA1A2"
scenario[${n}]="${xml[${n}]} - Test Simple Pub-Fwd-Recv through Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,A2Subscriber
component[3]=wsDriver,m1,plain/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.MultiFWD.cfg"
component[3]=wsDriver,m2,mqtt_clearRetainedA1A2A3.xml,ALL

components[${n}]="${component[1]}  ${component[3]} "

((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.MultiFWD"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v3Subscriber
component[3]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v5Subscriber
component[4]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v3Subscriber
component[5]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v5Subscriber
component[6]=wsDriver,m1,plain/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,plain/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

((n+=1))




#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder V3 Source Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD_V3Source"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder with  V3 Source Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.MultiFWD_V3Source.cfg"

components[${n}]="${component[1]}  "

((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder V3 Source MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.MultiFWD_V3Source"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders with V3 Source Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v3Subscriber
component[3]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v5Subscriber
component[4]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v3Subscriber
component[5]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v5Subscriber
component[6]=wsDriver,m1,plain/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,plain/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

((n+=1))




#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder V3 Destination Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD_V3Destination"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder V3 Destination Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.MultiFWD_V3Destination.cfg"

components[${n}]="${component[1]}  "

((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder V3 Destinatino MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.MultiFWD_V3Destination"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders V3 Destination Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v3Subscriber
component[3]=wsDriver,m2,plain/${xml[${n}]}.xml,v3v5Subscriber
component[4]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v3Subscriber
component[5]=wsDriver,m2,plain/${xml[${n}]}.xml,v5v5Subscriber
component[6]=wsDriver,m1,plain/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,plain/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder LoadBalance with Instances Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_LoadBalance"
scenario[${n}]="${xml[${n}]} - Configure Load Balance with Instances Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.FWDLoadBalance.cfg"

components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------------
# Test Case  -  Forwarder LoadBalance with Instances MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.FwdLoadBalance"
scenario[${n}]="${xml[${n}]} - Test Load Balance with Instances Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,A2Subscriber
component[3]=wsDriver,m1,plain/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder FanIn Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_FanIn"
scenario[${n}]="${xml[${n}]} - Configure FanIn Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.FanIn.cfg"

components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------------
# Test Case  - FanIn multi_MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.FanIn"
scenario[${n}]="${xml[${n}]} - Test FanIn Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,A2Subscriber
component[3]=wsDriver,m1,plain/${xml[${n}]}.xml,A1Publisher
component[4]=wsDriver,m1,plain/${xml[${n}]}.xml,A2Publisher
component[5]=wsDriver,m1,plain/${xml[${n}]}.xml,A3Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]}  ${component[4]}  ${component[5]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder FanOut Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_FanOut"
scenario[${n}]="${xml[${n}]} - Configure FanOut Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.FanOut.cfg"

components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------------
# Test Case  - Forwarder MS FanOut to multi_WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.FanOut"
scenario[${n}]="${xml[${n}]} - Test FanOut Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,A1Subscriber
component[3]=wsDriver,m2,plain/${xml[${n}]}.xml,A2Subscriber
component[4]=wsDriver,m2,plain/${xml[${n}]}.xml,A3Subscriber
component[5]=wsDriver,m1,plain/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]}  ${component[5]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - Setup MaxTopic Forwarder Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MaxFWDTopics"
scenario[${n}]="${xml[${n}]} - Configure Max Forwarder Topics Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/Bridge.MaxFWDTopics.cfg"

components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------------
# Test Case  - Max Topics Forwarder MS to WIOTP 
#   (MUST CLEAN RETAIN BEFORE NEXT TC)
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridge.MaxFWDTopics"
scenario[${n}]="${xml[${n}]} - Test Max Forwarder Topics Bridge ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,plain/${xml[${n}]}.xml,A2Subscriber
component[3]=wsDriver,m1,plain/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]}  "

((n+=1))

#----------------------------------------------------
# Test Case  - Cleanup and Clear old RETAINed Msgs 
#----------------------------------------------------
xml[${n}]="Clean_up"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Clear UP after ${scenario_at}"
# Clear retained
component[1]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL
# deConfigure policies
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|1"
component[3]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|2"
component[4]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
