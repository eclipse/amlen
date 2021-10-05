#! /bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="REST APIs."

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

echo "Running with NODEJS Components:"  >> ${logfile}
echo "NODE VERSION:  "`node --version`  >> ${logfile}
echo "MOCHA VERSION: "`mocha --version` >> ${logfile}

source getIfname.sh  $A1_USER  $A1_HOST  $A1_IPv4_1 "A1_INTFNAME_1"
source getIfname.sh  $A1_USER  $A1_HOST  $A1_IPv4_2 "A1_INTFNAME_2"
if [ $A_COUNT -ge 2 ] ; then
  source getIfname.sh  $A2_USER  $A2_HOST  $A2_IPv4_1 "A2_INTFNAME_1"
  source getIfname.sh  $A2_USER  $A2_HOST  $A2_IPv4_2 "A2_INTFNAME_2"
fi


#----------------------------------------------------------
# Test Case  - Setup Simple Bridge MA_A1 to MS_A2 Config 
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="BridgeSetup_Simple"
scenario[${n}]="${xml[${n}]} - Configure Simple Bridge Config"
timeouts[${n}]=180
# Start Bridge
component[1]=cAppDriver,m1,"-e|../scripts/startMQTTBridge.sh","-o|../common/Bridge.A1A2.cfg"
# Configure policies
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../mqtt_td_tests/AAABridge/bridge.cli"

components[${n}]="${component[1]} ${component[2]}       "

((n+=1))


#---------------------------------------------------------
#       "admin/accept/license"
#
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-00-LicensedUsage"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for LicensedUsage"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/bridge/restapi-AcceptLicense.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))


#---------------------------------------------------------
#       "admin/config/Forwarder"
#
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-01-Forwarder"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for Forwarder"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/bridge/restapi-ConfigForwarder.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))


#---------------------------------------------------------
#       "admin/config/Connection"
#
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-02-Connection"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for Connection"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/bridge/restapi-ConfigConnection.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))


#---------------------------------------------------------
# Test Case 99 - Synopsis
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-99-Synopsis"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Synopsis of RESTAPI Execution"

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|${M1_TESTROOT}/restapi/synopsis.sh","","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "



