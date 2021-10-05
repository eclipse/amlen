#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Durable Subscription - 00"

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

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	#----------------------------------------------------
	# Scenario 1 - jms_DS_001
	#----------------------------------------------------
	xml[${n}]="jms_DS_001"
	timeouts[${n}]=30
	scenario[${n}]="${xml[${n}]} - Test 1 - Basic Durable Subscribe/Unsubscribe and error paths."
	component[1]=jmsDriver,m1,durablesub/jms_DS_001.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_002
	#----------------------------------------------------
	xml[${n}]="jms_DS_002"
	timeouts[${n}]=30
	scenario[${n}]="${xml[${n}]} - Test 2 - Durable Subscriptions with valid and invalid Message Selectors"
	component[1]=jmsDriver,m1,durablesub/jms_DS_002.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 3 - jms_DS_003
	#----------------------------------------------------
	xml[${n}]="jms_DS_003"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 3 - Durable Subscription, messages sent while subscriber is inactive are received when it becomes active. (Two hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_003.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_003.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 3 - jms_DS_004
	#----------------------------------------------------
	xml[${n}]="jms_DS_004"
	timeouts[${n}]=40
	scenario[${n}]="${xml[${n}]} - Test 4 - Durable Subscription messages sent while disconnected are received when reconnected. Depends on jms_DS_003. "
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_004.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_004.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 3 - jms_DS_004b
	#----------------------------------------------------
	xml[${n}]="jms_DS_004b"
	timeouts[${n}]=40
	scenario[${n}]="${xml[${n}]} - Test 4b - Receive more messages on a DS which had its topic name modified in another connection (Two Hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_004b.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_004b.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_005
	#----------------------------------------------------
	xml[${n}]="jms_DS_005"
	timeouts[${n}]=40
	scenario[${n}]="${xml[${n}]} - Test 5 - IPv6: Durable Subscription. Two subscribers going inactive at different points receive all msgs. (Three hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_005.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_005.xml,rmdt
	component[4]=jmsDriver,m1,durablesub/jms_DS_005.xml,rmdr2
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_006
	#----------------------------------------------------
	xml[${n}]="jms_DS_006"
	timeouts[${n}]=40
	scenario[${n}]="${xml[${n}]}.xml - Test 6 - Durable Subscription with NoLocal.(Two hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_006.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_006.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_007
	#----------------------------------------------------
	xml[${n}]="jms_DS_007"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 7 - Durable Subscription with NoLocal, and resubscribe (start/stop consumer but not producer). (Two hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_007.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_007.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_008
	#----------------------------------------------------
	xml[${n}]="jms_DS_008"
	timeouts[${n}]=40
	scenario[${n}]="${xml[${n}]}.xml - Test 8 - Resubscribe with same Subscription name but different subscription values. (Two hosts)"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_008.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_008.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
fi

#----------------------------------------------------
# Scenario 1 - jms_DS_009
#----------------------------------------------------
xml[${n}]="jms_DS_009"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 9 - Durable Subscription with Client Acknowledge. (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_009.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_009.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_DS_010
#----------------------------------------------------
xml[${n}]="jms_DS_010"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 10 - Durable Subscription with Client Acknowledge, disconnect/reconnect. (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_010.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_010.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_DS_010_async
#----------------------------------------------------
xml[${n}]="jms_DS_010_async"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 10 - Durable Subscription with Client Acknowledge asynchronous consumer, disconnect/reconnect. (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_010_async.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_010_async.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_DS_011
#----------------------------------------------------
xml[${n}]="jms_DS_011"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 11 - Durable Subscription with selectors (Two hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_011.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_011.xml,rmdt
component[4]=jmsDriver,m2,durablesub/jms_DS_011.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario - jms_DS_012_SingleSession_MT
#----------------------------------------------------
xml[${n}]="jms_DS_012_SingleSession_MT"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Durable Subscription Single Session Acking test. Messages received out order. "
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_012_SingleSession_MT.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m1,durablesub/jms_DS_012_SingleSession_MT.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	#----------------------------------------------------
	# Scenario - jms_DS_013
	#----------------------------------------------------
	xml[${n}]="jms_DS_013"
	timeouts[${n}]=30
	scenario[${n}]="${xml[${n}]} - Exception in onMessage using AUTO_ACK"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_013.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_013.xml,rmdt
	component[4]=searchLogsEnd,m2,durablesub/jms_DS_013.comparetest
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_closeAsync
	#----------------------------------------------------
	xml[${n}]="jms_DS_closeAsync"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Durable Subscription with Client Acknowledge asynchronous consumer, close connection during onMessage"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_closeAsync.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_closeAsync.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_closeSync
	#----------------------------------------------------
	xml[${n}]="jms_DS_closeSync"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Durable Subscription with Client Acknowledge synchronous consumer, close connection during receive"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_closeSync.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_closeSync.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_stopAsync
	#----------------------------------------------------
	xml[${n}]="jms_DS_stopAsync"
	timeouts[${n}]=80
	scenario[${n}]="${xml[${n}]} - Durable Subscription with Client Acknowledge asynchronous consumer, stop connection during onMessage"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_stopAsync.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_stopAsync.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 1 - jms_DS_stopSync
	#----------------------------------------------------
	xml[${n}]="jms_DS_stopSync"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Durable Subscription with Client Acknowledge synchronous consumer, stop connection during receive"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_stopSync.xml,rmdr
	component[3]=jmsDriver,m1,durablesub/jms_DS_stopSync.xml,rmdt
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
fi

#----------------------------------------------------
# Scenario 1 - jms_DS_flowAsync
# NOTE: I need to add a compare test, that the cache never gets 
# above 6. (ClientMessageCache=6) 
#----------------------------------------------------
xml[${n}]="jms_DS_flowAsync"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Durable Subscription with FlowControl asynchronous consumer, stop connection during onMessage"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_flowAsync.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_flowAsync.xml,rmdt
component[4]=searchLogsEnd,m2,durablesub/jms_DS_flowAsync.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#
#----------------------------------------------------
# Scenario zombie1 - jms_DS_zombie1
#----------------------------------------------------
xml[${n}]="jms_DS_zombie1"
timeouts[${n}]=170
scenario[${n}]="${xml[${n}]} - Test DS Zombie1 -- TopicProducer and DurableConsumer acting like TCP silently dropped."
component[1]=sync,m1
component[2]=jmsDriver,m1,durablesub/jms_DS_zombie1.xml,rmdr
component[3]=jmsDriver,m2,durablesub/jms_DS_zombie1.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	#----------------------------------------------------
	# Scenario 1 - jms_DS_fairness
	#----------------------------------------------------
	xml[${n}]="jms_DS_fairness"
	timeouts[${n}]=350
	scenario[${n}]="${xml[${n}]} - DS Fairness"
	component[1]=sync,m1
	component[2]=jmsDriver,m2,durablesub/jms_DS_fairness.xml,tx
	component[3]=jmsDriver,m1,durablesub/jms_DS_fairness.xml,rx
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))
fi

#----------------------------------------------------
# Scenario 1 - jms_DS_014
#----------------------------------------------------
xml[${n}]="jms_DS_014"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Deletion through CLI and Recreation of Durable Subscriptions"
component[1]=jmsDriver,m1,durablesub/jms_DS_014.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_DS_015
#----------------------------------------------------
xml[${n}]="jms_DS_015"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 15 - Durable Subscription with DisableACK and CLIENT_ACK"
component[1]=sync,m1
component[2]=jmsDriver,m2,durablesub/jms_DS_015.xml,rmdr
component[3]=jmsDriver,m1,durablesub/jms_DS_015.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
