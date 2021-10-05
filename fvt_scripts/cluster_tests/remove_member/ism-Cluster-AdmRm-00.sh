#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# 1. QoS 0 messages only
# 2. QoS 0 and QoS 1 mixed
# 3. QoS 0 and QoS 2 mixed
# 4. QoS 0, QoS 1 and QoS 2 mixed
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster Admin-Remove-Member Tests" 

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
# Scenario 1 - Crash cluster member and remove the inactive member while other member is down, verify results
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterRM_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Administratively remove a cluster member and verify results."
component[1]=sync,m1
component[2]=wsDriver,m2,remove_member/${xml[${n}]}.xml,A1Subscribe,-o_-l_9
component[3]=wsDriver,m2,remove_member/${xml[${n}]}.xml,A2Subscribe
component[4]=wsDriver,m2,remove_member/${xml[${n}]}.xml,A3Subscribe
component[5]=wsDriver,m2,remove_member/${xml[${n}]}.xml,A4Subscribe
component[6]=wsDriver,m1,remove_member/${xml[${n}]}.xml,A1Publish,-o_-l_9
component[7]=wsDriver,m1,remove_member/${xml[${n}]}.xml,A2Publish,-o_-l_9
component[8]=wsDriver,m1,remove_member/${xml[${n}]}.xml,A3Publish,-o_-l_9
component[9]=wsDriver,m1,remove_member/${xml[${n}]}.xml,A4Publish,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))
# expected state when finished:
# A1, A2, A4 running production
# A3 down and removed

# bring up A3 and do the error cases testing here
# need to bring up A3 - probably will end up in MM
# teardown on A3 - should generate new UID - then join cluster again

#----------------------------------------------------
# Scenario 2 - Bring up A3 and rejoin cluster
#----------------------------------------------------
xml[${n}]="Admin_remove_A3-rejoin"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Bring up A3 and rejoin cluster"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|3|-v|2|-skv|1"
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|3"
#trying the restart a second time just for fun
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_RUNNING"
component[6]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|3"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3|-c|remove_member/remove_member.cli|-a|3"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  # If we are running in EC2 we don't have multicast support so use discovery server list
	component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-n|${MQKEY}_ClusterAdmRm|-v|2|-ca|${A3_IPv4_INTERNAL_1}|-ma|${A3_IPv4_INTERNAL_1}"
	component[10]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_RUNNING"
  components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}"
else
component[9]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3CM|-c|remove_member/remove_member.cli|-a|3"
component[10]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|createCluster|-n|${MQKEY}_ClusterAdmRm|-l|3|-v|2"
component[11]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_RUNNING"
  components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]}"
fi
((n+=1))
# expected state when finished:
# All 4 running production


#----------------------------------------------------
# Scenario 3 - Admin remove error cases
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterRM_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Admin remove error cases."
component[1]=wsDriverWait,m1,remove_member/${xml[${n}]}.xml,TryRemoveActive
component[2]=searchLogsEnd,m1,remove_member/AdmRm_002.comparetest
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeInactiveMember|-m|1|-m2r|3"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
# expected state when finished:
# A1, A2, A4 running production
# A3 down and removed


#----------------------------------------------------
# Scenario 2 - Test for correct propagation of removed member info
#----------------------------------------------------
# A1 = will be removed
# A2 = Cluster Member 2
# A4 = Cluster Member 4
xml[${n}]="Admin_remove_check_propagation"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - check propagation of removed member after full shutdown"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|4"
component[4]=sleep,10
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|60|-v|2|-s|STATUS_RUNNING"
component[9]=sleep,10
component[10]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|1|-d|1|-t|30"
component[11]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|1|-d|1|-t|30"
component[12]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeInactiveMember|-m|2|-v|2|-m2r|1"
component[13]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|1|-d|0|-t|30"
component[14]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|1|-d|0|-t|30"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]}"
((n+=1))
# expected state when finished:
# A2, A4 running production
# A3, A1 down and removed

#----------------------------------------------------
# Scenario 3 - Admin remove in a cluster of 2
#----------------------------------------------------
# A2 = Cluster Member 2
# A4 = Cluster Member 4 - will be removed
xml[${n}]="Admin_remove_cluster_of_2"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - ensure admin remove in cluster of 2 works properly"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|4"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeInactiveMember|-m|2|-v|2|-m2r|4"
component[3]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|0|-d|0"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[5]=sleep,5
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|0|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))
# expected state when finished:
# A2 running production
# A1, A3, A4 down and removed


#----------------------------------------------------
# Scenario 4 - bring removed member up while others all down
#----------------------------------------------------
# A2 = Cluster Member 2 - will go down
# A4 = Cluster Member 4 - will come back up
xml[${n}]="Admin_remove_cluster_of_2-Removed-Back-Up-Alone"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - removed member comes back up alone"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|60|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A4_HOST}:${A4_PORT}|-c|0|-d|1"
component[5]=sleep,5
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A2_HOST}:${A2_PORT}|-c|0|-d|0"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))
# expected state when finished:
# A2 running production
# A4 maintenance mode
# A1, A3 down and removed


#----------------------------------------------------
# Scenario 2 - Start servers before leaving test bucket
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="startServers_cleanup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Admin Remove - Restart servers after test complete"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[3]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[7]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))
# expected state when finished:
# A2 running production
# A1, A3, A4 maintenance mode 






#-------------------------------------------------------
#-------------------------------------------------------
# Teardown
# servers end up in maintenance mode so teardown is included here to get around test fails
#-------------------------------------------------------
#-------------------------------------------------------


#----------------------------------------------------
# Scenario 0 - tearDown - disable cluster then restart-maintenance-stop
#----------------------------------------------------
# All appliances running
xml[${n}]="tearDown"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  RemoveMember Tests: tear down all appliances to singleton machines"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|4|-v|2|-skv|1"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[3]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|2"
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|3"
component[5]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|4"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|60|-v|2|-s|STATUS_RUNNING"
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|60|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario  - Delete AdmRm test related configuration 
#----------------------------------------------------
xml[${n}]="clusterRM_config_cleanup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Delete admin-remove-member test related configuration"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|remove_member/remove_member.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|remove_member/remove_member.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|remove_member/remove_member.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|remove_member/remove_member.cli|-a|4"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario  - reset cluster sync timeouts 
#----------------------------------------------------
xml[${n}]="clusterRM_reset_sync_timeouts"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Cleanup config used in common topic tree tests.  "
component[1]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|1|-r|1"
component[2]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|2|-r|1"
component[3]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-r|1"
component[4]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|4|-r|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
