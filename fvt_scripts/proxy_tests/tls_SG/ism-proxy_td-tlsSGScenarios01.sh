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
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxyTLSSNI.cfg"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|tls_policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - proxy_tls01
#----------------------------------------------------
xml[${n}]="testproxy_tls01_NoTLS"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test that non-TLS connections succeed for SGEnabled=true configuration [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 2 - proxy_tls02
#----------------------------------------------------
xml[${n}]="testproxy_tls02_NoStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections fail for SGEnabled=true config when there is no client cert store [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 3 - proxy_tls03
#----------------------------------------------------
xml[${n}]="testproxy_tls03_MqttsEpStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=true config when client cert store contains mqtts ep cert only [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 4 - proxy_tls04
#----------------------------------------------------
xml[${n}]="testproxy_tls04_ProxyDfltStore"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections fail for SGEnabled=true config when client cert store contains dflt proxy cert only [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 5 - proxy_tls05
#----------------------------------------------------
xml[${n}]="testproxy_tls05_ProxyDfltStore_CltCrtNoMatch"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=true config when client cert store contains dflt proxy cert and non-matching client cert. Also test connections fail when partial or exact match betw CN/SAN & clientId is required. [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 6 - proxy_tls06
#----------------------------------------------------
xml[${n}]="testproxy_tls06_ProxyDfltStore_CltCrtPartialMatchd"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=true config when client cert store contains dflt proxy cert and CN partial matching client cert. Also test connections succeed when partial match betw CN/SAN & clientId is required and fails when exact match is required. [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 7 - proxy_tls07
#----------------------------------------------------
xml[${n}]="testproxy_tls07_ProxyDfltStore_CltCrtExactMatchd"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=true config when client cert store contains dflt proxy cert and SAN exact matching client cert. Also check that connections succeed when partial or exact match betw CN/SAN & clientId is required. [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 8 - proxy_tls08
#----------------------------------------------------
xml[${n}]="testproxy_tls08_ProxyDfltStore_CltCrtPartialMatchg"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=true config when client cert store contains dflt proxy cert and SAN partial matching client cert. Also check that connections succeed when partial match betw CN/SAN & clientId is required and failes when exact match is required. [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 9 - proxy_tls09
#----------------------------------------------------
xml[${n}]="testproxy_tls09_ProxyDfltStore_CltCrtExactMatchg"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test TLS connections succeed for SGEnabled=false config when client cert store contains dflt proxy cert and CN exact matching client cert. Also check that connections succeed when partial or exact match betw CN/SAN & clientId is required. [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_SG/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case clean up
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup"
scenario[${n}]="${xml[${n}]} - Kill the proxy"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=sleep,5
component[2]=cAppDriver,m1,"-e|./killProxy.sh"
component[3]=sleep,5
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="proxy_tls_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup for ism-proxy_td-tlsScenarios01 "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|tls_policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

