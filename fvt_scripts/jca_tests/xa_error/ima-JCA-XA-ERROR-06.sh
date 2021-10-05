#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA XA Error Tests - 06"

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

cluster=`grep -r -i cluster server.definition | grep -v grep`
if [[ "${cluster}" == "" ]] ; then
    echo "Not in a cluster"
    cluster=0
else
    cluster=1
fi

########################################
##  Evil Resource Adapter Test Cases  ##
## These tests are in the 7000 series ##
########################################

#----------------------------------------------------
# Scenario x - jca_xaerror_none_statelessBMTUT
#----------------------------------------------------
xml[${n}]="jca_xaerror_none_statelessBMTUT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - A check to make sure Evil RA works"
component[1]=jmsDriver,m1,xa_error/jca_xaerror_none_statelessBMTUT.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_prepare_statelessBMTUT
#----------------------------------------------------
xml[${n}]="jca_xaerror_prepare_statelessBMTUT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in prepare() in XA"
component[1]=jmsDriver,m1,xa_error/jca_xaerror_prepare_statelessBMTUT.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_commit_statelessBMTUT
#----------------------------------------------------
xml[${n}]="jca_xaerror_commit_statelessBMTUT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in commit() in XA"
component[1]=jmsDriver,m1,xa_error/jca_xaerror_commit_statelessBMTUT.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_start_statelessBMTUT
#----------------------------------------------------
xml[${n}]="jca_xaerror_start_statelessBMTUT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in start() in XA"
component[1]=jmsDriver,m1,xa_error/jca_xaerror_start_statelessBMTUT.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_end_statelessBMTUT
#----------------------------------------------------
xml[${n}]="jca_xaerror_end_statelessBMTUT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in end() in XA"
component[1]=jmsDriver,m1,xa_error/jca_xaerror_end_statelessBMTUT.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_none_statelessCMT
#----------------------------------------------------
xml[${n}]="jca_xaerror_none_statelessCMT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - A check to make sure Evil RA works"
component[1]=jmsDriver,m1,xa_error/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_prepare_statelessCMT
#----------------------------------------------------
xml[${n}]="jca_xaerror_prepare_statelessCMT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in prepare() in XA"
component[1]=jmsDriver,m1,xa_error/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_commit_statelessCMT
#----------------------------------------------------
xml[${n}]="jca_xaerror_commit_statelessCMT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in commit() in XA"
component[1]=jmsDriver,m1,xa_error/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_start_statelessCMT
#----------------------------------------------------
xml[${n}]="jca_xaerror_start_statelessCMT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in start() in XA"
component[1]=jmsDriver,m1,xa_error/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_end_statelessCMT
#----------------------------------------------------
xml[${n}]="jca_xaerror_end_statelessCMT"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - EvilRA throws Error in end() in XA"
component[1]=jmsDriver,m1,xa_error/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_rollback_statelessBMTUT
#----------------------------------------------------
#xml[${n}]="jca_xaerror_rollback_statelessBMTUT"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - EvilRA throws Error in rollback() in XA"
#component[1]=jmsDriver,m1,xa_error/jca_xaerror_rollback_statelessBMTUT.xml,ALL
#component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_forget_statelessBMTUT
#----------------------------------------------------
#xml[${n}]="jca_xaerror_forget_statelessBMTUT"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - EvilRA throws Error in forget() in XA"
#component[1]=jmsDriver,m1,xa_error/jca_xaerror_forget_statelessBMTUT.xml,ALL
#component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_recover_statelessBMTUT
#----------------------------------------------------
#xml[${n}]="jca_xaerror_recover_statelessBMTUT"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - EvilRA throws Error in recover() in XA"
#component[1]=jmsDriver,m1,xa_error/jca_xaerror_recover_statelessBMTUT.xml,ALL
#component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_gettransactiontimeout_statelessBMTUT
#----------------------------------------------------
#xml[${n}]="jca_xaerror_gettransactiontimeout_statelessBMTUT"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - EvilRA throws Error in gettransactiontimeout() in XA"
#component[1]=jmsDriver,m1,xa_error/jca_xaerror_gettransactiontimeout_statelessBMTUT.xml,ALL
#component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario x - jca_xaerror_settransactiontimeout_statelessBMTUT
#----------------------------------------------------
#xml[${n}]="jca_xaerror_settransactiontimeout_statelessBMTUT"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - EvilRA throws Error in set_transaction_timeout() in XA"
#component[1]=jmsDriver,m1,xa_error/jca_xaerror_settransactiontimeout_statelessBMTUT.xml,ALL
#component[2]=searchLogsEnd,m1,xa_error/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario x - getWASlogs
#----------------------------------------------------
xml[${n}]="getWASlogs"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - get WAS logs"

component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh","-o|trace"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))
