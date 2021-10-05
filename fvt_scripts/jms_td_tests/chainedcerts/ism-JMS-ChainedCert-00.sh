#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Chained Certs - 00"

typeset -i n=0
typeset -i nn=0

java -version 1>out 2>&1
 nn=$(grep IBM out | wc -l)
 if [[ $nn -gt 0 ]] ; then
   TLS12="Yes"
 else
   nn=$(grep "1\.6\.0" out | wc -l)
   if [[ $nn -gt 0 ]] ; then
     TLS12="No"
   else
     TLS12="Yes"
   fi
 fi

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
# Scenario 0 - policy_setup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="jms_chainedcerts_config_setup"
timeouts[${n}]=160
scenario[${n}]="${xml[${n}]} - Setup"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|chainedcerts/chaincerts_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_chainedcert_001
#----------------------------------------------------
xml[${n}]="jms_chainedcert_001"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 001"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_001.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_chainedcert_002
#----------------------------------------------------
xml[${n}]="jms_chainedcert_002"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 002"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_002.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_chainedcert_003
#----------------------------------------------------
xml[${n}]="jms_chainedcert_003"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 003"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_003.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - jms_chainedcert_004
#----------------------------------------------------
xml[${n}]="jms_chainedcert_004"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 004"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_004.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_chainedcert_005
#----------------------------------------------------
xml[${n}]="jms_chainedcert_005"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 005"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_005.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_chainedcert_006
#----------------------------------------------------
xml[${n}]="jms_chainedcert_006"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 006"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_006.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_chainedcert_007
#----------------------------------------------------
xml[${n}]="jms_chainedcert_007"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 007"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_007.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_chainedcert_008
#----------------------------------------------------
xml[${n}]="jms_chainedcert_008"
timeouts[${n}]=45
scenario[${n}]="${xml[${n}]} - Using chained server certificates 008"
component[1]=jmsDriver,m1,chainedcerts/jms_chainedcert_008.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 99 - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="chainedcerts_config_cleanup"
timeouts[${n}]=160
scenario[${n}]="${xml[${n}]} - Cleanup in JMS chainedcerts test bucket"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|chainedcerts/chaincerts_config.cli"
components[${n}]="${component[1]}"
((n+=1))


