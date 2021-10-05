#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Queues - 00"

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

if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Scenario 0 - jms_queue_objects
#----------------------------------------------------
xml[${n}]="jms_queue_objects"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 0 - Create JMS Queue Objects"
component[1]=jmsDriver,m1,queues/jms_queue_objects.xml,ALL,-o_-l_9
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_queues_001
#----------------------------------------------------
xml[${n}]="jms_queues_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Test sending and receiving messages using a queue, on just one client."
component[1]=jmsDriver,m1,queues/jms_queues_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1b - jms_queues_001browser
#----------------------------------------------------
xml[${n}]="jms_queues_001browser"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 Browser - JMS Queue browser with single enumeration"
component[1]=jmsDriver,m1,queues/jms_queues_001browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_queues_002
#----------------------------------------------------
xml[${n}]="jms_queues_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 2 - Receive messages as they are sent to the queue (Two Hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_002.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_002.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2b - jms_queues_002browser
#----------------------------------------------------
xml[${n}]="jms_queues_002browser"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Test 2 Browser - JMS Queue browser with 2 enumerations"
component[1]=jmsDriver,m1,queues/jms_queues_002browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_queues_003
#----------------------------------------------------
xml[${n}]="jms_queues_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 3 - Receive messages left on a queue (Two Hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_003.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_003.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3b - jms_queues_003browser
#----------------------------------------------------
xml[${n}]="jms_queues_003browser"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 3 Browser - 2 JMS Queue browsers"
component[1]=jmsDriver,m1,queues/jms_queues_003browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_queues_004
#----------------------------------------------------
xml[${n}]="jms_queues_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 4 - Receive both messages left on queue and actively being sent, as well as to a Queue with AllowSend=False"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_004.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m2,queues/jms_queues_004.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4b - jms_queues_004browser
#----------------------------------------------------
xml[${n}]="jms_queues_004browser"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Test 4 Browser - 3 JMS Queue browsers"
component[1]=jmsDriver,m1,queues/jms_queues_004browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_queues_005
#----------------------------------------------------
xml[${n}]="jms_queues_005"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 5 - Send and receive on multiple queues"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_005.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5b - jms_queues_005browser
#----------------------------------------------------
xml[${n}]="jms_queues_005browser"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 5 Browser - 2 JMS Queue browsers with message selectors"
component[1]=jmsDriver,m1,queues/jms_queues_005browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_queues_006
#----------------------------------------------------
xml[${n}]="jms_queues_006"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 6 - Message expiration on queues"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_006.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_006.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6A - jms_queues_006
#----------------------------------------------------
xml[${n}]="jms_queues_006_async"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 6_async - Message expiration on queues with Asynchronous receive"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_006_async.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m2,queues/jms_queues_006_async.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6b - jms_queues_006browser
#----------------------------------------------------
xml[${n}]="jms_queues_006browser"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 6 Browser - JMS Queue browser with message expiration"
component[1]=jmsDriver,m1,queues/jms_queues_006browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_queues_007p
#----------------------------------------------------
xml[${n}]="jms_queues_007p"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 7p - Recovery with persistent messages on queues"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_007p.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_007p.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_queues_007np
#----------------------------------------------------
xml[${n}]="jms_queues_007np"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 7np - Recovery with non-persistent messages on queues"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_007np.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_007np.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7b - jms_queues_007browser
#----------------------------------------------------
xml[${n}]="jms_queues_007browser"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 7 Browser - JMS Queue browser with async consumer"
component[1]=jmsDriver,m1,queues/jms_queues_007browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - jms_queues_008
#----------------------------------------------------
xml[${n}]="jms_queues_008"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 8 - DiscardMessages Queue CLI command (Part One)"
component[1]=jmsDriver,m2,queues/jms_queues_008.xml,rmdt
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jms_queues_008
#----------------------------------------------------
xml[${n}]="jms_queues_008"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 8 - DiscardMessages Queue CLI command (Part Two)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|discard|-c|restpolicy_config.cli"
component[2]=jmsDriver,m1,queues/jms_queues_008.xml,rmdr
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 8b - jms_queues_008browser
#----------------------------------------------------
xml[${n}]="jms_queues_008browser"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test 8 Browser - JMS Queue browsers"
component[1]=jmsDriver,m1,queues/jms_queues_008browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_queues_009
# 
#----------------------------------------------------
xml[${n}]="jms_queues_009"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Test 9 - JMS Queue with concurrent producers and consumers"
component[1]=jmsDriver,m1,queues/jms_queues_009.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9b - jms_queues_009browser
#----------------------------------------------------
xml[${n}]="jms_queues_009browser"
timeouts[${n}]=125
scenario[${n}]="${xml[${n}]} - Test 9 Browser - JMS Queue browsers"
component[1]=jmsDriver,m1,queues/jms_queues_009browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10b - jms_queues_010browser
#----------------------------------------------------
xml[${n}]="jms_queues_010browser"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 10 Browser - JMS Queue browsers without start connection"
component[1]=jmsDriver,m1,queues/jms_queues_010browser.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_queues_011
#----------------------------------------------------
xml[${n}]="jms_queues_011"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Test 11 - Queues Message Selection, Multi-Consumer"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_011.xml,rmdr
component[3]=jmsDriver,m1,queues/jms_queues_011.xml,rmdt
component[4]=jmsDriver,m1,queues/jms_queues_011.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_queues_011_async
#----------------------------------------------------
xml[${n}]="jms_queues_011_async"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Test 11_async - Queues Message Selection, Multi-Consumer Asynchronous"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_011_async.xml,rmdr
component[3]=jmsDriver,m1,queues/jms_queues_011_async.xml,rmdt
component[4]=jmsDriver,m1,queues/jms_queues_011_async.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - jms_queues_012
# Start the producer (rmdt) first in this test. 
#----------------------------------------------------
xml[${n}]="jms_queues_012"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test 2 - Receive messages as they are sent to the queue(Two Hosts)"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_012.xml,rmdt
component[3]=jmsDriver,m1,queues/jms_queues_012.xml,rmdr
component[4]=jmsDriver,m1,queues/jms_queues_012.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - jms_queues_013async
#----------------------------------------------------
xml[${n}]="jms_queues_013"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - Test 13 Multi-consumer, synchrounous, multi-session"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_013.xml,rmdr
component[3]=jmsDriver,m1,queues/jms_queues_013.xml,rmdt
component[4]=jmsDriver,m1,queues/jms_queues_013.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 13_async - jms_queues_013async
#----------------------------------------------------
xml[${n}]="jms_queues_013_async"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test 13 Multi-consumer, asynchrounous multi-session"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_013_async.xml,rmdr
component[3]=jmsDriver,m1,queues/jms_queues_013_async.xml,rmdt
component[4]=jmsDriver,m1,queues/jms_queues_013_async.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_queues_014
#----------------------------------------------------
xml[${n}]="jms_queues_014"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - IPv6 Test 14 - JMS Queues silly AsyncTransactionSend Producer"
component[1]=jmsDriver,m1,queues/jms_queues_014.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_queues_014b
#----------------------------------------------------
xml[${n}]="jms_queues_014b"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - IPv6 Test 14 - JMS Queues silly AsyncTransactionSend Producer, Multi Consumer"
component[1]=jmsDriver,m1,queues/jms_queues_014b.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_queues_014c
#----------------------------------------------------
xml[${n}]="jms_queues_014c"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - IPv6 Test 14 - JMS Queues (silly) AsyncTransactionSend & Sync Producer, Async & Sync Multi Consumer threaded"
component[1]=jmsDriver,m1,queues/jms_queues_014c.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_queues_014d
#----------------------------------------------------
xml[${n}]="jms_queues_014d"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Ipv6 Test 14 - JMS Queues (silly) AyncTransactionSend & Sync Producers and Multi (two each) Sync & Async Consumers on different processes"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_014d.xml,p1
component[3]=jmsDriver,m2,queues/jms_queues_014d.xml,p2
component[4]=jmsDriver,m1,queues/jms_queues_014d.xml,c1
component[5]=jmsDriver,m1,queues/jms_queues_014d.xml,c2
component[6]=jmsDriver,m2,queues/jms_queues_014d.xml,c3
component[7]=jmsDriver,m2,queues/jms_queues_014d.xml,c4
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#----------------------------------------------------
# Scenario 15 - jms_queues_stopAsync
#----------------------------------------------------
xml[${n}]="jms_queues_stopAsync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Stop connection during asynchronous receive"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_stopAsync.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_stopAsync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 16 - jms_queues_closeAsync
#----------------------------------------------------
xml[${n}]="jms_queues_closeAsync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Close connection during asynchronous receive"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_closeAsync.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_closeAsync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 17 - jms_queues_stopSync
#----------------------------------------------------
xml[${n}]="jms_queues_stopSync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Stop connection during synchronous receive"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_stopSync.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_stopSync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 17 - jms_queues_closeSync
#----------------------------------------------------
xml[${n}]="jms_queues_closeSync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Close connection during synchronous receive"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_closeSync.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_closeSync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

