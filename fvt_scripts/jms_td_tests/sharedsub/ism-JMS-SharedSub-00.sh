#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Shared Subscription - 00"

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

if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Notes on testcase naming:
#		
#	mc = MultiConnection: consumers are in different connections.
#	scg = SingleConnection with generated clientID: consumers are in the same connection
#	scng = SingleConnection with specified clientID: consumers are in the same connection
#	DS = DurableSubscription (shared) 
#	NDS = NondDurableSubcription (shared) 
#----------------------------------------------------

#----------------------------------------------------
# Setup Scenario  - jms Shared Subsription setup
#----------------------------------------------------
xml[${n}]="jms_sharedsub_policy_config_sssetup"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for Shared Subscription testing (internal LDAP)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|sssetup|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#***************************************************
# Section 1 - Shared Non-Durable Subscriptions
#***************************************************

#----------------------------------------------------
# Scenario  - jms_sharedNDS_mcbasic
#----------------------------------------------------

xml[${n}]="jms_sharedNDS_mcBasic"
scenario[${n}]="${xml[${n}]} - Testing non-durable Shared Subscription with multiple consumers in multiple connections. "
timeouts[${n}]=150
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/jms_sharedNDS_mcBasic.xml,cons1
component[3]=jmsDriver,m2,sharedsub/jms_sharedNDS_mcBasic.xml,cons2
component[4]=jmsDriver,m2,sharedsub/jms_sharedNDS_mcBasic.xml,cons3
component[5]=jmsDriver,m1,sharedsub/jms_sharedNDS_mcBasic.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedNDS_scngBasic
#----------------------------------------------------

xml[${n}]="jms_sharedNDS_scngBasic"
scenario[${n}]="${xml[${n}]} - Non-durable Shared Subscription. Multiple consumers single connection. Non-Generated ClientID"
timeouts[${n}]=150
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/jms_sharedNDS_scngBasic.xml,cons1
component[3]=jmsDriver,m2,sharedsub/jms_sharedNDS_scngBasic.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedNDS_scngReconnect 
# Must follow jms_sharedNDS_scngBasic
#----------------------------------------------------

xml[${n}]="jms_sharedNDS_scngReconnect"
scenario[${n}]="${xml[${n}]} - Non-durable Shared Subscription, connected to what was in jms_sharedNDS_scngBasic"
timeouts[${n}]=200
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/jms_sharedNDS_scngReconnect.xml,cons1
component[3]=jmsDriver,m2,sharedsub/jms_sharedNDS_scngReconnect.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedNDS_scgTrans
#----------------------------------------------------

xml[${n}]="jms_sharedNDS_scgTrans"
scenario[${n}]="${xml[${n}]} - Non-durable Shared Subscription, transacted, rollbacks.Generated ClientID"
timeouts[${n}]=150
component[1]=sync,m1
component[2]=jmsDriver,m2,sharedsub/jms_sharedNDS_scgTrans.xml,cons1
component[3]=jmsDriver,m1,sharedsub/jms_sharedNDS_scgTrans.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#***************************************************
# Section 2 - Shared Durable Subscriptions
#***************************************************

#----------------------------------------------------
# Scenario  - jms_sharedDS_mcbasic
#----------------------------------------------------

xml[${n}]="jms_sharedDS_mcBasic"
scenario[${n}]="${xml[${n}]} - Test Durable Shared Subscription with multiple consumers in multiple connections"
timeouts[${n}]=450
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/jms_sharedDS_mcBasic.xml,cons1
component[3]=jmsDriver,m2,sharedsub/jms_sharedDS_mcBasic.xml,cons2
component[4]=jmsDriver,m1,sharedsub/jms_sharedDS_mcBasic.xml,cons3
component[5]=jmsDriver,m2,sharedsub/jms_sharedDS_mcBasic.xml,prod1
component[6]=jmsDriver,m2,sharedsub/jms_sharedDS_mcBasic.xml,collector
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedDS_scgbasic
#----------------------------------------------------

xml[${n}]="jms_sharedDS_scgBasic"
scenario[${n}]="${xml[${n}]} - Test Durable Shared Subscription. Multiple consumers single connection. Generated ClientID"
timeouts[${n}]=250
component[1]=sync,m1
component[2]=jmsDriver,m1,sharedsub/jms_sharedDS_scgBasic.xml,cons1
component[3]=jmsDriver,m2,sharedsub/jms_sharedDS_scgBasic.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedDS_scngBasic
#----------------------------------------------------

