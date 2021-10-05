#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CLI Stat Topics, MQQTClient and Connection - 00"

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
# cli_stat_topics_adminobjs_001
#----------------------------------------------------
xml[${n}]="cli_stat_topics_adminobjs_001"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Creation of Connection Factory Administered Objects and Policy Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|stat_topics/stat_topics_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# cli_stat_topics_test_001
#----------------------------------------------------
xml[${n}]="cli_stat_topics_test_001"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - send test data to topic"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_001.properties"
component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_001.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# cli_stat_topics_test_002
#----------------------------------------------------
EXPECTED_JSON=stat_topics/json/stat_test_result_expected_001.json
OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_001.json
NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_001.json
xml[${n}]="cli_stat_topics_test_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - validate stat command."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
component[2]=cAppDriverWait,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_002.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

if [ "${FULLRUN}" == "TRUE" ] ; then

    #----------------------------------------------------
    # cli_stat_topics_test_003
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_003"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_002.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_003.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_004
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_002.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_002.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_002.json
    xml[${n}]="cli_stat_topics_test_004"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_004.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_005
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_005"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_003.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_005.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_006
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_003.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_003.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_003.json
    xml[${n}]="cli_stat_topics_test_006"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_003|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_006.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_007
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_007"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_004.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_007.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_008
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_004.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_004.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_004.json
    xml[${n}]="cli_stat_topics_test_008"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_007|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_008.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

 
    #----------------------------------------------------
    # cli_stat_mqttclient_test_001
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_101.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_101.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_101.json
    xml[${n}]="cli_stat_mqttclient_test_001"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - stat MQTTClient ClientID=statTestConsumer1"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_101|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriverWait,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_mqttclient_test_driver_001.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_mqttclient_test_002
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_102.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_102.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_102.json
    xml[${n}]="cli_stat_mqttclient_test_002"
    timeouts[${n}]=30
    #scenario[${n}]="${xml[${n}]} - stat MQTTClient FilePrefix=stat_mqttclient_002 ClientID=statTestConsumer2"
    scenario[${n}]="${xml[${n}]} - stat MQTTClient ClientID=statTestConsumer2"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_102|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriverWait,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_mqttclient_test_driver_002.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_mqttclient_test_003
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_103.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_103.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_103.json
    xml[${n}]="cli_stat_mqttclient_test_003"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - stat MQTTClient ResultCount=10"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_103|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriverWait,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./stringCountHelper.sh","-o|statTestConsumer1-|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_topics/compare/cli_stat_mqttclient_test_driver_003.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))


    #----------------------------------------------------
    # cli_stat_topics_test_009
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_009"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_005.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_009.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_019
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_005.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_005.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_005.json
    xml[${n}]="cli_stat_topics_test_010"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_010.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_011
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_011"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_006.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_011.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_012
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_006.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_006.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_006.json
    xml[${n}]="cli_stat_topics_test_012"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_012.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_013
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_013"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_007.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_013.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_014
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_007.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_007.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_007.json
    xml[${n}]="cli_stat_topics_test_014"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_014.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_015
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_015"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_008.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_015.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_016
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_008.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_008.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_008.json
    xml[${n}]="cli_stat_topics_test_016"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_003|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_016.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_017
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_017"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_009.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_017.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_018
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_009.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_009.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_009.json
    xml[${n}]="cli_stat_topics_test_018"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_018.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_019
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_019"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_010.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_019.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_020
    #----------------------------------------------------
    EXPECTED_JSON=stat_topics/json/stat_test_result_expected_010.json
    OBTAINED_JSON=stat_topics/json/stat_test_result_obtained_010.json
    NO_TIMES_JSON=stat_topics/json/stat_test_result_no_times_010.json
    xml[${n}]="cli_stat_topics_test_020"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_009|-c|stat_topics/stat_topics_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriverWait,m1,"-e|./removeTimesHelper.sh","-o|${OBTAINED_JSON}|${NO_TIMES_JSON}"
    component[4]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${NO_TIMES_JSON}"
    component[5]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_020.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_stat_topics_test_021
    #----------------------------------------------------
    xml[${n}]="cli_stat_topics_test_021"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_topics/config/stat_topic_test_config_011.properties"
    component[2]=searchLogsEnd,m1,stat_topics/compare/cli_stat_topics_test_driver_021.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

fi

#----------------------------------------------------
#  cli_stat_topics_adminobjs_002
#----------------------------------------------------
xml[${n}]="cli_stat_topics_adminobjs_002"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Clean-up of Connection Factory Administered Objects and Policies"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|stat_topics/stat_topics_config.cli"
components[${n}]="${component[1]} "
((n+=1))

