#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="mqtt HAScenarios01"

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
# Test Case 0 - mqtt_HA01
#----------------------------------------------------
xml[${n}]="testmqtt_HApopulate"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - Test that messages in store before HA switchover are still there [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m2,../jms_td_tests/HA/jms_HA_005.xml,rmdr
component[3]=jmsDriver,m1,../jms_td_tests/HA/jms_HA_005.xml,rmdr2
component[4]=wsDriver,m1,HA/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_HA01
#----------------------------------------------------
xml[${n}]="testmqtt_HA01"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Test that message in store are available on secondary after failover [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,receive,-o_-l_7
component[4]=runCommand,m1,../common/serverKill.sh,1,testmqtt_HA01
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_HA03
#----------------------------------------------------
xml[${n}]="testmqtt_HA03"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Test that original primary comes back up and becomes primary after new one fails [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,receive,-o_-l_7
component[4]=runCommand,m1,../common/serverRestart1Kill2.sh,1,testmqtt_HA03
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 script to re-ready the killed server - temporary
#----------------------------------------------------
xml[${n}]="testmqtt_HA01StartStandby"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Start standby, back to original HA config "
component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.py","-o|-a|startStandby|-f|testmqtt_HA01StartStandby.log"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_HA08 - multiple publishers RM same topic then full restart
#----------------------------------------------------
xml[${n}]="testmqtt_HA08"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - send RM from multiple publishers on same topic then restart both servers [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriverWait,m1,HA/${xml[${n}]}.xml,publish1
component[3]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|verifyStatus|-status|STATUS_RUNNING|-t|30|-m1|1"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|verifyStatus|-status|STATUS_STANDBY|-t|30|-m1|2"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}" 
((n+=1))


#----------------------------------------------------
# Test Case 3 - mqtt_HA03
#----------------------------------------------------
xml[${n}]="testmqtt_HA06"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Test that original primary comes back up and becomes primary after new one fails [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,publish,-o_-l_7
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,receive,-o_-l_7
component[4]=runCommand,m1,../common/serverKill.sh,1,testmqtt_HA06
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


#----------------------------------------------------
# Test Case 4 script to re-ready the killed server - temporary
#----------------------------------------------------
xml[${n}]="testmqtt_HA01Cleanup"
timeouts[${n}]=1200
scenario[${n}]="${xml[${n}]} - Reset HA to preferred primary as primary [ repairA1n2.sh ]}.xml ]"
component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-f|testmqtt_HA01disableHA.log"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 5 script to re-ready the killed server - temporary
#----------------------------------------------------
xml[${n}]="testmqtt_HA01Cleanup1"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Reset HA to preferred primary as primary [ repairA1n2.sh ]}.xml ]"
component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA|-f|testmqtt_HA01setupHA.log"
components[${n}]="${component[1]}"
((n+=1))


