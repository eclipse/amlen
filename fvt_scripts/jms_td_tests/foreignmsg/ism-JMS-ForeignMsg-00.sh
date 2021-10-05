#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS Foreign Messages - 00"

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
# Scenario 1 - jms_foreignmsg_001
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_001t"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Send MQ messages with ISM"
component[1]=runCommandNow,m1,../common/jms_mq_setup.sh
component[2]=jmsDriver,m1,foreignmsg/jms_foreignmsg_001t.xml,ALL
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_foreignmsg_002
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_002t"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Send ISM messages with MQ"
component[1]=jmsDriver,m1,foreignmsg/jms_foreignmsg_002t.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_foreignmsg_003_Foreign
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_003_ForeignReplyTo"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Test 3 Foreign"
component[1]=jmsDriver,m1,foreignmsg/jms_foreignmsg_003_ForeignReplyTo.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_foreignmsg_003_Ism
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_003_IsmReplyTo"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Test 3 Ism"
component[1]=jmsDriver,m1,foreignmsg/jms_foreignmsg_003_IsmReplyTo.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_foreignmsg_004_Foreign
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_004_ForeignReplyTo"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Test 4 Foreign"
component[1]=jmsDriver,m1,foreignmsg/jms_foreignmsg_004_ForeignReplyTo.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_foreignmsg_004_Ism
#----------------------------------------------------
xml[${n}]="jms_foreignmsg_004_IsmReplyTo"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Foreign Messages - Test 4 Ism"
component[1]=jmsDriver,m1,foreignmsg/jms_foreignmsg_004_IsmReplyTo.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_001
#----------------------------------------------------
xml[${n}]="jms_replyTo_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set ReplyTo and send message back (single) - Test 1"
component[1]=jmsDriver,m1,foreignmsg/jms_replyTo_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_002
#----------------------------------------------------
xml[${n}]="jms_replyTo_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set Reply To  and send messages back (multiple)- Test 2"
component[1]=sync,m1
component[2]=jmsDriver,m1,foreignmsg/jms_replyTo_002.xml,rmdr
component[3]=jmsDriver,m2,foreignmsg/jms_replyTo_002.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_003
#----------------------------------------------------
xml[${n}]="jms_replyTo_003"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Reply To on Temporary Topics - Test 3"
component[1]=sync,m1
component[2]=jmsDriver,m1,foreignmsg/jms_replyTo_003.xml,rmdr
component[3]=jmsDriver,m1,foreignmsg/jms_replyTo_003.xml,rmdr2
component[4]=jmsDriver,m2,foreignmsg/jms_replyTo_003.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_004
#----------------------------------------------------
xml[${n}]="jms_replyTo_004"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Reply To on Temporary Topic"
component[1]=sync,m1
component[2]=jmsDriver,m2,foreignmsg/jms_replyTo_004.xml,rmdr
component[3]=jmsDriver,m1,foreignmsg/jms_replyTo_004.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_005
#----------------------------------------------------
xml[${n}]="jms_replyTo_005"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Reply To on Temporary Queue"
component[1]=sync,m1
component[2]=jmsDriver,m2,foreignmsg/jms_replyTo_005.xml,rmdr
component[3]=jmsDriver,m1,foreignmsg/jms_replyTo_005.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_replyTo_006
#----------------------------------------------------
xml[${n}]="jms_replyTo_006"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Reply To on Temporary Topic/Queue GVT"
component[1]=sync,m1
component[2]=jmsDriver,m2,foreignmsg/jms_replyTo_006.xml,rmdr
component[3]=jmsDriver,m1,foreignmsg/jms_replyTo_006.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))
