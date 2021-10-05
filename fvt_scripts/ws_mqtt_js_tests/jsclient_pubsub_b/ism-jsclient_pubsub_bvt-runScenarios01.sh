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

if [[ "${FULLRUN}" == "TRUE" ]]  ; then

#if [[ "${M1_BROWSER_LIST}" == *firefox* ]]
#then
#----------------------------------------------------
# jsclient_pubsub_1
#----------------------------------------------------

xml[${n}]="jsclient_pubsub_1_b"
scenario[${n}]="${xml[${n}]} - Basic publish/subscribe for the JavaScript Client"
timeouts[${n}]=17
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|timeout","-o|15|firefox|${URL_M1_IPv4}/${httpSubDir}/jsclient_pubsub_b.html?test=${xml[${n}]}","-s-DISPLAY=localhost:1"
component[2]=searchLogsEnd,m1,${scenario_set_name}/${xml[${n}]}.comparetest#components[${n}]="${component[1]} ${component[2]}"

((n+=1))
fi

