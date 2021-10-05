#! /bin/bash

#--------------------------------------------------------------
#  This script defines the scenarios for the JSON Plugin tests.
#--------------------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM JSON Plugin Setup"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case runningon the target machine.
#   <machineNumber_ismSetup>
#		m1 is the machine 1 in ismSetup, m2 is the machine 2 and so on...
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
# Test Case 0 - install_plugin
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="install_plugin"
scenario[${n}]="${xml[${n}]} - Install the json plug-in"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./pluginFunctions.sh","-o|-a|install|-f|installPlugin.log|-p|jsonplugin.zip|-n|json_msg"
component[2]=searchLogsEnd,m1,installPlugin.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - update_plugin
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="update_plugin"
scenario[${n}]="${xml[${n}]} - Update the json plug-in"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./pluginFunctions.sh","-o|-a|update|-f|updatePlugin.log|-n|json_msg|-c|jsonplugin.json"
component[2]=searchLogsEnd,m1,updatePlugin.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

