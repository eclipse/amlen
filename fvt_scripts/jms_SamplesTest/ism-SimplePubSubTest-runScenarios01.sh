#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Simple Pub/Sub Tests for Echo and Topic"

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
# Scenario 0 - SimplePubSubTest 0
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
#xml[${n}]="SimpleEchoPubSubTest0"
#timeouts[${n}]=30
#scenario${n}="[${xml[${n}]}] Sample - SimpleEchoPubSubTest default"
#component[1]=javaAppDriver,m1,-e_SimpleEchoSubscriber,-o_RequestT_9,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=javaAppDriver,m2,-e_SimpleEchoPublisher,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[3]=searchLogsEnd,m1,SimpleEchoPubSubTest_0m1.comparetest
#component[4]=searchLogsEnd,m2,SimpleEchoPubSubTest_0m2.comparetest
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#((n+=1))

#----------------------------------------------------
# Scenario 1 - SimplePubSubTest 1
#----------------------------------------------------

xml[${n}]="SimpleEchoPubSubTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - SimpleEchoPubSubTest 2 pairs"
component[1]=javaAppDriver,m1,-e_SimpleEchoSubscriber,-o_RequestT1_15,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[2]=javaAppDriver,m1,-e_SimpleEchoSubscriber,-o_RequestT2_12,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[3]=javaAppDriver,m2,-e_SimpleEchoPublisher,-o_RequestT1_messageT1_5,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[4]=javaAppDriver,m2,-e_SimpleEchoPublisher,-o_RequestT2_messageT2_4,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
component[5]=searchLogsEnd,m1,SimpleEchoPubSubTest_1m1.comparetest
component[6]=searchLogsEnd,m2,SimpleEchoPubSubTest_1m2.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))

#----------------------------------------------------
# Scenario 3 - SimplePubSubTest 3
#----------------------------------------------------

#xml[${n}]="SimpleTPubSubTest0"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} Sample - SimpleTPubSubTest default"
#component[1]=javaAppDriver,m1,-e_SimpleTSubscriber,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[2]=javaAppDriver,m2,-e_SimpleTPublisher,,-s-IMAServer=${A1_IPv4_1}-IMAPort=${IMAPort}
#component[3]=searchLogsEnd,m1,SimpleTPubSubTest_0m1.comparetest
#component[4]=searchLogsEnd,m2,SimpleTPubSubTest_0m2.comparetest
#components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#((n+=1))

#----------------------------------------------------
# Scenario 4 - SimplePubSubTest 4
#----------------------------------------------------

xml[${n}]="SimpleTPubSubTest1"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} Sample - SimpleTPubSubTest 2 pairs using IPv6 Address"
component[1]=javaAppDriver,m1,-e_SimpleTSubscriber,-o_RequestT1_15,-s-IMAServer=${A1_IPv6_1}-IMAPort=${IMAPort}
component[2]=javaAppDriver,m1,-e_SimpleTSubscriber,-o_RequestT2_12,-s-IMAServer=${A1_IPv6_1}-IMAPort=${IMAPort}
component[3]=javaAppDriver,m2,-e_SimpleTPublisher,-o_RequestT1_messageT1_15,-s-IMAServer=${A1_IPv6_1}-IMAPort=${IMAPort}
component[4]=javaAppDriver,m2,-e_SimpleTPublisher,-o_RequestT2_messageT2_12,-s-IMAServer=${A1_IPv6_1}-IMAPort=${IMAPort}
component[5]=searchLogsEnd,m1,SimpleTPubSubTest_1m1.comparetest
component[6]=searchLogsEnd,m2,SimpleTPubSubTest_1m2.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))

