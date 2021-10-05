#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Temporary Topic and Queue Tests"

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
# JMS_TemporaryQueueTest_0
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="jms_TemporaryQueueTest0"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS jms_TemporaryQueueTest default."
#component[1]=javaAppDriver,m1,-e_TemporaryQueueTest,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryQueueTest_0.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))


#----------------------------------------------------
# Removed Tests: QueueTest1, QueueTest2 and QueueTest3
# due to current non-support of Queues as of 6-1-12
#----------------------------------------------------

#----------------------------------------------------
# JMS_TemporaryQueueTest_1
#----------------------------------------------------

#xml[${n}]="jms_TemporaryQueueTest1"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryQueueTest 128 10000"
#component[1]=javaAppDriver,m1,-e_TemporaryQueueTest,-o_128_5000,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryQueueTest_1.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_TemporaryQueueTest_2
#----------------------------------------------------

#xml[${n}]="jms_TemporaryQueueTest2"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryQueueTest 16384 100"
#component[1]=javaAppDriver,m1,-e_TemporaryQueueTest,-o_16384_100,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryQueueTest_2.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_TemporaryQueueTest_3
#----------------------------------------------------

#xml[${n}]="jms_TemporaryQueueTest3"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryQueueTest 128 1000 50 50"
#component[1]=javaAppDriver,m1,-e_TemporaryQueueTest,-o_128_1000_50_50,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryQueueTest_3.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_TemporaryTopicTest_0
#----------------------------------------------------

#xml[${n}]="jms_TemporaryTopicTest0"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS jms_TemporaryTopicTest default."
#component[1]=javaAppDriver,m1,-e_TemporaryTopicTest,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryTopicTest_0.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

#----------------------------------------------------
# JMS_TemporaryTopicTest_1
#----------------------------------------------------

xml[${n}]="jms_TemporaryTopicTest1"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryTopicTest 128 5000"
component[1]=javaAppDriver,m1,-e_TemporaryTopicTest,-o_128_5000,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,TemporaryTopicTest_1.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_TemporaryTopicTest_2
#----------------------------------------------------

xml[${n}]="jms_TemporaryTopicTest2"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryTopicTest 5000 50 using IPv6 Address"
component[1]=javaAppDriver,m1,-e_TemporaryTopicTest,-o_5000_50,-s-IMAServer=${A1_IPv6_1}-IMAPort=${IMAPort}
component[2]=searchLogsEnd,m1,TemporaryTopicTestREMOTE_2.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# JMS_TemporaryTopicTest_3  Not implemented yet in ISM
#---------------------------------------------------

#xml[${n}]="jms_TemporaryTopicTest3"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - JMS TemporaryTopicTest 128 1000 50 50"
#component[1]=javaAppDriver,m1,-e_TemporaryTopicTest,-o_128_1000_50_50,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=searchLogsEnd,m1,TemporaryTopicTest_3.comparetest
#components[${n}]="${component[1]} ${component[2]}"
#((n+=1))

