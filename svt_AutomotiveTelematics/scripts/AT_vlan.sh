#!/bin/bash 

# 'Source' the Automated Env. Setup file, testEnv.sh,  if it exists
#  otherwise a User's Manual run is assumed and ISmsetup file provides machine env. information.
if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
#  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi


#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


source ./load.sh


scenario_set_name="ATELM Scenarios AT_vlan"
source ../scripts/commonFunctions.sh
source template5.sh

test_template_set_prefix "atelm_AT_vlan_"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#               component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or    component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#       where:
#   <SubControllerName>
#               SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#               m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController
#               The configuration file to drive the test case using this controller.
#       <OTHER_OPTS>    is used when configuration file may be over kill,
#                       parameters are passed as is and are processed by the subController.
#                       However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#       component[x]=sync,<machineNumber_ismSetup>,
#       where:
#               <m1>                    is the machine 1
#               <m2>                    is the machine 2
#
#   Sleep:
#       component[x]=sleep,<seconds>
#       where:
#               <seconds>       is the number of additional seconds to wait before the next component is started.
#
#


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------
# setup ha
#----------------------------
test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

((++n))
#----------------------------
# setup switch vlan  (must wait 30ish seconds to become active)
#----------------------------
printf -v tname "atelm_AT_vlan_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup switch vlan"
timeouts[${n}]=60
lc=0


component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runBASH.sh,-o|-s|{S}|-q|{Q}|-c|{C}|-z|{Z}|-e|/usr/bin/sshpass{S}-p{S}nimda{S}ssh{S}admin@10.10.174.7{S}{Q}enable{Z}configure{S}terminal{Z}interface{S}port{S}38{C}40{C}59{Z}switchport{S}trunk{S}allowed{S}vlan{S}2{C}168{Z}exit{Z}write{Z}end{Q},"
component[$((++lc))]="sleep,30"

test_template_finalize_test
test_template5_ShowSummary

((++n))

#----------------------------
# setup appliance vlan
#----------------------------
printf -v tname "atelm_AT_vlan_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup vlan0"
timeouts[${n}]=60
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|delete{S}vlan-interface{S}vlan0,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|delete{S}vlan-interface{S}vlan0,"

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|create{S}vlan-interface{S}vlan0;userdata{S}private;{S}sub-interface{S}eth6;outbound-priority{S}5;vlan-identifier{S}168;ip;use-dhcp{S}false;address{S}10.168.1.135/23;exit;exit"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|create{S}vlan-interface{S}vlan0;userdata{S}private;{S}sub-interface{S}eth6;outbound-priority{S}5;vlan-identifier{S}168;ip;use-dhcp{S}false;address{S}10.168.1.138/23;exit;exit"


test_template_finalize_test
test_template5_ShowSummary

((++n))


#----------------------------
# delete existing data
#----------------------------
printf -v tname "atelm_AT_vlan_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60
lc=0

component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
component[$((++lc))]="sleep,10"
component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"
component[$((++lc))]="sleep,10"

test_template_finalize_test
test_template5_ShowSummary

((++n))
#----------------------------
#-
#----------------------------
printf -v tname "atelm_AT_vlan_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - create endpoints"
timeouts[${n}]=60
lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}Endpoint{S}{Q}Name=SVT-IntranetEndpoint{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}Endpoint{S}{Q}Name=SVT-CarsInternetEndpoint{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}MessagingPolicy{S}{Q}Name=SVTPubMsgPol-car{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}MessagingPolicy{S}{Q}Name=SVTSubMsgPol-app{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}ConnectionPolicy{S}{Q}Name=SVTUnsecureConnectPolicy{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}delete{S}ConnectionPolicy{S}{Q}Name=SVTSecureCarsConnectPolicy{Q},"
component[$((++lc))]="sleep,10"

# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}MessageHub{S}{Q}Name=SVTAutoTeleHub{Q}{S}{Q}Description=SVT{S}Automotive{S}Telematics{S}Hub{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}ConnectionPolicy{S}{Q}Name=SVTSecureCarsConnectPolicy{Q}{S}{Q}Protocol=MQTT{C}JMS{Q}{S}{Q}Description=SVTSecureConnectionPolicyForCars{Q}{S}{Q}GroupID=svtCarsInternet{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}ConnectionPolicy{S}{Q}Name=SVTUnsecureConnectPolicy{Q}{S}{Q}Protocol=MQTT{C}JMS{Q}{S}{Q}Description=SVTUnsecureConnectionPolicyForTestAutomation{Q},"
{Q}
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}MessagingPolicy{S}{Q}Name=SVTSubMsgPol-app{Q}{S}{Q}Destination=/APP/1/*{Q}{S}{Q}DestinationType=Topic{Q}{S}{Q}ActionList=Subscribe{Q}{S}{Q}Protocol=MQTT{C}JMS{Q}{S}{Q}MaxMessages=20000000{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}MessagingPolicy{S}{Q}Name=SVTPubMsgPol-car{Q}{S}{Q}Destination=/APP/1/CAR/*{Q}{S}{Q}DestinationType=Topic{Q}{S}{Q}ActionList=Publish{Q}{S}{Q}Protocol=MQTT{Q}{S}{Q}MaxMessages=20000000{Q},"

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}Endpoint{S}{Q}Name=SVT-CarsInternetEndpoint{Q}{S}{Q}Enabled=True{Q}{S}{Q}Port=16112{Q}{S}{Q}MessageHub=SVTAutoTeleHub{Q}{S}{Q}Interface=${A1_IPv4_2}{Q}{S}{Q}MaxMessageSize=256MB{Q}{S}{Q}ConnectionPolicies=SVTSecureCarsConnectPolicy{Q}{S}{Q}MessagingPolicies=SVTPubMsgPol-car{Q}{S}{Q}SecurityProfile=SVTSecProf{Q},"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-c|{C}|-q|{Q}|-e|imaserver{S}create{S}Endpoint{S}{Q}Name=SVT-IntranetEndpoint{Q}{S}{Q}Enabled=True{Q}{S}{Q}Port=16999{Q}{S}{Q}MessageHub=SVTAutoTeleHub{Q}{S}{Q}Interface=${A1_IPv4_VLAN0}{Q}{S}{Q}MaxMessageSize=256MB{Q}{S}{Q}ConnectionPolicies=SVTUnsecureConnectPolicy{Q}{S}{Q}MessagingPolicies=SVTSubMsgPol-app{Q},"

test_template_finalize_test
test_template5_ShowSummary

((++n))



#----------------------------
#-
#----------------------------

declare HOURS=0
declare MINUTES=0

if [ "${A1_TYPE}" == "ESX" ]; then
  HOURS=$((17))
  MINUTES=$((60*${HOURS}))
  timeouts[$n]=$((60*${MINUTES}+300))
else
  HOURS=$((16))
  MINUTES=$((60*${HOURS}))
  MINUTES=10
  timeouts[$n]=$((60*${MINUTES}+300))
fi

declare lc=0
#declare BATCH=2000
declare ID=0
declare STACK="-Xss1024m"
#declare SSL="-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks{S}-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
declare SSL="-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
declare SHARE="-Xshareclasses"
declare HEALTHCENTER="-Xhealthcenter"


xml[$n]="atelm_AT2_0$n"
test_template_initialize_test "atelm_AT_vlan_0$n"
scenario[$n]="atelm_AT2_0$n "

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/CAR/#|-s|tcp://${A1_IPv4_VLAN0}:16999+tcp://${A2_IPv4_VLAN0}:16999|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0|-v,-s|JVM_ARGS=-Xshareclasses"
test_template_compare_string[${lc}]="subscribed to topic"

component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/CAR/#|-s|tcp://${A1_IPv4_VLAN0}:16999+tcp://${A2_IPv4_VLAN0}:16999|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-N|${MINUTES}|-receiverTimeout|0:0,-s|JVM_ARGS=-Xshareclasses"

#component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/CAR/#|-s|tcp://${A1_IPv4_VLAN0}:16999|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-N|${MINUTES}|-receiverTimeout|0:0,-s|JVM_ARGS=-Xshareclasses"


for i in $(eval echo {1..${TOTAL}}); do
      printf -v USER "c%07d" ${ID}
      component[$((++lc))]="javaAppDriver,${WHERE[$i]},-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_2}|-server2|${A2_IPv4_2}|-port|16112|-verbose|false|-vverbose|false|-userName|${USER}|-password|imasvtest|-mode|connect_once|-paho|${HOWMANY[$i]}|-minutes|${MINUTES}|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-ssl|true,-s|JVM_ARGS=-Xshareclasses|${STACK}|${SSL}"
#     component[$((++lc))]="javaAppDriver,${WHERE[$i]},-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_2}|-port|16112|-verbose|false|-vverbose|false|-userName|${USER}|-password|imasvtest|-mode|connect_once|-paho|${HOWMANY[$i]}|-minutes|${MINUTES}|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-ssl|true,-s|JVM_ARGS=-Xshareclasses|${STACK}|${SSL}"

       ((ID+=${HOWMANY[$i]}))
done

# component[$((++lc))]="sleep,30"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="sleep,60"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
# component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_02|SVT_AT_VARIATION_QUICK_EXIT=true"
# test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"

#component[$((++lc))]="searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9"
#for i in $(eval echo {1..${M_COUNT}}); do
  #component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
#done

test_template_finalize_test
test_template5_ShowSummary

unset lc
unset MESSAGES
unset MINUTES


((++n))
#----------------------------
# disable ha
#----------------------------
# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

#----------------------------
# unconfigure vlan
#----------------------------

printf -v tname "atelm_AT_vlan_%02d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - unconfig vlan"
timeouts[${n}]=60

lc=0
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|delete{S}vlan-interface{S}vlan0,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|delete{S}vlan-interface{S}vlan0,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runBASH.sh,-o|-s|{S}|-q|{Q}|-c|{C}|-z|{Z}-e|/usr/bin/sshpass{S}-p{S}nimda{S}ssh{S}admin@10.10.174.7{S}{Q}enable{Z}configure{S}terminal{Z}interface{S}port{S}38{C}40{C}59{Z}no{S}vlan{S}168{Z}exit{Z}write{Z}exit{Q},"

test_template_finalize_test
test_template5_ShowSummary

((++n))

