#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Delivery Mode - 00"

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
# Scenario 1 - jms_DM_001
#----------------------------------------------------
xml[${n}]="jms_DM_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Delivery Mode (set and verify on receiver)"
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_001.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_001.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_DM_002
#----------------------------------------------------
xml[${n}]="jms_DM_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 2 - Delivery Mode combinations of TTL, Priority, Delivery Mode"
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_002.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_002.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_DM_003
#----------------------------------------------------
xml[${n}]="jms_DM_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 3 - Delivery Mode: Persistent Messages to Durable Subscription, with IMA Server stop/start."
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_003.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_003.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_DM_004
#----------------------------------------------------
xml[${n}]="jms_DM_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 4 - Delivery Mode: Persistent Messages to non-durable subscriber, with IMA server stop/start."
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_004.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_004.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_DM_005
#----------------------------------------------------
xml[${n}]="jms_DM_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 5 - Delivery Mode: Non-Persistent Messages to non-durable subscriber, with IMA server stop/start."
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_005.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_DM_006
#----------------------------------------------------
xml[${n}]="jms_DM_006"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 6 - Delivery Mode: Non=Persistent Messages to durable Subscriber, with ISM server stop/start."
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_006.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_006.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_DM_007
#----------------------------------------------------
xml[${n}]="jms_DM_007"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test 7 - Test Delivery Mode. Expired Messages should not be received."
component[1]=sync,m1
component[2]=jmsDriver,m1,deliverymode/jms_DM_007.xml,rmdr
component[3]=jmsDriver,m2,deliverymode/jms_DM_007.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


