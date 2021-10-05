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
fi

#----------------------------------------------------
# Setup Scenario  - jms message expiration setup
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_expiry_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for JMS Queue Expiry testing"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupHA|-c|expiry/expiry_config.cli"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|expiry/expiry_config.cli|setupHA"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_Expiry_mixed"
scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest for JMS and MQTT in HA"
timeouts[${n}]=250
component[1]=sync,m1
component[2]=jmsDriver,m1,expiry/HA_expiry_test_jms.xml,ALL
component[3]=wsDriver,m1,expiry/HA_expiry_test_mqtt.xml,ALL
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Standby
# A2 - Primary
xml[${n}]="HA_Expiry_swapPrimary"
scenario[${n}]="${xml[${n}]} - swapPrimary back to A1"
timeouts[${n}]=250
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_Expiry_jms_queues"
scenario[${n}]="${xml[${n}]} - Testing messages are discarded at rest from a queue in HA"
timeouts[${n}]=250
component[1]=jmsDriver,m1,expiry/HA_expiry_jms_queues.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_Expiry_queues_swapPrimary"
scenario[${n}]="${xml[${n}]} - swapPrimary back to A1"
timeouts[${n}]=250
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_expiryDyn_test_jms"
scenario[${n}]="${xml[${n}]} - Testing messages expire and policy updates are synchronized in HA"
timeouts[${n}]=280
component[1]=sync,m1
component[2]=jmsDriver,m1,expiry/HA_expiryDyn_test_jms.xml,ALL
component[3]=wsDriver,m1,expiry/HA_expiryDyn_test_mqtt.xml,ALL
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_expiryDyn_test_startStandby"
scenario[${n}]="${xml[${n}]} - startStandby again in case previous test failed"
timeouts[${n}]=250
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_Expiry_queues_swapPrimary"
scenario[${n}]="${xml[${n}]} - swapPrimary back to A1"
timeouts[${n}]=250
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

# expiry + discard combo test

#----------------------------------------------------
# Cleanup Scenario  - jms message expiration cleanup
#----------------------------------------------------
# A1 - Primary
# A2 - Standby
xml[${n}]="HA_expiry_config_cleanup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for Shared Subscription testing"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupHA|-c|expiry/expiry_config.cli"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|expiry/expiry_config.cli|cleanupHA"
components[${n}]="${component[1]} "
((n+=1))
