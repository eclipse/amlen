#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CLI Security Tests - 00"

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
    xml[${n}]="cli_security_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# cli_security_tests_adminobjs_001
#----------------------------------------------------
xml[${n}]="cli_security_tests_adminobjs_001"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Creation of Connection Factory Administered Objects and Policy Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|security/cli_security_tests.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|protocol_tlsv1|-c|security/cli_security_tests.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# cli_security_test_001
#----------------------------------------------------
xml[${n}]="cli_security_test_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - send test data to topic"
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_001.properties|-verbose"
component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_001.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [ "${FULLRUN}" == "TRUE" ] ; then

    #----------------------------------------------------
    # cli_security_tests_adminobjs_002
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_002"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - change protocol to TLSv1.2"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|protocol_tlsv1_2|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_002
    #----------------------------------------------------
    xml[${n}]="cli_security_test_002"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_002.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_002.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_tests_adminobjs_003
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_003"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - change protocol to TLSv1.1"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|protocol_tlsv1_1|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_003
    #----------------------------------------------------
    xml[${n}]="cli_security_test_003"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_003.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_003.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    # SSLv3 no longer supported so this should fail now
    #----------------------------------------------------
    # cli_security_tests_adminobjs_004
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_004"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - change protocol to SSLv3"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|protocol_sslv3|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    # SSLv3 is no longer supported
    ##----------------------------------------------------
    ## cli_security_test_004
    ##----------------------------------------------------
    #xml[${n}]="cli_security_test_004"
    #timeouts[${n}]=60
    #scenario[${n}]="${xml[${n}]} - send test data to topic - Fails due to SSLv3"
    #component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_004.properties|-verbose"
    #component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_004.comparetest
    #components[${n}]="${component[1]} ${component[2]}"
    #((n+=1))

    #----------------------------------------------------
        # cli_security_tests_adminobjs_005
        # SSLv3 is no longer supported.
        # This just verifies that trying to set SSLv3 fails in restapi
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_005"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - enable client auth"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enable_client_auth|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))


    #----------------------------------------------------
    # cli_security_tests_adminobjs_006
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_006"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - set protocol to TLSv1.2"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|protocol_tlsv1_2|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_006
    #----------------------------------------------------
    xml[${n}]="cli_security_test_006"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send test data to topic - client auth"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_002.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_006.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_tests_adminobjs_007
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_007"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - delete trusted certificates"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disable_client_auth_cert|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_007
    #----------------------------------------------------
    xml[${n}]="cli_security_test_007"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - try to send - should fail"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_002.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_007.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_tests_adminobjs_008
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_008"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - disable client authentication flag"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disable_client_auth_flag|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_008
    #----------------------------------------------------
    xml[${n}]="cli_security_test_008"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send should work"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_002.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_008.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_009
    #----------------------------------------------------
    xml[${n}]="cli_security_test_009"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - wrong user id test"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_005.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_009.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_010
    #----------------------------------------------------
    xml[${n}]="cli_security_test_010"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - wrong password test"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_006.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_010.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_011
    #----------------------------------------------------
    xml[${n}]="cli_security_test_011"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - send too low of a protocol"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_001.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_011.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

    #----------------------------------------------------
    # cli_security_tests_adminobjs_009
    #----------------------------------------------------
    xml[${n}]="cli_security_tests_adminobjs_009"
    timeouts[${n}]=75
    scenario[${n}]="${xml[${n}]} - update endpoint to use different security profile"
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|ep_update_sec_prof|-c|security/cli_security_tests.cli"
    components[${n}]="${component[1]} "
    ((n+=1))

    #----------------------------------------------------
    # cli_security_test_012
    #----------------------------------------------------
    xml[${n}]="cli_security_test_012"
    timeouts[${n}]=60
    scenario[${n}]="${xml[${n}]} - should work after ep gets new security profile"
    component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.test.stat.StatHelper,-o|-host|${A1_IPv4_1}|-keyfileroot|${M1_TESTROOT}|-configdata|security/config/security_test_config_001.properties|-verbose"
    component[2]=searchLogsEnd,m1,security/compare/cli_security_test_driver_012.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))

fi

#----------------------------------------------------
#  cli_security_tests_adminobjs_010
#----------------------------------------------------
xml[${n}]="cli_security_tests_adminobjs_010"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - test good alphanumeric names for security profiles "
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|gvtcreate_good|-c|security/cli_security_tests.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
#  cli_security_tests_adminobjs_011
#----------------------------------------------------
xml[${n}]="cli_security_tests_adminobjs_011"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - test bad alphanumeric names for security profiles"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|gvtcreate_bad|-c|security/cli_security_tests.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
#  cli_security_tests_adminobjs_012
#----------------------------------------------------
xml[${n}]="cli_security_tests_adminobjs_012"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Clean-up of Connection Factory Administered Objects and Policies"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|security/cli_security_tests.cli"
components[${n}]="${component[1]} "
((n+=1))


if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="cli_security_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
