#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Link Aggregation - Active-Backup and LACP."

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
# Test Case 0 - Setup Link Aggregation
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="LinkAgg-setup"
RATE=120
MESSAGES=1
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
scenario[${n}]="${xml[${n}]} - Setup WAS Environment"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/makeAggregateInterface.sh","-o|A1"
#component[2]=searchLogsEnd,m1,makeAggregateInterface-setup.comparefile
#components[${n}]=" ${component[1]} ${component[2]} "
components[${n}]=" ${component[1]}  "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#----------------------------------------------------------------
# Test Case 1 - Link Aggregation Automated Execution
#----------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="Link_Agg_01"
RATE=12000
MESSAGES=1
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
scenario[${n}]="${xml[${n}]} - Link Aggregation Active-Backup(eth6-7) and LACP(eth0-1-2)"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/subscriberAggregateInterface.sh","-o|A1|1"
component[2]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/subscriberAggregateInterface.sh","-o|A1|2"
# ?sleep
component[4]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/statusAggregateInterface.sh","-o|A1|1"
component[5]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/statusAggregateInterface.sh","-o|A1|2"
component[6]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/publisherAggregateInterface.sh","-o|A1|1"
component[7]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/publisherAggregateInterface.sh","-o|A1|2"

components[${n}]="${component[1]} ${component[2]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))


#-------------------------------------------------------------------
# Test Case 2 -Link Aggregation Cleanup Env.
#-------------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="Link_Agg-cleanup"
RATE=100
MESSAGES=3600
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
scenario[${n}]="${xml[${n}]} - Link Aggregation Environment"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/removeAggregateInterface.sh","-o|A1"
#component[2]=searchLogsEnd,m1,ism-HA_JMS_RA_WAS-config-cleanup.comparefile
#components[${n}]=" ${component[1]} ${component[2]} "
components[${n}]="${component[1]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))


