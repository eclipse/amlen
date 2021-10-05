#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the teardown
#  of the state for the retained message testing.
#
#
#----------------------------------------------------

scenario_set_name="Cluster RM Teardown"

typeset -i n=0

#----------------------------------------------------
# Teardown Scenario - remove cluster, remove endpoint config, restore server_IMA.cfg
#----------------------------------------------------

xml[${n}]="clusterRM_endpoint_teardown"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Cleanup config used in retained messages tests.  "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC1|-c|retained_messages/clusterRM_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC2|-c|retained_messages/clusterRM_config.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC3|-c|retained_messages/clusterRM_config.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC4|-c|retained_messages/clusterRM_config.cli|-a|4"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC5|-c|retained_messages/clusterRM_config.cli|-a|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

xml[${n}]="clusterRM_config_teardown"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Cleanup config used in common topic tree tests.  "
component[1]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-r|1"
component[2]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|4|-r|1"
component[3]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|5|-r|1"
component[4]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|1|-r|1"
component[5]=cAppDriverLog,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|2|-r|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

xml[${n}]="clusterRM_cluster_teardown"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - tear down all appliances to singleton machines"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|4|5|-v|2"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="clusterRM_cluster_cleanstore"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - clean store on all appliances"
component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|1"
component[2]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|2"
component[3]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|3"
component[4]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
component[5]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))
