#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA SSL Tests - 02"

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
# Scenario 1 - jca_ssltopic_001
#----------------------------------------------------
xml[${n}]="jca_ssltopic_001"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - JCA SSL Topic 001"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,ssl/jca_ssltopic_001.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,ssl/jca_ssltopic_001.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_sslqueue_001
#----------------------------------------------------
xml[${n}]="jca_sslqueue_001"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - JCA SSL Queue 001"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ssl/jca_sslqueue_001.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jca_ssltopic_002
#----------------------------------------------------
xml[${n}]="jca_ssltopic_002"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - JCA SSL Topic 002"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,ssl/jca_ssltopic_002.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,ssl/jca_ssltopic_002.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jca_sslqueue_002
#----------------------------------------------------
xml[${n}]="jca_sslqueue_002"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - JCA SSL Queue 002 - User authentication on outbound"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ssl/jca_sslqueue_002.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,ssl/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jca_ssltopic_003
#----------------------------------------------------
xml[${n}]="jca_ssltopic_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - JCA SSL Topic 003"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ssl/jca_ssltopic_003.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jca_ssltopic_004
#----------------------------------------------------
xml[${n}]="jca_ssltopic_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - JCA SSL Topic 004"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ssl/jca_ssltopic_004.xml,ALL
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
