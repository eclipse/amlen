#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT Slow via WSTestDriver"

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

SLOWDISKPERSISTENCE="No"
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] || [[ "${A1_TYPE}" =~ "DOCKER" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
    SLOWDISKPERSISTENCE="Yes"
fi


if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize
    #----------------------------------------------------
    xml[${n}]="mqtt_slow_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_slow_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_slow_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli for the MaxMessages behavior bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupMM|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_slow01
#----------------------------------------------------
xml[${n}]="testmqtt_slow01"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=0"
component[1]=wsDriver,m1,slow/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_slow02
#----------------------------------------------------
xml[${n}]="testmqtt_slow02"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=0. Multiple subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,slow/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_slowShared
#----------------------------------------------------
xml[${n}]="testmqtt_slowShared02"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=0 Shared Subscription."
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,slow/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# Test Case 3- mqtt_slow03
#----------------------------------------------------
xml[${n}]="testmqtt_slow03"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=1"
component[1]=wsDriver,m1,slow/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 4- mqtt_slow04
#----------------------------------------------------
xml[${n}]="testmqtt_slow04"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=2"
component[1]=wsDriver,m1,slow/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Test Case 5 - mqtt_slow05
#----------------------------------------------------
if [[ "${A1_LOCATION}" == "remote" ]] ; then

	#  Skip the testmqtt_slow05 testcase on Remote Servers.
	echo "  Skipping testmqtt_slow05 on this setup. JMS to remote servers isn't fast enough to create needed conditions."

else


#----------------------------------------------------
# Test Case 5 - mqtt_slow05 -- test can be rather slow
# on certain machines, and impossibly slow on
# systems with DiskPersistence enabled.
#
# It's not possible to create the conditions needed
# for this test on DiskPersisence.. so skip it.
#
#----------------------------------------------------
xml[${n}]="testmqtt_slow05"
timeouts[${n}]=260
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at Mixed QoS, and with wildchild who doesn't discard messages."
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m2,slow/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=jmsDriver,m1,slow/testmqtt_slow05_JMSSub.xml,JMSSub
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

fi

#----------------------------------------------------
# Test Case 8 - mqtt_slow08 vr00m b00m b00m
#----------------------------------------------------
xml[${n}]="testmqtt_slow08"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at Mixed QoS, and with cleanSession=true"
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,TX0,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,TX1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,slow/${xml[${n}]}.xml,TX2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m1,slow/${xml[${n}]}.xml,RX_Hiccup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[6]=wsDriver,m1,slow/${xml[${n}]}.xml,RX_Fast,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[7]=wsDriver,m1,slow/${xml[${n}]}.xml,Monitor,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#----------------------------------------------------
# Test Case 9 - mqtt_maxmsgBehavior01
#----------------------------------------------------
xml[${n}]="testmqtt_maxmsgBehavior01"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test MaxMessagesBehavior=DiscardOldMessages behavior at QoS=1. Multiple subscribers. Check Defect if 96002 is fixed yet "
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,slow/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,slow/${xml[${n}]}.xml,Pub3,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case 10 - mqtt_maxmsgBehavior02
#----------------------------------------------------
xml[${n}]="testmqtt_maxmsgBehavior02"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test MaxMessagesBehavior=DiscardOldMessages behavior at QoS=2. Multiple subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,slow/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,slow/${xml[${n}]}.xml,Pub3,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Test Case 1 -testmqtt_maxmsgsDynamic_001
#----------------------------------------------------
xml[${n}]="testmqtt_maxmsgsDynamic_001"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test Dynamic Policy Updates for MQTT. MaxMessageBehavior and MaxMessages"
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,ds_setup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 -testmqtt_maxmsgsDynamic_002
# This is dependent on the prior testmqtt_maxmsg_Dynamic_001.
#----------------------------------------------------
xml[${n}]="testmqtt_maxmsgsDynamic_002"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - Test Dynamic Policy Updates for MQTT. MaxMessageBehavior and MaxMessages"
component[1]=sync,m1
component[2]=wsDriver,m1,slow/${xml[${n}]}.xml,Subs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,slow/${xml[${n}]}.xml,Pubs,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}  ${component[3]}"
((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_slow_policy_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup for mqtt slow scenarios. (MaxMessageBehavior tests) "
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupMM|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1
    #----------------------------------------------------
    xml[${n}]="mqtt_slow_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
