#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JCA Basic Tests - 01"

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
# Scenario 0 - jca_retained_001
#----------------------------------------------------
xml[${n}]="jca_retained_001"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - JCA retained 001"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent
component[1]=jmsDriver,m1,basic/jca_retained_001.xml,ReceiveRetained,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enabletrace|-c|jca_policy_config.cli"
components[${n}]="${component[1]} ${component[2]}"
#components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - Clear retained messages.
#----------------------------------------------------
xml[${n}]="mqtt_clearRetained.xml"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JMS Retained Messages in HA Test Cleanup"
component[1]=wsDriver,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jca_topic_001
#----------------------------------------------------
xml[${n}]="jca_topic_001"
timeouts[${n}]=250
scenario[${n}]="${xml[${n}]} - JCA topic 001"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,basic/jca_topic_001.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,basic/jca_topic_001.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_topic_002
#----------------------------------------------------
xml[${n}]="jca_topic_002"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - JCA topic 002"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,basic/jca_topic_002.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,basic/jca_topic_002.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_topic_003
#----------------------------------------------------
xml[${n}]="jca_topic_003"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - JCA topic 003"

if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,basic/jca_topic_003.xml,Cluster
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,basic/jca_topic_003.xml,Single
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

ORIGTIMEOUTMULTIPLIER=${TIMEOUTMULTIPLIER}
export TIMEOUTMULTIPLIER=4.0
#----------------------------------------------------
# Scenario x - jca_queue_001
#----------------------------------------------------
xml[${n}]="jca_queue_001"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - JCA queue 001"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,basic/jca_queue_001.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_shared_001
#----------------------------------------------------
xml[${n}]="jca_shared_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - JCA shared 001 - nondurable shared subscription without a clientid"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_shared_002
#----------------------------------------------------
xml[${n}]="jca_shared_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - JCA shared 002 - nondurable shared subscription with a clientid"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	#----------------------------------------------------
	# Scenario x - jca_shared_003
	#----------------------------------------------------
	xml[${n}]="jca_shared_003"
	timeouts[${n}]=400
	scenario[${n}]="${xml[${n}]} - JCA shared 003 - durable shared subscription without a clientid"
	
	# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
	component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
	# Use searchlogs to verify the servlet ran correctly
	component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
	components[${n}]="${component[1]} ${component[2]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario x - jca_shared_004
	#----------------------------------------------------
	xml[${n}]="jca_shared_004"
	timeouts[${n}]=400
	scenario[${n}]="${xml[${n}]} - JCA shared 004 - durable shared subscription with a clientid"
	
	# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
	if [ ${cluster} == 1 ] ; then
	    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
	    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
	else
	    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
	    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
	fi
	components[${n}]="${component[1]} ${component[2]}"
	((n+=1))
fi

#----------------------------------------------------
# Scenario x - jca_jndi_topic
#----------------------------------------------------
xml[${n}]="jca_jndi_topic"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - JCA JNDI Topic"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
if [ ${cluster} == 1 ] ; then
    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Cluster,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.cluster.comparetest
else
    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,Single,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.single.comparetest
fi
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_jndi_queue
#----------------------------------------------------
xml[${n}]="jca_jndi_queue"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - JCA JNDI Queue"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,ALL,,"-s|BITFLAG=-DIMAEnforceObjectMessageSecurity=false"
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_convertMsgType_001
#----------------------------------------------------
xml[${n}]="jca_convertMsgType_001"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - JCA ConvertMessageType"

# WS Test Driver starts a consumer on the log topic of messages sent by the driver
component[1]=wsDriver,m1,basic/${xml[${n}]}.xml,ALL
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario x - jca_api_001
#----------------------------------------------------
xml[${n}]="jca_api_001"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - JCA api 001 - Test various JMS API's in the RA environment"

# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,ALL
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

if [[ "${WASType}" =~ "was" ]] && [[ "${FULLRUN}" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Scenario x - jca_translation
    #----------------------------------------------------
    xml[${n}]="jca_translation"
    timeouts[${n}]=200
    scenario[${n}]="${xml[${n}]} - JCA mock translations"

    # JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
    component[1]=jmsDriver,m1,basic/${xml[${n}]}.xml,ALL
    # Use searchlogs to verify the servlet ran correctly
    component[2]=searchLogsEnd,m1,basic/${xml[${n}]}.comparetest
    components[${n}]="${component[1]} ${component[2]}"
    #components[${n}]="${component[1]}"
    ((n+=1))
fi

export TIMEOUTMULTIPLIER=${ORIGTIMEOUTMULTIPLIER}

#----------------------------------------------------
# Scenario x - getWASlogs
#----------------------------------------------------
xml[${n}]="getWASlogs"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - get WAS logs"

component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh","-o|trace"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disabletrace|-c|jca_policy_config.cli"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
