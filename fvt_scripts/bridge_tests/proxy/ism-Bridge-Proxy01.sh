#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MessageGateway and MQTTBridge with Proxy"
scenario_directory="bridge_tests/proxy"
scenario_file="ism-Bridge-Proxy01.sh"
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
xml[${n}]="BridgePxSetup_Simple"
scenario[${n}]="${xml[${n}]} - Configure Simple Bridge Config ${scenario_at}"
timeouts[${n}]=180
# Start Proxies
component[1]=cAppDriver,m1,"-e|../proxy_tests/startProxy.sh","-o|../common/BridgePx.P1_Proxy-iot2.cfg|.|proxy/BridgePx.P1_Proxy-iot2.acl"
component[2]=cAppDriver,m2,"-e|../proxy_tests/startProxy.sh","-o|../common/BridgePx.P2_Proxy-iot2.cfg|.|proxy/BridgePx.P2_Proxy-iot2.acl|P2"
# Configure policies that can be done now to kill time and keep the sleep before the startBridge small
component[3]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|1"
component[4]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|2"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_A.cli|-a|3"
#Clean any retained before starting (and give time for Proxy to connect to MGtwy Srv)
component[6]=wsDriverWait,m1,mqtt_clearRetainedA1A2A3.xml,ALL
# Start Bridge
component[7]=sleep,5
component[8]=cAppDriverLogWait,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.P1P2.cfg"
# Configure policies for Bridge
component[9]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"

# CAUTION ON ORDER:  Proxies need time to connect to MessageGateway Server before starting the Bridge or they are not ready for connections.
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} "

###  TEMP  ###  
((n+=1))

#----------------------------------------------------------
# Test Case  - Simple MS to WIOTP 
#   (MUST CLEAN RETAIN BEFORE NEXT TC)
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.SimpleA1A2"
scenario[${n}]="${xml[${n}]} - Test Simple Pub-Fwd-Recv through Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,proxy/${xml[${n}]}.xml,P1Publisher
component[3]=wsDriverWait,m2,proxy/${xml[${n}]}.xml,P2Subscriber
#  ,-o_-l_9  add this to the end for detailed client logging

components[${n}]="${component[1]} ${component[2]}  ${component[3]} "

###  TEMP  ###  
((n+=1))



#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.MultiFWD.cfg"
component[3]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL

components[${n}]="${component[1]}  ${component[3]} "

###  TEMP  ###  ((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.MultiFWD"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v3Subscriber,-o_-l_9
component[3]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v5Subscriber,-o_-l_9
component[4]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v3Subscriber,-o_-l_9
component[5]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v5Subscriber,-o_-l_9
component[6]=wsDriver,m1,proxy/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,proxy/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

###  TEMP  ###  ((n+=1))




#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder V3 Source Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD_V3Source"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder with  V3 Source Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.MultiFWD_V3Source.cfg"
component[3]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL

components[${n}]="${component[1]}  ${component[3]} "

###  TEMP  ###  
((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder V3 Source MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.MultiFWD_V3Source"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders with V3 Source Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v3Subscriber
component[3]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v5Subscriber
component[4]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v3Subscriber
component[5]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v5Subscriber
component[6]=wsDriver,m1,proxy/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,proxy/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

###  TEMP  ###  
((n+=1))




#----------------------------------------------------------
# Test Case  - Setup Multi_Forwarder V3 Destination Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_MultiFWD_V3Destination"
scenario[${n}]="${xml[${n}]} - Configure Multiple Forwarder V3 Destination Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.MultiFWD_V3Destination.cfg"
component[3]=wsDriverWait,m2,mqtt_clearRetainedA1A2A3.xml,ALL

components[${n}]="${component[1]}  ${component[3]} "

###  TEMP  ###  ((n+=1))

#----------------------------------------------------------
# Test Case  - Multi_Forwarder V3 Destinatino MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.MultiFWD_V3Destination"
scenario[${n}]="${xml[${n}]} - Test Multile Forwarders V3 Destination Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v3Subscriber,-o_-l_9
component[3]=wsDriver,m2,proxy/${xml[${n}]}.xml,v3v5Subscriber,-o_-l_9
component[4]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v3Subscriber,-o_-l_9
component[5]=wsDriver,m2,proxy/${xml[${n}]}.xml,v5v5Subscriber,-o_-l_9
component[6]=wsDriver,m1,proxy/${xml[${n}]}.xml,v3Publisher
component[7]=wsDriver,m1,proxy/${xml[${n}]}.xml,v5Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[7]} "

###  TEMP  ###  ((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder LoadBalance with Instances Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgePxSetup_LoadBalance"
scenario[${n}]="${xml[${n}]} - Configure Load Balance with Instances Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.FWDLoadBalance.cfg"

components[${n}]="${component[1]} "

###  TEMP  ###  ((n+=1))

#----------------------------------------------------------
# Test Case  -  Forwarder LoadBalance with Instances MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.FwdLoadBalance"
scenario[${n}]="${xml[${n}]} - Test Load Balance with Instances Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,A2Subscriber,-o_-l_9
component[3]=wsDriver,m1,proxy/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} "

