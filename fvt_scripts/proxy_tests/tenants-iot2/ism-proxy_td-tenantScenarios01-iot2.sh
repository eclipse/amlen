#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="ISM MQTT via WSTestDriver"
scenario_directory="tenants-iot2"
scenario_file="ism-proxy_td-tenantScenarios01-iot2.sh"
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

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="proxy_tenants-iot2_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP ${scenario_at}"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - start_proxy
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="start_proxy"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket connect to an IP address and port ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./startProxy.sh","-o|proxy-iot2.cfg"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# Test Case 1 - testproxy_tenant01-iot2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant01-iot2"
scenario[${n}]="${xml[${n}]} - Connect a device on IoT2 requiring user/password, test that no user/password fails, test that org name is properly added in topic sent on to MessageSight"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - testproxy_tenant02-iot2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant02-iot2"
scenario[${n}]="${xml[${n}]} - Connect an application on IoT2 requiring user/password, test that tenant name is properly added in topic sent on to MessageSight, test that tenant not in cfg files fails"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - testproxy_tenant03-quickstart2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant03-quickstart2"
scenario[${n}]="${xml[${n}]} - Connect a device on Quickstart2 requiring no user/password, test that tenant name is properly added in topic sent on to MessageSight"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case 4 - testproxy_tenant04-quickstart2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant04-quickstart2"
scenario[${n}]="${xml[${n}]} - Connect an application on Quickstart2 requiring no user/password, test that tenant name is properly added in topic sent on to MessageSight"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case 5 - testproxy_tenant05-quickstart2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant05-quickstart2"
scenario[${n}]="${xml[${n}]} - Connect application and device on quickstart2, only device publish and only application subscribe"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case 6 - testproxy_tenant06-quickstart2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant06-quickstart2"
scenario[${n}]="${xml[${n}]} - Connect application quickstart2, check QuickStart Rules"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case 7 - testproxy_tenant07-iot2
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testproxy_tenant07-iot2"
scenario[${n}]="${xml[${n}]} - Connect device on iot2, check Registered Org rules"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,tenants-iot2/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case clean up
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="cleanup"
scenario[${n}]="${xml[${n}]} - Kill the proxy ${scenario_at}"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|./killProxy.sh"
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup ${scenario_at}"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="proxy_tenants-iot2_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean ${scenario_at}"
    timeouts[${n}]=20
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

