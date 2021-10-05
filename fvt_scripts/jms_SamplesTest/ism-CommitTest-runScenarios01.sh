#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Commit Tests"

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
# JMS_CommitTest_0
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="jms_CommitTest0"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS CommitTest default."
#component[1]=javaAppDriver,m1,-e_CommitTest,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,CommitTest_0.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_CommitTest_1
#----------------------------------------------------

xml[${n}]="jms_CommitTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS CommitTest 65536 100"
component[1]=javaAppDriver,m1,-e_CommitTest,-o_65536_100,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitTest_1.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_CommitTest_2
#----------------------------------------------------

xml[${n}]="jms_CommitTest2"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS CommitTest 128 10000"
component[1]=javaAppDriver,m1,-e_CommitTest,-o_128_10000,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitTest_2.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_CommitTest_3
#----------------------------------------------------

xml[${n}]="jms_CommitTest3"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS Commit Test 16384 1000 50 50"
component[1]=javaAppDriver,m1,-e_CommitTest,-o_16384_1000_50_50,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitTest_3.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_CommitListenerTest_4
#----------------------------------------------------

#xml[${n}]="jms_CommitListenerTest4"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS CommitListenerTest default."
#component[1]=javaAppDriver,m1,-e_CommitListenerTest,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,CommitListenerTest_4.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_CommitListenerTest_5
#----------------------------------------------------

xml[${n}]="jms_CommitListenerTest5"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS CommitListenerTest 65536 100."
component[1]=javaAppDriver,m1,-e_CommitListenerTest,-o_65536_100,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitListenerTest_5.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_CommitListenerTest_6
#----------------------------------------------------

xml[${n}]="jms_CommitListenerTest6"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS CommitListenerTest 128 10000."
component[1]=javaAppDriver,m1,-e_CommitListenerTest,-o_128_10000,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitListenerTest_6.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_CommitListenerTest_7
#----------------------------------------------------

xml[${n}]="jms_CommitListenerTest7"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS CommitListenerTest 16384 1000 50 50."
component[1]=javaAppDriver,m1,-e_CommitListenerTest,-o_16384_1000_50_50,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,CommitListenerTest_7.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

