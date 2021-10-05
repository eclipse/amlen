#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Wildcards - 00"

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
# Setup Scenario  - jms wildcard setup
#----------------------------------------------------
xml[${n}]="jms_wildcard_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for wildcard testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|./wildcard/wildcard_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#*******************************************************************
# Section 1 - Topic Messaging Policies without variable replacements
#*******************************************************************

#----------------------------------------------------
# Scenario 1 - jms_wildcard_001
#----------------------------------------------------
xml[${n}]="jms_wildcard_001"
scenario[${n}]="${xml[${n}]} - Testing topic messaging policy with a single asterisk in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_wildcard_002
#----------------------------------------------------
xml[${n}]="jms_wildcard_002"
scenario[${n}]="${xml[${n}]} - Testing topic messaging policy with two asterisks in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_wildcard_003
#----------------------------------------------------
xml[${n}]="jms_wildcard_003"
scenario[${n}]="${xml[${n}]} - Testing topic messaging policy with three asterisks in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_003.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#**************************************************************************
# Section 2 - Subscription Messaging Policies without variable replacements
#**************************************************************************

#----------------------------------------------------
# Scenario 4 - jms_wildcard_004
#----------------------------------------------------
xml[${n}]="jms_wildcard_004"
scenario[${n}]="${xml[${n}]} - Testing subscription messaging policy with 1 asterisk in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_004.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_wildcard_005
#----------------------------------------------------
xml[${n}]="jms_wildcard_005"
scenario[${n}]="${xml[${n}]} - Testing subscription messaging policy with 2 asterisks in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_005.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_wildcard_006
#----------------------------------------------------
xml[${n}]="jms_wildcard_006"
scenario[${n}]="${xml[${n}]} - Testing subscription messaging policy with 3 asterisks in different positions"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_006.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#**************************************************************************
# Section 3 - Variable replacement in destination / ClientID
#**************************************************************************

#----------------------------------------------------
# Scenario 7 - jms_wildcard_007
#----------------------------------------------------
xml[${n}]="jms_wildcard_007"
scenario[${n}]="${xml[${n}]} - Test variations of messaging policy variable replacement"
timeouts[${n}]=150
component[1]=jmsDriver,m1,wildcard/jms_wildcard_007.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 99 - jms wildcard cleanup
#----------------------------------------------------
# Clean up objects created for wildcard test cases
xml[${n}]="jms_wildcard_config_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 99 - Delete policies and endpoints for wildcard testing"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|./wildcard/wildcard_config.cli"
components[${n}]="${component[1]}"
((n+=1))
