#! /bin/bash

# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

# Set the name of this set of scenarios

scenario_set_name="ISM MQTT via WSTestDriver"
scenario_directory="monitor"
scenario_file="ism-proxy_td-monitorScenarios01.sh"
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
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port ${scenario_at}}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
# Start two proxies for multi-proxy tests
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxyProt3.cfg"
component[2]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxy2Prot3.cfg"
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_monitor01"
scenario[${n}]="${xml[${n}]} - Connect through tenant requiring user/password, test that no user/password fails, test that tenant name is properly added in topic sent on to MessageSight"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,monitor/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_monitor02"
scenario[${n}]="${xml[${n}]} - Connect through tenant requiring user/password, test that no user/password fails, test that tenant name is properly added in topic sent on to MessageSight, test that tenant not in cfg files fails"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,monitor/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_monitor03_MultiProxyStealCID"
scenario[${n}]="${xml[${n}]} - Test steals of the same client ID between two proxies ${scenario_at}"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,monitor/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_ws1
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_monitor04_MultiProxyConnDisconn"
scenario[${n}]="${xml[${n}]} - Test rapid connects/disconnects of the same client ID between two proxies"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,monitor/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case clean up
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup_confirmDisconnect"
scenario[${n}]="${xml[${n}]} - Kill the proxy and confirm final retained message is disconnect ${scenario_at}"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,monitor/${xml[${n}]}.xml,ALL
component[2]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} ${component[2]}"

((n+=1))
