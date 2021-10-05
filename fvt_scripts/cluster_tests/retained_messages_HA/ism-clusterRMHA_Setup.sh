#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the setup
#  of the state for the common topic tree retained message
#  testing.
#
#----------------------------------------------------

scenario_set_name="Cluster RMHA Setup"

typeset -i n=0

#----------------------------------------------------
# Step 1 - Clean store on all servers
#----------------------------------------------------
xml[${n}]="clusterRMHA_cleanStore"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Clean the store of the servers that are to be used in the retained HA testing."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Step 2 - Configure the reduced sync timeouts and enable singleNIC mode on the servers
#----------------------------------------------------
xml[${n}]="clusterRMHA_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - set shorter cluster sync timeouts and enable single nic mode for the retained HA tests"
component[1]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|1|-i|10|-d|40"
component[2]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|2|-i|10|-d|40"
component[3]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-i|10|-d|40"
component[4]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|4|-i|10|-d|40"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Step 3 - Configure the endpoint/cluster/HA config on the servers
#----------------------------------------------------
xml[${n}]="clusterRMHA_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set up the Endpoint/Cluster/HA config on the servers to be used in the retained HA testing."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|retained_messages_HA/clusterRMHA_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|retained_messages_HA/clusterRMHA_config.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3|-c|retained_messages_HA/clusterRMHA_config.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC4|-c|retained_messages_HA/clusterRMHA_config.cli|-a|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC5|-c|retained_messages_HA/clusterRMHA_config.cli|-a|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Step 4A - restart the servers to affect the cluster/HA config updates
# 
#----------------------------------------------------
xml[${n}]="clusterRMHA_restart_to_be_removed"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Restart the servers to affect the cluster/HA config updates on the servers to be used in the retained HA testing."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|3"
component[3]=sleep,10
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|4"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))

#----------------------------------------------------
# Step 4 - restart the servers to affect the cluster/HA config updates
# Modify to remove the restarts, which will happen in the prior step 
# when autorestart for Enable/DisableCluster membership is dropped. 
#----------------------------------------------------
xml[${n}]="clusterRMHA_verify_started"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Restart the servers to affect the cluster/HA config updates on the servers to be used in the retained HA testing."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_STANDBY"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_STANDBY"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "
((n+=1))

