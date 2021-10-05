#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# 1. Add member while publishing
# 2. Remove member while publishing
# 3. Reconnect durable after add new member
# 4. Reconnect durable after active member crash
# 5. Subscribe durably, remove from cluster, publish messages, rejoin cluster
# 6. Add a server with retained messages when cluster has none
# 7. Add a server with retained messages when server has some (no conflicts)
# 8. Add a server with retained messages when server has some (conflicts)
# 9. Add a server with retained messages when server has some (mixed conflicts/no conflicts)
# 10. Publish retained on A. Remove A. Add C.
# 11. Publish retained on A. C is down. Remove A. Bring up C. Add D.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Modify the cluster while messaging is being performed " 

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
#----------------------------------------------------
# All appliances running
xml[${n}]="testmqtt_clusterCM_012-checkCleanStore"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - clean store should be be prohibited on a server in maintenance mode with cluster enabled"
component[1]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartMaintenance|-i|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_MAINTENANCE"
component[3]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,CheckCleanStore
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|70|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - Bring a server into the cluster while publishing MQTT to existing cluster member
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Not in cluster
# A3 = Cluster Member 2
xml[${n}]="testmqtt_clusterCM_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Add a server to the cluster while messages are flowing "
component[1]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1Publish
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-n|${MQKEY}_ClusterCM"
component[3]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server2Subscribe
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - Remove a server from the cluster while publishing MQTT to remaining cluster member
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterCM_002"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Remove a server from the cluster while messages are flowing "
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeA
component[3]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server2Publish
#component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|1"
component[4]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeB
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - Reconnect durable subscriber after adding server to cluster
#----------------------------------------------------
# A1 = Not in cluster
# A2 = Cluster Member 1
# A2 = Cluster Member 2
xml[${n}]="testmqtt_clusterCM_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Add a member to cluster with existing durable subscriptions "
component[1]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeA
component[2]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server1PublishA
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_ClusterCM|-ca|${A1_IPv4_INTERNAL_1}|-ma|${A1_IPv4_INTERNAL_1}"
else
  component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_ClusterCM|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}"
fi
component[4]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeB
component[5]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1PublishB
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - Reconnect durable subscriber after restarting a cluster member which was down
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterCM_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Kill a cluster member while messages are flowing "
component[1]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|2|-d|0|-t|60"
component[2]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeA
component[3]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server2Publish
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|killClusterMember|-m|1"
component[5]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server2Publish
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[7]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|2|-d|0|-t|60"
component[8]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeB
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))

#----------------------------------------------------
# Scenario x - Restart dead cluster member 1 and teardown
#----------------------------------------------------
# A1 = dead
# A2 = Cluster Member 1
# A3 = Cluster Member 2
#xml[${n}]="testmqtt_clusterCM_004"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - Teardown cluster"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
#components[${n}]="${component[1]}"
#((n+=1))

#----------------------------------------------------
# Scenario 5 - Subscribe durably, remove from cluster, publish messages, rejoin cluster
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterCM_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Subscribe, remove, add, rejoin"
component[1]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeA
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|1|-v|2"
component[3]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server2Publish
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_ClusterCM|-v|2|-ca|${A1_IPv4_INTERNAL_1}|-ma|${A1_IPv4_INTERNAL_1}"
else
  component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_ClusterCM|-v|2|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}"
fi
component[5]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1SubscribeB
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Scenario 6 - add 2 more members to cluster to make 5
#----------------------------------------------------
# A1 = Running
xml[${n}]="createClusterOf5"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - add 2 members to have cluster of 5"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  # If we are running in EC2 we don't have multicast support so use discovery server list
  component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|4|-n|${MQKEY}_ClusterCM|-b|2|-v|2|-ca|${A4_IPv4_INTERNAL_1}|-ma|${A4_IPv4_INTERNAL_1}"
  component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|5|-n|${MQKEY}_ClusterCM|-b|1|-v|2|-ca|${A5_IPv4_INTERNAL_1}|-ma|${A5_IPv4_INTERNAL_1}"
  components[${n}]="${component[1]} ${component[2]}"
else
  component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|createCluster|-n|${MQKEY}_ClusterCM|-l|4|5|-v|2"
  components[${n}]="${component[1]}"
fi
((n+=1))


