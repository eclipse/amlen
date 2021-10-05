#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Delete ClientSet and Close Connection functionality " 

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
# Setup Scenario  - Configure ClusterMembership on the 2 servers. 
#----------------------------------------------------
#xml[${n}]="clusterCS_config_setup"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} -ClientSet -Set minimal required parameters for configuring a Cluster of 2 for Client Set testing "
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|cluster_manipulation/cluster_manipulation.cli|-a|1"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
#	components[${n}]="${component[1]} ${component[2]}"
#else	
#	component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1CM|-c|cluster_manipulation/cluster_manipulation.cli"
#	component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2CM|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#fi	
#((n+=1))


#----------------------------------------------------
# Scenario 0 - Cleanup Client Set Delete #0
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp0"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete ALL, Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ClientSet_Verify0Subs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Scenario 1 - Basic Client Set Delete
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_basicClientSet"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete, Durable Subscriptions, MQTT disconnected, "
component[10]=sync,m1
component[1]=wsDriverWait,m1,client_set/${xml[${n}]}.xml,subscribe1
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|stopClusterMember|-m|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|10|-s|STATUS_STOPPED"
component[4]=wsDriverWait,m1,client_set/${xml[${n}]}.xml,publish1
component[7]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-BasicClientSet_Delete.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[9]=wsDriverWait,m1,client_set/${xml[${n}]}.xml,receive1
component[11]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[12]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[13]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[14]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"

#components[${n}]="${component[10]} ${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]}"
components[${n}]="${component[10]} ${component[1]}                                 ${component[4]}                                 ${component[7]}                 ${component[9]} ${component[11]} ${component[12]} ${component[13]} ${component[14]}"
((n+=1))


#----------------------------------------------------
# Scenario 2 - Cleanup Client Set Delete #1
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete ALL, Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ClientSet_Verify0Subs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "
((n+=1))


#----------------------------------------------------
# Scenario 3 - Mult Client Delete ClientSet
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_multiClientSet"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  ClientSet - Multi Client Delete, Durable Subscriptions, MQTT disconnected, "
component[1]=sync,m1
component[2]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe1
component[3]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe2
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe5
component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish1
component[8]=wsDriverWait,m2,client_set/${xml[${n}]}.xml,publish2
component[9]=sleep,2
component[10]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-MultiClientSet_Delete.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[15]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive1
component[16]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive2
component[17]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[18]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
component[19]=wsDriverWait,m2,client_set/${xml[${n}]}.xml,receive5
component[20]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[21]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[22]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[23]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}    ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]} ${component[22]} ${component[23]} "
((n+=1))


#----------------------------------------------------
# Scenario 4 - Cleanup Client Set Delete #2
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp2"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete ALL, Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ClientSet_Verify0Subs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "
((n+=1))

 

#----------------------------------------------------
# Scenario 5 - Active Connection Clients, ClientSet Delete
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_activeClientSet"
timeouts[${n}]=1000
scenario[${n}]="${xml[${n}]} -  ClientSet - Multi Client Delete, Durable Subscriptions, Active MQTT connections, "
component[1]=sync,m1
component[2]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe1
component[3]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe2
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe5
component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish1
component[8]=wsDriver,m2,client_set/${xml[${n}]}.xml,publish2
component[9]=sleep,60
component[10]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ActiveClientSet_Delete.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[15]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive1
component[16]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive2
component[17]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[18]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
component[19]=wsDriverWait,m2,client_set/${xml[${n}]}.xml,receive5
component[20]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[21]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[22]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[23]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}    ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]} ${component[22]} ${component[23]} "

((n+=1))

#----------------------------------------------------
# Scenario 6 - Cleanup Client Set Delete #3
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp3"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete ALL, Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ClientSet_Verify0Subs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "
((n+=1))



#----------------------------------------------------
# Scenario 7 - sleep@9 is necessary for Connection stats to show up.
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_activeCloseConn"
timeouts[${n}]=1000
scenario[${n}]="${xml[${n}]} -  ClientSet - Monitor Subscription, MQTTClient and Active Connection when Close Connection issued "
component[1]=sync,m1
component[2]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe1
component[3]=wsDriver,m1,client_set/${xml[${n}]}.xml,subscribe2
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe5
component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish1
component[8]=wsDriver,m2,client_set/${xml[${n}]}.xml,publish2
component[9]=sleep,60
component[10]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-CloseConnection.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[15]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive1
component[16]=wsDriver,m1,client_set/${xml[${n}]}.xml,receive2
component[17]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[18]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
component[19]=wsDriverWait,m2,client_set/${xml[${n}]}.xml,receive5
component[20]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1|-v|2"
component[21]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[22]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[23]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"

components[${n}]=" ${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]}    ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]} ${component[22]} ${component[23]} "

((n+=1))



#----------------------------------------------------
# Scenario 8 - Cleanup Client Set Delete #4
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp4"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} -  ClientSet - Delete ALL, Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-ClientSet_Verify0Subs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
components[${n}]="${component[1]} "
#((n+=1))





#----------------------------------------------------
# Teardown Scenario  - Reset ClusterMembership Defaults
#----------------------------------------------------
#xml[${n}]="clusterCs_config_cleanup"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} - Reset ClusterMembership configuration in DeleteClientSet tests"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|cluster_manipulation/cluster_manipulation.cli|-a|1"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#
#components[${n}]="${component[1]} ${component[2]} "
#((n+=1))

