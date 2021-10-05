#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTTV5 Properties through Proxy"

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
    xml[${n}]="proxy_tenants-iot2_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - set policies for the mqttv5 Proxy Properties"
# Start Proxy with MQTTv5 Config
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxy-iot2.cfg|.|proxy-iot2.acl"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} "

((n+=1))


#----------------------------------------------------
# Test Case  - proxymqttV5_sharedSub_SubId
#----------------------------------------------------
xml[${n}]="proxymqttV5_sharedSub_SubId"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test @SharedSubscriptions (w-w/o WCs) with SubIDs"
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,SubID_WC
component[3]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,SubID1
component[4]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,SubID2
component[5]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,Publisher
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "

((n+=1))



#----------------------------------------------------------
# Test Case 1 - MQTTv3 PUB upgrade v3 to V5 Pub
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxymqttV5_publishV3_PubUpV5"
scenario[${n}]="${xml[${n}]} - MQTTv3 PUB upgrade v3 to V5 Pub"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,receiveV3
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------------
# Test Case 1 - MQTTv3 SUB upgrade v3 to V5 Sub
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxymqttV5_publishV3_SubUpV5"
scenario[${n}]="${xml[${n}]} - MQTTv3 SUB upgrade v3 to V5 Sub"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,mqttv5_sharedsub/${xml[${n}]}.xml,receiveV3
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup and gather proxy logs.  
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup policies for v5 connect bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/rmScratch.sh"
component[3]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} ${component[2]} ${component[3]}  "
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="proxy_tenants-iot2_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

