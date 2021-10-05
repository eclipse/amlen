#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="mqtt SSL - 00"

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

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize
    #----------------------------------------------------
    xml[${n}]="mqtt_ha_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - Populate store to make sure store remains across
#               switch to HA
#----------------------------------------------------
xml[${n}]="testmqtt_HApopulate"
timeouts[${n}]=150
scenario[${n}]="${xml[${n}]} - Populate store before HA switchover [ ${xml[${n}]}.xml ]"
component[1]=sync,m1
component[2]=jmsDriver,m2,../jms_td_tests/HA/jms_HA_005.xml,rmds
component[3]=jmsDriver,m1,../jms_td_tests/HA/jms_HA_005.xml,rmds2
component[4]=jmsDriver,m1,../jms_td_tests/HA/jms_HA_005.xml,rmdt
component[5]=wsDriver,m2,HA/${xml[${n}]}.xml,populate,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))


#----------------------------------------------------
# Test Case 1 - HA Setup
#----------------------------------------------------
xml[${n}]="testmqtt_HASetup"
timeouts[${n}]=240
scenario[${n}]="${xml[${n}]} - Configure HA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA"
#component[1]=cAppDriver,m1,"-e|../scripts/haFunctions.sh","-o|-a|setupHA"
components[${n}]="${component[1]}"
((n+=1))

