#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# Thests tests are meant to verify that messaging still works under various CLI
# configurations, rather than just testing the various CLI settings.
#
# 1. Join with DiscoveryServerList IPv4, MC discovery on
# 2. Join with DiscoveryServerList IPv6, MC discovery on
# 3. Join with DiscoveryServerList IPv4, MC discovery off
# 4. Join with DiscoveryServerList IPv6, MC discovery off
# 5. ControlAddress IPv6
# 6. Messaging Address IPv6
# 7. All IPv6
# 8. Multicast discovery off, no discovery list
#
# 1. Valid updates while adding
# 2. Invalid updates while adding
# 3. Valid updates while removing
# 4. Invalid updates while removing
# 5. Valid updates while active
# 6. Invalid updates while active
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Modify the cluster while messaging is being performed " 

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
# Scenario 1a - Discovery server list ipv4 multicast on
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_DiscServerList_mcDiscTrue_IPv4"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast on"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI|-b|1|2|3|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI|-b|1|2|3|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI|-b|1|2|3|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1b - Test messaging server 1 to 2
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterCLI_001a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast on. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1c - Test messaging server 2 to 1
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterCLI_001b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast on. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1d - Teardown cluster for next group of tests
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Discovery Server List IPv4 With Multicast on"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2a - Discovery server list ipv6 multicast on
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_DiscServerList_mcDiscTrue_IPv6"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast on"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_2|-b|1|2|3|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-i|6"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_2|-b|1|2|3|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-i|6"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_2|-b|1|2|3|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}|-i|6"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_002a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast on. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_002b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast on. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Discovery Server List IPv6 With Multicast on"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3a - Discovery server list ipv4 multicast off
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_DiscServerList_mcDiscFalse_IPv4"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast off"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_3|-b|1|2|3|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-u|false"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_3|-b|1|2|3|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-u|false"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_3|-b|1|2|3|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}|-u|false"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_003a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast off. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_003b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv4 With Multicast off. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3c - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Discovery Server List IPv4 With Multicast off"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4a - Discovery server list ipv6 multicast off
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_DiscServerList_mcDiscFalse_IPv6"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast off"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_4|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-i|6|-u|false"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_4|-b|1|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-i|6|-u|false"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_4|-b|1|2|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}|-i|6|-u|false"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_004a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast off. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_004b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discovery Server List IPv6 With Multicast off. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Discovery Server List IPv6 With Multicast off"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5a - ControlAddress IPv6
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_ControlAddress_IPv6"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv6 Messaging address ipv4"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_5|-ca|${A1_IPv6_1}|-ma|${A1_IPv4_1}|-u|true"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_5|-ca|${A2_IPv6_1}|-ma|${A2_IPv4_1}|-u|true"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_5|-ca|${A3_IPv6_1}|-ma|${A3_IPv4_1}|-u|true"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_005a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv6 Messaging address ipv4. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_005b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv6 Messaging address ipv4. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 5d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Control address ipv6 Messaging address ipv4"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6a - MessagingAddress IPv6
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_MessagingAddress_IPv6"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv4 messaging address ipv6"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_6|-ca|${A1_IPv4_1}|-ma|${A1_IPv6_1}|-u|true"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_6|-ca|${A2_IPv4_1}|-ma|${A2_IPv6_1}|-u|true"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_6|-ca|${A3_IPv4_1}|-ma|${A3_IPv6_1}|-u|true"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_006a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv4 messaging address ipv6. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_006b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Control address ipv4 messaging address ipv6. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_006"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Control address ipv4 messaging address ipv6"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7a - MessagingAddress not set
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_No_MessagingAddress"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Messaging address not set"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_7|-ca|${A1_IPv4_1}|-ma|0|-i|4|-u|true"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_7|-ca|${A2_IPv4_1}|-ma|0|-i|4|-u|true"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_7|-ca|${A3_IPv4_1}|-ma|0|-i|4|-u|true"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_007a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Messaging address not set. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_007b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Messaging address not set. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 7d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_007"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Messaging address not set"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8a - TLS Enabled
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_MessagingUseTLS"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - TLS Enabled"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_8|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-i|4|-u|true|-tls|true"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_8|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-i|4|-u|true|-tls|true"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_8|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}|-i|4|-u|true|-tls|true"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_008a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - TLS Enabled. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_008b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - TLS Enabled. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 8d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_008"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. TLS Enabled"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9a - 1 entry discovery list
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_SingleServerDiscList"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Discoery list with just 1 server"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_9|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-i|4|-u|true"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_9|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-i|4|-u|false|-b|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_9|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}|-i|4|-u|false|-b|1"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 9b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_009a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 9c - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_009b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 9d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="teardown_cluster_009"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. TLS Enabled"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10a - Normal Cluster
#----------------------------------------------------
# A1 = single
# A2 = single
# A3 = single
xml[${n}]="setup_10_NormalCluster"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_10|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_10|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-v|2|-n|${MQKEY}_ClusterCLI_10|-ca|${A3_IPv4_1}|-ma|${A3_IPv4_1}"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 10b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_010a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 10c
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="testmqtt_clusterCLI_010b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 10d - Remove member 3 without restart
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = ClusterMember3
xml[${n}]="setup_10_RemoveServer_3"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Remove server 3 from cluster without a restart"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10e - Remove member 1 without restart
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = single
xml[${n}]="setup_10_RemoveServer_1"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Remove server 1 from cluster without a restart"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|1|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10f - Remove member 2 without restart
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
# A3 = single
xml[${n}]="setup_10_RemoveServer_2"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Remove server 2 from cluster without a restart"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|removeClusterMember|-m|2|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10g - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="teardown_cluster_011"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Multicast with different discovery ports"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|3|-v|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11a - Multicast and diff discovery ports
#----------------------------------------------------
# A1 = single
# A2 = single
xml[${n}]="setup_DifferentDiscoveryPorts"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Multicast with different discovery ports and Discovery List"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-v|2|-n|${MQKEY}_ClusterCLI_11|-ca|${A1_IPv4_1}|-ma|${A1_IPv4_1}|-i|4|-u|true|-dp|9105|-b|2"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-v|2|-n|${MQKEY}_ClusterCLI_11|-ca|${A2_IPv4_1}|-ma|${A2_IPv4_1}|-i|4|-u|true|-dp|9104"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 11b - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_011a"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 2 rcv on 1"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11c - Add a test that does some messaging
# Testcases are copied and renamed from CommonTopicTree
# Directory in cluster_tests/ismAutoCluster_ofMany.sh
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="testmqtt_clusterCLI_011b"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Normal cluster. pub to 1 rcv on 2"
component[1]=sync,m1
component[2]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,receive
component[3]=wsDriver,m1,cluster_cli/${xml[${n}]}.xml,publish
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11d - Teardown cluster for next group of tests
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="teardown_cluster_011"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Teardown cluster. Multicast with different discovery ports"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|tearDown|-l|1|2|-v|2"
components[${n}]="${component[1]}"
((n+=1))
