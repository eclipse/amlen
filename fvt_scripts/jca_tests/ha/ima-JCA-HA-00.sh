#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

scenario_set_name="JCA HA Tests - 00"

typeset -i n=0

cluster=`grep -r -i cluster server.definition | grep -v grep`
if [[ "${cluster}" == "" ]] ; then
    echo "Not in a cluster"
    cluster=0
else
    cluster=1
fi

# The only test actually doing a failover here is jca_ha_002
# So we will want to make A1 primary again at the end of the
# test bucket so that cleanup works properly.

#------------------------------------------------------------------
xml[${n}]="enabletrace"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - enable trace"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enabletrace|-c|jca_policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))
#------------------------------------------------------------------

if [[ "${FULLRUN}" == "FALSE" ]] ; then
	#----------------------------------------------------
	# Scenario x - Clear retained messages.
	#----------------------------------------------------
	xml[${n}]="mqtt_clearRetained.xml"
	timeouts[${n}]=30
	scenario[${n}]="${xml[${n}]} - JMS Retained Messages in HA Test Cleanup"
	component[1]=wsDriver,m1,../jms_td_tests/msgdelivery/mqtt_clearRetained.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
fi

#------------------------------------------------------------------
# Scenario 1 - jca_ha_001 (does not do a failover)
#------------------------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jca_ha_001"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - JCA HA 001"
component[1]=jmsDriver,m1,ha/jca_ha_001.xml,ALL
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,ha/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#------------------------------------------------------------------
# Scenario 2 - jca_ha_002 (does a failover)
#------------------------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jca_ha_002"
timeouts[${n}]=250
scenario[${n}]="${xml[${n}]} - JCA HA 002"
# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ha/jca_ha_002.xml,ALL
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,ha/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#------------------------------------------------------------------
# Scenario 3 - jca_ha_003 (does not do a failover)
#------------------------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="jca_ha_003"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - JCA HA 003"
# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
component[1]=jmsDriver,m1,ha/jca_ha_003.xml,ALL
# Use searchlogs to verify the servlet ran correctly
component[2]=searchLogsEnd,m1,ha/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#------------------------------------------------------------------
# Scenario 4 - jca_ha_004 (does a failover)
#------------------------------------------------------------------
# A1 = Standby
# A2 = Primary
#xml[${n}]="jca_ha_004"
#timeouts[${n}]=200
#scenario[${n}]="${xml[${n}]} - JCA HA 004"
# JMS Test Driver starts a consumer on the replyTo topic of messages sent by the servlet
#component[1]=jmsDriver,m1,ha/jca_ha_004.xml,ALL
#components[${n}]="${component[1]}"
#((n+=1))

#------------------------------------------------------------------
# Scenario 5 - jca_ha_005 (does a failover)
#------------------------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="jca_ha_005"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - JCA HA 005"
component[1]=jmsDriver,m1,ha/jca_ha_005.xml,ALL
component[2]=searchLogsEnd,m1,ha/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#------------------------------------------------------------------
# Scenario X - getWASlogs
# Get the logs before HA cleanup overwrites the entire
# log file with disconnect exceptions
#------------------------------------------------------------------
xml[${n}]="getWASlogs"
timeouts[${n}]=200
scenario[${n}]="${xml[${n}]} - get WAS logs"
component[1]=cAppDriver,m1,"-e|./scripts_was/getRAtrace.sh","-o|all"
# Use searchlogs to verify the servlet ran correctly
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# stopPrimary - We have an odd number of failovers,
# so get back to where A1 is primary.
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="stopPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - stop primary server A2. A1 should become primary"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|stopPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# startStandby
#----------------------------------------------------
# A1 = Primary
# A2 = Stopped
xml[${n}]="startStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - start A2 into standby mode"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#------------------------------------------------------------------
xml[${n}]="disabletrace"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - disable trace"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|disabletrace|-c|jca_policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))
#------------------------------------------------------------------
