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

declare -r internet_ep=ssl://${A1_IPv4_1}:16111
declare -r intranet_ep=tcp://${A1_IPv4_2}:16999
declare -r demo_ep=tcp://${A1_IPv4_1}:16102

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
printf -v tname "jmqtt_02_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup topic monitor"
timeouts[${n}]=300

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|cleanup|-a|1|-c|./topicmonitor.cli,"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|cleanup2|-a|1|-c|./topicmonitor.cli,"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|setup|-a|1|-c|./topicmonitor.cli,"

test_template_finalize_test

((++n))



#----------------------------------------------------
# Test Case 0 - 
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 0 100,000 msgs [1 Pub; 4 Sub] with Topic Filter Subscription, SSL"
timeouts[${n}]=900

# Set up the components for the test in the order they should start
component[1]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|#|-s|${demo_ep}|-i|a05|-u|a05|-p|imasvtest|-n|0,"
component[2]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/A/0|-s|${intranet_ep}|-i|a01|-u|a01|-p|imasvtest|-n|0,"
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/A/+|-s|${intranet_ep}|-i|a02|-u|a02|-p|imasvtest|-n|0,"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/+/0|-s|${intranet_ep}|-i|a03|-u|a03|-p|imasvtest|-n|0,"
component[5]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/#|-s|${intranet_ep}|-i|a04|-u|a04|-p|imasvtest|-n|0,"

component[6]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/A/0|-s|${intranet_ep}|-i|a01|-u|a01|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[7]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/A/+|-s|${intranet_ep}|-i|a02|-u|a02|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[8]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/+/0|-s|${intranet_ep}|-i|a03|-u|a03|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[9]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|/APP/#|-s|${intranet_ep}|-i|a04|-u|a04|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[10]=javaAppDriver,m5,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|0|-t|#|-s|${demo_ep}|-i|a05|-u|a05|-p|imasvtest|-n|10000|-vv,-s|JVM_ARGS=-Xss1024K"

component[11]=javaAppDriver,m6,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|0|-t|/APP/A/0|-s|${internet_ep}|-i|a06|-u|a06|-p|imasvtest|-n|10000|-w|9000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

component[12]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[13]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[14]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[15]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9
component[16]=searchLogsEnd,m5,${xml[${n}]}_m5.comparetest,9
component[17]=searchLogsEnd,m6,${xml[${n}]}_m6.comparetest,9

test_template_compare_string[6]="Received 10000 messages."
test_template_compare_string[7]="Received 10000 messages."
test_template_compare_string[8]="Received 10000 messages."
test_template_compare_string[9]="Received 10000 messages."
test_template_compare_string[10]="Received 10000 messages."
test_template_compare_string[11]="Published 10000 messages to topic /APP/A/0"

test_template_finalize_test
((n+=1))


#----------------------------------------------------
#----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - verify topic monitor"
timeouts[${n}]=60

component[1]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|#"
component[2]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|#"
component[3]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|#"
component[4]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|5|-t|#"

component[5]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/CAR/#"
component[6]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|0|-t|/CAR/#"
component[7]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/CAR/#"
component[8]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/CAR/#"

component[9]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/APP/#"
component[10]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/APP/#"
component[11]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/APP/#"
component[12]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|4|-t|/APP/#"

component[13]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/USER/#"
component[14]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|0|-t|/USER/#"
component[15]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/USER/#"
component[16]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/USER/#"

test_template_finalize_test
((n+=1))


###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#------------------------------------
# RTC Task 18578, measurements taken show that this workload
# takes ~ 2 minutes to send/receieve 10,000 messages, thus it is not
# realistic to expect this workload to finish 1 M messages in under 4 minutes.
# The total number of messages sent/received needs to be lowered to 10,000
# in order to complete in the time allotted to the test.
#------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 1 10,000 msgs [1 Pub; 4 Sub] with Topic Filter Subscription, SSL"
timeouts[${n}]=900
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|#|-s|${demo_ep}|-i|c0000005|-u|c0000005|-p|imasvtest|-n|0,"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/A/1|-s|${internet_ep}|-i|c0000001|-u|c0000001|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[5]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/A/+|-s|${internet_ep}|-i|c0000002|-u|c0000002|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[6]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/+/1|-s|${internet_ep}|-i|c0000003|-u|c0000003|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[7]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

