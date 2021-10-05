#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA Heuristic XA Tests - 08"

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

#----------------------------------------------------
# Scenario 1 - heur_commit_cmt
#----------------------------------------------------
xml[${n}]="heur_commit_cmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA CMT Commit"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9003|-a|commit"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - heur_commit_bmt
#----------------------------------------------------
xml[${n}]="heur_commit_bmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA BMT Commit"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9001|-a|commit"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - heur_rollback_cmt
#----------------------------------------------------
xml[${n}]="heur_rollback_cmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA CMT Rollback"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9003|-a|rollback"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - heur_rollback_bmt
#----------------------------------------------------
xml[${n}]="heur_rollback_bmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA BMT Rollback"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9001|-a|rollback"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - heur_prepCommit_cmt
#----------------------------------------------------
xml[${n}]="heur_prepCommit_cmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA CMT crash on prepare then commit"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9002|-a|commit"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - heur_prepRB_cmt
#----------------------------------------------------
xml[${n}]="heur_prepRB_cmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA CMT crash on prepare then rollback"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9002|-a|rollback"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - heur_prepCommit_bmt
#----------------------------------------------------
xml[${n}]="heur_prepCommit_bmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA BMT crash on prepare then commit"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9000|-a|commit"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - heur_prepRB_bmt
#----------------------------------------------------
xml[${n}]="heur_prepRB_bmt"
timeouts[${n}]=350
scenario[${n}]="${xml[${n}]} - JCA Heuristic XA BMT crash on prepare then rollback"

component[1]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Subscribe,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|./heuristic_xa/heuristic_xa.sh","-o|-n|9000|-a|rollback"
component[3]=jmsDriver,m1,heuristic_xa/${xml[${n}]}.xml,Consume,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"

components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario x - checkWAS
#----------------------------------------------------
xml[${n}]="checkWAS"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - Make sure WAS is still running after heuristic tests"

component[1]=cAppDriver,m1,"-e|./heuristic_xa/checkWAS.sh"
component[2]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh","-o|trace"

components[${n}]="${component[1]} ${component[2]}"
((n+=1))
