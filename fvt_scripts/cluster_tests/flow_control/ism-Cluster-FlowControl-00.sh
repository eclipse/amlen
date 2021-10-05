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

scenario_set_name="Cluster Flow Control Tests" 

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
# Scenario 1 - QoS 0
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterFC_001"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Publish and subscribe at QoS 0"
component[1]=sync,m1
component[2]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
component[3]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
component[4]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
component[5]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
component[6]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
component[7]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))



if [[ "${A1_TYPE}" == "DOCKER" ]] && [[ "${A1_LOCATION}" == "local" ]]; then
    #----------------------------------------------------
    # Scenario 2b - QoS 1 with tcpkill 
    #----------------------------------------------------
    # Only runs on docker
    # A1 = Cluster Member 1
    # A2 = Cluster Member 2
    # A3 = Cluster Member 3
    xml[${n}]="testmqtt_clusterFC_002-kil"
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Publish and subscribe at QoS 1 with tcpkill"
    component[1]=sync,m1
    component[2]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
    component[3]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
    component[4]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
    component[5]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
    component[6]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
    component[7]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
    ((n+=1))

    #----------------------------------------------------
    # Scenario 2c - QoS 0 with tcpkill 
    #----------------------------------------------------
    # Only runs on docker
    # A1 = Cluster Member 1
    # A2 = Cluster Member 2
    # A3 = Cluster Member 3
    xml[${n}]="testmqtt_clusterFC_005-kil"
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Publish and subscribe at QoS 0 with tcpkill"
    component[1]=sync,m1
    component[2]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
    component[3]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
    component[4]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
    component[5]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
    component[6]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
    component[7]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
    ((n+=1))


else
    #----------------------------------------------------
    # Scenario 2 - QoS 1
    #----------------------------------------------------
    # runs on non-docker
    # A1 = Cluster Member 1
    # A2 = Cluster Member 2
    # A3 = Cluster Member 3
    xml[${n}]="testmqtt_clusterFC_002"
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Publish and subscribe at QoS 1"
    component[1]=sync,m1
    component[2]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
    component[3]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
    component[4]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
    component[5]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
    component[6]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
    component[7]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
    ((n+=1))

fi

#----------------------------------------------------
# Scenario 3 - QoS 2
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterFC_003"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Publish and subscribe at QoS 2"
component[1]=sync,m1
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|restartServer|-i|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|1|-t|30|-v|2|-s|STATUS_RUNNING"
component[4]=cAppDriverLogWait,m1,"-e|./verifyConnectedServers.py","-o|-s|${A1_HOST}:${A1_PORT}|-c|2|-d|0|-t|30"
component[5]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
component[6]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
component[7]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
component[8]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
component[9]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
component[10]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
component[11]=searchLogsEnd,m1,flow_control/FC_003.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - Mixed QoS
#----------------------------------------------------
# A1 = Cluster Member 1
# A2 = Cluster Member 2
# A3 = Cluster Member 3
xml[${n}]="testmqtt_clusterFC_004"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Publish and subscribe at all QoS"
component[1]=sync,m1
component[2]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server1Subscribe
component[3]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server2Subscribe
component[4]=wsDriver,m1,flow_control/${xml[${n}]}.xml,server3Subscribe
component[5]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server1Publish
component[6]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server2Publish
component[7]=wsDriver,m2,flow_control/${xml[${n}]}.xml,server3Publish
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#myA1HOST=`echo ${A1_HOST} | cut -d' ' -f 1`
#myA2HOST=`echo ${A2_HOST} | cut -d' ' -f 1`
#myA3HOST=`echo ${A3_HOST} | cut -d' ' -f 1`
#----------------------------------------------------
# Scenario x - get trace
# Save this in case we don't use mounted docker
# drive?
#----------------------------------------------------
# All appliances running
#xml[${n}]="getTrace"
#timeouts[${n}]=50
#scenario[${n}]="${xml[${n}]} - get trace from docker"
#component[1]=cAppDriver,m1,"-e|ssh,-o|${A1_USER}@${myA1HOST}|docker|exec|${A1_SRVCONTID}|cat|/var/log/messagesight/imatrace.log|>|./imatrace.log.A1.txt"
#component[2]=cAppDriver,m1,"-e|ssh,-o|${A2_USER}@${myA2HOST}|docker|exec|${A2_SRVCONTID}|cat|/var/log/messagesight/imatrace.log|>|./imatrace.log.A2.txt"
#component[3]=cAppDriver,m1,"-e|ssh,-o|${A3_USER}@${myA3HOST}|docker|exec|${A3_SRVCONTID}|cat|/var/log/messagesight/imatrace.log|>|./imatrace.log.A3.txt"
#component[4]=cAppDriver,m1,"-e|scp,-o|${A1_USER}@${myA1HOST}:~/imatrace.log.A1.txt|."
#component[5]=cAppDriver,m1,"-e|scp,-o|${A2_USER}@${myA2HOST}:~/imatrace.log.A2.txt|."
#component[6]=cAppDriver,m1,"-e|scp,-o|${A3_USER}@${myA3HOST}:~/imatrace.log.A3.txt|."
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
#((n+=1))
