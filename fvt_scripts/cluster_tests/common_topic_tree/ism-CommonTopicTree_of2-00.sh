#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Simple tests to verify Basic Cluster functionality " 

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
# Scenario 1 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on Server 2 , receive Messages on Server 1. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on Server 1, receive Messages on Server 2. (reverse of prior test) "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on both servers, receive all Messages on both. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - and setup (Part 1) for scenario 12. 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on both servers, wildcard receiver and setup for Expiry test"
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))
#----------------------------------------------------
# Scenario 12/13 - and setup scenario 12 adn 13. 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_012"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - testmqtt_clusterCTT_013 CommonTopicTree. Setup for Multiple Expiry tests. Create Needed Subscriptions and such"
component[1]=wsDriver,m1,common_topic_tree/testmqtt_clusterCTT_012_Expiry.xml,SetupDSMember1
component[2]=wsDriver,m1,common_topic_tree/testmqtt_clusterCTT_012_Expiry.xml,SetupDSMember2
component[3]=wsDriver,m1,common_topic_tree/testmqtt_clusterCTT_013_Expiry.xml,SetupDSMember1
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Scenario 12 setup (Part 2)  Publish messages with Expiry.
# do it now, so they will expire before we get to the 
# actual test.  The actual test is the same name, later in 
# this script. 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_012_Expiry"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Setup for a Expiry test. Publish messages that expire. "
component[1]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,PublishMember1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,PublishMember2
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Publish on both servers, wildcard receivers, Mixed JMS and MQTT. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=jmsDriver,m1,common_topic_tree/${xml[${n}]}_JMS.xml,JMSSub1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish1
component[6]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
component[7]=jmsDriver,m2,common_topic_tree/${xml[${n}]}_JMS.xml,JMSPub1
component[8]=jmsDriver,m2,common_topic_tree/${xml[${n}]}_JMS.xml,JMSPub2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))


#----------------------------------------------------
# Scenario 6 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_006"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} -  Complex CommonTopicTree at QoS=0. Publish on both servers, receive subsets of messages on both. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_007"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Complex CommonTopicTree with complex wildcard. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_008"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} -  Complex CommonTopicTree at QoS= 1 and 2. Publish on both servers, receive subsets of messages on both. "
component[1]=sync,m1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - Basic retained messages.  
#
# 
# Some people may wonder why test 10 comes before test 9.
# Since test 9 takes the server down which currently (May 2015)
# breaks the forwarding.. I'm going out of order.. and 
# since test 9 takes the server down, I'll do some checking
# to make sure my retained messages are still there after. 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_010_RM"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, Retained Messages forwarded to Remote server and received from there. "
component[1]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,publish
component[2]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,receiveLocal
component[3]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,receiveRemote
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Scenario 12. Verify Expiry worked on remote servers. 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_012_Expiry"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Verify messages expire on remote and local servers, including those in the retained topic tree. "
component[1]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveDSMember1
component[2]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveDSMember2
component[3]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember1
component[4]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_009_DS"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, publishing when a member is down, messages held in ForwardingQueue "
#component[x]=sync,m1
component[1]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,SetupReceive
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|10|-s|STATUS_STOPPED"
component[4]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,publish
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[7]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,receive
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - Basic retained messages.  
#
# Check that the retained messages are still there after
# server up/down. 
# 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_011_RM"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, Retained Messages forwarded to Remote server and received from both servers after they stop/start "
component[1]=wsDriver,m1,common_topic_tree/${xml[${n}]}.xml,receiveLocal
component[2]=wsDriver,m2,common_topic_tree/${xml[${n}]}.xml,receiveRemote
components[${n}]="${component[1]} ${component[2]} "
((n+=1))

#----------------------------------------------------
# Scenario 10 - Basic Unsetting of retained messages.  
#
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_010_RMU"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, Unsetting Retained Messages across a cluster. Will fail until we drop the code for unset. "
component[1]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,publish
component[2]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,receiveLocal
component[3]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,receiveRemote
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Scenario 11 - Unset retained while server B is down 
#  
# Checks that unsetting of retained messages gets
# synchronised to a server that was down when the
# unsetting messages were published.
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_011_RMU"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - CommonTopicTree. Leo test. "
component[1]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,setupRetained
component[2]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,verifyRetained
component[3]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[4]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,unsetRetained
component[5]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,verifyNoMsgs1
component[6]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[7]=cAppDriverLogWait,m2,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
component[8]=sleep,30
component[9]=wsDriverWait,m2,common_topic_tree/${xml[${n}]}.xml,verifyNoMsgs2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - Note that we have to sleep for 45 seconds (or more)
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_013_Expiry"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, publishing when a member is down, messages held in ForwardingQueue should expire and be deleted."
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|1|-s|STATUS_STOPPED"
component[3]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,PublishMember2
component[4]=sleep,45
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[7]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveDSMember1
component[8]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember1
component[9]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))


#----------------------------------------------------
# Scenario 14 - )
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_014_FQ"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, Messages held in ForwardingQueue should be persistent across server restart and out-of-sync retained trees can happen."
component[1]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,SetupDSMember1
component[2]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,SetupDSMember2
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[4]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,PublishMember2
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|1"
component[8]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveDSMember1
component[9]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveDSMember2
component[10]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember1
component[11]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - )
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_014_FQU"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  CommonTopicTree, Clearing Retained messages when the servers do not have the same sets of retained"
component[1]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,PublishMember1
component[2]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember1
component[3]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,ReceiveNewMember2
components[${n}]="${component[1]} ${component[2]} ${component[3]} "
((n+=1))

#----------------------------------------------------
# Scenario 15 - )
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCTT_015_FQ"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  CommonTopicTree and ClusterStats.  Messages should stop being forwarded when subscription no longer exists. "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[3]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,prepare
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[5]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,Pubmore
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
component[8]=wsDriverWait,m1,common_topic_tree/${xml[${n}]}.xml,UnSub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))

