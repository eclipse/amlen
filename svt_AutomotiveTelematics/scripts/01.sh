#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 01"

typeset -i n=0

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


#----------------------------------------------------
# Test Case 0 
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characompareers valid in a filename, it will be part of the logfile name.
#
#xml[${n}]="atelm_01_00"
#test_template_initialize_test "${xml[${n}]}"
#scenario[${n}]="${xml[${n}]} - MQTT Java Vehicles, 90 Cars/JVM 10min Run"
#timeouts[${n}]=800
#
## Set up the components for the test in the order they should start
#component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_1}:16102|-n|9|-v"
#test_template_compare_string[1]="Received 9 messages"  
#test_template_compare_count[1]="1"
#
#component[2]=javaAppDriver,m1,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|90|0|10|1"
#test_template_compare_string[2]="SVTVehicleScale Success" 
#test_template_compare_count[2]="1"
#test_template_metrics_v1[2]="1"; 
#
#component[3]=javaAppDriver,m2,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|90|0|10|1"
#test_template_compare_string[3]="SVTVehicleScale Success" 
#test_template_compare_count[3]="1"
#test_template_metrics_v1[3]="1";
#
#component[4]=javaAppDriver,m3,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|0|90|10|0"
#test_template_compare_string[4]="SVTVehicleScale Success"
#test_template_compare_count[4]="1" 
#test_template_metrics_v1[4]="1"; 
#
#component[5]=searchLogsEnd,m1,atelm_01_00_m1.comparetest,9 
#component[6]=searchLogsEnd,m2,atelm_01_00_m2.comparetest,9 
#component[7]=searchLogsEnd,m3,atelm_01_00_m3.comparetest,9 
#
#component[11]=sleep,10
#
#test_template_runorder="1 11 5 6 7 2 3 4 "
#test_template_finalize_test
#
#### Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))
####----------------------------------------------------
#### Test Case 1
####----------------------------------------------------
#### The prefix of the XML configuration file for the driver
#xml[${n}]="atelm_01_01"
#test_template_initialize_test "${xml[${n}]}"
#scenario[${n}]="${xml[${n}]} - ISM JMS Vehicles, 90 Cars/JVM 10min Run"
#timeouts[${n}]=800
##
#### Set up the components for the test in the order they should start
#component[1]=javaAppDriver,m1,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/#|-s|tcp://${A1_IPv4_1}:16102|-n|9|-v"
#test_template_compare_string[1]="Received 9 messages"  
#
#component[2]=javaAppDriver,m1,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|90|0|0|10|1"
#component[3]=javaAppDriver,m2,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|90|0|0|10|1"
#component[4]=javaAppDriver,m3,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|90|0|0|10|0"
#
##----------------------------------------
## Specify compare test, and metrics for components 2-4
##----------------------------------------
#for v in {2..4}; do
#    test_template_compare_string[$v]="SVTVehicleScale Success" 
#    test_template_metrics_v1[$v]="1"; 
#done
#
#component[5]=searchLogsEnd,m1,atelm_01_01_m1.comparetest,9
#component[6]=searchLogsEnd,m2,atelm_01_01_m2.comparetest,9
#component[7]=searchLogsEnd,m3,atelm_01_01_m3.comparetest,9
#
#component[11]=sleep,10
#
#test_template_runorder="1 11 5 6 7 2 3 4 "
#test_template_finalize_test
#
####
#((n+=1))
###----------------------------------------------------
### Test Case 2
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
#xml[${n}]="atelm_01_${n}"
#test_template_initialize_test "${xml[${n}]}"
#scenario[${n}]="${xml[${n}]} - MQTT JAVA and ISM JMS Vehicles, 90 Cars/JVM 10min Run"
#timeouts[${n}]=200
#
### Set up the components for the test in the order they should start
#component[1]=javaAppDriver,m3,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-q|0|-t|/APP/#|-s|tcp://${A1_IPv4_1}:16999|-n|9|-v"
#component[2]=javaAppDriver,m2,"-e|com.ibm.ima.samples.jms.JMSSample,-o|-a|subscribe|-t|/APP/#|-s|tcp://${A1_IPv4_1}:16999|-n|9|-v"
#
#component[3]=javaAppDriver,m1,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_1}|-port|16111|-ssl|true|-appid|1|-jms|4096|-mqtt|5000|-paho|0|-minutes|2|-mode|connect_once|-listener|false|-stats|false,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[4]=javaAppDriver,m2,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_1}|-port|16111|-ssl|true|-appid|1|-jms|4096|-mqtt|0|-paho|5000|-minutes|2|-mode|connect_once|-listener|false|-stats|false,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#component[5]=javaAppDriver,m3,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${A1_IPv4_1}|-port|16111|-ssl|true|-appid|1|-jms|4096|-mqtt|0|-paho|5000|-minutes|2|-mode|connect_once|-listener|false|-stats|false,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=$M1_TESTROOT/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#
##----------------------------------------
## Specify compare test, and metrics for components 3-8
##----------------------------------------
#for v in {3..5}; do
#    test_template_compare_string[$v]="SVTVehicleScale Success" 
#    test_template_metrics_v1[$v]="1"; 
#done
#
#component[6]=searchLogsEnd,m1,${xml[$n]}_m1.comparetest,9
#component[7]=searchLogsEnd,m2,${xml[$n]}_m2.comparetest,9
#component[8]=searchLogsEnd,m3,${xml[$n]}_m3.comparetest,9
#
#test_template_runorder=`echo {1..10}`
#test_template_finalize_test


source template.sh

test_template_define testcase=atelm_01_00 description="MQTT JAVA and ISM JMS Vehicles, Cars/JVM 10min Run" \
                     timeout=900 minutes=10 qos=0 order=false stats=false listen=false \
                     mqtt_subscribers=1 jms_subscribers=1 \
                     mqtt_vehicles=2000 paho_vehicles=4000 jms_vehicles=4096









