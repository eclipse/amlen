#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="AAA Limited Gateway v1.0 Tests"
scenario_directory="AAAgateway"
scenario_file="ism-AAAgateway01.sh"
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


if [[ "$A1_LDAP" == "FALSE" ]] ; then
#?not here    ((n+=1))
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="proxyACL_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP ${scenario_at}"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
fi


((n+=1))
#----------------------------------------------------
# Test Case 0 - START Proxy
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="start_proxy"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|AAAgateway/gateway.cli|-a|1"
component[2]=cAppDriver,m1,"-e|./startProxy.sh","-o|../common/proxy.ACLfile.cfg|.|WIoTP_ACLfile.cfg"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"


((n+=1))
##----------------------------------------------------
## Test Case 1 - mqtt_ws1 sanity check as sample
##----------------------------------------------------
## The prefix of the XML configuration file for the driver
#xml[${n}]="testproxy_publish05"
#scenario[${n}]="${xml[${n}]} - Test RETAIN, simple scenario ${scenario-at}"
#timeouts[${n}]=60
## Set up the components for the test in the order they should start
#component[1]=sync,m1
#component[2]=wsDriver,m1,publish/${xml[${n}]}.xml,publish
#component[3]=wsDriver,m1,publish/${xml[${n}]}.xml,receive
#components[${n}]="${component[1]} ${component[2]} ${component[3]} "




((n+=1))
#----------------------------------------------------
# Test Case 1 - Proxy ACL - In Group Retain and WildCard Durable Subscription
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_InGroup"
scenario[${n}]="${xml[${n}]} - Test RETAIN, Durable WildCard"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
##
## HOW TO TRACE WSDriver
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org1:deviceType1:00123,-o_-l_9
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1,-o_-l_9
##
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org1:deviceType1:00123
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org2:appType1

component[5]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1
component[6]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org2:deviceTypeX:deviceTX-66

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[5]} ${component[6]} "



((n+=1))
#----------------------------------------------------
# Test Case 2 - Proxy ACL - Recovery MS Restart with active Proxy Pub.
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_Recovery"
scenario[${n}]="${xml[${n}]} - Test Recovery after Message Sight Restart ${scenario_at}"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
##
## HOW TO TRACE WSDriver
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org1:deviceType1:00123,-o_-l_9
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1,-o_-l_9
##
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org1:deviceType1:00123
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org2:appType1

component[5]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1
component[6]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,d:Org2:deviceTypeX:deviceTX-66

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[5]} ${component[6]} "
##components[${n}]="${component[1]} ${component[2]}                   ${component[5]}  "




((n+=1))
#----------------------------------------------------
# Test Case 3 - Proxy ACL - Error Paths of ACL Group
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_ErrorPath"
scenario[${n}]="${xml[${n}]} - Error Paths of AClfile ${scenario_at}"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,1.0-d:Org1:deviceType1:00123
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.1-d:OrgNotHere:deviceType1:00123
component[4]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.2-d:Org1:BadType:00123
component[5]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.3-d:Org1:deviceType2:NoInACL
component[6]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.4-d:Org2:deviceType4:deviceT4-1
component[7]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.5-d:Org1:deviceType4:40100
component[8]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,2.6-d:Org2:deviceTypeY:deviceTY-69

component[9]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,3.1-a:Org1:App01
component[10]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,3.2-a:Org1:App02


components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} "



((n+=1))
#----------------------------------------------------
# Test Case - Proxy ACL - Undeliverable
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_Undeliverable"
scenario[${n}]="${xml[${n}]} - ACLs lost on Reboot, allows Undeliverable msgs to be Queued IoT Tracker ISSUE 1285"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,g:Org1:gwType:gwID
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} "



((n+=1))
#----------------------------------------------------
# Test Case - Proxy ACL - Undeliverable and Attempted QoS:0 ACKed
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_Undeliverable_mixedQoS"
scenario[${n}]="${xml[${n}]} - ACLs lost on Reboot, allows Undeliverable QoS:0 msgs to be Acked IoT Tracker ISSUE 1285"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,g:Org1:gwType:gwID
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} "



((n+=1))
#----------------------------------------------------
# Test Case - Proxy ACL - Undeliverable MQTTv5
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="proxyACL_Undeliverable.v5"
scenario[${n}]="${xml[${n}]} - (MQTTv5) ACLs lost on Reboot, allows Undeliverable msgs to be Queued IoT Tracker ISSUE 1285"
timeouts[${n}]=600
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,g:Org1:gwType:gwID
component[3]=wsDriver,m1,AAAgateway/${xml[${n}]}.xml,a:Org1:appType1

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} "



((n+=1))
#----------------------------------------------------
# Test Case Proxy clean up
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="stop_proxy"
scenario[${n}]="${xml[${n}]} - Proxy Cleanup ${scenario_at}"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./killProxy.sh"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} "


if [[ "$A1_LDAP" == "FALSE" ]] ; then
    ((n+=1))
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="proxyACL_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean ${scenario_at}"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
fi