xml[${n}]="jms_sharedDS_scngBasic"
scenario[${n}]="${xml[${n}]} - Durable Shared Subscription. Multiple consumers single connection. Non-Generated ClientID"
timeouts[${n}]=250
component[1]=sync,m1
component[2]=jmsDriver,m2,sharedsub/jms_sharedDS_scngBasic.xml,cons1
component[3]=jmsDriver,m1,sharedsub/jms_sharedDS_scngBasic.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario  - jms_sharedDS_scngReconnect 
# Must follow jms_sharedDS_scngBasic
#----------------------------------------------------

xml[${n}]="jms_sharedDS_scngReconnect"
scenario[${n}]="${xml[${n}]} - Durable Shared Subscription, connected to what was in jms_sharedDS_scngBasic"
timeouts[${n}]=250
component[1]=sync,m1
component[2]=jmsDriver,m2,sharedsub/jms_sharedDS_scngReconnect.xml,cons1
component[3]=jmsDriver,m1,sharedsub/jms_sharedDS_scngReconnect.xml,prod1
component[4]=jmsDriver,m1,sharedsub/jms_sharedDS_scngReconnect.xml,prod2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


#----------------------------------------------------
# Scenario  - jms_shared_scgMixed
#----------------------------------------------------

xml[${n}]="jms_shared_scgMixed"
scenario[${n}]="${xml[${n}]} - Mixed Shared Subscription, same DurableName, Generated ClientID"
timeouts[${n}]=200
component[1]=sync,m1
component[2]=jmsDriver,m2,sharedsub/jms_shared_scgMixed.xml,cons1
component[3]=jmsDriver,m1,sharedsub/jms_shared_scgMixed.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#***************************************************
# Section 3 - Shared Durable Subscriptions Security
#***************************************************

#----------------------------------------------------
# Scenario  - jms_shared_SecWildcards_001.
#----------------------------------------------------

xml[${n}]="jms_shared_secWildcards_001"
scenario[${n}]="${xml[${n}]} - MessagingPolicy,Type=Subscription authorization tests with Wildcard Subscription Messaging Policy"
timeouts[${n}]=30
component[1]=jmsDriver,m2,sharedsub/jms_shared_secWildcards_001.xml,cons1
components[${n}]="${component[1]} "
((n+=1))



#----------------------------------------------------
# Scenario  - jms_shared_SecUserID_001. 
#----------------------------------------------------

xml[${n}]="jms_shared_secUserID_001"
scenario[${n}]="${xml[${n}]} - MessagingPolicy,Type=Subscription authorization tests with UserID"
timeouts[${n}]=30
component[1]=jmsDriver,m1,sharedsub/jms_shared_secUserID_001.xml,cons1
components[${n}]="${component[1]}"
((n+=1))

#***************************************************
# Section 4 - GVT Subscription Policy
#***************************************************

#----------------------------------------------------
# Scenario  - jms_sharedDS_GVT
#----------------------------------------------------

xml[${n}]="jms_sharedDS_GVT"
scenario[${n}]="${xml[${n}]} - Test Durable Shared Subscription with multiple consumers in multiple connections"
timeouts[${n}]=150
component[1]=sync,m1
component[2]=jmsDriver,m2,sharedsub/jms_sharedDS_GVT.xml,cons1
component[3]=jmsDriver,m1,sharedsub/jms_sharedDS_GVT.xml,cons2
component[4]=jmsDriver,m1,sharedsub/jms_sharedDS_GVT.xml,cons3
component[5]=jmsDriver,m2,sharedsub/jms_sharedDS_GVT.xml,prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Scenario 99 - policy_cleanup sscleanup
# If there are non-durable shared subscriptions hanging
# around when this fails, it needs to be defected. 
# Non-Durable shared subscriptions should NEVER
# exist unless there is an active consumer. 
#----------------------------------------------------
# Clean up objects created for shared subscriptions test cases
xml[${n}]="jms_sharedsub_policy_config_sscleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Verify no non-durable shared subscriptions exist after we run (internal LDAP)."
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|sscleanup|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))
