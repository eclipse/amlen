#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for retained
#  message testing in a clustered HA environment
#
#
#----------------------------------------------------

scenario_set_name="Set of tests to verify the functionality of retained messages in a clustered HA environment." 

typeset -i n=0

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Scenario 1 - publish to one pair, then check availability of retained messages on the other pair.
#----------------------------------------------------
# A1 = ClusterMember1 HA group 12
# A2 = ClusterMember2 HA group 12
# A3 = ClusterMember3 HA group 34
# A4 = ClusterMember4 HA group 34
xml[${n}]="testmqtt_clusterRMHA_001"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Retained Messages in HA cluster.  Test basic retained sync still works in HA env"
component[1]=wsDriverWait,m1,retained_messages_HA/${xml[${n}]}.xml,setupRetainedOnPair1
component[2]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOnPair2
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - publish to one pair, fail-over same pair, then check availability of retained messages on same pair.
#----------------------------------------------------
# A1 = ClusterMember1 HA group 12
# A2 = ClusterMember2 HA group 12
# A3 = ClusterMember3 HA group 34
# A4 = ClusterMember4 HA group 34
xml[${n}]="testmqtt_clusterRMHA_002"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Retained Messages in HA cluster.  Test basic publish/failover/receive on same cluster HA node"
component[1]=wsDriverWait,m1,retained_messages_HA/${xml[${n}]}.xml,setupRetainedOnPair1
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[3]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOnPair1
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_STANDBY"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - publish to one pair, fail-over second pair, then check availability of retained messages on second pair.
#----------------------------------------------------
# A1 = ClusterMember1 HA group 12
# A2 = ClusterMember2 HA group 12
# A3 = ClusterMember3 HA group 34
# A4 = ClusterMember4 HA group 34
xml[${n}]="testmqtt_clusterRMHA_003"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Retained Messages in HA cluster.  Test basic publish/fail-over/receive in ha cluster"
component[1]=wsDriverWait,m1,retained_messages_HA/${xml[${n}]}.xml,setupRetainedOnPair1
component[2]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOnPair2
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|3"
component[4]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_STANDBY"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[6]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOnPair2AfterFailOver
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - publish to one pair in cluster, verify receipt on second pair, then failover first pair before adding a new cluster member
#----------------------------------------------------
# A1 = ClusterMember1 HA group 12
# A2 = ClusterMember2 HA group 12
# A3 = ClusterMember3 HA group 34
# A4 = ClusterMember4 HA group 34
# A5 = Stand-alone server to be added to the cluster
xml[${n}]="testmqtt_clusterRMHA_004"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Retained Messages in HA cluster.  Test sync to new node after originator pair failover"
component[1]=wsDriverWait,m1,retained_messages_HA/${xml[${n}]}.xml,setupRetainedOnPair1
component[2]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOnPair2
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[4]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|5|-n|${MQKEY}_CLUSTER_CTT_RMHA|-ca|${A5_IPv4_1}|-cp|9209|-ma|${A5_IPv4_1}|-mp|9210|-dp|9106|-ttl|1|-u|true|-tls|false"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_RUNNING"
component[7]=wsDriverWait,m2,retained_messages_HA/${xml[${n}]}.xml,verifyRetainedOn5
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|4"
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[10]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_STANDBY"
component[11]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_STANDBY"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]}"
((n+=1))


