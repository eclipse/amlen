#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM MQTT via WSTestDriver"
scenario_directory="publish"
scenario_file="ism-proxy_td-publishScenarios01.sh"
scenario_at=" "+${scenario_directory}+"/"+${scenario_file}

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
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./startProxy.sh"
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_publish05"
scenario[${n}]="${xml[${n}]} - Test RETAIN, simple scenario ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,publish/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,publish/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_publish06"
scenario[${n}]="${xml[${n}]} - Test RETAIN, second message replaces first ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,publish/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,publish/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_publish07"
scenario[${n}]="${xml[${n}]} - Test RETAIN, if already subscribed, RETAIN flag should be off ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,publish/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,publish/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_publish08"
scenario[${n}]="${xml[${n}]} - Test RETAIN, if already subscribed/ durable, RETAIN flag should be off ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,publish/${xml[${n}]}.xml,publish
component[3]=wsDriver,m1,publish/${xml[${n}]}.xml,receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#----------------------------------------------------
# Test Case clean up
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup"
scenario[${n}]="${xml[${n}]} - Cleanup ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} "

((n+=1))
