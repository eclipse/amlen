#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CLI Stat Queues - 00"

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
# stat_queues_adminobjs_001
#----------------------------------------------------
xml[${n}]="cli_stat_queue_adminobjs_001"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Creation of Connection Factory Administered Objects and Policy Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|stat_queues/stat_queues_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# stat_queues_test_001
#----------------------------------------------------
xml[${n}]="cli_stat_queues_test_001"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - send test data to queue"
component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_001.properties"
component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_001.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# stat_queues_test_002
#----------------------------------------------------
EXPECTED_JSON=stat_queues/json/stat_test_result_expected_001.json
OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_001.json
xml[${n}]="cli_stat_queues_test_002"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - validate stat command."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_002.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

if [ "${FULLRUN}" == "TRUE" ] ; then

    #----------------------------------------------------
    # stat_queues_test_003
    #----------------------------------------------------
    xml[${n}]="cli_stat_queues_test_003"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to queue"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_002.properties"
    component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_003.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
    
    #----------------------------------------------------
    # stat_queues_test_004
    #----------------------------------------------------
    EXPECTED_JSON=stat_queues/json/stat_test_result_expected_002.json
    OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_002.json
    xml[${n}]="cli_stat_queues_test_004"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_004.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))
    
    #----------------------------------------------------
    # stat_queues_test_005
    #----------------------------------------------------
    xml[${n}]="cli_stat_queues_test_005"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to queue"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_003.properties"
    component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_005.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
    
    #----------------------------------------------------
    #  stat_queues_test_006
    #----------------------------------------------------
    EXPECTED_JSON=stat_queues/json/stat_test_result_expected_003.json
    OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_003.json
    xml[${n}]="cli_stat_queues_test_006"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_006.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    #----------------------------------------------------
    # stat_queues_test_007
    #----------------------------------------------------
    xml[${n}]="cli_stat_queues_test_007"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to queue"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_004.properties"
    component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_007.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # stat_queues_test_008
    #----------------------------------------------------
    EXPECTED_JSON=stat_queues/json/stat_test_result_expected_004.json
    OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_004.json
    xml[${n}]="cli_stat_queues_test_008"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_008.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    #----------------------------------------------------
    # stat_queues_test_009
    #----------------------------------------------------
    xml[${n}]="cli_stat_queues_test_009"
    timeouts[${n}]=230
    scenario[${n}]="${xml[${n}]} - send test data to queue"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_005.properties"
    component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_009.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
    
    #----------------------------------------------------
    # stat_queues_test_010
    #----------------------------------------------------
    EXPECTED_JSON=stat_queues/json/stat_test_result_expected_005.json
    OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_005.json
    xml[${n}]="cli_stat_queues_test_010"
    timeouts[${n}]=100
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_010.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    #----------------------------------------------------
    # stat_queues_test_011
    #----------------------------------------------------
    xml[${n}]="cli_stat_queues_test_011"
    timeouts[${n}]=200
    scenario[${n}]="${xml[${n}]} - send test data to queue"
    component[1]=javaAppDriver,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-configdata|stat_queues/config/stat_test_config_006.properties"
    component[2]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_011.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # stat_queues_test_012
    #----------------------------------------------------
    EXPECTED_JSON=stat_queues/json/stat_test_result_expected_006.json
    OBTAINED_JSON=stat_queues/json/stat_test_result_obtained_006.json
    xml[${n}]="cli_stat_queues_test_012"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - validate stat command."
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stat_test_002|-c|stat_queues/stat_queues_tests.cli"
    component[2]=cAppDriver,m1,"-e|./mvHelper.sh","-o|reply.json|${OBTAINED_JSON}"
    component[3]=cAppDriver,m1,"-e|./compJSONfiles.py","-o|${EXPECTED_JSON}|${OBTAINED_JSON}"
    component[4]=searchLogsEnd,m1,stat_queues/compare/cli_stat_queues_test_driver_012.comparetest
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[3]}"
    ((n+=1))

fi

#----------------------------------------------------
#  stat_queues_adminobjs_002
#----------------------------------------------------
xml[${n}]="cli_stat_queue_adminobjs_002"
timeouts[${n}]=125
scenario[${n}]="${xml[${n}]} - Clean-up of Connection Factory Administered Objects and Policies"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|stat_queues/stat_queues_config.cli"
components[${n}]="${component[1]} "
((n+=1))

##----------------------------------------------------
## stat_queues_restart
##----------------------------------------------------
#xml[${n}]="cli_stat_topics_restart_001"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - Server restart for Queue stat tests"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|restart|-c|stat_queues/stat_queues_config.cli"
#component[2]=sleep,10
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

