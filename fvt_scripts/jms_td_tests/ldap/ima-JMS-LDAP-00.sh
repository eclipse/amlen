#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS LDAP - 00"

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
# Scenario 1 - cli_setup
#----------------------------------------------------
xml[${n}]="jms_ldap_cli_setup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - LDAP Setup"
# Make sure it is stopped so that we pick up https
component[1]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|stop"
component[2]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|start"
component[3]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|setup"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|ldap/ldap_policy_config.cli"
component[5]=sleep,20
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - external_ldap_001
#----------------------------------------------------
xml[${n}]="jms_external_ldap_001"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 1 - External LDAP Test 1"
component[1]=jmsDriver,m1,ldap/external_ldap_001.xml,user1
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - external_ldap_002
#----------------------------------------------------
xml[${n}]="jms_external_ldap_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 2 - Crash LDAP and expect connect failure"
component[1]=cAppDriverLog,m1,"-e|./ldap/ldap.sh","-o|-a|stop"
component[2]=sleep,3
component[3]=jmsDriver,m1,ldap/external_ldap_002.xml,user2Fail
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - external_ldap_003
#----------------------------------------------------
xml[${n}]="jms_external_ldap_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 3 - Restart LDAP and expect connect success"
component[1]=cAppDriverLog,m1,"-e|./ldap/ldap.sh","-o|-a|start"
component[2]=sleep,10
component[3]=jmsDriver,m1,ldap/external_ldap_002.xml,user2
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - external_ldap_004
#----------------------------------------------------
xml[${n}]="jms_external_ldap_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 4 - Nested Groups true"
component[1]=jmsDriver,m1,ldap/external_ldap_003.xml,user3
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - external_ldap_005
#----------------------------------------------------
xml[${n}]="jms_external_ldap_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 5 - UserIdMap mail and Nested Groups True"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|updateUserIdMap|-c|ldap/ldap_policy_config.cli"
component[2]=jmsDriver,m1,ldap/external_ldap_002.xml,user2_mail
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - external_ldap_006
#----------------------------------------------------
xml[${n}]="jms_external_ldap_006"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 6 - UserIdMap mail and Nested Groups False"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disableNested|-c|ldap/ldap_policy_config.cli"
component[2]=jmsDriver,m1,ldap/external_ldap_003.xml,user3Fail_mail
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - external_ldap_007
#----------------------------------------------------
xml[${n}]="jms_external_ldap_007"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 7 - Nested Group Search False move user001"
component[1]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|update|-o|group001|-g|group002|-u|user001"
component[2]=jmsDriver,m1,ldap/external_ldap_001.xml,user1Fail_mail
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - external_ldap_008
#----------------------------------------------------
xml[${n}]="jms_external_ldap_008"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 8 - Nested Group Search True"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enableNested|-c|ldap/ldap_policy_config.cli"
component[2]=jmsDriver,m1,ldap/external_ldap_001.xml,user1_mail
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - external_ldap_009
#----------------------------------------------------
xml[${n}]="jms_external_ldap_009"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 9 - Fail with invalid password"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetUserIdMap|-c|ldap/ldap_policy_config.cli"
component[2]=jmsDriver,m1,ldap/external_ldap_002.xml,user2Fail_password
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - cli_cleanup
#----------------------------------------------------
xml[${n}]="jms_ldap_cli_cleanup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - LDAP Cleanup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|ldap/ldap_policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if ! [[ "${A1_TYPE}" =~ "VMWARE_windows" ]] ; then
	#----------------------------------------------------
	# Scenario x - cli_setup_ssl
	#----------------------------------------------------
	xml[${n}]="jms_ldap_cli_setup_ssl"
	timeouts[${n}]=180
	scenario[${n}]="${xml[${n}]} - LDAP setup ssl"
	# cleanup does a server/restart imaserver so wait a bit to make sure server is back up before continuing
	component[1]=sleep,20
	component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|sslSetup|-c|ldap/ldap_policy_config.cli"
	component[3]=sleep,20
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
	((n+=1))

	#----------------------------------------------------
	# Scenario 2 - external_ldap_ssl_001
	#----------------------------------------------------
	xml[${n}]="jms_external_ldap_ssl_001"
	timeouts[${n}]=90
	scenario[${n}]="${xml[${n}]} - Test 1 (SSL VERSION) - External LDAP Test 1 (SSL VERSION) "
	component[1]=jmsDriver,m1,ldap/external_ldap_ssl_001.xml,user1
	components[${n}]="${component[1]}"
	((n+=1))

	#----------------------------------------------------
	# Scenario x - cli_cleanup_ssl
	#----------------------------------------------------
	xml[${n}]="jms_ldap_cli_cleanup_ssl"
	timeouts[${n}]=180
	scenario[${n}]="${xml[${n}]} - LDAP Cleanup ssl"
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|ldap/ldap_policy_config.cli"
	component[2]=sleep,20
	components[${n}]="${component[1]} ${component[2]}"
	((n+=1))
fi

#----------------------------------------------------
# Scenario x - stop ldap
#----------------------------------------------------
xml[${n}]="jms_ldap_stop"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - LDAP Stop"
component[1]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|cleanup"
component[2]=cAppDriverLogWait,m1,"-e|./ldap/ldap.sh","-o|-a|stop"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
