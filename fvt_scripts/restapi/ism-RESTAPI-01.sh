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

#---------------------------------------------------------
#       "Reset Config"
# Set Env back to as Pristine as I can expect...
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-00-ResetConfig"
timeouts[${n}]=70
scenario[${n}]="${xml[${n}]} - RESTAPI for Service Reset Config"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-ServiceResetCfg.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
#       "Logging Group"
# Test Case 1 - AdminLog
#
# Test Cases removed ?for good?: AuditLogControl, TraceFilter, TraceMessageOptions, , TraceMessageLocation
# Test Cases returned from redesign: TraceBackup, TraceBackupCount, TraceBackupDestination, 
# Test Cases came back:  TraceConnection
# Test Cases came out of RRC:1536: LogLocation, Syslog
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-01-AdminLog"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for AdminLog"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-AdminLog.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#----------------------------------------------------------------
# Test Case 2 - ConnectionLog
#----------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-02-ConnectionLog"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for ConnectionLog"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-ConnectionLog.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 3 - LogLevel
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-03-LogLevel"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for LogLevel"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-LogLevel.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 4 - SecurityLog
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-04-SecurityLog"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for SecurityLog"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-SecurityLog.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 5 - TraceBackup
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-05-TraceBackup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceBackup"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceBackup.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 6 - 
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-06-TraceBackupCount"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceBackupCount"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceBackupCount.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 7 - 
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-07-TraceBackupDestination"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceBackupDestination"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceBackupDestination.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#DEFECT 94207 
((n+=1))
#---------------------------------------------------------
# Test Case 8 - TraceConnection
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-08-TraceConnection"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceConnection"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceConnection.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 9 - TraceLevel
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-09-TraceLevel"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceLevel"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceLevel.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 10 - TraceMax
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-10-TraceMax"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceMax"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceMax.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 11 - TraceMessageData
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-11-TraceMessageData"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceMessageData"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceMessageData.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 12 - TraceOptions
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-12-TraceOptions"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceOptions"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceOptions.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 13 - TraceSelected
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-13-TraceSelected"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TraceSelected"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-TraceSelected.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 14 - LogLocation
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-14-LogLocation"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for LogLocation"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-LogLocation.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 15 - Syslog
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-15-Syslog"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for Syslog"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/logging/restapi-Syslog.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# "Message Flow Group"
# Test Case 20 - Message Hub
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-20-MessageHub"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for MessageHub"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-MessageHub.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 21 - ConnectionPolicy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-21-ConnectionPolicy"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - RESTAPI for ConnectionPolicy"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-ConnectionPolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))
#---------------------------------------------------------
# Test Case 22 - MessagingPolicy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="RESTAPI-22-MessagingPolicy"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - RESTAPI for MessagingPolicy"

# Set up the components for the test in the order they should start
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-MessagingPolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
#components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 23 - Endpoint
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-23-Endpoint"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for Endpoint"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-Endpoint.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 24 - Queue
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-24-Queue"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - RESTAPI for Queue"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-Queue.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "


####Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))
##---------------------------------------------------------
## Test Case 25 - Subscription  NEEDS REWORKS remove MQTT MOCHA
##---------------------------------------------------------
## The name of the test case and the prefix of the XML configuration file for the driver
##TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="RESTAPI-25-Subscription"
#timeouts[${n}]=60
#scenario[${n}]="${xml[${n}]} - RESTAPI for Subscription"
#
## Set up the components for the test in the order they should start
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-Subscription.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
#components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 26 - TopicPolicy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-26-TopicPolicy"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for TopicPolicy"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-TopicPolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 27 - QueuePolicy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-27-QueuePolicy"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for QueuePolicy"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-QueuePolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 28 - SubscriptionPolicy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-28-SubscriptionPolicy"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for SubscriptionPolicy"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-SubscriptionPolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 29 - Plugin
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-29-Plugin"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - RESTAPI for Plugin"

