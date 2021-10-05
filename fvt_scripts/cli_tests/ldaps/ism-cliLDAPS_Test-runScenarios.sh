#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CLI_LDAP_Tests"

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
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
# Scenario Initial CLEANUP - cli_ldaps_cleanup_init
#----------------------------------------------------
xml[${n}]="cli_ldaps_cleanup_init"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - cleanup LDAP configuration"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_reset.cli"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="cli_ldaps_clearLDAPcert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - initial cleanup for LDAP tests - clear LDAP cert"
component[1]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/mnt/A1/messagesight/data/certificates/LDAP/*"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="cli_ldaps_init_restartSucceed"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - ldaps URL - restart after reinitializing LDAP config"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_srvrestart.cli|-s|restart"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_nocert_restartFail.comparetest
#components[${n}]="${component[1]} ${component[2]}"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 0 - Start the LDAP server on M1
#----------------------------------------------------
xml[${n}]="ldaps_ldapserver_setup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - LDAP Setup"
# Make sure it is stopped so that we pick up https
component[1]=cAppDriverLogWait,m1,"-e|../jms_td_tests/ldap/ldap.sh","-o|-a|stop"
component[2]=cAppDriverWait,m1,"-e|../jms_td_tests/ldap/ldap.sh","-o|-a|start"
component[3]=cAppDriverLogWait,m1,"-e|../jms_td_tests/ldap/ldap.sh","-o|-a|setup"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - cli_ldaps_nocert
#----------------------------------------------------
xml[${n}]="cli_ldaps_nocert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - simple LDAP return code test for secure LDAP - no certificate"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_nocert.cli"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="conns_closed_after_nocert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - check netstat and confirm there are no lingering connections after LDAPS with nocert"
component[1]=cAppDriverLog,m1,"-e|../scripts/checkNetstat.sh","-o|${LDAP_SERVER_1}|${LDAP_SERVER_1_SSLPORT}","-s|CHECK_NETSTAT_LOG=${xml[${n}]}-cAppDriverLog-checkNetstat.sh-M1-1.log"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 cli_ldaps_nocert2 - Server restart fails if cert file disappears after config/enable
#----------------------------------------------------
xml[${n}]="cli_ldaps_cfgldaps"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - configure ldaps to prepare for restart filure if cert file is missing"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_cfgldaps.cli"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="cli_ldaps_aftercfg_restartSucceed"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - restore the ldaps cert and successfully restart"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_srvrestart.cli|-s|restart"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="cli_ldaps_clearLDAPcert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - initial cleanup for LDAP tests - clear LDAP cert"
component[1]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/mnt/A1/messagesight/data/certificates/LDAP/*"
components[${n}]="${component[1]}"
((n+=1))


xml[${n}]="cli_ldaps_nocert_restartFail"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - ldaps URL - restart with no certificate"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_srvrestart.cli|-s|restart"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="conns_closed_afterRestart_nocert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - check netstat and confirm there are no lingering connections after restart LDAPS with nocert"
component[1]=cAppDriverLog,m1,"-e|../scripts/checkNetstat.sh","-o|${LDAP_SERVER_1}|${LDAP_SERVER_1_SSLPORT}","-s|CHECK_NETSTAT_LOG=${xml[${n}]}-cAppDriverLog-checkNetstat.sh-M1-1.log"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="cli_ldaps_resetcert"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - restore the ldaps cert and successfully restart"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_resetcrt.cli"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="cli_ldaps_restartSucceed"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - restore the ldaps cert and successfully restart"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|ldaps/ldaps_srvrestart.cli|-s|restart"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|40|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

######################################################################################################################
##
## TODO: Feburary 2019 -- UPDATE THE TESTS, BELOW FOR MORE COMPETE LDAPS CONFIG TESTING!
##       NOTE:
##         The subset of tests, above, was restored when defect 223308 was reported from the field.  
##         These CLI tests (from the legacy version of MessageSight where a CLI was used instead of REST) were never
##         ported to the REST API and that is why defect 223308 was not caught sooner. Between the tests, above,
##         that have been restored and updates made to the LDAP tests under jms_td_tests, we now have some coverage 
##         again.  But it would be good to restore many of the tests, below, as well.
##
######################################################################################################################

#
##----------------------------------------------------
## Scenario CLEANUP - rc_test_ldap_cleanup_nocert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldap_cleanup_nocert"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - cleanup LDAP configuration"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_cleanup.cli"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario 1 - rc_test_ldaps_replaceWithWrongCert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldaps_replaceWithWrongCert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - Test that a valid but incorrect certificate prevents successful SSL connections"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldaps_replaceWithWrongCert.cli|-s|breakit"
##component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_cert.comparetest
##components[${n}]="${component[1]} ${component[2]}"
#components[${n}]="${component[1]}"
#((n+=1))
#
#xml[${n}]="conns_closed_after_replaceWithWrongCert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - check netstat and confirm there are no lingering connections after LDAPS with wrong cert"
#component[1]=cAppDriverLog,m1,"-e|../scripts/checkNetstat.sh","-o|${LDAP_SERVER_1}|${LDAP_SERVER_1_SSLPORT}","-s|CHECK_NETSTAT_LOG=${xml[${n}]}-cAppDriverLog-checkNetstat.sh-M1-1.log"
#components[${n}]="${component[1]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_wrongcert_enableLDAP"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP enable LDAP with wrong certificate"
#component[1]=enableLDAP,m1,rc_tests/rc_test_ldaps_wrongcert_restart.enableLDAP
#components[${n}]="${component[1]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_wrongcert_restartFail"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - LDAP enabled - restart with wrong certificate"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_restart.cli|-s|restart"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_wrongcert_restartFail.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_wrongcert_disableLDAP"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP disable LDAP with wrong certificate"
#component[1]=disableLDAP,m1,rc_tests/rc_test_ldaps_wrongcert.disableLDAP
#components[${n}]="${component[1]}"
#((n+=1))
#   
#xml[${n}]="cli_rc_test_ldaps_wrongcert_restartAfterDisableLDAP"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - restart after disabling LDAP with wrong certificate"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_restart.cli|-s|restart"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario 2 - rc_test_ldaps_replaceWithRightCert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldaps_replaceWithRightCert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - Test that restoring the correct certificate allows SSL connections"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldaps_replaceWithWrongCert.cli|-s|fixit"
##component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_cert.comparetest
##components[${n}]="${component[1]} ${component[2]}"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario 2 - rc_test_ldaps_restartWithRightCertSucceeds
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldaps_rightcert_restartSucceeds"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - LDAP enabled - restart with right certificate succeeds"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_restart.cli|-s|restart"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_rightcert_restartSuccess.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario CLEANUP - rc_test_ldap_cleanup_ldaps_replace*Cert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldap_cleanup_ldaps_replaceCert"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - cleanup LDAP configuration after ldaps_replaceCert"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_cleanup.cli"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario 1 - rc_test_ldaps_cert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldaps_cert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP return code test for secure LDAP - NOTE: This test will fail if there is already an LDAP cert file on the system"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldaps_cert.cli"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_cert.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))
#
#xml[${n}]="conns_closed_after_cert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - check netstat and confirm there are no lingering connections after LDAPS with cert"
#component[1]=cAppDriverLog,m1,"-e|../scripts/checkNetstat.sh","-o|${LDAP_SERVER_1}|${LDAP_SERVER_1_SSLPORT}","-s|CHECK_NETSTAT_LOG=${xml[${n}]}-cAppDriverLog-checkNetstat.sh-M1-1.log"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario CLEANUP - rc_test_ldap_cleanup_cert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldap_cleanup_cert"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - cleanup LDAP configuration"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_cleanup.cli"
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario Create corrupt cert file - rc_test_ldap_empty_cert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldap_emptyLDAPcert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - corrupt cert file tests"
#component[1]=emptyLDAPcert,m1,rc_tests/rc_test_ldap_corruptcert.emptyLDAPcert
#components[${n}]="${component[1]}"
#((n+=1))
#
##----------------------------------------------------
## Scenario 1 - rc_test_ldaps_corruptcert
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldaps_corruptcert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP return code test for corrupt LDAP cert"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldaps_corruptcert.cli"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_corruptcert.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))
#
#xml[${n}]="conns_closed_after_corruptcert"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - check netstat and confirm there are no lingering connections after LDAPS with corrupt cert"
#component[1]=cAppDriverLog,m1,"-e|../scripts/checkNetstat.sh","-o|${LDAP_SERVER_1}|${LDAP_SERVER_1_SSLPORT}","-s|CHECK_NETSTAT_LOG=${xml[${n}]}-cAppDriverLog-checkNetstat.sh-M1-1.log"
#components[${n}]="${component[1]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_corruptcert_enableLDAP"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP enable LDAP with corrupt certificate"
#component[1]=cAppDriverWait,m1,"-e|../scripts/updateLDAPurl.sh","-o|${LDAP_SERVER_1}|ldap|${LDAP_SERVER_1_PORT}|ldaps|${LDAP_SERVER_1_SSLPORT}"
#component[2]=enableLDAP,m1,rc_tests/rc_test_ldaps_corruptcert_restart.enableLDAP
#component[3]=emptyLDAPcert,m1,rc_tests/rc_test_ldap_corruptcert_emptyLDAPcert.enableLDAP
#components[${n}]="${component[1]} ${component[2]}  ${component[3]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_corruptcert_restartFail"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - LDAP enabled - restart with corrupt certificate"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_restart.cli|-s|restart"
#component[2]=searchLogsEnd,m1,rc_tests/rc_test_ldaps_corruptcert_restartFail.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))
#
#xml[${n}]="cli_rc_test_ldaps_corruptcert_disableLDAP"
#timeouts[${n}]=40
#scenario[${n}]="${xml[${n}]} - simple LDAP disable LDAP with corrupt certificate"
#component[1]=disableLDAP,m1,rc_tests/rc_test_ldaps_corruptcert_restart.disableLDAP
#components[${n}]="${component[1]}"
#((n+=1))
#
##
## Always run the CLEANUP last to assure the 
## LDAP configurations for these tests do not affect
## subsequent automation scenarios
##----------------------------------------------------
## Scenario Final CLEANUP - rc_test_ldap_cleanup_final
##----------------------------------------------------
#xml[${n}]="cli_rc_test_ldap_cleanup_final"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - cleanup LDAP configuration"
#component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-c|rc_tests/rc_test_ldap_cleanup.cli"
#components[${n}]="${component[1]}"
#((n+=1))
#

## ALWAYS inluced this cleanup/stop at the end of this script
#----------------------------------------------------
# Scenario ***FINAL - cleanup stop ldap***
#----------------------------------------------------
xml[${n}]="jms_ldap_stop"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - LDAP Stop"
component[1]=cAppDriverLogWait,m1,"-e|../jms_td_tests/ldap/ldap.sh","-o|-a|cleanup"
component[2]=cAppDriverLogWait,m1,"-e|../jms_td_tests/ldap/ldap.sh","-o|-a|stop"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
