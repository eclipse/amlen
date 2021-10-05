#! /bin/bash

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#----------------------------------------------------
#  THIS SCRIPT IS FOR USE WHEN RUNNING CLIENTS AGAINST 
#  A SERVER IN DIFFERENT DATACENTER. IN THAT CASE A BASIC
#  TEST RUN TO VERIFY THE SERVER CONFIGURES AND RUNS
#  IN THAT ENVIRONEMENT IS ALL THAT IS NEEDED. 
#  
#  COMPLEX SCENARIO TESTING SHOULD BE DONE ON SETUPS WHERE
#  THE CLIENTS AND SERVERS ARE IN THE SAME DATACENTER. 
# 
#  IT IS IMPORTANT TO NOT ADD EVERY TEST TO THIS SCRIPT! 
#
#----------------------------------------------------
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Message Delivery. Basic tests only. For use testing on REMOTE SERVERS. "

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
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
# Setup Scenarios 
#
#     mqtt_clearRetained
#     Sometimes prior tests created retained messages 
#     and forget to remove them. 
#     This simple MQTT xml will remove any strays 
#
# Setup some Subscriptions with Expiring messages.  
#
#	  Create some Durable Subscriptions, and load them 
#	  messages, some of which will expire. It takes 
#	  a while for the messages to expire, and then up to 
#     30 seconds for the Engine to start its discard thread,  
#     and then some time for the discard thread to do its work. 
#     
#----------------------------------------------------
xml[${n}]="jms_clearRetained" 
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} Various Setup Scenarios for later message expiration tests"
component[1]=sync,m1
component[2]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
component[3]=jmsDriver,m1,msgdelivery/MessageExpiry_01_jms.xml,ds_setup
component[4]=wsDriver,m2,msgdelivery/MessageExpiry_01_mqtt.xml,ds_setup,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[5]=jmsDriver,m1,msgdelivery/MessageExpiry_01_jms.xml,pub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario - jms_msgdelivery_RetainedExp_Wildcards
# This sets up some retained messages that should expire
# before jms_msgdelivery_Retained_Wildcards runs later in the test bucket. 
# It needs to be enabled once the Message Expiration 
# on retained messages is working. 
# IMPORTANT:  There needs to be some tests in between the setup tests and 
# jms_msgdelivery_Retained_Wildcards running, so the messages have 
# time to expire and be discarded. 
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_RetainedExp_Wildcards"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Publish some Retained messages that Expire as prep for later test"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_RetainedExp_Wildcards.xml,PubRetExp
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario - jms_msgdelivery_RetainedExpAdmin_Wildcards
# This sets up some retained messages that should expire based
# on an administratively set expiration, and must expire
# before jms_msgdelivery_Retained_Wildcards runs later in the test bucket. 
# It needs to be enabled once the Message Expiration 
# on retained messages is working. 
# IMPORTANT:  There needs to be some tests in between the setup tests and 
# jms_msgdelivery_Retained_Wildcards running, so the messages have 
# time to expire and be discarded. 
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_RetainedExpAdmin_Wildcards"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Publish some Retained messages with mqtt and jMS that Expire via Admin set Expiration as prep for later test"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_RetainedExpAdmin_Wildcards.xml,PubRetExp
component[2]=wsDriver,m2,msgdelivery/mqtt_msgdelivery_RetainedExpAdmin_Wildcards.xml,PubRetExp,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_msgdlvr1_001
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_001"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Sync and Async msg delivery, Plus all Header fields set/get and stealing ClientID (Single host)"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_001.xml,ALL,,"-s|IMADeliveryThreads=0"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_msgdlvr1_003
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Suspend/Resume connections and discriminating between similiar Topic names and duplicate ClientIDs (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_003.xml,rmdr,,"-s|ClientID=MyTestClientID"
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_003.xml,rmdt,,"-s|ClientID=MyTestClientID"
component[4]=searchLogsEnd,m2,msgdelivery/jms_msgdelivery_003.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_msgdlvr1_004
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Message Delivery with NoLocal. Calls Connection.Stop() on stopped connection. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_004.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_004.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario Message Expiry
# NOTE: This test must be several minutes after the 
# setup step at the start of this bucket. 
#----------------------------------------------------
xml[${n}]="jms_MessageExpiry_01"
timeouts[${n}]=60
scenario[${n}]=" jms_msdelivery MessageExpiry_01*.xml. Verify expired messages sent earlier in run-scenarios are not received.. "
component[1]=sync,m1
component[2]=wsDriver,m2,msgdelivery/MessageExpiry_01_mqtt.xml,Collector,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[3]=wsDriver,m1,msgdelivery/MessageExpiry_01_mqtt.xml,QoS_0Sub,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[4]=wsDriver,m1,msgdelivery/MessageExpiry_01_mqtt.xml,QoS_1Sub,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[5]=wsDriver,m1,msgdelivery/MessageExpiry_01_mqtt.xml,QoS_2Sub,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[6]=wsDriver,m1,msgdelivery/MessageExpiry_01_mqtt.xml,SharedSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
component[7]=jmsDriver,m1,msgdelivery/MessageExpiry_01_jms.xml,subs
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_msgdelivery_Retained_Wildcards
#
# Un-comment the two lines once expiration in place
# exists for Retained messages.. That just adds
# a check that the retained messages sent in the
# jms_msgdelivery_RetainedExp_Wildcards.xml 
# and jms_msgdeliver_RetainedExpAdmin_Wildcards.xml 
# were actually discarded by the server. 
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_Retained_Wildcards"
scenario[${n}]="${xml[${n}]} - Testing multiple JMS Retained messages and expired Retained messages via wildcard subscriptions. (Two host)"
timeouts[${n}]=30
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_Retained_Wildcards.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_Retained_Wildcards.xml,DurSub
component[4]=jmsDriver,m1,msgdelivery/jms_msgdelivery_Retained_Wildcards.xml,rmdt
component[5]=wsDriver,m2,msgdelivery/mqtt_msgdelivery_Retained_Wildcards.xml,MqttSub,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=../common/ibm.jks"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_msgdlvr1_002
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - IPv6: Multiple Connections Message Delivery with trace logging and setting Priority - Test 2  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_002.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_002.xml,rmdt
component[4]=jmsDriver,m2,msgdelivery/jms_msgdelivery_002.xml,rmdr2
component[5]=searchLogsEnd,m1,msgdelivery/jms_msgdelivery_002.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#
#----------------------------------------------------
# Scenario DurableSubscription Retained  - jms_DS_Retained_Wildcards
# Placed in the msgdelivery bucket on purpose so the retained messages are
# cleaned up by the MQTT cleanup later in this bucket.
#----------------------------------------------------
xml[${n}]="jms_DS_Retained_Wildcards"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test DS Retained Messages on Durable Subscriptions."
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_DS_Retained_Wildcards.xml,rmdr
component[3]=jmsDriver,m2,msgdelivery/jms_DS_Retained_Wildcards.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario 4 - jms_msgdlvr1_004 sync
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_004_Sync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Message Delivery, single session with 4 Synchronous consumers. Session.recover() "
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_004_Sync.xml,rmdr
component[3]=jmsDriver,m2,msgdelivery/jms_msgdelivery_004_Sync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_msgdlvr1_005
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_005"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Multiple Consumers with and without NoLocal (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_005.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_msgdlvr1_006
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_006"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Two consumers, one with MessageSelector, one without. Also Call CloseSession on closed session.(Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_006.xml,rmdr,,"-s|IMADeliveryThreads=100"
component[3]=jmsDriver,m2,msgdelivery/jms_msgdelivery_006.xml,rmdt
component[4]=searchLogsEnd,m1,msgdelivery/jms_msgdelivery_006.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_msgdlvr1_007
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_007"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Two Consumers, with Message Selectors matching different properties. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_007.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_007.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_msgdlvr1_008
# 
# In an attempt to keep AF test times from getting to long, 
# the following is an experiment in running two tests at once. 
# Two UNRELATED tests. Component 4 is completely independent, but
# requires a 20+ second wait while messages expire. Piggbacking
# it here means we don't have to wait for those 45 seconds.
# 
#----------------------------------------------------
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_008"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test1: Two Consumers with Message Selectors that match different values of same property. Test2: jms_RetainedExp_ND_001: MessageExpiration on a Non-durable JMS Sub"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_008.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_008.xml,rmdt
component[4]=jmsDriver,m1,msgdelivery/jms_RetainedExp_ND_001.xml,PubSub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))



