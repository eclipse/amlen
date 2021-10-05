#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="PROXY TLS via WSTestDriver"

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
# Test Case 0 - proxy_tls00
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="start_proxy"
scenario[${n}]="${xml[${n}]} - Setup for TLS tests - configure policies and start proxy"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
# Start the proxy
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxyTLS.cfg"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|tls_policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - testproxy_tls01
#----------------------------------------------------
xml[${n}]="testproxy_tls01_NoStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port with no store and self signed server cert - conn should fail [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_basic/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - testproxy_tls02
#----------------------------------------------------
xml[${n}]="testproxy_tls02_WrongStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port with wrong cert in store for self signed server cert - conn should fail [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_basic/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 3 - testproxy_tls03
#----------------------------------------------------
xml[${n}]="testproxy_tls03_MqttsEpStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_basic/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 4 - proxy_tls04
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tls04_MqttsEpStore_WithUser"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port with a user [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_basic/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="proxy_tls_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup for ism-proxy_td-tlsBasicScenarios01 "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|tls_policy_config.cli"
component[2]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

