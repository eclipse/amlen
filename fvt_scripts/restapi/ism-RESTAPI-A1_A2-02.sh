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

scenario_set_name="REST APIs.A1_A2-ImportExport"

typeset -i n=0

echo "Running with NODEJS Components:"  >> ${logfile}
echo "NODE VERSION:  "`node --version`  >> ${logfile}
echo "MOCHA VERSION: "`mocha --version` >> ${logfile}

source getIfname.sh  $A1_USER  $A1_HOST  $A1_IPv4_1 "A1_INTFNAME_1"
source getIfname.sh  $A1_USER  $A1_HOST  $A1_IPv4_2 "A1_INTFNAME_2"
source getIfname.sh  $A2_USER  $A2_HOST  $A2_IPv4_1 "A2_INTFNAME_1"
source getIfname.sh  $A2_USER  $A2_HOST  $A2_IPv4_2 "A2_INTFNAME_2"

#---------------------------------------------------------
# Test Case 00 - Export - Import
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="RESTAPI-00-ExportImport"
timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} - RESTAPI for ExportImport"

# Set up the components for the test in the order they should start
component[1]=cAppDriverWait,m1,"-e|../jms_td_tests/HA/updateAllowSingleNIC.sh"
component[2]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|test/server/restapi-Export.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION|A2_HOSTNAME_OS=$A2_HOSTNAME_OS|A2_HOSTNAME_OS_SHORT=$A2_HOSTNAME_OS_SHORT|A2_SERVERNAME=$A2_SERVERNAME"
component[3]=cAppDriverWait,m1,"-e|../cli_tests/export_import/copy_file.sh","-o|/var/messagesight/userfiles/ExportServerConfig|/var/messagesight/."
component[4]=cAppDriverWait,m1,"-e|../cli_tests/export_import/copy_file.sh","-o|/var/messagesight/userfiles/imaBackup.*|/var/messagesight/."
component[5]=cAppDriverWait,m1,"-e|mocha","-o|test/server/restapi-ServiceResetCfg.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION|A2_HOSTNAME_OS=$A2_HOSTNAME_OS|A2_HOSTNAME_OS_SHORT=$A2_HOSTNAME_OS_SHORT|A2_SERVERNAME=$A2_SERVERNAME"
component[6]=cAppDriverWait,m1,"-e|../cli_tests/export_import/copy_file.sh","-o|/var/messagesight/ExportServerConfig|/var/messagesight/userfiles/."
component[7]=cAppDriverRConlyChk,m1,"-e|mocha","-o|test/server/restapi-Import.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION|A2_HOSTNAME_OS=$A2_HOSTNAME_OS|A2_HOSTNAME_OS_SHORT=$A2_HOSTNAME_OS_SHORT|A2_SERVERNAME=$A2_SERVERNAME"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} "


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
component[1]=cAppDriver,m1,"-e|${M1_TESTROOT}/restapi/synopsis.sh","","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}|A1_HOSTNAME_OS=$A1_HOSTNAME_OS|A1_HOSTNAME_OS_SHORT=$A1_HOSTNAME_OS_SHORT|A1_SERVERNAME=$A1_SERVERNAME|IMA_AUTOMATION=$IMA_AUTOMATION|A2_HOSTNAME_OS=$A2_HOSTNAME_OS|A2_HOSTNAME_OS_SHORT=$A2_HOSTNAME_OS_SHORT|A2_SERVERNAME=$A2_SERVERNAME"
components[${n}]="${component[1]} "