#----------------------------------------------------
# Scenario 7 - verify store after remove from cluster
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
# A4 = Cluster Member 4
xml[${n}]="testmqtt_clusterCM_013-verifyStore"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - verify store is valid after removing from cluster gracefully"
component[1]=sync,m1
component[2]=wsDriverWait,m1,cluster_manipulation/mqtt_clearRetained.xml,ALL
component[3]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,CleanSession
component[4]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server4Subscribe
component[5]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server2Subscribe
component[6]=jmsDriverWait,m1,cluster_manipulation/jms_Queues_ForStoreVerification.xml,QProd
component[7]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server5Publish
component[8]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,server4Publish
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|4|-v|2"
component[10]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-s|STATUS_RUNNING"
component[11]=wsDriverWait,m1,cluster_manipulation/${xml[${n}]}.xml,VerifyStore
component[12]=jmsDriverWait,m1,cluster_manipulation/jms_Queues_ForStoreVerification.xml,QCons
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]}"
((n+=1))

#----------------------------------------------------
# Scenario 7a - verify store after remove from cluster Part 2
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
# A4 = Standalone server who was removed in prior step. 
xml[${n}]="testmqtt_clusterCM_013-verifyStore_Part_2"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - ClusterManipulation verify Restart servers and verify all is well. "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|4|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))


#----------------------------------------------------
# Scenario 7c - clear retained tree and clean store A4
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
# A4 = Standalone server who was removed in prior step. 
xml[${n}]="testmqtt_clusterCM_013-verifyStore_reset_for_next"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - ClusterManipulation verify Restart servers and verify all is well. "
component[1]=wsDriverWait,m1,cluster_manipulation/mqtt_clearRetained.xml,ALL
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Cleanup to run store verify test again
#----------------------------------------------------
xml[${n}]="clusterCM_store_verify_reset"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set cluster configs for A4  "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC4|-c|cluster_manipulation/cluster_manipulation.cli|-a|4"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
	components[${n}]="${component[1]}"
else	
	component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC4CM|-c|cluster_manipulation/cluster_manipulation.cli|-a|4"
	components[${n}]="${component[1]} ${component[2]}"
fi	
((n+=1))


#----------------------------------------------------
# Add A4 back to cluster
#----------------------------------------------------
# A1 = Running
xml[${n}]="add_A4_back_to_cluster"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - add A4 back to cluster to run store verify test again"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  # If we are running in EC2 we don't have multicast support so use discovery server list
  component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|4|-n|${MQKEY}_ClusterCM|-b|2|-v|2|-ca|${A4_IPv4_INTERNAL_1}|-ma|${A4_IPv4_INTERNAL_1}"
  components[${n}]="${component[1]}"
else
  component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|createCluster|-n|${MQKEY}_ClusterCM|-l|4|-v|2"
  components[${n}]="${component[1]}"
fi
((n+=1))

#----------------------------------------------------
# Scenario 8 - verify store after remove from cluster - with kill during disable process
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
# A4 = Cluster Member 4
xml[${n}]="testmqtt_clusterCM_013-verifyStore-removekill"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - verify store is valid after removing from cluster and killing during disable process"
component[1]=sync,m1
component[2]=wsDriverWait,m1,cluster_manipulation/mqtt_clearRetained.xml,ALL
component[3]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,CleanSession
component[4]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,server4Subscribe
component[5]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,server2Subscribe
component[6]=jmsDriverWait,m1,cluster_manipulation/jms_Queues_ForStoreVerification.xml,QProd
component[7]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,server5Publish
component[8]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,server4Publish
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-dkil|1|-m|4|-v|2"
component[10]=sleep,5
component[11]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[12]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-s|STATUS_RUNNING"
component[13]=wsDriverWait,m1,cluster_manipulation/testmqtt_clusterCM_013-verifyStore.xml,VerifyStore
component[14]=jmsDriverWait,m1,cluster_manipulation/jms_Queues_ForStoreVerification.xml,QCons
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]}"
((n+=1))

#----------------------------------------------------
# Scenario 8a - verify store after remove from cluster 2 - Part 2
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
# A4 = Standalone server who was removed in prior step. 
xml[${n}]="testmqtt_clusterCM_013-verifyStore-removekill_Part_2"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - ClusterManipulation verify Restart servers and verify all is well after 2nd run. "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|4|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "
((n+=1))


#----------------------------------------------------
# Scenario 9- publish to A2 after verify store test
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterCM_014-publishAfter"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - publish 20 messages with maxmessages 10"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1Subscribe
component[3]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server2Publish
component[4]=wsDriver,m1,cluster_manipulation/${xml[${n}]}.xml,server1Receive
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


