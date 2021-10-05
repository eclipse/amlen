#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the setup
#  of the state for the common topic tree retained message
#  testing.
#
#----------------------------------------------------

scenario_set_name="Cluster RM Setup"

typeset -i n=0

#----------------------------------------------------
# Step 1 - Configure the endpoint on the 3 servers
#----------------------------------------------------
xml[${n}]="clusterRM_endpoint_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set minimal required parameters for configuring a Cluster of 2 and configure message hubs for retained messages bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|retained_messages/clusterRM_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|retained_messages/clusterRM_config.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3|-c|retained_messages/clusterRM_config.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC4|-c|retained_messages/clusterRM_config.cli|-a|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC5|-c|retained_messages/clusterRM_config.cli|-a|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Step 2 - Configure the sync timeouts on the 3 servers
#----------------------------------------------------
xml[${n}]="clusterRM_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - set shorter cluster sync timeouts for tests"
component[1]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-i|10|-d|40"
component[2]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|4|-i|10|-d|40"
component[3]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|5|-i|10|-d|40"
component[4]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|1|-i|10|-d|40"
component[5]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|2|-i|10|-d|40"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Step 3 - Configure a cluster of the 3 servers
#----------------------------------------------------
xml[${n}]="clusterRM_cluster_setup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - create cluster IMACLUSTER with 3 members"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|createCluster|-n|${MQKEY}_CLUSTER_CTT_RM|-l|1|2|3"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

