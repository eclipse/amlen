#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA Local Transaction Tests - 03"

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
# Scenario 1 - jca_xa_1001
#----------------------------------------------------
xml[${n}]="jca_xa_1001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - DB2 sanity check - A simple DB2 local transaction"
component[1]=jmsDriver,m1,trans/jca_xa_1001.xml,ALL
component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jca_enablerb_001
#----------------------------------------------------
xml[${n}]="jca_enablerb_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - enable rollback = true BMT"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_enablerb_002
#----------------------------------------------------
xml[${n}]="jca_enablerb_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - enable rollback = true CMTNS"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jca_enablerb_003
#----------------------------------------------------
xml[${n}]="jca_enablerb_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - enable rollback = false BMT"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jca_enablerb_004
#----------------------------------------------------
xml[${n}]="jca_enablerb_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - enable rollback = false CMTNS"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_pause_001
#----------------------------------------------------
xml[${n}]="jca_pause_001"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - MDB Pause - this test should not cause the MDB to pause"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_pause_002
#----------------------------------------------------
xml[${n}]="jca_pause_002"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - MDB Pause - this test should cause the MDB to pause"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - resumeEndpoints2
#----------------------------------------------------
xml[${n}]="resumeEndpoints2"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Resume Endpoints"

component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_pause_003
#----------------------------------------------------
xml[${n}]="jca_pause_003"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - ignoreFailuresOnStart activation spec property test"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - resumeEndpoints3
#----------------------------------------------------
xml[${n}]="resumeEndpoints3"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Resume Endpoints"

component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_translevel_001
#----------------------------------------------------
xml[${n}]="jca_translevel_001"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - transactionLevelSupport = Local with CMTNS MDB and CMTR EJB"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_translevel_002
#----------------------------------------------------
xml[${n}]="jca_translevel_002"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - transationLevelSupport = Local with BMT MDB and EJB"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_translevel_003
#----------------------------------------------------
xml[${n}]="jca_translevel_003"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - transationLevelSupport = NoTransaction with CMTR MDB and Supports EJB"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_translevel_004
#----------------------------------------------------
xml[${n}]="jca_translevel_004"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - transationLevelSupport = NoTransaction with BMTUT MDB"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,trans/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,trans/${xml[${n}]}.single.comparetest
fi
    
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - getWASlogs
#----------------------------------------------------
xml[${n}]="getWASlogs"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - get WAS logs"

component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh","-o|all"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))
