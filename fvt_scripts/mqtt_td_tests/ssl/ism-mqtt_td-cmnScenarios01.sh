#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT SSL CMN via WSTestDriver"

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
    xml[${n}]="mqtt_ssl_cmn_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_ssl05
#----------------------------------------------------
xml[${n}]="mqtt_ssl_cmn_setup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect over an SSL connection [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupcmn|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverStopRestart.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


#----------------------------------------------------
# Test Case 1 - mqtt_ssl05
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl05"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,${xml[${n}]}.xml,ALL,-o_-l_9
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_ssl06
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl06"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_ssl07
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl07"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 - mqtt_ssl08
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl08"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage in ClientID on ConnectionPolicy with asterisk character"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 5 - mqtt_ssl09a
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl09a"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage in ClientID on ConnectionPolicy with asterisk character"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 6 - mqtt_ssl09b
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="ssl/testmqtt_ssl09b"
scenario[${n}]="${xml[${n}]} - Test MQTT/WebSocket CommonName usage in ClientID on ConnectionPolicy with asterisk character"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|updatecmn|-c|policy_config.cli"
component[2]=wsDriver,m1,${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]} "

((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="mqtt_ssl_cmn_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup for ism-mqtt_td-cmnScenarios01 "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupcmn|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverStopRestart.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="mqtt_ssl_cmn_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
