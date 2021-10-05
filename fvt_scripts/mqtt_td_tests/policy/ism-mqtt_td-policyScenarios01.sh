#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT Policy via WSTestDriver"

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
    xml[${n}]="mqtt_policy_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - testmqtt_policy01
#----------------------------------------------------
xml[${n}]="testmqtt_policy01"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to publish and subscribe to authorized topic"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace|-c|policy_config.cli"
component[3]=sync,m1
component[4]=wsDriver,m1,policy/${xml[${n}]}.xml,publish,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m1,policy/${xml[${n}]}.xml,receive,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - testmqtt_policy03
#----------------------------------------------------
xml[${n}]="testmqtt_policy03"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test ability to subscribe to authorized topic as single level wildcard"
component[1]=sync,m1
component[2]=wsDriver,m1,policy/${xml[${n}]}.xml,publish,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,policy/${xml[${n}]}.xml,receive,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,policy/${xml[${n}]}.xml,receive2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


#----------------------------------------------------
# Test Case 2 - testmqtt_policy04
#----------------------------------------------------
if [[ "${A1_LOCATION}" == "remote" ]] ; then

	#  Skip the JMS SharedSubs bucket on Remote Servers.
	echo "  Skipping testmqtt_policy04 on this setup, which doesn't support IPv6"

else 

	xml[${n}]="testmqtt_policy04"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test ability to subscribe to authorized topic. (IPv6 address is used in TopicPolicy to exclude some topics)."
	component[1]=sync,m1
	component[2]=wsDriver,m1,policy/${xml[${n}]}.xml,publish,-o_-l_9,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
	component[3]=wsDriver,m1,policy/${xml[${n}]}.xml,receive,-o_-l_9,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
	component[4]=wsDriver,m1,policy/${xml[${n}]}.xml,receive2,-o_-l_9,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
	
fi	

#----------------------------------------------------
# Test Case 3 - testmqtt_policy05
#----------------------------------------------------
xml[${n}]="testmqtt_policy05"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test connection which requires user/password"
component[1]=wsDriver,m1,policy/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 4 - testmqtt_policy06
#----------------------------------------------------
xml[${n}]="testmqtt_policy06"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Turn off requirement for user/password"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|turnoff|-c|policy_config.cli"
component[2]=wsDriver,m1,policy/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 5 - testmqtt_policy07
#----------------------------------------------------
xml[${n}]="testmqtt_policy07"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Turn back on requirement for user/password"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|turnon|-c|policy_config.cli"
component[2]=wsDriver,m1,policy/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 6 - testmqtt_policy08
#----------------------------------------------------
xml[${n}]="testmqtt_policy08"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test ClientID variable in messaging policy destination"
component[1]=wsDriver,m1,policy/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 7 - testmqtt_policy09
#----------------------------------------------------
xml[${n}]="testmqtt_policy09"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test UserID variable in messaging policy destination"
component[1]=wsDriver,m1,policy/${xml[${n}]}.xml,ALL,-o_-l_9,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="mqtt_policy_cleanup"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Cleanup in mqtt policy test bucket"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1
    #----------------------------------------------------
    xml[${n}]="mqtt_policy_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
