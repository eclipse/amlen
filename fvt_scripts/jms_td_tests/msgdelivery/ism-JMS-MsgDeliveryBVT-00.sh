#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Message Delivery BVT - 00"

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
# Scenario 2 - jms_msgdlvr1_002
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Multiple Connections Message Delivery, trace logging and setting Priority Plus retained messages- Test 2  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_002.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_002.xml,rmdt
component[4]=jmsDriver,m1,msgdelivery/jms_msgdelivery_002.xml,rmdr2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Cleanup Scenario - mqtt_clearRetained
#     Some of the prior tests created retained messages. 
#     This simple MQTT xml will remove those before
#     we do any Wildcard testing which would find them and fail., 
#----------------------------------------------------
xml[${n}]="jms_clearRetained"
timeouts[${n}]=60
scenario[${n}]="Clear any retained messages before running JMS Wildcard tests [ ${xml[${n}]}.xml ]"
component[1]=wsDriver,m1,msgdelivery/mqtt_clearRetained.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_msgdlvr1_006
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_006"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Basic Message Selection for BVT. (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_006.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_006.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario 11 - jms_msgdlvr1_011
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_011"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Basic Wildcard topics for BVT.  (Two host)"
component[1]=sync,m1
component[2]=jmsDriver,m1,msgdelivery/jms_msgdelivery_011.xml,rmdr
component[3]=jmsDriver,m1,msgdelivery/jms_msgdelivery_011.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 47 - jms_msgdelivery_sys
#----------------------------------------------------
xml[${n}]="jms_msgdelivery_sys"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - $SYS tests"
component[1]=jmsDriver,m1,msgdelivery/jms_msgdelivery_sys.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - policy_cleanup
#----------------------------------------------------
xml[${n}]="jms_policy_cleanup"
timeouts[${n}]=180
scenario[${n}]="${xml[${n}]} - Cleanup with local LDAP"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1  
    #----------------------------------------------------
    xml[${n}]="jms_policy_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
