#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Web Sockets Sample App"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
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
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="cmqtt_05_00"
scenario[${n}]="${xml[${n}]} - mqttsample 69 messages with Topic Filter Subscription"
#timeouts[${n}]=1000
timeouts[${n}]=800

# Set up the components for the test in the order they should start
component[1]=subController,m1,"-e|../scripts/run-scenarios.sh|05_m1.sh",,"-s|RS_IAM=subcontroller"
component[2]=subController,m2,"-e|../scripts/run-scenarios.sh|05_m2.sh",,"-s|RS_IAM=subcontroller"
component[3]=subController,m3,"-e|../scripts/run-scenarios.sh|05_m3.sh",,"-s|RS_IAM=subcontroller"
component[4]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[5]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[6]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}  "


