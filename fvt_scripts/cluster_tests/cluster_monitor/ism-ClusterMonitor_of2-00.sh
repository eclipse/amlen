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


xml[${n}]="deleteAllRetained"
scenario[${n}]="${xml[${n}]} - Setup stepDelete any RETAINed messages"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,cluster_monitor/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Set trace 9 for debug
#----------------------------------------------------
#xml[${n}]="trace9"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} - Set trace level 9"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|trace9|-c|common_topic_tree/clusterCTT_config.cli"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|trace9|-c|common_topic_tree/clusterCTT_config.cli|-a|2"
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario 2 - )
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCM_001"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  ClusterMonitoring Simple verification in a test with a cluster of 2. "
component[1]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,prepare
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,check
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
component[6]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,Pubmore
component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
component[9]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,UnSub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
((n+=1))

#----------------------------------------------------
# Set trace level 7
#----------------------------------------------------
#xml[${n}]="trace7"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} - Set trace level 7"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|trace7|-c|common_topic_tree/clusterCTT_config.cli"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|trace7|-c|common_topic_tree/clusterCTT_config.cli|-a|2"
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario 3 - )
# The syncing of deleted retained messages
# seems to make this test not work now.  
# 
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
#xml[${n}]="testmqtt_clusterCM_002"
#timeouts[${n}]=120
#scenario[${n}]="${xml[${n}]} -  ClusterMonitoring Simple verification of expired messages."
#component[1]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,prepare
c#omponent[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
#component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
#component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|2"
#component[5]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,Pubmore
#component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|startClusterMember|-m|2"
#component[7]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_RUNNING"
##component[8]=wsDriverWait,m1,cluster_monitor/${xml[${n}]}.xml,UnSub
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
#((n+=1))

#----------------------------------------------------
# Scenario 4 - )
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCM_003"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - ClusterMonitoring. Publish on both servers, Check that rates/sentmsgs exist."
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_monitor/${xml[${n}]}.xml,receive1
component[3]=wsDriver,m1,cluster_monitor/${xml[${n}]}.xml,receive2
component[4]=wsDriver,m2,cluster_monitor/${xml[${n}]}.xml,publish1
component[5]=wsDriver,m2,cluster_monitor/${xml[${n}]}.xml,publish2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))
