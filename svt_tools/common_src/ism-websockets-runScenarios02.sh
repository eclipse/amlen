#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Web Sockets Sample App"

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

#----------------------------------------------------
# Test Case 0 - 
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="ISM_WebSocketTestCase_1"
scenario[${n}]="${xml[${n}]} - wsclient"

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n3|-s9|-T2|-M32|-u"
component[2]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n3|-p9|-T2|-M32|-u"
component[3]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n9|-s3|-T2|-M32|-u"
component[4]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n9|-p3|-T2|-M32|-u"
component[5]=searchLogsEnd,m1,ISM_WebSocketTestCase_1-runscenarios02.comparetest,9
components[${n}]="${component[1]} ${component[3]} ${component[2]} ${component[4]} ${component[5]}  "

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))
###----------------------------------------------------
### Test Case 1 -
###----------------------------------------------------
### The prefix of the XML configuration file for the driver
xml[${n}]="ISM_WebSocketTestCase_2"
scenario[${n}]="${xml[${n}]} - wsclient"

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n300|-s9|-T2|-M32|-u"
component[2]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n300|-p9|-T2|-M32|-u"
component[3]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n900|-s3|-T2|-M32|-u"
component[4]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n900|-p3|-T2|-M32|-u"
component[5]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n30|-s90|-T2|-M32|-u"
component[6]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n30|-p90|-T2|-M32|-u"
component[7]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n90|-s30|-T2|-M32|-u"
component[8]=cAppDriver,m1,"-e|${M1_ISM_SDK}/application/client_ship/bin/wsclient","-o|-aA1_IPv4_2|-o16102|-n90|-p30|-T2|-M32|-u"
component[9]=searchLogsEnd,m1,ISM_WebSocketTestCase_2-runscenarios02.comparetest,9
components[${n}]="${component[1]} ${component[3]} ${component[5]} ${component[7]} ${component[2]} ${component[4]} ${component[6]} ${component[8]} ${component[9]} "

