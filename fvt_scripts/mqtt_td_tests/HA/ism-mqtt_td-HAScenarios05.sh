#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="mqtt HAScenarios05"

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
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


#----------------------------------------------------
# Test Case 1 - HA Setup (rearrange primary == A1)
#----------------------------------------------------
xml[${n}]="testmqtt_HASetup"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Configure HA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 0 - mqtt_slow_policyconfig_setup
#----------------------------------------------------
xml[${n}]="policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli for the MaxMessages behavior bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupMM|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 0 - 
#----------------------------------------------------
xml[${n}]="policy_setup_DSS"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli for durable SS bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupSS|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------------------------------------
# Test Case 0 - mqtt_HA05 MaxMessages DiscardOldMessages Behavior after Failover
#----------------------------------------------------------------------------------
xml[${n}]="testmqtt_slow02ha"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Test Slow Subscriber behavior at QoS=0. Multiple subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,HA/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} "
((n+=1))

# the last test just has some shell actions to swap primary back to A1..
# but doesn't really wait for the server to come up?

#----------------------------------------------------
# Test Case 1 - mqtt_maxmsgBehaviorHA
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="testmqtt_maxmsgBehaviorHA"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test MaxMessagesBehavior=DiscardOldMessages behavior at QoS=1 in HA. Multiple subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,HA/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,HA/${xml[${n}]}.xml,Pub3,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Swap Primary
# Using cluster.py, but this is just HA, not cluster.
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="swapPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Swap primary server in HA pair"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|swapPrimary"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_STANDBY"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_maxmsgBehavior2HA
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="testmqtt_maxmsgBehavior2HA"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test MaxMessagesBehavior=DiscardOldMessages behavior at QoS=2 in HA. Multiple subscribers"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub1,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[3]=wsDriver,m1,HA/${xml[${n}]}.xml,PubSub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[4]=wsDriver,m1,HA/${xml[${n}]}.xml,Sub2,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
component[5]=wsDriver,m2,HA/${xml[${n}]}.xml,Pub3,,"-s|BITFLAG=-Djavax.net.ssl.trustStore=certs/A1/ibm.jks"
components[${n}]="${component[1]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Swap Primary
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="swapPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Swap primary server in HA pair"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|restartClusterMember|-m|2|-v|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|verifyStatus|-m|2|-t|30|-s|STATUS_STANDBY"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Test Case testmqtt_DSS_01_HA
#----------------------------------------------------
xml[${n}]="testmqtt_DSS_01_HA"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test that shared messages in store are available on secondary after failover [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,Pubs,-o_-l_7
component[3]=wsDriver,m2,HA/${xml[${n}]}.xml,Subs,-o_-l_7
component[4]=wsDriver,m2,HA/${xml[${n}]}.xml,Collector,-o_-l_7
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#component[5]=runCommand,m1,../common/serverKill.sh,1,testmqtt_DSS_01_HA
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Test Case testmqtt_DSS_02_HA
#----------------------------------------------------
xml[${n}]="testmqtt_DSS_02_HA"
timeouts[${n}]=360
scenario[${n}]="${xml[${n}]} - Test that original primary comes back up and becomes primary after new one fails [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=wsDriver,m1,HA/${xml[${n}]}.xml,Pubs,-o_-l_7
component[3]=wsDriver,m2,HA/${xml[${n}]}.xml,Subs,-o_-l_7
component[4]=wsDriver,m2,HA/${xml[${n}]}.xml,Collector,-o_-l_7
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#component[5]=runCommand,m1,../common/serverRestart1Kill2.sh,1,testmqtt_DSS_02_HA
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))



#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="policy_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup from ism-mqtt_td-HAScenarios05.sh"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupMM|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

xml[${n}]="policy_cleanup_DSS"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup durable SS config items"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupSS|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1
    #----------------------------------------------------
    xml[${n}]="mqtt_ha_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