#----------------------------------------------------
# Cleanup Scenario - mqtt_clearRetained
#     Some of the prior tests created retained messages. 
#     This simple MQTT xml will remove those before
#     we do any Wildcard testing which would find them and fail., 
#----------------------------------------------------
xml[${n}]="jms_clearRetained"
timeouts[${n}]=60
scenario[${n}]="Clear any retained messages before running JMS Wildcard tests [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jms_msgdlvr1_010
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_010"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Wildcard topics. Valid and invalid use of plus sign. Closes closed Connection as well. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_010.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_010.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_msgdlvr1_011
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_011"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Wildcard topics. Valid and invalid use of pound sign.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_011.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_011.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
#----------------------------------------------------
# Scenario 1 - jms_msgdlvr1_plus
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_plus"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Wildcard Topics, complex plus sign.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_plus.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_plus.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_msgdlvr1_pound
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_pound"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Wildcard topics, complex pound sign.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_pound.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_pound.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - jms_msgdlvr1_012
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_012"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Closing session while retrieving messages. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_012.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_012.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - jms_msgdlvr1_013
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_013"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Client Acknowledge and session recovery (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_013.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_013.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_msgdlvr1_014
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_014"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Producer dynamically sending to multiple destinations.   (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_014.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_014.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 17 - jms_msgdlvr1_017
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_017"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Purely GVT Characters for Topics, Property names and values, clientID and message bodies."
component[1]=sync,m1
component[2]=jmsDriver,m2,msgdelivery/jms_msgdelivery_017.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_017.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 18 - jms_msgdlvr1_018
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_018"
timeouts[${n}]=65
scenario[${n}]="${xml[${n}]} - Large Messages. Below and Above Policy limits."
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_018.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 21 - Disable/Enable of an Endpoint 
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_defect83703"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Disable/Enable Endpoint to test defect 83703 in the next scenario"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|DisEn|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_msgdelivery_defect73753
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_defect73753"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Large Messages, verify cache does not grow as it did in defect 73753"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_defect73753.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 21 - jms_msgdlvr1_021_np
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_021_np"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CLIENT_ACK enabled and server reboot with NON_PERSISTENT(Two host)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disable|-c|restpolicy_config.cli"
component[2]=sync,m1
component[3]=jmsDriver,m2,msgdelivery/jms_msgdelivery_021_np.xml,rmdr
component[4]=jmsDriver,m1,msgdelivery/jms_msgdelivery_021_np.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

# ----------------------------------------------------
# jms_msgdelivery_MP_max
# ----------------------------------------------------
xml[${n}]="jms_msgdelivery_MP_max"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - MessagingPolicy update Max Messages and make sure the new value takes effect"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_MP_max.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
