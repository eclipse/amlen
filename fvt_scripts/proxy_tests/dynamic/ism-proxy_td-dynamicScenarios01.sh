#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#   Used By: Leslie J. Rodriguez
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
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxydynamic.cfg|users.json"
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - testproxy_dynamic01
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_dynamic01"
scenario[${n}]="${xml[${n}]} - Test that dynamic authentication doesn't override normal"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,dynamic/${xml[${n}]}.xml,ALL,-o_-v_4
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - testproxy_dynamic02
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_dynamic02"
scenario[${n}]="${xml[${n}]} - Test that dynamic authentication does add to normal"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,dynamic/${xml[${n}]}.xml,ALL,-o_-v_4
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - testproxy_dynamic03
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_dynamic03"
scenario[${n}]="${xml[${n}]} - Test that dynamic authentication with simultaneous requests"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,dynamic/${xml[${n}]}.xml,ALL,-o_-v_4_-l_7
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - testproxy_dynamic04
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_dynamic04"
scenario[${n}]="${xml[${n}]} - Test that dynamic authentication can fail connect"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,dynamic/${xml[${n}]}.xml,ALL,-o_-v_4
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 5 - testproxy_dynamic05
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_dynamic05"
scenario[${n}]="${xml[${n}]} - Test that dynamic authentication can access IP of client"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,dynamic/${xml[${n}]}.xml,ALL,-o_-v_4
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

#Script to pull logs to test machine scp -1_proxyroot to own machine.log and read results. 
