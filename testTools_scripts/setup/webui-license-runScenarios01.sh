#!/bin/bash

##
## WARNING : This script is used to accept license agreement.
##  This script is referenced by testTools_scripts/setup/web_ui_accept_license.sh
##

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case runningon the target machine.
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

scenario_set_name="license"
echo "M1_BROWSER_LIST=${M1_BROWSER_LIST}"
if [ -z "${M1_BROWSER_LIST}" ]; then
	FIRST_BROWSER="firefox"
else
	#IFS=. read -a BROWSERS << "${M1_BROWSER_LIST}"
	BROWSERS=(${M1_BROWSER_LIST//,/ })
	FIRST_BROWSER="${BROWSERS[0]}"
fi
## FIXME: Selenium requires that we setup PATH for iexplore to work
if [ "${FIRST_BROWSER}" == "iexplore" ]; then
    FIRST_BROWSER="firefox"
fi

CLASSNAME="com.ibm.ism.test.testcases.license.LicenseAgreementTest"
URL="http://${A1_HOST}:9080/ISMWebUI"
USER="${A1_USER}"
PASSWORD="${USER}"
ARG1="URL=${URL}"
ARG2="USER=${USER}"
ARG3="PASSWORD=${PASSWORD}"
ARG4="BROWSER=${FIRST_BROWSER}"
ARG5="DEBUG=true"

#----------------------------------------------------
# webui_license_agreement_1
#----------------------------------------------------
xml[${n}]="webui_license_agreement_1"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} Web UI License Acceptance ${ARG1} ${ARG2} ${ARG3} ${ARG4} ${ARG5}"
component[1]=javaAppDriver,m1,-e_${CLASSNAME},-o|${ARG1}|${ARG2}|${ARG3}|${ARG4}|${ARG5},-s-DISPLAY=localhost:1
component[2]=searchLogsEnd,m1,webui_license_agreement_1.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))
