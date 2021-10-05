#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#  Used By: Leslie J. Rodriguez
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="Client Proxy Config Tests"

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
# Test Case 0 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="start_proxy"
scenario[${n}]="${xml[${n}]} - Test proxy configuration"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxytestconfig.cfg"
components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------
# Test Case kill proxy
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup"
scenario[${n}]="${xml[${n}]} - Kill the proxy"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} "
#
((n+=1))

xml[${n}]="configSetAndGet"
scenario[${n}]="${xml[${n}]} - Verify proxy config tests passed"
timeouts[${n}]=60
component[1]=cAppDriver,m1,"-e|./getProxyConfigLogs.sh"
component[2]=searchLogsEnd,m1,config/configSearchLogs.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#Script to pull logs to test machine scp -1_proxyroot to own machine.log and read results. 
