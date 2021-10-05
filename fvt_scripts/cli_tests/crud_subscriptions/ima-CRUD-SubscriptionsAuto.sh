#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CLI Create update delete (CRUD) tests for Subscriptions"

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
# cli_crud_subscriptions_cleanup_001
#----------------------------------------------------
xml[${n}]="cli_crud_subscriptions_cleanup_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup configuration objects used in Subscription CRUD tests"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|crud_subscriptions/crud_subscriptions_tests.cli"
#component[1]=cAppDriverRC,m1,"-e|mocha","-o|test/crud_scriptions/crud_subscriptions_test_cleanup.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "

 ((n+=1))
##----------------------------------------------------
## cli_crud_subscriptions_setup_001
##----------------------------------------------------
#xml[${n}]="cli_crud_subscriptions_setup_001"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - Setup configuration objects used in Subscription CRUD tests"
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/crud_scriptions/crud_subscriptions_test.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
#components[${n}]="${component[1]} "


#----------------------------------------------------
#cli_crud_subscriptions_allops_002
#----------------------------------------------------
 xml[${n}]="cli_crud_subscriptions_allops_002"
 timeouts[${n}]=60
 scenario[${n}]="${xml[${n}]} - Perform CRUD operations for Subscription CRUD tests"
 component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|create|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|show|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|delete|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|showfail|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|recreate|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[6]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|show|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[7]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|update|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 component[8]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|show|-c|crud_subscriptions/crud_subscriptions_tests.cli"
 components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
 ((n+=1))

#----------------------------------------------------
#cli_crud_subscriptions_allops_results_003
#----------------------------------------------------
 xml[${n}]="cli_crud_subscriptions_allops_results_003"
 timeouts[${n}]=60
 scenario[${n}]="${xml[${n}]} - Check results of CRUD operations for Subscription CRUD tests"
 component[1]=searchLogsEnd,m1,crud_subscriptions/compare/create1.comparetest
 component[2]=searchLogsEnd,m1,crud_subscriptions/compare/delete1.comparetest
 component[3]=searchLogsEnd,m1,crud_subscriptions/compare/recreate1.comparetest
 component[4]=searchLogsEnd,m1,crud_subscriptions/compare/update1.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"

((n+=1))
#----------------------------------------------------
# cli_crud_stat_subscriptions_test_001
#----------------------------------------------------
EXPECTED_JSON=crud_subscriptions/json/crud_stat_subscription_001_expected.json
OBTAINED_JSON=crud_subscriptions/json/crud_stat_subscription_001_obtained.json
xml[${n}]="cli_crud_stat_subscriptions_test_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - validate CRUDSubscription stats"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|crud_stat_test_002|-c|crud_subscriptions/crud_stat_subscriptions_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[4]=searchLogsEnd,m1,crud_subscriptions/compare/crud_stat_subscriptions_001.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))
#----------------------------------------------------
# cli_crud_data_subscriptions_002
#----------------------------------------------------
xml[${n}]="cli_crud_data_subscriptions_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - create global shared durable subscription CRUDSubscription1"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-verbose|-configdata|crud_subscriptions/config/crud_data_config_002.properties"
component[2]=searchLogsEnd,m1,crud_subscriptions/compare/crud_data_subscriptions_002.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))
#----------------------------------------------------
# cli_crud_stat_subscriptions_test_002
#----------------------------------------------------
EXPECTED_JSON=crud_subscriptions/json/crud_stat_subscription_002_expected.json
OBTAINED_JSON=crud_subscriptions/json/crud_stat_subscription_002_obtained.json
xml[${n}]="cli_crud_stat_subscriptions_test_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - validate CRUDSubscription stats2"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|crud_stat_test_001|-c|crud_subscriptions/crud_stat_subscriptions_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[4]=searchLogsEnd,m1,crud_subscriptions/compare/crud_stat_subscriptions_002.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