# Part 1
component[1]=cAppDriverWait,m1,"-e|../scripts/run-cli.sh","-o|-s|part1|-c|${M1_TESTROOT}/restapi/test/message-flow/restapi-Plugin.cli"
component[2]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/message-flow/restapi-Plugin.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
# Part 2
component[10]=cAppDriverWait,m1,"-e|../scripts/run-cli.sh","-o|-s|part2|-c|${M1_TESTROOT}/restapi/test/message-flow/restapi-Plugin.cli"
component[11]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-Plugin_2.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
# Set up the components for the test in the order they should start
components[${n}]=" ${component[1]} ${component[2]}  ${component[10]} ${component[11]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 30 - PluginDebugPort
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-30-PluginDebugPort"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginDebugPort"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginDebugPort.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 31 - PluginDebugServer
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-31-PluginDebugServer"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginDebugServer"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginDebugServer.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 32 - PluginMaxHeapSize
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-32-PluginMaxHeapSize"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginMaxHeapSize"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginMaxHeapSize.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 33 - PluginPort
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-33-PluginPort"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginPort"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginPort.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 34 - PluginServer
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-34-PluginServer"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginServer"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginServer.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 35 - PluginVMArgs
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-35-PluginVMArgs"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for PluginVMArgs"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/message-flow/restapi-PluginVMArgs.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "



###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# "Security Group"
# Test Case 40 - AdminEndpoint
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-40-AdminEndpoint"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - RESTAPI for AdminEndpoint"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-AdminEndpoint.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 41 - CertificateProfile
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-41-CertificateProfile"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for CertificateProfile"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-CertificateProfile.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 42 - Configuration Policy
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-42-ConfigurationPolicy"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for ConfigurationPolicy"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-ConfigurationPolicy.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 43 - FIPS
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-43-FIPS"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for FIPS"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-FIPS.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 44 - LDAP
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-44-LDAP"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - RESTAPI for LDAP"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-LDAP.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 45 - LTPAProfile
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-45-LTPAProfile"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for LTPAProfile"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-LTPAProfile.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 46 - OAuthProfile
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-46-OAuthProfile"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for OAuthProfile"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-OAuthProfile.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 47 - SecurityProfile
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-47-SecurityProfile"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for SecurityProfile"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-SecurityProfile.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 48 - ClientCertificate
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-48-ClientCertificate"
timeouts[${n}]=450
scenario[${n}]="${xml[${n}]} - RESTAPI for ClientCertificate"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-ClientCertificate.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 49 - TrustedCertificate
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-49-TrustedCertificate"
timeouts[${n}]=450
scenario[${n}]="${xml[${n}]} - RESTAPI for TrustedCertificate"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-TrustedCertificate.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 50 - PreSharedKey
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-50-PreSharedKey"
timeouts[${n}]=450
scenario[${n}]="${xml[${n}]} - RESTAPI for PreSharedKey"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-PreSharedKey.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 51 - CRLProfile
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-51-CRLProfile"
timeouts[${n}]=450
scenario[${n}]="${xml[${n}]} - RESTAPI for CRLProfile"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/security/restapi-CRLProfile.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION|M1_URL=$URL_M1_IPv4"
components[${n}]="${component[1]} "

