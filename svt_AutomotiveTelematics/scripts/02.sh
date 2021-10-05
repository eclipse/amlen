#!/bin/bash 

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
#  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh


# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 02"

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

source template.sh


#----------------------------------------------------
# Test Cases 
#----------------------------------------------------

echo M_COUNT=${M_COUNT}

#----------------------------------------------------
# Test Case 0 -  clean any outstanding subscriptions - when other tests don't clean up after themselves, it can break this test.
# Thus  , we will delete any outstanding durable subcriptions that are still active on the appliance before executing this test.
#----------------------------------------------------
xml[${n}]="atelm_02_0${n}"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup for svt_atelm tests"
timeouts[${n}]=60

component[1]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
component[2]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"

test_template_finalize_test

((n+=1))


test_template_define testcase=atelm_02_01 description=mqtt_only \
                     timeout=180 minutes=2 qos=0 order=false stats=false listen=false  \
                     run_mqttbench=false mqtt_subscribers=1 jms_subscribers=0  \
                     mqtt_vechicles=0 paho_vehicles=1 jms_vehicles=0

((n++))

test_template_define testcase=atelm_02_02 description=Java_MQTT_and_Paho_Vehicles,_10K_Car_10min_Run \
                     timeout=1800 minutes=10 qos=0 order=false stats=false listen=false \
                     mqtt_subscribers=1 jms_subscribers=1 \
                     mqtt_vehicles=0 paho_vehicles=1000 jms_vehicles=0 

((n++))
 
test_template_define testcase=atelm_02_03 description=JMS_Vehicles,_10K_Car_10min_Run \
                     timeout=1200 minutes=5 qos=0 order=false stats=false listen=false \
                     mqtt_subscribers=1 jms_subscribers=1 \
                     mqtt_vehicles=0 paho_vehicles=0 jms_vehicles=1500
 



