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
#
#----------------------------------------------------
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Queues - for REMOTE SERVERS"

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
timeouts[${n}]=90
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
timeouts[${n}]=90
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
timeouts[${n}]=90
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
# Scenario 5 - jms_queues_005
#----------------------------------------------------
xml[${n}]="jms_queues_005"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Test 5 - Send and receive on multiple queues"
component[1]=sync,m1
component[2]=jmsDriver,m1,queues/jms_queues_005.xml,rmdr
component[3]=jmsDriver,m2,queues/jms_queues_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
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
timeouts[${n}]=90
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
timeouts[${n}]=90
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

