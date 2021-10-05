#! /bin/bash

#----------------------------------------------------
# This script defines the scenarios for the ism Test Automation Framework.
# It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!: MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="jsclient_connect_b"
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
# jsclient_connect_1_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_1_b"
scenario[${n}]="${xml[${n}]} - Connect with a single char clientId (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_2_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_2_b"
scenario[${n}]="${xml[${n}]} - Connect using a clientId with a space and other chars"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_3_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_3_b"
scenario[${n}]="${xml[${n}]} - Connect with a single space as the clientId (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_4_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_4_b"
scenario[${n}]="${xml[${n}]} - Connect with multiple clients (synchronized)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_5_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_5_b"
scenario[${n}]="${xml[${n}]} - Connect with a non-unique clientId (synchronized) (IPv6)"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_6_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_6_b"
scenario[${n}]="${xml[${n}]} - Connect to a server with a connection policy that only allows the MQTT protocol "
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|${xml[${n}]}_setup|-c|${scenario_set_name}/jsclient_connect_b.cli"
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_e8_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_e8_b"
scenario[${n}]="${xml[${n}]} - Connect to a server using a zero-length clientId"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_e10_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_e10_b"
scenario[${n}]="${xml[${n}]} - Connect to a server with a connection policy that does not allow the MQTT protocol "
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|${xml[${n}]}_setup|-c|${scenario_set_name}/jsclient_connect_b.cli"
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_e11_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_e11_b"
scenario[${n}]="${xml[${n}]} - Connect to a server with an Endpoint that does not allow the MQTT protocol "
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|${xml[${n}]}_setup|-c|${scenario_set_name}/jsclient_connect_b.cli"
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_e12_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_e12_b"
scenario[${n}]="${xml[${n}]} - Create an Endpoint that does not have a ConnectionPolicy -  CLI"
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|${xml[${n}]}_setup|-c|${scenario_set_name}/jsclient_connect_b.cli"
component[2]=cAppDriver,m1,"-e|timeout","-o|5|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_connect_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[3]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# jsclient_connect_cleanup_b
#----------------------------------------------------

xml[${n}]="jsclient_connect_cleanup_b"
scenario[${n}]="${xml[${n}]} - Clean up the server after the tests "
timeouts[${n}]=7
# Set up the components for the test in the order they should start
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|${scenario_set_name}/jsclient_connect_b.cli"
components[${n}]="${component[1]}"

((n+=1))

fi