component[8]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/A/1|-s|${internet_ep}|-i|c0000001|-u|c0000001|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[9]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/A/+|-s|${internet_ep}|-i|c0000002|-u|c0000002|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[10]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/+/1|-s|${internet_ep}|-i|c0000003|-u|c0000003|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[11]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/CAR/#|-s|${internet_ep}|-i|c0000004|-u|c0000004|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[12]=javaAppDriver,m5,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|#|-s|${demo_ep}|-i|c0000005|-u|c0000005|-p|imasvtest|-n|10000|-vv,-s|JVM_ARGS=-Xss1024K"

component[13]=javaAppDriver,m6,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|1|-t|/CAR/A/1|-s|${intranet_ep}|-i|c0000006|-u|c0000006|-p|imasvtest|-n|10000|-w|9000,"

component[14]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[15]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[16]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[17]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9
component[18]=searchLogsEnd,m5,${xml[${n}]}_m5.comparetest,9
component[19]=searchLogsEnd,m6,${xml[${n}]}_m6.comparetest,9

test_template_compare_string[8]="Received 10000 messages."
test_template_compare_string[9]="Received 10000 messages."
test_template_compare_string[10]="Received 10000 messages."
test_template_compare_string[11]="Received 10000 messages."
test_template_compare_string[12]="Received 10000 messages."
test_template_compare_string[13]="Published 10000 messages to topic /CAR/A/1"

test_template_finalize_test
((n+=1))

#----------------------------------------------------
#----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - verify topic monitor"
timeouts[${n}]=60

component[1]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0"
component[2]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|20000"
component[3]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0"
component[4]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|5"

component[5]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/CAR/#"
component[6]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/CAR/#"
component[7]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/CAR/#"
component[8]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|4|-t|/CAR/#"

component[9]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/APP/#"
component[10]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/APP/#"
component[11]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/APP/#"
component[12]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/APP/#"

component[13]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/USER/#"
component[14]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|0|-t|/USER/#"
component[15]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/USER/#"
component[16]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/USER/#"

test_template_finalize_test
((n+=1))


###----------------------------------------------------
### Test Case 2 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#------------------------------------
# RTC Task 18578, measurements taken show that this workload
# takes ~ 2 minutes to send/receieve 10,000 messages, thus it is not
# realistic to expect this workload to finish 1 M messages in under 4 minutes.
# The total number of messages sent/received needs to be lowered to 10,000
# in order to complete in the time allotted to the test.
#------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 1 10,000 msgs [1 Pub; 4 Sub] with Topic Filter Subscription, SSL"
timeouts[${n}]=600
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|#|-s|${demo_ep}|-i|a05|-u|a05|-p|imasvtest|-n|0,"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/A/1|-s|${intranet_ep}|-i|a01|-u|a01|-p|imasvtest|-n|0,"
component[5]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/A/+|-s|${intranet_ep}|-i|a02|-u|a02|-p|imasvtest|-n|0,"
component[6]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/+/1|-s|${intranet_ep}|-i|a03|-u|a03|-p|imasvtest|-n|0,"
component[7]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/#|-s|${intranet_ep}|-i|a04|-u|a04|-p|imasvtest|-n|0,"

component[8]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/A/1|-s|${intranet_ep}|-i|a01|-u|a01|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[9]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/A/+|-s|${intranet_ep}|-i|a02|-u|a02|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[10]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/+/1|-s|${intranet_ep}|-i|a03|-u|a03|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[11]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|/APP/#|-s|${intranet_ep}|-i|a04|-u|a04|-p|imasvtest|-n|10000,-s|JVM_ARGS=-Xss1024K"
component[12]=javaAppDriver,m5,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|1|-t|#|-s|${demo_ep}|-i|a05|-u|a05|-p|imasvtest|-n|10000|-vv,-s|JVM_ARGS=-Xss1024K"

component[13]=javaAppDriver,m6,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-q|1|-t|/APP/A/1|-s|${internet_ep}|-i|a06|-u|a06|-p|imasvtest|-n|10000|-w|9000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

component[14]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[15]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[16]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[17]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9
component[18]=searchLogsEnd,m5,${xml[${n}]}_m5.comparetest,9
component[19]=searchLogsEnd,m6,${xml[${n}]}_m6.comparetest,9

test_template_compare_string[8]="Received 10000 messages."
test_template_compare_string[9]="Received 10000 messages."
test_template_compare_string[10]="Received 10000 messages."
test_template_compare_string[11]="Received 10000 messages."
test_template_compare_string[12]="Received 10000 messages."
test_template_compare_string[13]="Published 10000 messages to topic /APP/A/1"

test_template_finalize_test
((n+=1))


#----------------------------------------------------
#----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - verify topic monitor"
timeouts[${n}]=60

component[1]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0"
component[2]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|30000"
component[3]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0"
component[4]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|5"

