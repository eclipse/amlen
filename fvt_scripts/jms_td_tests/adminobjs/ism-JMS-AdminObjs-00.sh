#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Admin Objects - 00"

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


#----------------------------------------------------
# Scenario 1 - jms_adminobjs_001
#----------------------------------------------------
xml[${n}]="jms_adminobjs_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of Connection Factory Administered Objects"
component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	#----------------------------------------------------
	# Scenario 2 - jms_adminobjs_002
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_002"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 2 - Additional creation of Connection Factory Administered Objects"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_002.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 4 - jms_adminobjs_004
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_004"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]}.xml - Test 4 - Update Connection Factory properties"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_004.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 5 - jms_adminobjs_005
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_005"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 5 - Additional Updates of Connection Factory properties"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_005.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 6 - jms_adminobjs_006
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_006"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 6 - Storing Admin Objects in LDAP server"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_006.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 7 - jms_adminobjs_007
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_007"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 7 - Storing custom properties in  Admin Objects"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_007.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
	
	#----------------------------------------------------
	# Scenario 8 - jms_adminobjs_008
	#----------------------------------------------------
	xml[${n}]="jms_adminobjs_008"
	timeouts[${n}]=60
	scenario[${n}]="${xml[${n}]} - Test 8 - Sending and receiving messages using JNDI Factory Objects with LDAP"
	component[1]=jmsDriver,m1,adminobjs/jms_adminobjs_008.xml,ALL
	components[${n}]="${component[1]}"
	((n+=1))
fi
