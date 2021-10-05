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
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="CSC Mfg'ing scripts from GUAD running in Austin."

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

#---------------------------------------------------------
# Test Case 0 - Run the CSC Automated MFG Script in the Austin Env.
#---------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="CSC_MFG-execution_01"
RATE=1800
MESSAGES=6
timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
scenario[${n}]="${xml[${n}]} - Setup and Execute the CSC Mfg Environment"

# Set up the components for the test in the order they should start
component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_misc/csc-2-austin-setup.sh","-o|${A1_HOST}|${ISM_AUTOMATION}"

components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))

#----------------------------------------------------------------
# Test Case 1 - CSC Scripts Automated Execution
#----------------------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="CSC_MFG-execution_01"
#RATE=600
#MESSAGES=1
#timeouts[${n}]=$(( ${RATE} * ${MESSAGES} ))
#scenario[${n}]="${xml[${n}]} - Running CSC Guad Mfg Scripts in Austin"

# Set up the components for the test in the order they should start
#component[1]=cAppDriverRC,m1,"-e|${M1_TESTROOT}/svt_jms/csc-2-austin-setup.sh","-o|${A1_HOST}"

#components[${n}]="${component[1]} "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
#((n+=1))

