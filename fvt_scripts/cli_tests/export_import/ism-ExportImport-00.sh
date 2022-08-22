#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Export_Import"

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
    xml[${n}]="export_import_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Scenario 2 - jms_Setup_001
#----------------------------------------------------
xml[${n}]="export_import_Setup_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Configure Server for export/import test"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|export_import/policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - 
#----------------------------------------------------
xml[${n}]="export_import_002"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Export and run import with ApplyConfig=false"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|export|-c|export_import/policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|import|-c|export_import/policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - 
#----------------------------------------------------
xml[${n}]="export_import_003"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Export, Reset Config and run import with ApplyConfig=true"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|export|-c|export_import/policy_config.cli"
component[2]=cAppDriverWait,m1,"-e|export_import/copy_file.sh","-o|/var/messagesight/userfiles/testExport|/var/messagesight/."
component[3]=cAppDriverWait,m1,"-e|../common/serverReset.sh"
component[4]=cAppDriverWait,m1,"-e|export_import/copy_file.sh","-o|/var/messagesight/testExport|/var/messagesight/userfiles/."
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|importApply|-c|export_import/policy_config.cli"
component[6]=cAppDriverWait,m1,"-e|../common/serverRestart.sh"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_Cleanup_001
#----------------------------------------------------
xml[${n}]="export_import_Cleanup_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Use Cleanup to verify that import got configuration correct"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|export_import/policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))


#########################
# Import backup from v1.2
#########################
#----------------------------------------------------
# Scenario 6 - 
#----------------------------------------------------
xml[${n}]="export_import_010"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Import v1.2 backup with ApplyConfig=true"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|putFile|-c|export_import/policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|importApplyV12|-c|export_import/policy_config.cli"
component[3]=cAppDriverWait,m1,"-e|../common/serverRestart.sh"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_Cleanup_010
#----------------------------------------------------
xml[${n}]="export_import_Cleanup_010"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Use Cleanup to verify that v1.2 import got configuration correct"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|export_import/policy_config_v12.cli"
components[${n}]="${component[1]}"
((n+=1))


if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="export_import_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
