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
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxyTLSSNI_chain.cfg|tls_certs/chainrevcrl0.crl"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|tls_policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 01 - proxy_tls01
#----------------------------------------------------
xml[${n}]="testproxy_tls01_ClientCertExpired"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test connection to Proxy TLS port fails when using an expired client certificate for client cert authentication [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))


#----------------------------------------------------
# Test Case 02 - proxy_tls02
#----------------------------------------------------
xml[${n}]="testproxy_tls02_ClientCertWrongSigner" 
timeouts[${n}]=30 
scenario[${n}]="${xml[${n}]} - Test connect to Proxy TLS port fails using a client certificate that is signed by a different CA [ ${xml[${n}]}.xml ]" 
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 03 - proxy_tls03
#----------------------------------------------------
xml[${n}]="testproxy_tls03_ClientCertCNMatchWrongSigner" 
timeouts[${n}]=30 
scenario[${n}]="${xml[${n}]} - Test connect to Proxy TLS fails using a client certificate CN matching client ID but signed by a different CA [ ${xml[${n}]}.xml ]" 
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 04 - proxy_tls04
#----------------------------------------------------
xml[${n}]="testproxy_tls04_ClientCertSANMatchWrongSigner" 
timeouts[${n}]=30 
scenario[${n}]="${xml[${n}]} - Test connect to Proxy TLS fails using a client certificate SAN matching client ID but signed by a different CA [ ${xml[${n}]}.xml ]" 
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))



#----------------------------------------------------
# Test Case 05b - proxy_tls05b
#----------------------------------------------------
xml[${n}]="testproxy_tls05b_ClientCertEmptyCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" CRL (cert 2 of 3) [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 05c - proxy_tls05c
#----------------------------------------------------
xml[${n}]="testproxy_tls05c_ClientCertEmptyCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" CRL (cert 3 of 3) [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))


#----------------------------------------------------
# Test Case 05a - proxy_tls05a
#----------------------------------------------------
xml[${n}]="testproxy_tls05a_ClientCertEmptyCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" CRL (cert 1 of 3) [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 06a - proxy_tls06a
#----------------------------------------------------
xml[${n}]="testproxy_tls06a_ClientCert2CrtsInCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with CRL and 2 reovoked certs - conn for revoked cert should fail [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverWait,m1,"-e|./copyCRL.sh","-o|tls_certs/chainrevcrl2.crl|/tmp/myca.crl"
component[2]=sleep,65
component[3]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 06b - proxy_tls06b
#----------------------------------------------------
xml[${n}]="testproxy_tls06b_ClientCert2CrtsInCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with CRL and 2 reovoked certs - conn for revoked cert should fail [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 06c - proxy_tls06c
#----------------------------------------------------
xml[${n}]="testproxy_tls06c_ClientCert2CrtsInCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with CRL and 2 reovoked certs - conn for non-revoked cert should succeed [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 07a - proxy_tls07a
#----------------------------------------------------
xml[${n}]="testproxy_tls07a_ClientCertEmptyDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" DER format CRL (cert 1 of 3) [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverWait,m1,"-e|./copyCRL.sh","-o|tls_certs/chainrevcrl0.der|/tmp/myca.crl"
component[2]=sleep,65
component[3]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 07b - proxy_tls07b
#----------------------------------------------------
xml[${n}]="testproxy_tls07b_ClientCertEmptyDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" DER format CRL (cert 2 of 3) [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 07c - proxy_tls07c
#----------------------------------------------------
xml[${n}]="testproxy_tls07c_ClientCertEmptyDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication and "empty" DER format CRL (cert 3 of 3) [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Test Case 08a - proxy_tls08a
#----------------------------------------------------
xml[${n}]="testproxy_tls08a_ClientCert1CrtInDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with DER format CRL and 1 reovoked cert - conn for revoked cert should fail [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverWait,m1,"-e|./copyCRL.sh","-o|tls_certs/chainrevcrl1.der|/tmp/myca.crl"
component[2]=sleep,65
component[3]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 08b - proxy_tls08b
#----------------------------------------------------
xml[${n}]="testproxy_tls08b_ClientCert1CrtInDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with DER format CRL and 1 reovoked cert - conn should succeed for cert that is not revoked [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 08c - proxy_tls08c
#----------------------------------------------------
xml[${n}]="testproxy_tls08c_ClientCert1CrtInDERCRL"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to connect to Proxy TLS port using client certificate authentication with DER format CRL and 1 reovoked cert - conn should succeed for cert that is not revoked [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,tls_invalidCerts/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
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

