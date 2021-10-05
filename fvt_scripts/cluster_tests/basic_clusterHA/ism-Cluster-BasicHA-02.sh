#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# ism-Cluster-BasicHA-02.sh
# 1. Disable HA on a clustered HA pair
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Cluster HA Tests Part 3" 

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



#A1 and A2 require manual restart, as they are not in cluster.  

xml[${n}]="clusterBasicHA_teardown"
timeouts[${n}]=69
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: Cleanup endpoint/cluster/HA config in BasicHA "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC1|-c|basic_clusterHA/policy_config.cli"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC2|-c|basic_clusterHA/policy_config.cli|-a|2"
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|2"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC3|-c|basic_clusterHA/policy_config.cli|-a|3"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC5|-c|basic_clusterHA/policy_config.cli|-a|5"
component[7]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanC4|-c|basic_clusterHA/policy_config.cli|-a|4"
component[8]=sleep,10
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
((n+=1))


#----------------------------------------------------
# Step 4 - The servers were restarted in the prior step. 
# Verify they came back up correctly.  
# A1 and A5 is in MM because when we disable HA on a standby, it restarts in MM
#
#----------------------------------------------------
xml[${n}]="cluster_BasicHA_verify_started_TD"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - basic_clusterHA group: Verify servers running"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_MAINTENANCE"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|3|-t|30|-v|2|-s|STATUS_RUNNING"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|4|-t|30|-v|2|-s|STATUS_RUNNING"
component[6]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|5|-t|30|-v|2|-s|STATUS_MAINTENANCE"
component[7]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh,-o|-a|restartProduction|-i|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "
((n+=1))

xml[${n}]="clusterBasicHA_cluster_cleanstore"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} -  basic_clusterHA group: clean store on all appliances"
component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|1"
component[2]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|2"
component[3]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|3"
component[4]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|4"
component[5]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|cleanStore|-m|5"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

