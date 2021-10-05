#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT SharedSub00 via WSTestDriver"

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
    xml[${n}]="mqtt_sharedsub_00_M1_LDAP_setup"
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
scenario[${n}]="${xml[${n}]} - run policy_config.cli and other setup for the SharedSubscriptions bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupSS|-c|policy_config.cli"
component[2]=wsDriver,m2,sharedsub/testMixedProtocol_Setup.xml,Setup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))



#----------------------------------------------------
# Test Case 1 - mqtt_sharedsub_error01
#----------------------------------------------------
xml[${n}]="testmqtt_sharedsub_error01"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT non-durable Shared Subscriptions at QoS=0"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Setup - set trace level to 9
#----------------------------------------------------
xml[${n}]="mqtt_sharedsub_00_setup_trace"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set trace level to 9 for debugging purposes  "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup1|-c|sharedsub/testpolicy.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_sharedsub_error02
#----------------------------------------------------
xml[${n}]="testmqtt_sharedsub_error02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions and Unsubscribe for MQTT and JMS non-durable Shared Subscriptions at QoS=2"
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
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT joining non-durable Shared Subscriptions created by JMS"
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
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT and JMS non-durable Shared Subscriptions"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
component[3]=jmsDriver,m1,sharedsub/testmqtt_dursharedsub_error02_JMS.xml,Cons1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Setup - set trace level back
#----------------------------------------------------
xml[${n}]="mqtt_sharedsub_00_cleanup_trace"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set trace level to 5 after debugging purposes  "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup1|-c|sharedsub/testpolicy.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_dursharedsub_error03
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error03"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions for MQTT joining non-durable Shared Subscriptions created by JMS"
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
scenario[${n}]="${xml[${n}]} - Test Unsubscribe in Mixed Protocol"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
component[3]=jmsDriver,m1,sharedsub/testmqtt_dursharedsub_error04_JMS.xml,Cons1
components[${n}]="${component[1]} ${component[2]}  ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_sec01
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_sec01"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Security Conditions MQTT Durable Shared Subscriptions Subscribe and Unsubscribe"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_sec02
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_sec02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Security Conditions MQTT Durable Shared Subscriptions, joining an existing durable shared subscription"
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_NDS_01 -- 
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_01"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=0"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_NDS_01_MixedQoS -- 
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_01_MixedQoS"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at Mixed QoS=1, QoS=2"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_NDS_02 -- Mixed Protocols
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_02"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=0 with both JMS and MQTT subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=jmsDriver,m1,sharedsub/testmqtt_NDS_02_JMS.xml,JMSCons1
component[4]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Test Case 4 - mqtt_NDS_02_MixedQoS -- Mixed Protocols mixed QoS
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_02_MixedQoS"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=1,2 with both JMS and MQTT subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=jmsDriver,m1,sharedsub/testmqtt_NDS_02_MixedQoS_JMS.xml,JMSCons1
component[4]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_NDS_03 -- Subscribers coming and going
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_03"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=0, Busy test with lots of Subscribes and Unsubscribes "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,BusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_NDS_03_MixedQoS -- Subscribers coming and going
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_03_MixedQoS"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=1&2, Busy test with lots of Subscribes and Unsubscribes "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,BusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 6 - mqtt_NDS_04 -- Subscribers coming and going
# without unsubscribing.  
#----------------------------------------------------
xml[${n}]="testmqtt_NDS_04"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=0, Busy test with lots of Disconnects and no unsubscribes "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,BusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_NDS_04_MixedQoS -- Subscribers coming and going
# without unsubscribing.  
#------------------------------------------------
xml[${n}]="testmqtt_NDS_04_MixedQoS"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Non-Durable Shared Subscription at QoS=0, Busy test with lots of Disconnects and no unsubscribes "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,BusySubs
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Test Case  - mqtt_SS_05_MixedQoS -- Mixed Durable and non-Durable Shared
#----------------------------------------------------
xml[${n}]="testmqtt_SS_05_MixedQoS"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Non-Durable and Durable Shared Subscriptions on same topic with mixed QoS and same Subscription name."
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,DurSubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m2,sharedsub/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Test Case  - Mixed Protocols -- Subscribers coming and going
#----------------------------------------------------
xml[${n}]="mqtt_testMixedProtocol_01"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Durable Shared Subscription, Busy test with lots of Subscribers coming going, both JMS/MQTT subscribers "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/testMixedProtocol_01.xml,MqttBusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=jmsDriver,m2,sharedsub/testMixedProtocol_01_JMS.xml,JMSBusySubs
component[4]=wsDriver,m1,sharedsub/testMixedProtocol_01.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,sharedsub/testMixedProtocol_01.xml,Pubs1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} "
((n+=1))


if [ "${FULLRUN}" == "TRUE" ]; then

# These run only in Prod, as we discourage MessageGateway 
# customers from mixing Protocols on a 
# shared subscription. 
#----------------------------------------------------
# Test Case  - Mixed Protocols -- 
#----------------------------------------------------
xml[${n}]="mqtt_testMixedProtocol_02"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Durable Shared Subscription, Steady state receiving Mixed Protocols "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/testMixedProtocol_02.xml,MqttBusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=jmsDriver,m2,sharedsub/testMixedProtocol_02_JMS.xml,JMSBusySubs
component[4]=wsDriver,m1,sharedsub/testMixedProtocol_02.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,sharedsub/testMixedProtocol_02.xml,Pubs1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[6]=searchLogsEnd,m1,sharedsub/testMixedProtocol_02.comparetest
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


#----------------------------------------------------
# Test Case  - Mixed Protocols --
#----------------------------------------------------
xml[${n}]="mqtt_testMixedProtocol_03"
timeouts[${n}]=340
scenario[${n}]="${xml[${n}]} - Durable Shared Subscription, both JMS/MQTT subscribers "
component[1]=sync,m1
component[2]=wsDriver,m1,sharedsub/testMixedProtocol_03.xml,MqttBusySubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=jmsDriver,m2,sharedsub/testMixedProtocol_03_JMS.xml,JMSSub1
component[4]=jmsDriver,m2,sharedsub/testMixedProtocol_03_JMS.xml,JMSSub2
component[5]=wsDriver,m1,sharedsub/testMixedProtocol_03.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[6]=wsDriver,m2,sharedsub/testMixedProtocol_03.xml,Pubs1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[7]=wsDriver,m1,sharedsub/testMixedProtocol_03.xml,Pubs2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}  ${component[6]}  ${component[7]}"
((n+=1))

fi

#----------------------------------------------------
# Test Case   - mqtt_dursharedsub_error01
#----------------------------------------------------
xml[${n}]="testmqtt_dursharedsub_error01"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Error conditions and sub/unsub for MQTT Durable Shared Subscriptions. "
component[1]=wsDriver,m1,sharedsub/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_sharedsub_00_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup for mqtt SharedSub-00"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupSS|-c|policy_config.cli"
component[2]=wsDriver,m1,sharedsub/testMixedProtocol_Cleanup.xml,Cleanup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="mqtt_sharedsub_00_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