#REMOVED byDefect 95534 - ###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#REMOVED byDefect 95534 - ((n+=1))
#---------------------------------------------------------
# "Server Group and License"
#REMOVED byDefect 95534 - # Test Case 60 - AdminMode
#  - AdminMode is now INTERNAL ONLY - stays in schema
#  - REMOVED FOR REDESIGN:  BackupToDisk, RestoreFromDisk, HA
#  - REMOVED:  MemoryType and Version (AdminMode)
#---------------------------------------------------------
#REMOVED byDefect 95534 - # The name of the test case and the prefix of the XML configuration file for the driver
#REMOVED byDefect 95534 - #TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
#REMOVED byDefect 95534 - xml[${n}]="RESTAPI-60-AdminMode"
#REMOVED byDefect 95534 - timeouts[${n}]=60
#REMOVED byDefect 95534 - scenario[${n}]="${xml[${n}]} - RESTAPI for AdminMode"
#REMOVED byDefect 95534 - 
#REMOVED byDefect 95534 - # Set up the components for the test in the order they should start
#REMOVED byDefect 95534 - component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-AdminMode.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
#REMOVED byDefect 95534 - components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 61 - AdminUserID
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-61-AdminUserID"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for AdminUserID"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-AdminUserID.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 62 - AdminUserPassword
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-62-AdminUserPassword"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for AdminUserPassword"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-AdminUserPassword.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

##Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# Test Case 63 - ClusterMembership
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-63-ClusterMembership"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - RESTAPI for ClusterMembership"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-ClusterMembership.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 64 - EnableDiskPersistence
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-64-EnableDiskPersistence"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for EnableDiskPersistence"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-EnableDiskPersistence.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# Test Case 65 - Licensed Usage
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-65-LicensedUsage"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - RESTAPI for LicensedUsage"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/license/restapi-LicensedUsage.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# "Client Set Test Cases"
# Test Case 70 - ClientSet Scale and ForceDelete
# NOTE:  THIS Depends on OrgMovePub and OrgMoveSub being added to env variable files (ISMsetup.sh or testEnv.sh) 
#       !!BEFORE!! it is sourced and processed by PrepareTestEnvParameters.sh - see:  ismAuto-RESTAPI-01.sh
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-70-ClientSetScale"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - RESTAPI for ClientSet Scale and ForceDelete"

# Set up the components for the test in the order they should start
# OrgMove SETUP
component[1]=sync,m1
component[2]=cAppDriver,m1,"-e|./rm_file.sh","-o|A1|/mnt/A1/messagesight/data/export/*"
component[3]=cAppDriver,m1,"-e|./rm_file.sh","-o|A1|/mnt/A1/messagesight/data/import/*"
component[4]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
# OrgMove JMS Clients
component[10]=jmsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-jms.xml,rx0_setup
component[11]=jmsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-jms.xml,tx0_setup
# Org Move MQTT Clients
component[20]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,subscribe0
component[21]=wsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,publish0
component[22]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,subscribe1
component[23]=wsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,publish1
component[24]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,subscribe2
component[25]=wsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,publish2
component[26]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,subscribe3
component[27]=wsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,publish3
component[28]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,subscribe4
component[29]=wsDriverWait,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,publish4
#
component[30]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/clientset/restapi-Export_LargeClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME"
component[31]=cAppDriverWait,m1,"-e|./scp_file.sh","-o|A1|/mnt/A1/messagesight/data/export/*|A1|/mnt/A1/messagesight/data/import"
#
component[35]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/clientset/restapi-Import_LargeClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME"
#
component[40]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,receive0
component[41]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,receive1
component[42]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,receive2
component[43]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,receive3
component[44]=wsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-mqtt.xml,receive4
#
component[45]=jmsDriver,m2,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-jms.xml,rx0_postA1
component[46]=jmsDriver,m1,../cluster_tests/client_set/RESTAPI-OrgMove_LargeClientSet-jms.xml,tx0_postA1

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[10]} ${component[11]} ${component[20]} ${component[21]} ${component[22]} ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[35]} ${component[40]} ${component[41]} ${component[42]} ${component[43]} ${component[44]} ${component[45]} ${component[46]} "


##Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 71 - OrgMove ClientSet:Import,Export,Delete ErrorPath   (Happy Path is in 5 Server Env)
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-71-ClientSet"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - RESTAPI for OrgMove ClientSet:Import,Export,Delete ErrorPath"

# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|test/restapi.cli|-a|1"
component[3]=cAppDriver,m1,"-e|./rm_file.sh","-o|A1|/mnt/A1/messagesight/data/export/*"
component[4]=cAppDriver,m1,"-e|./rm_file.sh","-o|A1|/mnt/A1/messagesight/data/import/*"
component[5]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
# OrgMove SETUP
component[6]=jmsDriver,m2,../cluster_tests/client_set/RESTAPI_jms_OrgMove_ClientSet.xml,rx0_setup
component[7]=jmsDriver,m1,../cluster_tests/client_set/RESTAPI_jms_OrgMove_ClientSet.xml,tx0_setup
component[8]=wsDriver,m2,../cluster_tests/client_set/RESTAPI_OrgMove_ClientSetSetup.xml,subscribe0
component[9]=wsDriverWait,m1,../cluster_tests/client_set/RESTAPI_OrgMove_ClientSetSetup.xml,publish0
#
component[10]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/clientset/restapi-ExportClientSet_ErrorPath.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME"
#  ClientSet PUT must be tested manually, it corrupts binary files.
###component[11]=cAppDriverWait,m1,"-e|./scp_file2M1.sh","-o|A1|/mnt/A1/messagesight/data/export/*|."
component[11]=cAppDriverWait,m1,"-e|./scp_file.sh","-o|A1|/mnt/A1/messagesight/data/export/*|A1|/mnt/A1/messagesight/data/import/"
#
component[15]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/clientset/restapi-ImportClientSet_ErrorPath.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME"
#

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[9]} ${component[10]} ${component[11]} ${component[15]} "
#

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# Test Case 72 - Session Expiry
#---------------------------------------------------------
xml[${n}]="RESTAPI-72-SessionExpiry"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - server monitoring and MQTT clients for session expiration"
component[1]=sync,m1
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|test/restapi.cli|-a|1"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupSE|-c|test/restapi.cli|-a|1"
component[4]=wsDriverWait,m1,test/clientset/restapi_sessionexpiry_01.xml,SetupMQTTSession,-o_-l_9
component[5]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/clientset/restapi-SessionExpiry.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|SESSION_EXPIRY_TEST=SetupMQTTSession"
component[6]=wsDriverWait,m1,test/clientset/restapi_sessionexpiry_01.xml,SessionExpireMQTT,-o_-l_9
component[7]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/clientset/restapi-SessionExpiry.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|SESSION_EXPIRY_TEST=SessionExpireMQTT"
component[8]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupSE|-c|test/restapi.cli|-a|1"

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# "Server Group"
# Test Case 80 - ServerName
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-80-ServerName"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for ServerName"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-ServerName.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# Test Case 81 - ServerUID
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-81-ServerUID"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - RESTAPI for ServerUID"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-ServerUID.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

#---------------------------------------------------------
# Test Case 82 - XATransaction
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-82-XATransaction"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - RESTAPI for XATransaction"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-XATransaction.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 83 - TolerateRecoveryInconsistencies
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-83-TolerateRecoveryInconsistencies"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - RESTAPI for TolerateRecoveryInconsistencies"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-TolerateRecoveryInconsistencies.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "


###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# "MQ Group"
# Test Case 90 - MQ DestinationMappingRule
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-90-DestinationMappingRule"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - RESTAPI for DestinationMappingRule"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/mq/restapi-DestinationMappingRule.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 91 - MQConnectivityEnabled
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-91-MQConnectivityEnabled"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - RESTAPI for MQConnectivityEnabled"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/mq/restapi-MQConnectivityEnabled.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 92 - MQConnectivityLog
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-92-MQConnectivityLog"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - RESTAPI for MQConnectivityLog"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/mq/restapi-MQConnectivityLog.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
#---------------------------------------------------------
# Test Case 93 - QueueManagerConnection
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-93-QueueManagerConnection"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - RESTAPI for QueueManagerConnection"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/mq/restapi-QueueManagerConnection.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION"
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



