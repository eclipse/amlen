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


scenario_set_name="ATELM Scenarios AT1"
source ../scripts/commonFunctions.sh
source template5.sh

test_template_set_prefix "atelm_AT_ab_"

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
# setup act/bak link aggregate
#----------------------------
printf -v tname "atelm_AT_ab_%00d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup active backup"
timeouts[${n}]=60

lc=0

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|delete{S}aggregate-interface{S}agg0,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|delete{S}aggregate-interface{S}agg0,"

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|create{S}aggregate-interface{S}agg0;reset{S}member;member{S}eth2{S}eth3{S}eth4{S}eth5;aggregation-policy{S}active-backup;primary-member{S}eth2;ip;address{S}${A1_IPv4_AGG0}/23;exit;exit,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|create{S}aggregate-interface{S}agg0;reset{S}member;member{S}eth2{S}eth3{S}eth4{S}eth5;aggregation-policy{S}active-backup;primary-member{S}eth2;ip;address{S}${A1_IPv4_AGG0}/23;exit;exit,"

test_template_finalize_test
test_template5_ShowSummary

((++n))

#----------------------------
# setup ha
#----------------------------

test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
#----------------------------
# delete existing data
#----------------------------
printf -v tname "atelm_AT_ab_%02d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete data"
timeouts[${n}]=60

lc=0
component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"

test_template_finalize_test
test_template5_ShowSummary

((++n))

#----------------------------
#-
#----------------------------

if [ "${A1_TYPE}" == "ESX" ]; then
  declare HOURS=$((17))
  declare MINUTES=$((60*${HOURS}))
  timeouts[$n]=$((60*${MINUTES}+300))
else
  declare HOURS=$((17))
  declare MINUTES=$((60*${HOURS}))
  MINUTES=5
  timeouts[$n]=$((60*${MINUTES}+300))
fi

declare lc=0
#declare BATCH=2000
declare ID=0
declare STACK="-Xss1024m"
declare SSL="-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks{S}-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
declare SHARE="-Xshareclasses"
declare HEALTHCENTER="-Xhealthcenter"


xml[$n]="atelm_AT_ab_0$n"
test_template_initialize_test "atelm_AT_ab_0$n"
scenario[$n]="atelm_AT_ab_0$n "

#java svt.mqtt.mq.MqttSample -a subscribe -t /APP/1/CAR/# -s tcp://10.2.1.135:16999+tcp://10.2.1.138:16999 -n -1  -q 0 -c false -receiverTimeout 0:0
component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/CAR/#|-s|tcp://${A1_IPv4_2}:16999+tcp://${A2_IPv4_2}:16999|-i|u0000012|-u|u0000012|-p|imasvtest|-c|true|-e|false|-n|0|-v,-s|JVM_ARGS=-Xshareclasses"
test_template_compare_string[${lc}]="subscribed to topic"

component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runBASH.sh,-o|-b|true|-s|{S}|-q|{Q}|-e|java{S}-Xshareclasses{S}svt.mqtt.mq.MqttSample{S}-a{S}subscribe{S}-mqtt{S}3.1.1{S}-q{S}0{S}-t{S}/APP/1/CAR/#{S}-s{S}tcp://${A1_IPv4_2}:16999+tcp://${A2_IPv4_2}:16999{S}-i{S}u0000012{S}-u{S}u0000012{S}-p{S}imasvtest{S}-c{S}false{S}-e{S}false{S}-N{S}${MINUTES}{S}-receiverTimeout{S}0:0"

#component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/CAR/#|-s|tcp://${A1_IPv4_2}:16999+tcp://${A2_IPv4_2}:16999|-i|u0000012|-u|u0000012|-p|imasvtest|-c|false|-e|false|-N|${MINUTES}|-receiverTimeout|0:0,-s|JVM_ARGS=-Xshareclasses"

for i in $(eval echo {2..${M_COUNT}}); do
# CLIENTS=$(eval echo \$\{M${i}_COUNT\})
  CLIENTS=$(eval echo \$\{MYCOUNT[$i]\})
  BATCH=$(eval echo \$\{MYBATCH[$i]\})

  DIV=$(($CLIENTS/${BATCH}))
  REM=$(($CLIENTS%${BATCH}))

  if [ ${DIV} -gt 0 ]; then
    for j in $(eval echo {1..${DIV}}); do
      printf -v USER "c%07d" ${ID}
      component[$((++lc))]="cAppDriverWait,m${i},-e|${M1_TESTROOT}/scripts/svt-runBASH.sh,-o|-b|true|-s|{S}|-q|{Q}|-e|java{S}${STACK}{S}${SSL}{S}-Xshareclasses{S}svt.scale.vehicle.SVTVehicleScale{S}-server{S}${A1_IPv4_AGG0}{S}-server2{S}${A2_IPv4_AGG0}{S}-port{S}16112{S}-verbose{S}false{S}-vverbose{S}false{S}-userName{S}${USER}{S}-password{S}imasvtest{S}-mode{S}connect_once{S}-paho{S}${BATCH}{S}-minutes{S}${MINUTES}{S}-appid{S}1{S}-order{S}false{S}-qos{S}0{S}-stats{S}false{S}-listener{S}false{S}-ssl{S}true"


#      component[$((++lc))]="javaAppDriver,m${i},-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_AGG0}|-server2|${A2_IPv4_AGG0}|-port|16112|-verbose|false|-vverbose|false|-userName|${USER}|-password|imasvtest|-mode|connect_once|-paho|${BATCH}|-minutes|${MINUTES}|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-ssl|true,-s|JVM_ARGS=${STACK}|${SSL}|-Xshareclasses"
       ((ID+=${BATCH}))
    done
  fi
  if [ ${REM} -gt 0 ]; then
    printf -v USER "c%07d" ${ID}
    component[$((++lc))]="cAppDriverWait,m${i},-e|${M1_TESTROOT}/scripts/svt-runBASH.sh,-o|-b|true|-s|{S}|-q|{Q}|-e|java{S}${STACK}{S}${SSL}{S}-Xshareclasses{S}svt.scale.vehicle.SVTVehicleScale{S}-server{S}${A1_IPv4_AGG0}{S}-server2{S}${A2_IPv4_AGG0}{S}-port{S}16112{S}-verbose{S}false{S}-vverbose{S}false{S}-userName{S}${USER}{S}-password{S}imasvtest{S}-mode{S}connect_once{S}-paho{S}${REM}{S}-minutes{S}${MINUTES}{S}-appid{S}1{S}-order{S}false{S}-qos{S}0{S}-stats{S}false{S}-listener{S}false{S}-ssl{S}true"

#    component[$((++lc))]="javaAppDriver,m${i},-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_AGG0}|-server2|${A2_IPv4_AGG0}|-port|16112|-verbose|false|-vverbose|false|-userName|${USER}|-password|imasvtest|-mode|connect_once|-paho|${REM}|-minutes|${MINUTES}|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-ssl|true,-s|JVM_ARGS=${STACK}|${SSL}|-Xshareclasses"
     ((ID+=${REM}))
  fi
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


printf -v tname "atelm_AT_ab_%00d" ${n}

xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - unconfig link agg"
timeouts[${n}]=60

lc=0
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A1_HOST}|-e|delete{S}aggregate-interface{S}agg0,"
component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/svt-runCLI.sh,-o|-s|{S}|-a|${A2_HOST}|-e|delete{S}aggregate-interface{S}agg0,"

test_template_finalize_test
test_template5_ShowSummary

((++n))
