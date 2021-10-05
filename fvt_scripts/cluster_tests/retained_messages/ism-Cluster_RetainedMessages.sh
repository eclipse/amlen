#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for retained
#  message testing
#
#
#----------------------------------------------------

scenario_set_name="Set of tests to verify the functionality of retained messages in a cluster." 

typeset -i n=0

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Scenario 1 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterRM_001"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Retained Messages. Sync from non-originating cluster member"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[2]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[3]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetained
component[4]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[7]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetained2
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterRM_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Retained Messages. Conflicting Sync from non-originating cluster member and originator"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[2]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[3]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetained
component[4]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[7]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,waitForSync
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[10]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetained2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterRM_003"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Retained Messages. Sync from non-originating cluster member to new cluster member"
component[1]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[2]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetained
component[3]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[4]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|4|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A4_IPv4_1}|-cp|9207|-ma|${A4_IPv4_1}|-mp|9208|-dp|9106|-ttl|1|-u|true|-tls|false"
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[6]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetained2
component[7]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
xml[${n}]="testmqtt_clusterRM_004"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - CommonTopicTree Retained Messages. Check non-originating sync is requested from most up-to-date member"
component[1]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|4"
component[2]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-r|1"
component[3]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetainedRound1
component[4]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1 
component[5]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2
component[6]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[7]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[8]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetainedRound2
component[9]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1Round2
component[10]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2Round2 
component[11]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[12]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[13]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[14]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[15]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[16]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn4
component[17]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[18]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[19]=cAppDriverLogWait,m1,"-e|retained_messages/ism-clusterRM_UpdateClusterSyncTimeouts.sh,-o|-s|3|-i|10|-d|40"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]}  ${component[18]}  ${component[19]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
xml[${n}]="testmqtt_clusterRM_005"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - CommonTopicTree Retained Messages. Available non-originating server changes during sync"
component[1]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|4"
component[2]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[3]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2 
component[4]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[7]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[9]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,waitForSync
component[10]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[11]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[12]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
component[13]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn4
component[14]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[15]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[16]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[17]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
# A5 = ClusterMember5
xml[${n}]="testmqtt_clusterRM_006"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - CommonTopicTree Retained Messages. Test Non-originating sync to new cluster member after originating server has left the cluster"
component[1]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[2]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2 
component[3]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[4]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn4
component[5]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|1|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A1_IPv4_1}|-cp|9201|-ma|${A1_IPv4_1}|-mp|9202|-dp|9106|-ttl|1|-u|true|-tls|false"
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|5|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A5_IPv4_1}|-cp|9209|-ma|${A5_IPv4_1}|-mp|9210|-dp|9106|-ttl|1|-u|true|-tls|false"
component[7]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn5
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A1_IPv4_1}|-cp|9201|-ma|${A1_IPv4_1}|-mp|9202|-dp|9106|-ttl|1|-u|true|-tls|false"
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|4|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A4_IPv4_1}|-cp|9207|-ma|${A4_IPv4_1}|-mp|9208|-dp|9106|-ttl|1|-u|true|-tls|false"
component[10]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|5|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A5_IPv4_1}|-cp|9209|-ma|${A5_IPv4_1}|-mp|9210|-dp|9106|-ttl|1|-u|true|-tls|false"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}  ${component[8]} ${component[9]} ${component[10]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterRM_007"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - CommonTopicTree Retained Messages. Test basic Non-originating sync of null retained messages"
component[1]=sync,m1
component[2]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|4"
component[3]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|5"
component[4]=wsDriver,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[5]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1 
component[6]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2 
component[7]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[8]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForVerified
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[10]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,publishNullRetained
component[11]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyNullRetainedOn1 
component[12]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyNullRetainedOn2
component[13]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForNullRetained
component[14]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[15]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[16]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[17]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForSync
component[18]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyNoRetainedForNewSub
component[19]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[20]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|4"
component[21]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
# A5 = ClusterMember5
xml[${n}]="testmqtt_clusterRM_008"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - CommonTopicTree Retained Messages. Test non-orig sync of null retained messages to existing member from more up to date new member."
component[1]=sync,m1
component[2]=wsDriver,m1,retained_messages/${xml[${n}]}.xml,setupRetained
component[3]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1 
component[4]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn2 
component[5]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[6]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForVerified
component[7]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|3"
component[8]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,publishNullRetained
component[9]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyNullRetainedOn1 
component[10]=wsDriver,m2,retained_messages/${xml[${n}]}.xml,verifyNullRetainedOn2
component[11]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForNullRetained
component[12]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[13]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|4|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A4_IPv4_1}|-cp|9207|-ma|${A4_IPv4_1}|-mp|9208|-dp|9106|-ttl|1|-u|true|-tls|false"
component[14]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForSyncTo4
component[15]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[16]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|3"
component[17]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[18]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,waitForSync
component[19]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyNoRetainedForNewSub
component[20]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[21]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
# A5 = ClusterMember5
xml[${n}]="testmqtt_clusterRM_009"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Verify Retained Messages on a newly joining member are sent to the older members."
component[1]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,setupRetained5
component[2]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn5
component[3]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|5|-n|${MQKEY}_CLUSTER_CTT_RM|-ca|${A5_IPv4_1}|-cp|9209|-ma|${A5_IPv4_1}|-mp|9210|-dp|9106|-ttl|1|-u|true|-tls|false"
component[4]=sleep,5
component[5]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1
component[6]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[7]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetainedOn4
component[8]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[9]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
component[10]=sleep,5
component[11]=wsDriverWait,m2,retained_messages/${xml[${n}]}.xml,verifyRetained_on_Restarted2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]}"
((n+=1))


#----------------------------------------------------
# Scenario 9 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
# A4 = ClusterMember4
# A5 = ClusterMember5
xml[${n}]="testmqtt_clusterRM_009"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Recheck if retained messages have synced after 40 seconds"
component[1]=sleep,40
component[2]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetainedOn1
component[3]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetainedOn3
component[4]=wsDriverWait,m1,retained_messages/${xml[${n}]}.xml,verifyRetainedOn4
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
















