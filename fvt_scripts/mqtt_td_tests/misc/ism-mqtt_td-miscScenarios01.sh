#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT Misc via WSTestDriver"

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
    xml[${n}]="mqtt_misc_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_gvt01
#----------------------------------------------------
xml[${n}]="testmqtt_gvt01"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability use GVT characters in topic and ClientID [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
component[2]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_gvt03
#----------------------------------------------------
xml[${n}]="testmqtt_gvt03"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect with GVT characters in user/password [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|ADD_USERS|-c|../common/cli_gvt_user_and_group_test.cli"
component[2]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

# Next four not in userIDs yet.
#----------------------------------------------------
# Test Case 2 - mqtt_gvt05
#----------------------------------------------------
xml[${n}]="testmqtt_gvt05"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect with GVT characters C4 in user/password [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_gvt06
#----------------------------------------------------
xml[${n}]="testmqtt_gvt06"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect with GVT characters E3 in user/password [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 6 - mqtt_gvt02
#----------------------------------------------------
xml[${n}]="testmqtt_gvt02"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test ability to connect over an SSL connection [ ${xml[${n}]}.xml ]"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|DELETE_USERS|-c|../common/cli_gvt_user_and_group_test.cli"
component[2]=sync,m1
component[3]=wsDriverNocheck,m1,misc/${xml[${n}]}.xml,publish,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,misc/${xml[${n}]}.xml,receive,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=killComponent,m1,3,1,testmqtt_gvt02
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case 7 - testmqtt_connpolicy01
#----------------------------------------------------
xml[${n}]="testmqtt_connpolicy01"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test that MQTT protocol must be available on Connection policy [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 8 - testmqtt_connpolicy02
#----------------------------------------------------
xml[${n}]="testmqtt_connpolicy02"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test that MQTT protocol must be available on Endpoint policyn [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - testmqtt_restart01
#----------------------------------------------------
xml[${n}]="testmqtt_restart01"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - Test that messages in flight survive a server restart"
component[1]=sync,m1
component[2]=wsDriver,m2,misc/${xml[${n}]}.xml,receive,-o_-l_9
component[3]=wsDriver,m1,misc/${xml[${n}]}.xml,publish
component[4]=runCommand,m1,../common/serverRestart.sh,1,testmqtt_restart01
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - testmqtt_stat01
#----------------------------------------------------
xml[${n}]="testmqtt_stat01"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test that stat MQTTClient shows correct clients"
component[1]=sync,m1
component[2]=wsDriver,m1,misc/${xml[${n}]}.xml,receive,-o_-l_9
component[3]=runCommand,m1,../common/qStatMQTTClient01.sh,1,testmqtt_stat01
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - testmqtt_stat02
#----------------------------------------------------
xml[${n}]="testmqtt_stat02"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test that stat MQTTClient shows correct clients"
component[1]=sync,m1
component[2]=wsDriver,m1,misc/${xml[${n}]}.xml,receive,-o_-l_9
component[3]=runCommand,m1,../common/qStatMQTTClient02.sh,1,testmqtt_stat02
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - testmqtt_dcn01
#----------------------------------------------------
xml[${n}]="testmqtt_dcn01"
timeouts[${n}]=450
scenario[${n}]="${xml[${n}]} - Test using DisconnectedClientNotification 1"
component[1]=wsDriverWait,m1,misc/${xml[${n}]}.xml,ALL
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|dynamicDCNupdate|-c|policy_config.cli"
component[3]=searchLogsEnd,m1,misc/testmqtt_dcn01.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - testmqtt_dcn02
#----------------------------------------------------
xml[${n}]="testmqtt_dcn02"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test using DisconnectedClientNotification 2"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - testmqtt_longpasswd01
#----------------------------------------------------
xml[${n}]="testmqtt_longpasswd01"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test Long Username rejected"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupXXL|-c|policy_config.cli"
component[2]=wsDriverWait,m1,misc/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 15 - testmqtt_longpasswd02
#----------------------------------------------------
xml[${n}]="testmqtt_longpasswd02"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test Long Username wildcarded"
component[1]=wsDriver,m1,misc/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Cleanup XXL policies
#----------------------------------------------------
xml[${n}]="mqtt_misc_cleanup_xxl"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Cleanup XXL Policies (in misc test bucket)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupXXL|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="mqtt_misc_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup in misc test bucket"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1
    #----------------------------------------------------
    xml[${n}]="mqtt_misc__M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
