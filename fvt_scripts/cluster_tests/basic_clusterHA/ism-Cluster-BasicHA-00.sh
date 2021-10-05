#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# ism-Cluster-BasicHA-00.sh
# 1. Add an HA pair to a cluster
# 2. Convert 2 clustered servers to HA pair
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster HA Tests Part 1" 

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
# Reset cluster properties before running tests
#----------------------------------------------------
xml[${n}]="clusterHA_resetClusterProps"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - Reset cluster props to make sure we are clean from previous tests"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetClusterProps|-c|basic_clusterHA/policy_config.cli|-a|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetClusterProps|-c|basic_clusterHA/policy_config.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetClusterProps|-c|basic_clusterHA/policy_config.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetClusterProps|-c|basic_clusterHA/policy_config.cli|-a|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetClusterProps|-c|basic_clusterHA/policy_config.cli|-a|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - Add an HA pair to a cluster
# 1. set control and messaging address on the server that will be standby
# 2. convert server 1 and 2 to HA pair
# 3. enable cluster membership on serve r1
#		config replicates to server 2.
# 4. server 2 is stopped
# 5. verify server 2 stopped
# 6. restart server 1
# 7. start server 2
# 8. add server 3 to cluster
# 9. add server 4 to cluster
# 10. add server 5 to cluster
# 11. verify server 2 (HA primary) shows 3 connected servers in cluster
# 12. verify server 3 (standalone) shows 3 connected servers in cluster
# 13. verify server 4 (standalone) shows 3 connected servers in cluster
# 14. verify server 5 (standalone) shows 3 connected servers in cluster
#----------------------------------------------------
# A1 Standalone
# A2 Standalone
# A3 Standalone
# A4 Standalone
# A5 Standalone
xml[${n}]="clusterHA_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Enable Cluster on an HA Pair -- correctly"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|basic_clusterHA/policy_config.cli|-a|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA|-g|${MQKEY}_${xml[${n}]}_HAGroup1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|basic_clusterHA/policy_config.cli"
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|stopServer|-i|2"
component[5]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|checkStopped|-i|2"
component[6]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|1"
component[7]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|startServer|-i|2"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|addClusterMember|-m|3|-v|2|-ca|${A3_IPv4_INTERNAL_1}|-n|${MQKEY}_${xml[${n}]}_Cluster"
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|addClusterMember|-m|4|-v|2|-ca|${A4_IPv4_INTERNAL_1}|-n|${MQKEY}_${xml[${n}]}_Cluster"
component[10]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|addClusterMember|-m|5|-v|2|-ca|${A5_IPv4_INTERNAL_1}|-n|${MQKEY}_${xml[${n}]}_Cluster"
component[11]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|3|-d|0"
component[12]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|3|-d|0"
component[13]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|3|-d|0"
component[14]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A5_HOST}:${A5_PORT}|-c|3|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]}"
((n+=1))

# TODO: add messaging here between server 1 and 3
xml[${n}]="testmqtt_clusterHA_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - basic_clusterHA group:  messaging between HA pair and standalone in cluster"
component[1]=wsDriver,m1,basic_clusterHA/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - Attempt to add a backup server to a cluster member
#              (Enabling HA on a cluster member, but forgetting to
#				enable cluster!)
# 1. remove server 5 from cluster
# 2. Clean the store on server 5, so we can use it as a new HA server. 
# 3. set control and messaging address on server that will be standby (server 5)
# 4. convert server 4 and 5 to an HA pair, but they cannot sync as only one has
#    cluster configured. (We skip the restart in hafunctions.py
# 5. Restart the primary for HA.
# 6. Restart the standby for HA changes. 
# 7&8. Verify we failed to sync and ended up in Maintenace.
# 9. verify connected servers for server 2 shows 1 and disconnected shows 1. 
# 10. verify connected servers for server 3 shows 1 and disconnected shows 1.
#----------------------------------------------------
# A1 HA Primary cluster member
# A2 HA Standby of A1
# A3 Standalone cluster member
# A4 Standalone cluster member
# A5 Standalone cluster member
xml[${n}]="clusterHA_002Error"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Verify incorrect Cluster/HA config comes up in Maintenance Mode. "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py","-o|-a|removeClusterMember|-m|5|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC5|-c|basic_clusterHA/policy_config.cli|-a|5"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA|-g|${MQKEY}_${xml[${n}]}_HAGroup2|-m1|4|-m2|5|-skrs|1"
component[5]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|4"
component[6]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|5"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_MAINTENANCE"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_MAINTENANCE"
component[9]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|1|-d|1"
component[10]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|1|-d|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - Clean up the mess I made in step 2. 
# Get M4 and M5 back to standalone members.. one in the cluster, one with a 
# clean store. 
# 1. Disable HA on M4 
# 2. restart M4
# 3. verify M4 is running in prod.
# 4. Verify M4 is connected to cluster. 
# 5. Disable HA on M5 
# 6. restart M5
# 7. verify M5 is running in prod.
# 8. Clean M5's store again for next step. 
# 9&10. Verify the other servers. 
#----------------------------------------------------
# A1 HA Primary cluster member
# A2 HA Standby of A1
# A3 Standalone cluster member
# A4 MaintenanceMode, configured for HA and Cluster.  
# A5 MaintenanceMode, configured for HA and NOT Cluster.
xml[${n}]="clusterHA_002Recover"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Get the servers out of MM after testing they came up in Maintenance Mode. "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetHA|-c|basic_clusterHA/policy_config.cli|-a|4"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|4"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|2|-d|0"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetHA|-c|basic_clusterHA/policy_config.cli|-a|5"
component[6]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|5"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
component[9]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|2|-d|0"
component[10]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|2|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - Add a backup server to a cluster member
#              (Enabling HA on a cluster member)
# 
# 1. Enable ClusterMembership and set control and messaging address
#    on server that will be standby (server 5)
# 2. convert server 4 and 5 to an HA pair
# (Temporary step 3:  restart server 5 because HA Functions couldn't 
# if HAFunctions is not failing, then the restart of member 5 can be removed .
# 3. verify connected servers for server 2 shows 2
# 4. verify connected servers for server 3 shows 2
# 5. verify connected servers for server 4 shows 2
#----------------------------------------------------
# A1 HA Primary cluster member
# A2 HA Standby of A1
# A3 Standalone cluster member
# A4 Standalone cluster member
# A5 Standalone Server, not in cluster. 
xml[${n}]="clusterHA_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Convert a standalone cluster member into an HA pair, correctly "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC5CM|-c|basic_clusterHA/policy_config.cli|-a|5"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA|-g|${MQKEY}_${xml[${n}]}_HAGroup2|-m1|4|-m2|5"
component[3]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|5"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|2|-d|0"
component[5]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A3_HOST}:${A3_PORT}|-c|2|-d|0"
component[6]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|2|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

# TODO: add messaging here between 1 3 4. leave durable sub on server 1
xml[${n}]="testmqtt_clusterHA_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: messaging between 2 HA pairs and standalone in cluster"
component[1]=wsDriver,m1,basic_clusterHA/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