component[5]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/CAR/#"
component[6]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/CAR/#"
component[7]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/CAR/#"
component[8]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/CAR/#"

component[9]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/APP/#"
component[10]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|20000|-t|/APP/#"
component[11]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/APP/#"
component[12]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|4|-t|/APP/#"

component[13]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/USER/#"
component[14]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|0|-t|/USER/#"
component[15]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/USER/#"
component[16]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/USER/#"

test_template_finalize_test
((n+=1))


###----------------------------------------------------
### Test Case 3 -
###----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - MqttSample QoS 2 10,000 msgs [1 Pub; 4 Sub] with Topic Filter Subscription, SSL"
timeouts[${n}]=500
component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

## Set up the components for the test in the order they should start
component[3]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|#|-s|${demo_ep}|-i|u0000005|-u|u0000005|-p|imasvtest|-n|0,"
component[4]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/A/2|-s|${internet_ep}|-i|u0000001|-u|u0000001|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[5]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/A/+|-s|${internet_ep}|-i|u0000002|-u|u0000002|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[6]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/+/2|-s|${internet_ep}|-i|u0000003|-u|u0000003|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[7]=javaAppDriverWait,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/#|-s|${internet_ep}|-i|u0000004|-u|u0000004|-p|imasvtest|-n|0|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

component[8]=javaAppDriver,m1,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/A/2|-s|${internet_ep}|-i|u0000001|-u|u0000001|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[9]=javaAppDriver,m2,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/A/+|-s|${internet_ep}|-i|u0000002|-u|u0000002|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[10]=javaAppDriver,m3,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/+/2|-s|${internet_ep}|-i|u0000003|-u|u0000003|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[11]=javaAppDriver,m4,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|/USER/#|-s|${internet_ep}|-i|u0000004|-u|u0000004|-p|imasvtest|-n|10000|-sslProtocol|TLSv1.2,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
component[12]=javaAppDriver,m5,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-c|false|-q|2|-t|#|-s|${demo_ep}|-i|u0000005|-u|u0000005|-p|imasvtest|-n|10000|-vv,-s|JVM_ARGS=-Xss1024K"

component[13]=javaAppDriver,m6,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|publish|-q|2|-t|/USER/A/2|-s|${intranet_ep}|-i|u0000006|-u|u0000006|-p|imasvtest|-n|10000|-w|9000,"

component[14]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[15]=searchLogsEnd,m2,${xml[${n}]}_m2.comparetest,9
component[16]=searchLogsEnd,m3,${xml[${n}]}_m3.comparetest,9
component[17]=searchLogsEnd,m4,${xml[${n}]}_m4.comparetest,9
component[18]=searchLogsEnd,m5,${xml[${n}]}_m5.comparetest,9
component[19]=searchLogsEnd,m6,${xml[${n}]}_m6.comparetest,9

test_template_compare_string[8]="Received 10000 messages."
test_template_compare_string[9]="Received 10000 messages."
test_template_compare_string[10]="Received 10000 messages."
test_template_compare_string[11]="Received 10000 messages."
test_template_compare_string[12]="Received 10000 messages."
test_template_compare_string[13]="Published 10000 messages to topic /USER/A/2"

test_template_finalize_test

((n+=1))

#----------------------------------------------------
#----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - verify topic monitor"
timeouts[${n}]=60

component[1]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0"
component[2]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|40000"
component[3]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0"
component[4]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|5"

component[5]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/CAR/#"
component[6]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/CAR/#"
component[7]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/CAR/#"
component[8]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/CAR/#"

component[9]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/APP/#"
component[10]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|20000|-t|/APP/#"
component[11]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/APP/#"
component[12]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|0|-t|/APP/#"

component[13]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|FailedPublishes|-v|0|-t|/USER/#"
component[14]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|10000|-t|/USER/#"
component[15]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|RejectedMsgs|-v|0|-t|/USER/#"
component[16]=cAppDriverLogWait,m1,"-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|Subscriptions|-v|4|-t|/USER/#"

test_template_finalize_test
((n+=1))


#----------------------------------------------------
# Test Case 4 -  clean any outstanding subscriptions - when other tests don't clean up after themselves, it can break this test.
# Thus  , we will delete any outstanding durable subcriptions that are still active on the appliance before executing this test.
#----------------------------------------------------
xml[${n}]="jmqtt_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for svt_jmqtt tests"
timeouts[${n}]=60

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-s|cleanup2|-a|1|-c|./topicmonitor.cli,"

test_template_finalize_test
((n+=1))

