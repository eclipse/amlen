#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS OAuth - 00"

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
# Scenario 0 - cli_setup
#----------------------------------------------------
xml[${n}]="jms_oauth_cli_setup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - OAuth CLI Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupLDAP|-c|oauth/oauth_policy_config.cli"
component[2]=sleep,20
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|oauth/oauth_policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 0 - jms_oauth_001
# UserInfoURL set, username=admin
# Policy does not verify user/group membership
#----------------------------------------------------
xml[${n}]="jms_oauth_001"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 1 - OAuth Test 1"
component[1]=jmsDriver,m1,oauth/jms_oauth_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_oauth_002
# UserInfoURL set, username=LTPAUser3
# Allowed by group membership in connection policy
#----------------------------------------------------
xml[${n}]="jms_oauth_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 2 - OAuth Test 2"
component[1]=jmsDriver,m1,oauth/jms_oauth_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_oauth_003
# UserInfoURL set, username=LTPAUser1
# Allowed by username in connection policy
#----------------------------------------------------
xml[${n}]="jms_oauth_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 3 - OAuth Test 3"
component[1]=jmsDriver,m1,oauth/jms_oauth_003.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_oauth_004
# UserInfoURL set, username=LTPAUser2
# Not allowed by username or group membership
# based on connection policies
# Also test wrong OAUTH username
#----------------------------------------------------
xml[${n}]="jms_oauth_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 4 - OAuth Test 4"
component[1]=jmsDriver,m1,oauth/jms_oauth_004.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_oauth_005
# Error testing
# Modify each value on OAuthProfile and expect
# failures
# Mismatched Auth Key
# Mismatched User Info Key
# Incorrect ResourceURL
# Incorrect UserInfoURL
#----------------------------------------------------
xml[${n}]="jms_oauth_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 5 - OAuth Test 5"
component[1]=jmsDriver,m1,oauth/jms_oauth_005.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_oauth_006
# Anonymous OAuth. No UserInfoURL
#----------------------------------------------------
xml[${n}]="jms_oauth_006"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 6 - OAuth Test 6"
component[1]=jmsDriver,m1,oauth/jms_oauth_006.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_oauth_notls
#----------------------------------------------------
xml[${n}]="jms_oauth_notls"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Test notls - OAuth Test with no TLS"
component[1]=jmsDriver,m1,oauth/jms_oauth_notls.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - cli_disable LDAP
# Update OAuthProfile and OAuthProfileCert to have
# GroupInfoKey set to "group"
# And disable LDAP
#----------------------------------------------------
xml[${n}]="jms_oauth_cli_disable_ldap"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - OAuth CLI Disable LDAP"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disableLDAP|-c|oauth/oauth_policy_config.cli"
component[2]=sleep,20
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 1b - jms_oauth_001_groupinfokey
# Username/Group membership not checked.
#----------------------------------------------------
xml[${n}]="jms_oauth_001_groupinfokey"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Test 1 - OAuth Test 1 with GroupInfoKey"
component[1]=jmsDriver,m1,oauth/jms_oauth_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2b - jms_oauth_002_groupinfokey
# Group membership should be allowed for LTPAUser3
# based on connection policy
#----------------------------------------------------
xml[${n}]="jms_oauth_002_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 2 - OAuth Test 2 with GroupInfoKey"
component[1]=jmsDriver,m1,oauth/jms_oauth_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3b - jms_oauth_003_groupinfokey
# User should be allowed for LTPAUser1
#----------------------------------------------------
xml[${n}]="jms_oauth_003_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 3 - OAuth Test 3 with GroupInfoKey"
component[1]=jmsDriver,m1,oauth/jms_oauth_003.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4b - jms_oauth_007_groupinfokey
# Allowed by group membership list (LTPAGroup4)
# for LTPAUser2
#----------------------------------------------------
xml[${n}]="jms_oauth_007_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 4 - OAuth Test 7 with GroupInfoKey group list"
component[1]=jmsDriver,m1,oauth/jms_oauth_007.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5b - jms_oauth_005_groupinfokey
# Test mismatched GroupInfoKey
#----------------------------------------------------
xml[${n}]="jms_oauth_005_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 5 - OAuth Test 5 with GroupInfoKey"
component[1]=jmsDriver,m1,oauth/jms_oauth_005.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - cli_same_url
# Update OAuthProfile and OAuthProfileCert to have
# matching ResourceURL and UserInfoURL
#----------------------------------------------------
xml[${n}]="jms_oauth_cli_same_url"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - OAuth CLI SameURL"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|sameURL|-c|oauth/oauth_policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2c - jms_oauth_002_samrURL
# Group membership should be allowed for LTPAUser3
# based on connection policy
#----------------------------------------------------
xml[${n}]="jms_oauth_002_sameURL"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 2 - OAuth Test 2 with GroupInfoKey sameURL"
component[1]=jmsDriver,m1,oauth/jms_oauth_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_oauth_008_groupinfokey
# Test , in group name GroupInfoKey
#----------------------------------------------------
xml[${n}]="jms_oauth_008_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 8 - OAuth Test 8 with GroupInfoKey"
component[1]=jmsDriver,m1,oauth/jms_oauth_008.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - jms_oauth_009_groupinfokey
# Test , in group name GroupInfoKey with ${GroupID}
# in messaging policy
#----------------------------------------------------
xml[${n}]="jms_oauth_009_groupinfokey"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 9 - OAuth Test 9 with GroupInfoKey and ${GroupID}"
component[1]=jmsDriver,m1,oauth/jms_oauth_009.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jms_oauth_010_truststore
# Test JMS Oauth with TrustStore setting
# in messaging policy
#----------------------------------------------------
xml[${n}]="jms_oauth_010_truststore"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 10 - OAuth Test 10 with TrustStore set"
component[1]=jmsDriver,m1,oauth/jms_oauth_010.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - cli_cleanup
#----------------------------------------------------
xml[${n}]="jms_oauth_cli_cleanup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - OAuth CLI Cleanup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|oauth/oauth_policy_config.cli"
component[2]=sleep,20
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
