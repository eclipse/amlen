#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS HA Retained Tests - 01"

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

#-------------------
# Retained Pre-clean
# Note: We would rather ignore left-over messages from previous tests,
# because cleaning up the HA envrionment after a failure here is very
# time consuming.
#-------------------
# A1 = Primary (this does matter)
# A2 = Standby
xml[${n}]="jms_clearRetained.xml"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JMS Retained Messages in HA Test Cleanup"
component[1]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 007 - jms_HAretained_007
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HAretained_007"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - JMS Retained Messages in HA Test"
component[1]=jmsDriver,m1,HA/jms_HAretained_007.xml,A
component[2]=jmsDriver,m1,HA/jms_HAretained_007.xml,B
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#--------------------------------------------
# Scenario 008 - jms_HA_DS_retained_008
#--------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HAretained_008"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - JMS DS retained Messages in HA Test"
component[1]=jmsDriver,m1,HA/jms_HAretained_008.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario - stop both, restart both, verify status
#----------------------------------------------------

xml[${n}]="stopBoth-startBoth"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - stop standby, stop primary, start standby, start primary"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|stopServer|-m1|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|stopServer|-m1|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startServer|-m1|2"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startServer|-m1|1"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|verifyStatus|-status|STATUS_STANDBY|-t|30|-m1|2"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|verifyStatus|-status|STATUS_RUNNING|-t|30|-m1|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


#-------------------
# Retained Cleanup
#-------------------
# A1 = Primary (this does matter)
# A2 = Standby
xml[${n}]="jms_clearRetained.xml"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JMS Retained Messages in HA Test Cleanup"
component[1]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

