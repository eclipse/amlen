#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Transactions - 00"

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

#----------------------------------------------------
# Scenario 1 - jms_trans_001
#----------------------------------------------------
xml[${n}]="jms_trans_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test for the commit() operation on a transacted session with a Topic. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,transactions/jms_trans_001.xml,rmdr
component[3]=jmsDriver,m2,transactions/jms_trans_001.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1q - jms_trans_001q
#----------------------------------------------------
xml[${n}]="jms_trans_001q"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test for the commit() operation on a transacted session with a Queue. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_001q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_001q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_trans_002
#----------------------------------------------------
xml[${n}]="jms_trans_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test for the rollback() operation on a transacted session with a Topic, with Retained Messages  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_002.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_002.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_trans_Retained 
#----------------------------------------------------
xml[${n}]="jms_trans_Retained"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test for the Retained being in publish order, not commit order.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_Retained.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_Retained.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario - mqtt_clearRetained
#     Some of the prior tests created retained messages. 
#     This simple MQTT xml will remove those before
#     we do any Wildcard testing which would find them and fail., 
#----------------------------------------------------
xml[${n}]="mqtt_clearRetained"
timeouts[${n}]=60
scenario[${n}]="Clear any retained messages before running JMS Wildcard tests [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
#----------------------------------------------------
# Scenario 2 - jms_trans_002q
#----------------------------------------------------
xml[${n}]="jms_trans_002q"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test for the rollback() operation on a transacted session with a Queue. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_002q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_002q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_trans_003
#----------------------------------------------------
xml[${n}]="jms_trans_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Extended rollback() test with multiple consumers on a Topic.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_003.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_003.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_trans_003q
#----------------------------------------------------
xml[${n}]="jms_trans_003q"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Extended rollback() test with multiple consumers on a Queue. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_003q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_003q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_trans_004
#----------------------------------------------------
xml[${n}]="jms_trans_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Extended rollback() test with multiple consumers on individual sessions  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_004.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_004.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_trans_005
#----------------------------------------------------
xml[${n}]="jms_trans_005"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Extended NESTED rollback() test (rollback() while consuming re-delivered messages).  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_005.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_trans_006
#----------------------------------------------------
xml[${n}]="jms_trans_006"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Extended NESTED rollback() test, with multiple sessions and while consuming re-delivered messages.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_006.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_006.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6q - jms_trans_006q
#----------------------------------------------------
xml[${n}]="jms_trans_006q"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Extended NESTED rollback() test, with mulitple sessions, re-delivered messges, on a Queue (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_006q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_006q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_trans_007
#----------------------------------------------------
xml[${n}]="jms_trans_007"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test for the commit() operation on a transacted session with connection stop/restart before commit() (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_007.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_007.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_trans_008
#----------------------------------------------------
xml[${n}]="jms_trans_008"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test rollback() operation on a transacted session with connection stop/restart before rollback(). (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_008.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_008.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - jms_trans_009
#----------------------------------------------------
xml[${n}]="jms_trans_009"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Transactions test 9 - Multiple consumers on 1 transacted session (receive between commit and rollback)"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_009.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_009.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 9q - jms_trans_009q
#----------------------------------------------------
xml[${n}]="jms_trans_009q"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Transactions test 9q - Multiple consumers on 1 transacted session (receive between commit and rollback) Queues"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_009q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_009q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jms_trans_010
#----------------------------------------------------
xml[${n}]="jms_trans_010"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Transactions test 10 - Multiple consumsers on 1 transacted session"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_010.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_010.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 10q - jms_trans_010q
#----------------------------------------------------
xml[${n}]="jms_trans_010q"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Transactions test 10 - Multiple consumsers on 1 transacted session Queues"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_010q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_010q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_trans_011
#----------------------------------------------------
xml[${n}]="jms_trans_011"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Transactions test 11 - Mixed transacted / Non-transacted consumers"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_011.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_011.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - jms_trans_012
#----------------------------------------------------
xml[${n}]="jms_trans_012"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Transactions test 12 - Mixed transacted / Non-transacted consumers with rollback"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_012.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_012.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - jms_trans_013
#----------------------------------------------------
xml[${n}]="jms_trans_013"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Transactions test 13 - Multiple consumers on a single queue"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_013.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_013.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_trans_014
#----------------------------------------------------
xml[${n}]="jms_trans_014"
timeouts[${n}]=10
scenario[${n}]="${xml[${n}]} - Transactions test 14 - Producers on Transacted Session with AsyncTransactionSend  enabled"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_014.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_014.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_trans_014T
#----------------------------------------------------
xml[${n}]="jms_trans_014T"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Transactions test 14 - Server crash test with Asynchronous Producer sending to a Topic"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_014T.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_014T.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_trans_014Q
#----------------------------------------------------
xml[${n}]="jms_trans_014Q"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} -IPv6 Transactions test 14 - Server crash test with Asynchronous Producer sending to a Queue"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_014Q.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_014Q.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 15 - jms_trans_stopSync
#----------------------------------------------------
xml[${n}]="jms_trans_stopSync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Transactions with synchronous consumer, stop connection during receive"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_stopSync.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_stopSync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 16 - jms_trans_closeSync
#----------------------------------------------------
xml[${n}]="jms_trans_closeSync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Transactions with synchronous consumer, close connection during receive"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_closeSync.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_closeSync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 17 - jms_trans_stopAsync
#----------------------------------------------------
xml[${n}]="jms_trans_stopAsync"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Transactions with asynchronous consumer, stop connection during onMessage"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_stopAsync.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_stopAsync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 18 - jms_trans_closeAsync
#----------------------------------------------------
xml[${n}]="jms_trans_closeAsync"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Transactions with asynchronous consumer, close connection during onMessage"
component[1]=sync,m1
component[2]=jmsDriver,m2,transactions/jms_trans_closeAsync.xml,rmdr
component[3]=jmsDriver,m1,transactions/jms_trans_closeAsync.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_trans_015
#----------------------------------------------------
xml[${n}]="jms_trans_015"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Test JMS transacted consumer with QoS 0 like JMS Publisher"
component[1]=sync,m1
component[2]=jmsDriver,m1,transactions/jms_trans_015.xml,rmdr
component[3]=jmsDriver,m2,transactions/jms_trans_015.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
