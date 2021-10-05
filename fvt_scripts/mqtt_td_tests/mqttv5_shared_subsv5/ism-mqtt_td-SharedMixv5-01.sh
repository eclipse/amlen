#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios
# 
# These are the set of $sharedSubscription tests that will be run with AllowProxyProtocol=true in server configs
# mostly error cases
# 

scenario_set_name="MQTT SharedMix01 via WSTestDriver"

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
    xml[${n}]="mqtt_sharedmix_01_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_sharedsub_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_sharedsub_00_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli and other setup for the SharedMix01 tests - AllowMqttProxyProtocol true"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupSS|-c|policy_config.cli"
component[2]=wsDriver,m2,sharedsub/testMixedProtocol_Setup.xml,Setup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))



#----------------------------------------------------
# Test Case 1 - mqtt_sharedsub_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedsub_error01"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT non-durable Shared Subscriptions at QoS=0 - AllowMqttProxyProtocol true"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_sharedsub_error02
#----------------------------------------------------
xml[${n}]="testmqtt_sharedsub_error02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions and Unsubscribe for MQTT and JMS non-durable Shared Subscriptions at QoS=2 - AllowMqttProxyProtocol true"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
component[3]=jmsDriver,m1,sharedsub/testmqtt_sharedsub_error02_JMS.xml,Cons1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 2a - mqtt_sharedsub_error03
#----------------------------------------------------
xml[${n}]="testmqtt_sharedsub_error03"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT joining non-durable Shared Subscriptions created by JMS - AllowMqttProxyProtocol true"
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/testmqtt_sharedsub_error03_JMS.xml,Cons1
component[3]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_dursharedsub_error02
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT and JMS non-durable Shared Subscriptions - AllowMqttProxyProtocol true"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
component[3]=jmsDriver,m1,sharedsub/testmqtt_dursharedsub_error02_JMS.xml,Cons1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Test Case  - mqtt_dursharedsub_error03
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error03"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT joining non-durable Shared Subscriptions created by JMS - AllowMqttProxyProtocol true"
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/testmqtt_dursharedsub_error03_JMS.xml,Cons1
component[3]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Test Case  - mqtt_sharedsub_error04
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error04"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Unsubscribe in Mixed Protocol - AllowMqttProxyProtocol true"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
component[3]=jmsDriver,m1,sharedsub/testmqtt_dursharedsub_error04_JMS.xml,Cons1
components[${n}]="${component[1]} ${component[2]}  ${component[3]}"
((n+=1))

# security tests - will enable later

#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_sec01
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_sec01"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Security Conditions MQTT Durable Shared Subscriptions Subscribe and Unsubscribe - AllowMqttProxyProtocol true"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_sec02
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_sec02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Security Conditions MQTT Durable Shared Subscriptions, joining an existing durable shared subscription - AllowMqttProxyProtocol true"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_error01
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error01"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions and sub/unsub for MQTT Durable Shared Subscriptions. - Allow MqttProxyProtocol true"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_sharedsub_00_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup for mqtt SharedMix-01"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupSS|-c|policy_config.cli"
component[2]=wsDriver,m1,sharedsub/testMixedProtocol_Cleanup.xml,Cleanup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="mqtt_sharedmix_01_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

