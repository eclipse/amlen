#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Java MQTT SSL Sample App"

typeset -i n=0

#declare -r internet_ep=ssl://${A1_IPv4_1}:16111+ssl://${A2_IPv4_1}:16111
#declare -r intranet_ep=tcp://${A1_IPv4_2}:16999+tcp://${A2_IPv4_2}:16999

declare -r internet_ep=ssl://${A1_IPv4_1}:16474
#declare -r intranet_ep=tcp://${A1_IPv4_2}:16999

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
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




#----------------------------
# recreate topic monitor
#----------------------------
printf -v tname "jmqtt_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for crl tests"
timeouts[${n}]=300

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|./crl.cli,"
#component[4]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA|-t|300"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|setup|-a|1|-c|./crl.cli,"

test_template_finalize_test

((++n))


#if false; then

###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
xml[${n}]="jmqtt_08_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 1 ${MESSAGES} msgs [1 Pub; 4 Sub], SSL"
timeouts[${n}]=900
MESSAGES=1000
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000001|-u|c0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000002|-u|c0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-6000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[5]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000003|-u|c0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-8000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[6]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-9900.jks|-Djavax.net.ssl.keyStorePassword=password"

component[7]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000001|-u|c0000004|-p|imasvtest|-n|${MESSAGES}|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[8]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000002|-u|c0000004|-p|imasvtest|-n|${MESSAGES}|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-6000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[9]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000003|-u|c0000004|-p|imasvtest|-n|${MESSAGES}|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-8000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[10]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|${MESSAGES}|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-9900.jks|-Djavax.net.ssl.keyStorePassword=password"

component[11]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|1|-t|/CAR/A/1|-s|${internet_ep}|-i|c0000005|-u|c0000005|-p|imasvtest|-n|${MESSAGES}|-w|9000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1.jks|-Djavax.net.ssl.keyStorePassword=password"


component[12]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

test_template_compare_string[7]="Received ${MESSAGES} messages."
test_template_compare_string[8]="Received ${MESSAGES} messages."
test_template_compare_string[9]="Received ${MESSAGES} messages."
test_template_compare_string[10]="Received ${MESSAGES} messages."
test_template_compare_string[11]="Published ${MESSAGES} messages to topic /CAR/A/1"

test_template_finalize_test
((n+=1))

#fi

#----------------------------
# recreate topic monitor
#----------------------------
printf -v tname "jmqtt_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - attach CRL"
timeouts[${n}]=300

component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|setup2|-a|1|-c|./crl.cli,"

test_template_finalize_test

((++n))

#if false; then

###----------------------------------------------------
### Test Case 2 -
###----------------------------------------------------
xml[${n}]="jmqtt_08_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 1 ${MESSAGES} msgs [1 Pub; 4 Sub], SSL"
timeouts[${n}]=900
MESSAGES=1000
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000001|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[4]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000002|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-6000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[5]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000003|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-8000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[6]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-9900.jks|-Djavax.net.ssl.keyStorePassword=password"

component[7]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|1|-t|/CAR/A/1|-s|${internet_ep}|-i|c0000005|-u|c0000005|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1.jks|-Djavax.net.ssl.keyStorePassword=password"


component[8]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

test_template_compare_string[3]="certificate_revoked"
test_template_compare_count[3]=">1"
test_template_compare_string[4]="Established connection"
test_template_compare_count[4]="1"
test_template_compare_string[5]="Established connection"
test_template_compare_count[5]="1"
test_template_compare_string[6]="Established connection"
test_template_compare_count[6]="1"
test_template_compare_string[7]="connect failed"
test_template_compare_count[7]=">1"

test_template_finalize_test
((n+=1))




#----------------------------
# recreate topic monitor
#----------------------------
printf -v tname "jmqtt_08_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - attach CRL"
timeouts[${n}]=300

component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|setup3|-a|1|-c|./crl.cli,"

test_template_finalize_test

((++n))

#if false; then

###----------------------------------------------------
### Test Case 2 -
###----------------------------------------------------
xml[${n}]="jmqtt_08_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 1 ${MESSAGES} msgs [1 Pub; 4 Sub], SSL"
timeouts[${n}]=900
MESSAGES=1000
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000001|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[4]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000002|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-6000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[5]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000003|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-8000.jks|-Djavax.net.ssl.keyStorePassword=password"
component[6]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-9900.jks|-Djavax.net.ssl.keyStorePassword=password"

component[7]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|1|-t|/CAR/A/1|-s|${internet_ep}|-i|c0000005|-u|c0000005|-p|imasvtest|-n|0|-Ns|30|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/trust.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.keyStore=/niagara/test/svt_jmqtt/certs2Kb/revoked/revoked-1.jks|-Djavax.net.ssl.keyStorePassword=password"


component[8]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

test_template_compare_string[3]="certificate_revoked"
test_template_compare_count[3]=">1"
test_template_compare_string[4]="certificate_revoked"
test_template_compare_count[4]=">1"
test_template_compare_string[5]="certificate_revoked"
test_template_compare_count[5]=">1"
test_template_compare_string[6]="certificate_revoked"
test_template_compare_count[6]=">1"
test_template_compare_string[7]="connect failed"
test_template_compare_count[7]=">1"

test_template_finalize_test
((n+=1))






#----------------------------------------------------
# Test Case 4 -  clean any outstanding subscriptions - when other tests don't clean up after themselves, it can break this test.
# Thus  , we will delete any outstanding durable subcriptions that are still active on the appliance before executing this test.
#----------------------------------------------------
xml[${n}]="jmqtt_08_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - cleanup after svt_jmqtt tests"
timeouts[${n}]=60

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|./crl.cli,"

test_template_finalize_test
((n+=1))

#fi
