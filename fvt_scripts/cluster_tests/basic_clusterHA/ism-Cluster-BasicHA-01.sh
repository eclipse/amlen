#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# ism-Cluster-BasicHA-01.sh
# 1. Failover a clustered Primary server 
# 2. Remove HA pair from cluster
# 3. Stop standalone cluster member
# 4. Restart standalone cluster member
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster HA Tests Part 2" 

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
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

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Scenario 1 - Failover A1 Primary server
#----------------------------------------------------
# A1 HA Primary cluster member
# A2 HA Standby of A2
# A3 Standalone cluster member
# A4 HA Primary cluster member
# A5 HA Standby of A4
xml[${n}]="clusterHA_003"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Failover A1 Primary server"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|crashPrimary|-m1|1|-m2|2"
component[2]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|2|-d|0"
component[3]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|2|-d|0"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|2|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

# TODO: add messaging here between 2 3 4. Should have some durable sub that was created on 1 first
xml[${n}]="testmqtt_clusterHA_003"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: messaging between 2 HA pairs and standalone in cluster after a failover"
component[1]=wsDriver,m1,basic_clusterHA/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1a - Restart A1 Standby server
#----------------------------------------------------
# A1 HA Standby (Dead)
# A2 HA Primary cluster member
# A3 Standalone cluster member
# A4 HA Primary cluster member
# A5 HA Standby of A4
xml[${n}]="clusterHA_004"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Restart A1 Standby server"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|startStandby|-m1|1|-m2|2"
component[2]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|2|-d|0"
component[3]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|2|-d|0"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|2|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - Remove HA pair A1 A2 from cluster
#----------------------------------------------------
# A1 HA Standby of A2
# A2 HA Primary cluster member
# A3 Standalone cluster member
# A4 HA Primary cluster member
# A5 HA Standby of A4
xml[${n}]="clusterHA_005"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Remove HA pair A1 A2 from cluster"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|removeClusterMember|-m|2|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|1|-d|0"
component[3]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|1|-d|0"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|checkClusterStatus|-m|1|-cs|DISABLED"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|checkClusterStatus|-m|2|-cs|DISABLED"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

# TODO: messaging only between 3 and 4 now

#----------------------------------------------------
# Scenario 3 - Stop standalone cluster member
#----------------------------------------------------
# A1 HA Primary (non clustered)
# A2 HA Standby of A1
# A3 Standalone cluster member
# A4 HA Primary cluster member
# A5 HA Standby of A4
xml[${n}]="clusterHA_006"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Stop standalone cluster member"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|killClusterMember|-m|3|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|0|-d|1"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

# TODO: Send to server 4

#----------------------------------------------------
# Scenario 4 - Restart standalone cluster member
#----------------------------------------------------
# A1 HA Primary (non clustered)
# A2 HA Standby of A1
# A3 dead
# A4 HA Primary cluster member
# A5 HA Standby of A4
xml[${n}]="clusterHA_007"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Restart standalone cluster member"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|startClusterMember|-m|3|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|1|-d|0"
component[3]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|1|-d|0"
components[${n}]="${component[1]} ${component[2]}  ${component[3]}"
((n+=1))

# TODO: Receive on server 3
