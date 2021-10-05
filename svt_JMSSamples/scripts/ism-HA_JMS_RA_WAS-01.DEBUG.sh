#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="HA JMS Clients using IMA Resource Adapter and WAS Ent. App."

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

#---------------------------------------------------------
# Test Case 0 - HA JMS Clients through IMA RA into WAS Setup Env.
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="HA_JMS_RA_WAS-setup"
RATE=100
MESSAGES=3600
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
#scenario[${n}]="${xml[${n}]} - Setup WAS Environment"

# Set up the components for the test in the order they should start
#component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_jms/ism-HA_JMS_RA_WAS-config.sh","-o|queue|setup"
#component[2]=searchLogsEnd,m1,ism-HA_JMS_RA_WAS-config-setup.comparefile
#components[${n}]=" ${component[1]} ${component[2]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))

#----------------------------------------------------------------
# Test Case 1 - HA JMS Clients through IMA RA into WAS Execution
#----------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="HA_JMS_RA_WAS-execution_01"
RATE=20
MESSAGES=20000000
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
scenario[${n}]="${xml[${n}]} - Running JMS HA Client using Queues to IMA RA in WAS Environment"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_jms/manualrunHAQueue","-o|5|0|487|false|DEBUG"
component[2]=cAppDriverRC,m2,"-e|${M2_TESTROOT}/svt_jms/manualrunHAQueue","-o|5|0|488|false|DEBUG"
component[3]=cAppDriverRC,m3,"-e|${M3_TESTROOT}/svt_jms/manualrunHAQueue","-o|5|0|491|false|DEBUG"
component[4]=cAppDriverRC,m4,"-e|${M4_TESTROOT}/svt_jms/manualrunHAQueue","-o|5|0|499|false|DEBUG"
component[9]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_jms/failover.sh","-o|M1;HA_JMS_RA_WAS-execution_01-cAppDriverRC-manualrunHAQueue-M1-1.log#M2;HA_JMS_RA_WAS-execution_01-cAppDriverRC-manualrunHAQueue-M2-2.log#M3;HA_JMS_RA_WAS-execution_01-cAppDriverRC-manualrunHAQueue-M3-3.log#M4;HA_JMS_RA_WAS-execution_01-cAppDriverRC-manualrunHAQueue-M4-4.log"
component[10]=searchLogsEnd,m1,manualrunHAQueue.487.comparefile
component[11]=searchLogsEnd,m2,manualrunHAQueue.488.comparefile
component[12]=searchLogsEnd,m3,manualrunHAQueue.491.comparefile
component[13]=searchLogsEnd,m4,manualrunHAQueue.499.comparefile

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))


#-------------------------------------------------------------------
# Test Case 2 - HA JMS Clients through IMA RA into WAS Cleanup Env.
#-------------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="HA_JMS_RA_WAS-cleanup"
#RATE=100
#MESSAGES=3600
#timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
#scenario[${n}]="${xml[${n}]} - Cleanup WAS Environment"

# Set up the components for the test in the order they should start
#component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_jms/ism-HA_JMS_RA_WAS-config.sh","-o|queue|cleanup"
#component[2]=searchLogsEnd,m1,ism-HA_JMS_RA_WAS-config-cleanup.comparefile
#components[${n}]=" ${component[1]} ${component[2]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))