###  TEMP  ###  ((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder FanIn Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgePxSetup_FanIn"
scenario[${n}]="${xml[${n}]} - Configure FanIn Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.FanIn.cfg"

components[${n}]="${component[1]} "

###  TEMP  ###  ((n+=1))

#----------------------------------------------------------
# Test Case  - FanIn multi_MS to WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.FanIn"
scenario[${n}]="${xml[${n}]} - Test FanIn Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,A2Subscriber,-o_-l_9
component[3]=wsDriver,m1,proxy/${xml[${n}]}.xml,A1Publisher
component[4]=wsDriver,m1,proxy/${xml[${n}]}.xml,A2Publisher
component[5]=wsDriver,m1,proxy/${xml[${n}]}.xml,A3Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]}  ${component[4]}  ${component[5]} "

###  TEMP  ###  ((n+=1))


#----------------------------------------------------------
# Test Case  - Setup Forwarder FanOut Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgePxSetup_FanOut"
scenario[${n}]="${xml[${n}]} - Configure FanOut Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.FanOut.cfg"

components[${n}]="${component[1]} "

###  TEMP  ###  ((n+=1))

#----------------------------------------------------------
# Test Case  - Forwarder MS FanOut to multi_WIOTP 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.FanOut"
scenario[${n}]="${xml[${n}]} - Test FanOut Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m2,proxy/${xml[${n}]}.xml,A1Subscriber,-o_-l_9
component[3]=wsDriver,m2,proxy/${xml[${n}]}.xml,A2Subscriber,-o_-l_9
component[4]=wsDriver,m2,proxy/${xml[${n}]}.xml,A3Subscriber,-o_-l_9
component[5]=wsDriver,m1,proxy/${xml[${n}]}.xml,A1Publisher

components[${n}]="${component[1]} ${component[2]}  ${component[3]} ${component[4]}  ${component[5]} "

###  TEMP  ###  ((n+=1))


#----------------------------------------------------------
# Test Case  - Setup MaxTopic Forwarder Bridge Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgePxSetup_MaxFWDTopics"
scenario[${n}]="${xml[${n}]} - Configure Max Forwarder Topics Bridge Config  ${scenario_at}"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriverLog,m1,"-e|../scripts/startMQTTBridge.sh","-o|-c|../common/BridgePx.MaxFWDTopics.cfg"

components[${n}]="${component[1]} "

###  TEMP  ###  
((n+=1))

#----------------------------------------------------------
# Test Case  - Max Topics Forwarder MS to WIOTP 
#   (MUST CLEAN RETAIN BEFORE NEXT TC)
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="bridgePx.MaxFWDTopics"
scenario[${n}]="${xml[${n}]} - Test Max Forwarder Topics Bridge  ${scenario_at}"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,proxy/${xml[${n}]}.xml,A1Publisher
component[3]=wsDriverWait,m2,proxy/${xml[${n}]}.xml,A2Subscriber
component[4]=wsDriver,m1,proxy/${xml[${n}]}.xml,A1PublisherCleanRetain

components[${n}]="${component[1]} ${component[2]}  ${component[3]}  ${component[4]} "

###  TEMP  ###  
((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup  
#----------------------------------------------------
xml[${n}]="CleanUpBridgePx"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup after ${scenario_at}"
component[1]=cAppDriver,m1,"-e|../common/rmScratch.sh"
component[2]=cAppDriver,m1,"-e|../proxy_tests/killProxy.sh"
component[3]=cAppDriver,m2,"-e|../proxy_tests/killProxy.sh","-o|P2"
# Clear retained
component[4]=wsDriver,m2,mqtt_clearRetainedA1A2A3.xml,ALL
# deConfigure policies
component[5]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|1"
component[6]=cAppDriverLog,m2,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_A.cli|-a|2"
component[7]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|bridge_B.cli|-b|1|-u|${B1_REST_USER}:${B1_REST_PW}"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "
