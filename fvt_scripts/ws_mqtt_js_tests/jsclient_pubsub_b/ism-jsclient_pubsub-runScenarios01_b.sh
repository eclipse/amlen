#! /bin/bash

#----------------------------------------------------
# This script defines the scenarios for the ism Test Automation Framework.
# It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!: MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="jsclient_pubsub_b"
httpSubDir="ws_mqtt_js_tests/$scenario_set_name"

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
# Tool SubController:
# component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o "-x <param> -y <params>" "
# where:
# <SubControllerName>
# SubController controls and monitors the test case runningon the target machine.
# <machineNumber_ismSetup>
# m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#
# Optional, but either a config_file or other_opts must be specified
# <config_file> for the subController
# The configuration file to drive the test case using this controller.
# <OTHER_OPTS> is used when configuration file may be over kill,
# parameters are passed as is and are processed by the subController.
# However, Notice the spaces are replaced with a delimiter - it is necessary.
# The syntax is '-o', <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
# ex: -o_-x_paramXvalue_-y_paramYvalue or -o|-x|paramXvalue|-y|paramYvalue
#
# DriverSync:
# component[x]=sync,<machineNumber_ismSetup>,
# where:
# <m1> is the machine 1
# <m2> is the machine 2
#
# Sleep:
# component[x]=sleep,<seconds>
# where:
# <seconds> is the number of additional seconds to wait before the next component is started.
#

# For these tests, source the file that sets up the URL prefix

source ../common/url.sh

echo "M1_BROWSER_LIST=${M1_BROWSER_LIST}"

if [[ "${M1_BROWSER_LIST}" == *firefox* ]]
then
#----------------------------------------------------
# jsclient_pubsub_clean_store
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_clean_store"
scenario[${n}]="${xml[${n}]} - Clean the store before running this set of tests"
timeouts[${n}]=130
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enabletrace|-c|${scenario_set_name}/jsclient_pubsub_b.cli"
component[2]=cAppDriver,m1,"-e|../common/clean_store.sh",-o_clean
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_setup_b
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_setup_b"
scenario[${n}]="${xml[${n}]} - Server setup via CLI for the jsclient_pubsub tests"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|${scenario_set_name}/jsclient_pubsub_b.cli"
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_1
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_1_b"
scenario[${n}]="${xml[${n}]} - Basic publish/subscribe for the JavaScript Client"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_2
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_2_b"
scenario[${n}]="${xml[${n}]} - QoS=1 on publish (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_3
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_3_b"
scenario[${n}]="${xml[${n}]} - QoS=2 on publish"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_4
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_4_b"
scenario[${n}]="${xml[${n}]} - Multiple topics, QoS=0 on publish (IPv6)"
timeouts[${n}]=12
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|10|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_5
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_5_b"
scenario[${n}]="${xml[${n}]} - Test for case-sensitivity in topic name"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_8
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_8_b"
scenario[${n}]="${xml[${n}]} - Specify a subscribe complete callback (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_10
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_10_b"
scenario[${n}]="${xml[${n}]} - Specify various callbacks for onsubscribe (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_11
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_11_b"
scenario[${n}]="${xml[${n}]} - Wildcard subscriptions"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|25|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_12
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_12_b"
scenario[${n}]="${xml[${n}]} - Mixed QoS on publish, QoS=0 on subscribe (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_13
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_13_b"
scenario[${n}]="${xml[${n}]} - Mixed QoS on publish, QoS=1 on subscribe"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_14
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_14_b"
scenario[${n}]="${xml[${n}]} - Mixed QoS on publish, QoS=2 on subscribe (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_15
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_15_b"
scenario[${n}]="${xml[${n}]} - Mixed QoS topics in one client"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_16
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_16_b"
scenario[${n}]="${xml[${n}]} - Mixed QoS client with wildcard subscriptions (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_17
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_17_b"
scenario[${n}]="${xml[${n}]} - Test retained"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_18
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_18_b"
scenario[${n}]="${xml[${n}]} - Retained message is not the last message"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_19
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_19_b"
scenario[${n}]="${xml[${n}]} - No unsubscribe before disconnect"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_22
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_22_b"
scenario[${n}]="${xml[${n}]} - Unsubscribe from the same topic twice"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_24
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_24_b"
scenario[${n}]="${xml[${n}]} - Test where publisher and subscriber are on different clients (synchronized)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_25
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_25_b"
scenario[${n}]="${xml[${n}]} - Connect to an Connection/Messaging Policy that authorizes clients for the MQTT and JMS protocols (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_26
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_26_b"
scenario[${n}]="${xml[${n}]} - Connect to an Connection/Messaging Policy that authorizes MQTT clients only"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_27
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_27_b"
scenario[${n}]="${xml[${n}]} - Connect to an Connection/MessagingPolicy that authorizes a specific user/topic (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e1
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e1_b"
scenario[${n}]="${xml[${n}]} - Test for invalid topic names on pub"
timeouts[${n}]=27
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|25|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e2
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e2_b"
scenario[${n}]="${xml[${n}]} - Test for invalid QoS values on pub"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e3
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e3_b"
scenario[${n}]="${xml[${n}]} - Test for invalid retain value on pub"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e4
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e4_b"
scenario[${n}]="${xml[${n}]} - Test for invalid wildcard placements on sub"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e6
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e6_b"
scenario[${n}]="${xml[${n}]} - Test for invalid QoS values on sub"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e7
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e7_b"
scenario[${n}]="${xml[${n}]} - Specify a non-function for subscribeCompleteCallback"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e8
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e8_b"
scenario[${n}]="${xml[${n}]} - Specify a non-function for the onmessage callback for a client"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

##----------------------------------------------------
## jsclient_pubsub_e11
##----------------------------------------------------
#
#xml[${n}]="jsclient_pubsub_e11_b"
#scenario[${n}]="${xml[${n}]} - Test for publish,subscribe,unsubscribe after disconnected"
#timeouts[${n}]=7
## Set up the components for the test in the order they should start
#component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
#component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#
#((n+=1))
#
#----------------------------------------------------
# jsclient_pubsub_e13
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e13_b"
scenario[${n}]="${xml[${n}]} - Test for unsubscribe with non-function callback"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e14
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e14_b"
scenario[${n}]="${xml[${n}]} - Unsubscribe from a topic with invalid wc placement"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e15
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e15_b"
scenario[${n}]="${xml[${n}]} - Attempt to publish/subscribe on an unauthorized protocol"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_e16
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_e16_b"
scenario[${n}]="${xml[${n}]} - Attempt to publish/subscribe on an unauthorized topic"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_pubsub_cleanup_b
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_cleanup_b"
scenario[${n}]="${xml[${n}]} - Server cleanup via CLI for the jsclient_pubsub tests"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|${scenario_set_name}/jsclient_pubsub_b.cli"
components[${n}]="${component[1]}"

((n+=1))

fi

