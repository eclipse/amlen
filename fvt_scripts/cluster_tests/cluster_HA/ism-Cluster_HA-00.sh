#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# 1. QoS 0 messages only
# 2. QoS 0 and QoS 1 mixed
# 3. QoS 0 and QoS 2 mixed
# 4. QoS 0, QoS 1 and QoS 2 mixed
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster-HA tests" 

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
# Scenario 1 - Test failover while sending qos1,2
#----------------------------------------------------
# A1 = Primary 1 - cluster with A3
# A2 = Standby 1
# A3 = Primary 2 - cluster with A1
# A4 = Standby 2 
xml[${n}]="testmqtt_cluster_HA_002"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Failover on receiver while publishing qos 1,2"
component[1]=sync,m1
component[2]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,cluster_HA/${xml[${n}]}.xml,A1publish
component[4]=wsDriver,m1,cluster_HA/${xml[${n}]}.xml,A3subscribe
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - Test failover while sending qos1,2
#----------------------------------------------------
# A1 = Primary 1 - cluster with A3
# A2 = Standby 1
# A3 = Standby 2 - cluster with A1
# A4 = Primary 2 
xml[${n}]="testmqtt_cluster_HA_003"
timeouts[${n}]=430
scenario[${n}]="${xml[${n}]} - Failover on publisher while publishing qos 1,2"
component[1]=sync,m1
component[2]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,CleanSession
component[3]=wsDriver,m1,cluster_HA/${xml[${n}]}.xml,A1publish
component[4]=wsDriver,m1,cluster_HA/${xml[${n}]}.xml,A4subscribe
#component[5]=searchLogsEnd,m1,cluster_HA/cluster_HA_003.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
#expected state
#A1 standby
#A2 primary
#A3 standby
#A4 primary


#----------------------------------------------------
# Scenario 3 - Test disabling HA
#----------------------------------------------------
# A1 = Standby 1
# A2 = Primary 1 - cluster with A4
# A3 = Standby 2
# A4 = Primary 2 - cluster with A2 
xml[${n}]="testmqtt_cluster_HA_004-disableHAtest"
timeouts[${n}]=400	
scenario[${n}]="${xml[${n}]} - Test disabling HA on cluster members"
# Test disabling HA on standby
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_STANDBY"
component[2]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,DisableHAonStandby
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
# process to bring up server 1 back into cluster/HA
component[5]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,RestartCleanStore
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[7]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[8]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,SetConfigs
component[9]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1"
component[10]=sleep,10
component[11]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
# all 4 servers should be running in production now
# Test disabling HA on Primary
component[12]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,DisableHAonPrimary
component[13]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|80|-v|2|-s|STATUS_MAINTENANCE"
component[14]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|60|-v|2|-s|STATUS_RUNNING"
component[15]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,EnableHAonA2
# process to bring up server 1 back into cluster/HA
component[16]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,RestartCleanStore
component[17]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_MAINTENANCE"
component[18]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[19]=wsDriverWait,m1,cluster_HA/${xml[${n}]}.xml,SetConfigs
component[20]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|1"
component[21]=sleep,10
component[22]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|60|-v|2|-s|STATUS_RUNNING"
# all 4 servers should be running in production now

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]} ${component[22]}"
((n+=1))


