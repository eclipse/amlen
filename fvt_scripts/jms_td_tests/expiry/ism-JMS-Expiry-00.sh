#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Message Expiration - 00"

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
    SLOWDISKPERSISTENCE="Yes"
fi

#----------------------------------------------------
# Setup Scenario  - jms message expiration setup
#----------------------------------------------------
xml[${n}]="jms_expiry_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for JMS Queue Expiry testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|expiry/expiry_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#***************************************************
# Section 1 - Queues
#***************************************************

#----------------------------------------------------
# Scenario  - jms_queueExpiry_001
#----------------------------------------------------

xml[${n}]="jms_queueExpiry_001"
scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue"
timeouts[${n}]=320
component[1]=jmsDriver,m1,expiry/jms_queueExpiry_001.xml,expiryA
component[2]=jmsDriver,m1,expiry/jms_queueExpiry_001.xml,expiryB
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ ${SLOWDISKPERSISTENCE} == "Yes" ]]  ; then
	
	#----------------------------------------------------
	# Scenario  - jms_queueExpiry_002_DiskSlow
	#----------------------------------------------------
	
	xml[${n}]="jms_queueExpiry_002_DiskSlow"
	scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue"
	timeouts[${n}]=180
	component[1]=jmsDriver,m1,expiry/jms_queueExpiry_002_DiskSlow.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario  - jms_queueExpiry_003_DiskSlow
	#----------------------------------------------------
	
	xml[${n}]="jms_queueExpiry_003_DiskSlow"
	scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue"
	timeouts[${n}]=240
	component[1]=jmsDriver,m1,expiry/jms_queueExpiry_003_DiskSlow.xml,expiryA
	component[2]=jmsDriver,m1,expiry/jms_queueExpiry_003_DiskSlow.xml,expiryB
	components[${n}]="${component[1]} ${component[2]}"
	((n+=1))

else 
	#----------------------------------------------------
	# Scenario  - jms_queueExpiry_002
	#----------------------------------------------------
	
	xml[${n}]="jms_queueExpiry_002"
	scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue"
	timeouts[${n}]=180
	component[1]=jmsDriver,m1,expiry/jms_queueExpiry_002.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario  - jms_queueExpiry_003
	#----------------------------------------------------
	
	xml[${n}]="jms_queueExpiry_003"
	scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue"
	timeouts[${n}]=240
	component[1]=jmsDriver,m1,expiry/jms_queueExpiry_003.xml,expiryA
	component[2]=jmsDriver,m1,expiry/jms_queueExpiry_003.xml,expiryB
	components[${n}]="${component[1]} ${component[2]}"
	((n+=1))

fi


#----------------------------------------------------
# Scenario  - jms_queueExpiryDyn_001
#----------------------------------------------------

xml[${n}]="jms_queueExpiryDyn_001"
scenario[${n}]="${xml[${n}]} - Testing Dynamic updates to Adminstrativley set MaxMessageTimeToLive in queue"
timeouts[${n}]=225
component[1]=jmsDriver,m1,expiry/jms_queueExpiryDyn_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario  - jms message expiration cleanup
#----------------------------------------------------
xml[${n}]="jms_expiry_config_cleanup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Cleanup of policies, endpoints, etc for Expiry testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|expiry/expiry_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#***************************************************
# Section 1 - Topics not dependent on Config. 
#***************************************************

#----------------------------------------------------
# Scenario 2 - jms_topicExpiry_Expiry_001
#----------------------------------------------------
xml[${n}]="jms_topicExpiry_001"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Non-durable JMS Subscription exceeding MaxMessage size with messages timing out. MaxMessageBehavior is RejectNew. "
component[1]=sync,m1
component[2]=jmsDriver,m1,expiry/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m2,expiry/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_topicExpiry_Expiry_002
#----------------------------------------------------
xml[${n}]="jms_topicExpiry_002"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - JMS Subscription, expiring messages that are inflight, but then nacked "
component[1]=sync,m1
component[2]=jmsDriver,m1,expiry/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m2,expiry/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