#----------------------------------------------------
# cli_crud_data_subscriptions_003
#----------------------------------------------------
xml[${n}]="cli_crud_data_subscriptions_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - send data to CRUDSubscription1"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|crud_subscriptions/config/crud_data_config_003.properties"
component[2]=searchLogsEnd,m1,crud_subscriptions/compare/crud_data_subscriptions_003.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# cli_crud_stat_subscriptions_test_003
# Note: this one uses searchLogsEnd instead of StatHelper
# because there is more than one valid answer. 
#----------------------------------------------------
EXPECTED_JSON=crud_subscriptions/json/crud_stat_subscription_003_expected.json
OBTAINED_JSON=crud_subscriptions/json/crud_stat_subscription_003_obtained.json
xml[${n}]="cli_crud_stat_subscriptions_test_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - validate CRUDSubscription stats3"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|crud_stat_test_001|-c|crud_subscriptions/crud_stat_subscriptions_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
#component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[3]=searchLogsEnd,m1,crud_subscriptions/compare/crud_stat_subscriptions_003.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# cli_crud_data_subscriptions_004
#----------------------------------------------------
xml[${n}]="cli_crud_data_subscriptions_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - receive data from global shared durable subscription CRUDSubscription1"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-verbose|-configdata|crud_subscriptions/config/crud_data_config_004.properties"
component[2]=searchLogsEnd,m1,crud_subscriptions/compare/crud_data_subscriptions_004.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))
#----------------------------------------------------
# cli_crud_stat_subscriptions_test_004
# Note: this one uses searchLogsEnd instead of StatHelper
# because there is more than one valid answer. 
#----------------------------------------------------
EXPECTED_JSON=crud_subscriptions/json/crud_stat_subscription_004_expected.json
OBTAINED_JSON=crud_subscriptions/json/crud_stat_subscription_004_obtained.json
xml[${n}]="cli_crud_stat_subscriptions_test_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - validate CRUDSubscription stats4"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|crud_stat_test_001|-c|crud_subscriptions/crud_stat_subscriptions_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
#component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[3]=searchLogsEnd,m1,crud_subscriptions/compare/crud_stat_subscriptions_004.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))
#----------------------------------------------------
# cli_crud_data_subscriptions_005
#----------------------------------------------------
xml[${n}]="cli_crud_data_subscriptions_005"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - unsubscribe from the global shared durable subscription CRUDSubscription1"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-verbose|-configdata|crud_subscriptions/config/crud_data_config_005.properties"
component[2]=searchLogsEnd,m1,crud_subscriptions/compare/crud_data_subscriptions_005.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))
#----------------------------------------------------
# cli_crud_stat_subscriptions_test_005
#----------------------------------------------------
EXPECTED_JSON=crud_subscriptions/json/crud_stat_subscription_005_expected.json
OBTAINED_JSON=crud_subscriptions/json/crud_stat_subscription_005_obtained.json
xml[${n}]="cli_crud_stat_subscriptions_test_005"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - validate CRUDSubscription stats5"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|crud_stat_test_002|-c|crud_subscriptions/crud_stat_subscriptions_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[4]=searchLogsEnd,m1,crud_subscriptions/compare/crud_stat_subscriptions_005.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))
#----------------------------------------------------
# cli_crud_subscriptions_delete_004
#----------------------------------------------------
xml[${n}]="cli_crud_subscriptions_delete_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Delete configuration objects used in Subscription CRUD tests"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|deletesubfail|-c|crud_subscriptions/crud_subscriptions_tests.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|delete|-c|crud_subscriptions/crud_subscriptions_tests.cli"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|deletefail|-c|crud_subscriptions/crud_subscriptions_tests.cli"
components[${n}]="${component[1]} ${component[2]} "

((n+=1))


#----------------------------------------------------
# cli_crud_subscriptions_restart_005
#----------------------------------------------------
xml[${n}]="cli_crud_subscriptions_restart_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Server restart for Queue CRUD tests"
component[1]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|1"
component[2]=sleep,5
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
