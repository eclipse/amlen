#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="TODO!:  replace with a description of what this test bucket tests"

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
# Test Case 0 - 
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="ISM_TestCase-0_Name"
scenario[${n}]="${xml[${n}]} - TODO!:  Test Case #0 Description"

# Set up the components for the test in the order they should start
component[1]=testDriver,m1,testDriver1.cfg
component[2]=testDriver,m6,testDriver6.cfg,-o_-x_Sheila_-y_otherOpts
component[3]=sync,m2
component[4]=sleep,5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

#component[5]=killComponent,m3,kill.cfg
#component[6]=runCommand,m4,runCommand.cfg
#component[7]=jmsDriver,m5,jmsDriver.cfg
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The name of the test case and the prefix of the XML configuration file for the driver
#xml[${n}]="ISM_TestCase-1_Name"
#scenario[${n}]="${xml[${n}]} - TODO!:  Test Case #1 Description"

### Set up the components for the test in the order they should start
#component[1]=toolController,machine,tool.cfg
#component[2]=toolController,machine,tool.cfg
#component[3]=toolController,machine,tool.cfg
#component[4]=toolController,machine,tool.cfg,-o|-x|paramXval|-y|paramYval
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
