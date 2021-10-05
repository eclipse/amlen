#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS HA Resync Tests - 03"

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
# Scenario 1 - admin cli setup
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_resync_configsetup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Setup cli objects for HA Resync test case"
component[1]=cAppDriverLogWait,m1,"-e|HA/run-cli-primary.sh","-o|HA/resync_config.cli|setup"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - rmdr / rmdt
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_resync_part1"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Send and receive JMS messages"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_resync_part1.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_resync_part1.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - kill primary
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jms_HA_resync_crashprimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Kill primary server"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|crashPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - rmdr2 / rmdt2
#----------------------------------------------------
# A1 = dead
# A2 = Primary
xml[${n}]="jms_HA_resync_part2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Continue sending and receiving on the standby"
component[1]=sync,m1
component[2]=jmsDriver,m2,HA/jms_resync_part2.xml,rmdr
component[3]=jmsDriver,m1,HA/jms_resync_part2.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - pause 2 mintes, restart standby
#----------------------------------------------------
# A1 = dead
# A2 = Primary
xml[${n}]="jms_HA_resync_sleep"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Pause for 2 minutes then restart the crashed server"
component[1]=sleep,120
component[2]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - kill primary
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jms_HA_resync_crashprimary2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Kill the primary server again"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|crashPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - rmdr3
#----------------------------------------------------
# A1 = Primary
# A2 = dead
xml[${n}]="jms_HA_resync_part3"
timeouts[${n}]=500
scenario[${n}]="${xml[${n}]} - Finish receiving messages"
component[1]=jmsDriver,m2,HA/jms_resync_part3.xml,rmdr
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - Restart standby
#----------------------------------------------------
# A1 = Primary
# A2 = dead
xml[${n}]="jms_HA_resync_restartStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Restart the standby server"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

