#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA Setup Objects - 00"

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
  
if [[ "${A1_LDAP}" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="jms_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Scenario 1 - jca_setup_001
#----------------------------------------------------
xml[${n}]="jca_setup_001"

# Need to have enough time for WAS cluster synchronization
# If the test kills the process in the middle of sync, things get messy,
# so hopefully 600 seconds is long enough for it to time out if it fails.
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JCA Setup IBM MessageSight Objects"

# "./was.sh -a prepare" should be run first
# It replaces M1_NAME in jca_policy_config.cli and test xml's with
# MQKEY (e.g. mar228)
component[1]=cAppDriverLog,m1,"-e|./scripts_was/was.sh","-o|-a|prepare"

# Next, create the IBM MessageSight configuration objects. This 
# needs to be done before configuring the App Server so that
# it can create the MDB MessageListener's
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|jca_policy_config.cli"

# Next, run jca_setup_001.xml which creates all of the JNDI objects
# Again, This needs to be done before configuring WAS so that the 
# App Server can find everything in ldap.
component[3]=jmsDriver,m1,setup/jca_setup_001.xml,ALL

# Finally, configure WAS. Depending on variables in testEnv,
# this will configure the appropriate server
component[4]=cAppDriverLogWait,m1,"-e|./scripts_was/was.sh","-o|-a|setup"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}" 

((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_setup_002
# Run searchlogs on was.py output if WASType
# is was85 or was80
#----------------------------------------------------
xml[${n}]="jca_setup_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JCA Setup Verify WAS Configuration"
component[1]=searchLogsEnd,m1,setup/was.sh.comparetest
components[${n}]="${component[1]}"
((n+=1))

#################################################################
# 12/4/14 - Attempt to uninstall the app before installing incase
# the environment was left dirty by a previous run.
#----------------------------------------------------
# Scenario 1 - jca_teardown_001
#----------------------------------------------------
xml[${n}]="jca_teardown_001"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JCA Uninstall application"
component[1]=cAppDriverLogWait,m1,"-e|./scripts_was/was.sh","-o|-a|uninstall"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_teardown_002
#----------------------------------------------------
xml[${n}]="jca_teardown_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JCA Uninstall application verification"
component[1]=searchLogsEnd,m1,teardown/was.sh.uninstall.comparetest
components[${n}]="${component[1]}"
((n+=1))
#################################################################

#----------------------------------------------------
# Scenario 3 - jca_retained_001
# Send a retained message before launching the
# JCA Test Application
#----------------------------------------------------
xml[${n}]="jca_retained_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JCA Retained Publish"
component[1]=jmsDriver,m1,basic/jca_retained_001.xml,SendRetained
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jca_setup_003
#----------------------------------------------------
xml[${n}]="jca_setup_003"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JCA App installation"
component[1]=cAppDriverLogWait,m1,"-e|./scripts_was/was.sh","-o|-a|install"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jca_setup_004
#----------------------------------------------------
xml[${n}]="jca_setup_004"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JCA App installation Verification"
component[1]=searchLogsEnd,m1,setup/was.sh.install.comparetest
components[${n}]="${component[1]}"
((n+=1))