# ----------------------------------------------------
# jms_queues_max
# ----------------------------------------------------
xml[${n}]="jms_queues_max"
timeouts[${n}]=95
scenario[${n}]="${xml[${n}]} - Queue update Max Messages"
component[1]=jmsDriver,m1,queues/jms_queues_max.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 19 - jms_queues_disallow_sending
#----------------------------------------------------
xml[${n}]="jms_queues_disallow_sending"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Set AllowSend=False whilst sending to Queue"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_disallow_sending.xml,rx
component[3]=jmsDriver,m2,queues/jms_queues_disallow_sending.xml,tx
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 18 - jms_queues_18
#----------------------------------------------------
xml[${n}]="jms_queues_18"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Use a closed QueueBrowser"
component[1]=jmsDriver,m1,queues/jms_queues_018.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario zombie1 - jms_queues_zombie1
#----------------------------------------------------
xml[${n}]="jms_queues_zombie1"
timeouts[${n}]=175
scenario[${n}]="${xml[${n}]} - Test Zombie1 -- QueueProducer and QueueConsumer acting like TCP silently dropped."
component[1]=jmsDriver,m2,queues/jms_queues_zombie1.xml,rmdt
component[2]=jmsDriver,m1,queues/jms_queues_zombie1.xml,rmdr
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 21 - jms_queues_021
#----------------------------------------------------
xml[${n}]="jms_queues_021"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - CLIENT_ACK enabled and server reboot (Two host)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enable|-c|restpolicy_config.cli"
component[2]=sync,m1
component[3]=jmsDriver,m2,queues/jms_queues_021.xml,rmdr
component[4]=jmsDriver,m1,queues/jms_queues_021.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 22 - jms_queues_022
#----------------------------------------------------
xml[${n}]="jms_queues_022"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - DisableACK enabled and server reboot with PERSISTENT queue (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,queues/jms_queues_022.xml,rmdr,-o_-l_9
component[3]=jmsDriver,m1,queues/jms_queues_022.xml,rmdt,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))






