#! /bin/bash

#----------------------------------------------------
# This script defines the scenarios for the ism Test Automation Framework.
# It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!: MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="jsclient_sec_b"
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
# jsclient_sec_setup_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_setup_b"
scenario[${n}]="${xml[${n}]} - Server setup via CLI for the jsclient_sec tests"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|${scenario_set_name}/jsclient_sec_b.cli"
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_2_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_2_b"
scenario[${n}]="${xml[${n}]} - Connect to ISM server with SSL disabled, using a valid, non-empty username and password (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_3_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_3_b"
scenario[${n}]="${xml[${n}]} - Connect to ISM server with an MQTT-only ConnectionPolicy"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_4_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_4_b"
scenario[${n}]="${xml[${n}]} - Connect to ISM server with an MQTT-only Endpoint (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_5_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_5_b"
scenario[${n}]="${xml[${n}]} - Connect to ISM server with a ConnectionPolicy that authorizes only one user"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_6_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_6_b"
scenario[${n}]="${xml[${n}]} - Connect to ISM server using an authorized user (wildcards in ID on ConnectionPolicy) (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e1_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e1_b"
scenario[${n}]="${xml[${n}]} - UserName is not specified (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e2_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e2_b"
scenario[${n}]="${xml[${n}]} - User does not exist (was invalid UTF-8 test)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e3_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e3_b"
scenario[${n}]="${xml[${n}]} - User does not exist (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e5_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e5_b"
scenario[${n}]="${xml[${n}]} - Incorrect password"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e9_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e9_b"
scenario[${n}]="${xml[${n}]} - User is not authorized (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e10_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e10_b"
scenario[${n}]="${xml[${n}]} - Insecure connection to a secure Endpoint"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e12_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e12_b"
scenario[${n}]="${xml[${n}]} - Protocol is JMS on the ConnectionPolicy"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e13_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e13_b"
scenario[${n}]="${xml[${n}]} - Protocol is JMS on the Endpoint (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e14_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e14_b"
scenario[${n}]="${xml[${n}]} - Empty userName and password"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e15_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e15_b"
scenario[${n}]="${xml[${n}]} - Null userName and password (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_e16_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_e16_b"
scenario[${n}]="${xml[${n}]} - Empty userName and non-empty password"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_sec_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_sec_cleanup_b
#----------------------------------------------------

xml[${n}]="jsclient_sec_cleanup_b"
scenario[${n}]="${xml[${n}]} - Clean up the server after the tests "
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|${scenario_set_name}/jsclient_sec_b.cli"
components[${n}]="${component[1]}"

((n+=1))

fi

