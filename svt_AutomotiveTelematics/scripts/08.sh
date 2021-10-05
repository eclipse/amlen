#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh
#source template6.sh

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


scenario_set_name="MsgExpry Scenarios 01"

test_template_set_prefix "jmqtt_07_"

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
# delete existing subscriptions and clients
#----------------------------
printf -v tname "atelm_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete all subscriptions and mqtt clients"
timeouts[${n}]=1800

component[1]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllSubscriptions.py"
component[2]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllMqttClients.py,-o|-m|1800"

test_template_finalize_test
((n+=1))



#----------------------------
# setup ha and oauth policies
#----------------------------
printf -v tname "atelm_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup ha and oauth policies"
timeouts[${n}]=600

component[1]="sleep,1"
component[1]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|300,"
component[2]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|oauth_policy.cli,"
component[3]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|2|-c|oauth_policy.cli,"
component[4]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|setup|-a|1|-c|oauth_policy.cli,"
component[5]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|setup|-a|2|-c|oauth_policy.cli,"
component[6]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|300,"

test_template_finalize_test

((++n))

#----------------------------
#----------------------------

declare -r demo_ep=tcp://${A1_IPv4_1}:21004


printf -v tname "atelm_08_%02d" ${n}
xml[${n}]="${tname}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.mqttv3.MqttSample QoS 0 msgs [1 Pub; 4 Sub] with Topic wildcard Subscription"
timeouts[${n}]=900

component[1]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/#|-s|tcp://${A1_IPv4_2}:16999|-n|0|-exact,-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
component[2]="javaAppDriverWait,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/APP/1/#|-s|tcp://${A1_IPv4_2}:16999|-n|0|-v,-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
component[3]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/#|-s|tcp://${A1_IPv4_2}:16999|-n|9000|-exact,-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
component[4]="javaAppDriver,m1,-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/APP/1/#|-s|tcp://${A1_IPv4_2}:16999|-n|9000|-v,-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
component[5]="sleep,3"
component[6]="javaAppDriver,m2,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0000000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[7]="javaAppDriver,m3,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0000500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[8]="javaAppDriver,m4,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0001000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[9]="javaAppDriver,m5,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0001500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[10]="javaAppDriver,m6,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0002000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[11]="javaAppDriver,m2,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0002500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[12]="javaAppDriver,m3,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0003000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[13]="javaAppDriver,m4,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0003500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[14]="javaAppDriver,m5,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0004000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[15]="javaAppDriver,m6,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0004500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#
#component[16]="javaAppDriver,m2,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0005000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[17]="javaAppDriver,m3,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0005500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[18]="javaAppDriver,m4,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0006000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[19]="javaAppDriver,m5,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0006500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[20]="javaAppDriver,m6,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0007000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#
#component[21]="javaAppDriver,m2,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0007500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[22]="javaAppDriver,m3,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0008000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[23]="javaAppDriver,m4,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0008500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[24]="javaAppDriver,m5,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0009000|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[25]="javaAppDriver,m6,-e|svt.scale.vehicle.SVTVehicle,-o|-server|${A1_IPv4_1}|-port|21004|-ssl|false|-userName|c0009500|-password|imasvtest|-mode|connect_once|-paho|500|-minutes|6|-appid|1|-order|false|-qos|0|-stats|false|-listener|false|-vverbose|true|-oauthProviderURI|${LTPAWAS_IP}:9443|-oauthUser|imaclient|-oauthPassword|password|-oauthKeyStore|../common_src/ibm.jks|-oauthKeyStorePassword|password|-oauthTrustStore|../common_src/ibm.jks|-oauthTrustStorePassword|password,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=/niagara/test/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

test_template_compare_string[3]="eceived 9000 messages."
test_template_compare_string[4]="received 9000 messages."
test_template_compare_string[6]="SVTVehicleScale Success"
test_template_compare_string[7]="SVTVehicleScale Success"
test_template_compare_string[8]="SVTVehicleScale Success"
test_template_compare_string[9]="SVTVehicleScale Success"
test_template_compare_string[10]="SVTVehicleScale Success"
test_template_compare_string[11]="SVTVehicleScale Success"
test_template_compare_string[12]="SVTVehicleScale Success"
test_template_compare_string[13]="SVTVehicleScale Success"
test_template_compare_string[14]="SVTVehicleScale Success"
test_template_compare_string[15]="SVTVehicleScale Success"
#test_template_compare_string[16]="SVTVehicleScale Success"
#test_template_compare_string[17]="SVTVehicleScale Success"
#test_template_compare_string[18]="SVTVehicleScale Success"
#test_template_compare_string[19]="SVTVehicleScale Success"
#test_template_compare_string[20]="SVTVehicleScale Success"
#test_template_compare_string[21]="SVTVehicleScale Success"
#test_template_compare_string[22]="SVTVehicleScale Success"
#test_template_compare_string[23]="SVTVehicleScale Success"
#test_template_compare_string[24]="SVTVehicleScale Success"
#test_template_compare_string[25]="SVTVehicleScale Success"

test_template_finalize_test

  echo
  echo ================================================================================================
  echo
  echo xml[${n}]=\"${xml[${n}]}\"
  echo test_template_initialize_test \"${xml[${n}]}\"
  echo scenario[${n}]=\"${scenario[${n}]}\"
  echo timeouts[${n}]=${timeouts[${n}]}

  echo
  for I in ${!component[@]}; do
     echo component[${I}]=\"${component[${I}]}\"
  done

  echo
  for I in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${I}]=\"${test_template_compare_string[${I}]}\"
  done

  echo
  for I in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${I}]=\"${test_template_metrics_v1[${I}]}\"
  done

  echo
  echo test_template_runorder=\"${test_template_runorder}\"
  echo test_template_finalize_test
  echo
  echo ================================================================================================
  echo




((n+=1))



#----------------------------
# delete existing subscriptions and clients
#----------------------------
printf -v tname "atelm_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete all subscriptions and mqtt clients"
timeouts[${n}]=1800

component[1]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllSubscriptions.py"
component[2]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllMqttClients.py,-o|-m|1800"

test_template_finalize_test
((n+=1))



#----------------------------
# disable ha
#----------------------------

printf -v tname "atelm_08_%02d" ${n}
xml[${n}]=${tname}
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
test_template_initialize_test "${xml[${n}]}"

component[1]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300"
component[2]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|oauth_policy.cli,"
component[3]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|2|-c|oauth_policy.cli,"

test_template_finalize_test
((n+=1))


