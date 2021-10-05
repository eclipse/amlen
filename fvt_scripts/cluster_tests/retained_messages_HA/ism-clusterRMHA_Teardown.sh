#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the teardown
#  of the state for the retained message testing.
#
#
#----------------------------------------------------

scenario_set_name="Cluster RMHA Teardown"

typeset -i n=0

#----------------------------------------------------
# Teardown Scenario - remove cluster, remove endpoint config, restore server_IMA.cfg
#----------------------------------------------------

xml[${n}]="clusterRMHA_endpoint_teardown"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Cleanup endpoint/cluster/HA config used in retained messages tests.  "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC1|-c|retained_messages_HA/clusterRMHA_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC2|-c|retained_messages_HA/clusterRMHA_config.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC3|-c|retained_messages_HA/clusterRMHA_config.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC4|-c|retained_messages_HA/clusterRMHA_config.cli|-a|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC5|-c|retained_messages_HA/clusterRMHA_config.cli|-a|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - disableHA
#----------------------------------------------------
# A1 = Primary
# A3 = Primary
xml[${n}]="disableHA_in_RMHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  Restart servers to disable HA and disable cluster and restart former standby members "
component[1]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|1|-s|STATUS_RUNNING"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|3|-s|STATUS_RUNNING"
component[3]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|5|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC2CM|-c|retained_messages_HA/clusterRMHA_config.cli|-a|2"
component[5]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|2|-s|STATUS_MAINTENANCE"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC4CM|-c|retained_messages_HA/clusterRMHA_config.cli|-a|4"
component[7]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|4|-s|STATUS_MAINTENANCE"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "
((n+=1))


xml[${n}]="clusterRMHA_config_teardown"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Restore default cluster sync timeouts.  "
component[1]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|1|-r|1"
component[2]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|2|-r|1"
component[3]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-r|1"
component[4]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|4|-r|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Step 4 - The servers were restarted in a prior step. 
# the standby's were put in Maintenance Mode.. restart them in production. 
# Verify they came back up correctly.  
#----------------------------------------------------
xml[${n}]="clusterRMHA_verify_started_TD"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Restart and  Verify servers running prior to Clean"
component[1]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartProduction|-i|2|-s|STATUS_RUNNING"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartProduction|-i|4|-s|STATUS_RUNNING"
component[3]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[5]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

xml[${n}]="clusterRMHA_cluster_cleanstore"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - clean store on all appliances"
component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|1"
component[2]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|2"
component[3]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|3"
component[4]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
component[5]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

