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
printf -v tname "jmqtt_07_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - delete all subscriptions and mqtt clients"
timeouts[${n}]=400

component[1]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllSubscriptions.py"
component[2]="cAppDriverLogWait,m1,-e|../scripts/svt-deleteAllMqttClients.py"

test_template_finalize_test
((n+=1))



#----------------------------
# setup ha and oauth policies
#----------------------------
printf -v tname "jmqtt_07_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup ha and oauth policies"
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|300"
component[2]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|oauth_policy.cli,"
component[3]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|2|-c|oauth_policy.cli,"
component[4]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|setup|-a|1|-c|oauth_policy.cli,"
component[5]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|setup|-a|2|-c|oauth_policy.cli,"
component[6]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|300"

test_template_finalize_test

((++n))

#----------------------------
#----------------------------

declare -r demo_ep=tcp://${A1_IPv4_1}:21004


printf -v tname "jmqtt_07_%02d" ${n}
xml[${n}]="${tname}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.samples.mqttv3.MqttSample QoS 0 msgs [1 Pub; 4 Sub] with Topic wildcard Subscription"
timeouts[${n}]=1800

# Set up the components for the test in the order they should start
component[1]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/0|-s|${demo_ep}|-n|0|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient1s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[2]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/+|-s|${demo_ep}|-n|0|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient2s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/+/0|-s|${demo_ep}|-n|0|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient3s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/#|-s|${demo_ep}|-n|0|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient4s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"

component[5]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|0|-t|/topic/A/0|-s|${demo_ep}|-n|1000000|-w|6000|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient1p|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[6]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|0|-t|/topic/A/1|-s|${demo_ep}|-n|1000000|-w|6000|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient2p|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[7]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|0|-t|/topic/B/0|-s|${demo_ep}|-n|1000000|-w|6000|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient3p|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[8]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|0|-t|/topic/B/1|-s|${demo_ep}|-n|1000000|-w|6000|-c|true|-oa|${LTPAWAS_IP}:9443|-i|imaclient4p|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"

component[9]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/0|-s|${demo_ep}|-n|1000000|-c|false|-oa|${LTPAWAS_IP}:9443|-i|imaclient1s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[10]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/A/+|-s|${demo_ep}|-n|2000000|-c|false|-oa|${LTPAWAS_IP}:9443|-i|imaclient2s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[11]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/+/0|-s|${demo_ep}|-n|2000000|-c|false|-oa|${LTPAWAS_IP}:9443|-i|imaclient3s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
component[12]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-q|0|-t|/topic/#|-s|${demo_ep}|-n|4000000|-c|false|-oa|${LTPAWAS_IP}:9443|-i|imaclient4s|-u|imaclient|-p|password|,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=$M1_TESTROOT/common/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"

component[13]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[14]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[15]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[16]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9

test_template_compare_string[5]="Published 1000000 messages to topic /topic/A/0"
test_template_compare_string[6]="Published 1000000 messages to topic /topic/A/1"
test_template_compare_string[7]="Published 1000000 messages to topic /topic/B/0"
test_template_compare_string[8]="Published 1000000 messages to topic /topic/B/1"

test_template_compare_string[9]="Received 1000000 messages."
test_template_compare_string[10]="Received 2000000 messages."
test_template_compare_string[11]="Received 2000000 messages."
test_template_compare_string[12]="Received 4000000 messages."

test_template_finalize_test
((n+=1))




#----------------------------
# disable ha
#----------------------------

# test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

printf -v tname "jmqtt_07_%02d" ${n}
xml[${n}]=${tname}
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
test_template_initialize_test "${xml[${n}]}"

component[1]="cAppDriverLogWait,m1,-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300"
component[2]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|oauth_policy.cli,"
component[3]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|2|-c|oauth_policy.cli,"

test_template_finalize_test
((n+=1))


