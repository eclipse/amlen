#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="PtPListener Tests"

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
# Scenario 0 - PtPListenerTest 0
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="PtPListenerTest0"
#timeouts[${n}]=30
#scenario[${n}]="[${xml${n}]} Sample - PtPListenerTest default"
#component[1]=javaAppDriver,m1,-e_PtPListenerTest,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,PtPListenerTest_0.comparetest,9
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# Scenario 1 - PtPListenerTest 1
#----------------------------------------------------

xml[${n}]="PtPListenerTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - PtPListenerTest 128 10000 Q1"
component[1]=javaAppDriver,m1,-e_PtPListenerTest,-o_128_10000_Q1,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,PtPListenerTest_1.comparetest,9
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - PtPListenerTest 2
#----------------------------------------------------

xml[${n}]="PtPListenerTest2"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - PtPListenerTest 16384 100 Q2"
component[1]=javaAppDriver,m1,-e_PtPListenerTest,-o_16384_100_Q2,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,PtPListenerTest_2.comparetest,9
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - PtPListenerTest 3
#----------------------------------------------------

#xml[${n}]="PtPListenerTest3"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - PtPListenerTest 128 50000 T31, TopicListenerTest 128 1000 T32"
#component[1]=javaAppDriver,m1,-e_PtPListenerTest,-o_128_50000_Q31,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=javaAppDriver,m1,-e_PtPListenerTest,-o_128_1000_Q32,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[3]=searchLogsEnd,m1,PtPListenerTest_3.comparetest,9
#components[${n}]="${component[1]} ${component[2]} ${component[3]}"
#((n+=1))


#----------------------------------------------------
# Scenario 1 - MQ_scenario1
#----------------------------------------------------

#xml[${n}]="MQ_scenario1"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Test the MQ Bridge Scenario1."
#component[1]=llmd,m1,MQ-scenario1.cfg,deletestores,createstores
#component[3]=llmDriver,m2,rmdt,rename,-l_6
#component[4]=searchLogsEnd,m1,entry_struct_llmd_mqq1.compare,1
#components[${n}]="${component[1]} ${component[3]} ${component[4]}"

#((n+=1))


