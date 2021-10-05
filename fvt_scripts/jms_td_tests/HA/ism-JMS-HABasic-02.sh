#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS HA Basic Failover - 02"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
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

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Scenario 2 - jms_HAtopic_002
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_topic_002"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Kill primary server in middle of sending/receiving messages (topics)"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_HAtopic_002.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_HAtopic_002.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 cleanup - jms_HAtopic_002_cleanup
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_topic_002_cleanup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jms_topic_002"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_HAqueue_003
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_queue_003"
timeouts[${n}]=250
scenario[${n}]="${xml[${n}]} - Kill primary server after sending and before receiving messages (queues)"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_HAqueue_003.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_HAqueue_003.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 cleanup - jms_HAqueue_003_cleanup
#----------------------------------------------------
# A1 = Primary
# A2 = Dead
xml[${n}]="jms_HA_queue_003_cleanup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jms_queue_003"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_HAtopictrans_004
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_topictrans_004"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Kill primary server after sending and before receiving messages (topic transactions)"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_HAtopictrans_004.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_HAtopictrans_004.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 cleanup - jms_HAtopictrans_004_cleanup
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_topictrans_004_cleanup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jms_topictrans_004"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_HAqueuetrans_005
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_queuetrans_005"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Kill primary server after sending and before receiving messages (queue transactions)"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_HAqueuetrans_005.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_HAqueuetrans_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 cleanup - jms_HAqueuetrans_005_cleanup
#----------------------------------------------------
# A1 = Primary
# A2 = Dead
xml[${n}]="jms_HA_queuetrans_005_cleanup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup after HA_jms_queuetrans_005"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - HA external ldap 001
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_external_ldap_001"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA External LDAP 001"
component[1]=jmsDriver,m1,ldap/external_ldap_ha.xml,user1
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6a - HA external ldap 001 restart standby
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_external_ldap_001_cleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA External LDAP 001 restart standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - HA external ldap 002
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_external_ldap_002"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA External LDAP 002"
component[1]=jmsDriver,m1,ldap/external_ldap_ha.xml,user1Fail_password
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - HA external ldap 003
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_external_ldap_003"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA External LDAP 003"
component[1]=jmsDriver,m1,ldap/external_ldap_ha.xml,user1Fail_mail
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8a - HA external ldap 003 restart Standby
#----------------------------------------------------
# A1 = Primary
# A2 = Dead
xml[${n}]="jms_HA_external_ldap_003_cleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA External LDAP 003 restart standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

# Squeeze in OAuth HA tests as long as we already have external
# LDAP setup!
#----------------------------------------------------
# Scenario 9 - OAuth setup
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_oauth_policysetup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JMS HA OAuth Policy setup"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|oauth/oauth_policy_config.cli|setupHA"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - HA OAuth 001
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_oauth_001"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA OAuth 001"
component[1]=jmsDriver,m1,oauth/jms_oauth_ha.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10a - HA OAuth 001 restart standby
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="jms_HA_oauth_001_cleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA OAuth 001 restart standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - HA OAuth 002
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_oauth_002"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA OAuth 002"
component[1]=jmsDriver,m1,oauth/jms_oauth_ha.xml,run2
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11a - HA OAuth 002 restart standby
#----------------------------------------------------
# A1 = Primary
# A2 = Dead
xml[${n}]="jms_HA_oauth_002_cleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - HA OAuth 001 restart standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - OAuth cleanup
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_oauth_policycleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JMS HA OAuth Policy cleanup"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|oauth/oauth_policy_config.cli|cleanupHA"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 reset - jms_HA_reset
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_policycleanup"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JMS HA Policy cleanup"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|restpolicy_config.cli|cleanup"
component[2]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|ldap/ldap_policy_config.cli|cleanupHA"
component[3]=cAppDriver,m1,"-e|HA/external_ldap.sh","-o|cleanup"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
