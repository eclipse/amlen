#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA XA CMT Tests - 05"

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
# Scenario 1 - jca_cmtr_topic
#----------------------------------------------------
xml[${n}]="jca_cmtr_topic"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB Required"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_cmtr_nodb2_topic
#----------------------------------------------------
xml[${n}]="jca_cmtr_nodb2_topic"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB Required - no DB2"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jca_cmtns_topic
#----------------------------------------------------
xml[${n}]="jca_cmtns_topic"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB NotSupported"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jca_cmtns_nodb2_topic
#----------------------------------------------------
xml[${n}]="jca_cmtns_nodb2_topic"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB NotSupported - no DB2"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jca_cmtr_queue
#----------------------------------------------------
xml[${n}]="jca_cmtr_queue"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB Required - Queue"

component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.comparetest

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jca_cmtns_queue
#----------------------------------------------------
xml[${n}]="jca_cmtns_queue"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Stateless Session Bean with Container Managed Transactions - MDB NotSupported - Queue"

component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.comparetest

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jca_cmt_rb_nonpersistent
#----------------------------------------------------
xml[${n}]="jca_cmt_rb_nonpersistent"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Container Managed Transactions with rollback - nonpersistent"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jca_cmt_rb_persistent
#----------------------------------------------------
xml[${n}]="jca_cmt_rb_persistent"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Container Managed Transactions with rollback - persistent"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.single.comparetest
fi

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - jca_cmt_rb_queue
#----------------------------------------------------
xml[${n}]="jca_cmt_rb_queue"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Container Managed Transactions with rollback - queue"

component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,xa_cmt/${xml[${n}]}.comparetest

components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jca_dest_full
#----------------------------------------------------
xml[${n}]="jca_dest_full"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - CMTR MDB and max messages"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
else
    component[1]=jmsDriver,m1,xa_cmt/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
fi

components[${n}]="${component[1]}"
((n+=1))

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
