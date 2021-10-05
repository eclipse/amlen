#! /bin/bash

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#----------------------------------------------------
#  THIS SCRIPT IS FOR USE WHEN RUNNING CLIENTS AGAINST 
#  A SERVER IN DIFFERENT DATACENTER. IN THAT CASE A BASIC
#  TEST RUN TO VERIFY THE SERVER CONFIGURES AND RUNS
#  IN THAT ENVIRONEMENT IS ALL THAT IS NEEDED. 
#  
#  COMPLEX SCENARIO TESTING SHOULD BE DONE ON SETUPS WHERE
#  THE CLIENTS AND SERVERS ARE IN THE SAME DATACENTER. 
# 
#  IT IS IMPORTANT TO NOT ADD EVERY TEST TO THIS SCRIPT! 
#
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS MaxMessageBehavior - for REMOTE SERVERS"

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

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="jms_policy_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - JMS_MaxMessage_policyconfig_setup
#----------------------------------------------------
xml[${n}]="jms_policy_setup_for_MaxMessages"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli for the MaxMessages behavior bucket (internal LDAP)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupMM|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Scenario 1 - jms_maxmsgs_001
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_001AutoAck"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Non-durable JMS Subscription exceeding MaxMessage size with behavior discardOldest clientcache=0 autoack"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - jms_maxmsgs_001
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_001ClientAck"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Non-durable JMS Subscription exceeding MaxMessage size with behavior discardOldest clientcache=0 clientack"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_maxmsgs_002
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Non-durable JMS Subscription exceeding MaxMessage size with behavior discardOldest clientcache=default"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_maxmsgs_003
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_003"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Durable JMS Subscription exceeding MaxMessage size with behavior discardOldest clientcache=default"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons2
component[4]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))



#----------------------------------------------------
# Scenario 5 and 5Async
# jms_maxmsgs_005 and jms_maxmsgs_005Async for Real Appliances
# or jms_maxmsgs_005Virtual and jms_maxmsgs_005AsyncVirtual for Vmware, KVM's and ESX.
# and baremetal 
# 
#----------------------------------------------------


#----------------------------------------------------
# Scenario 4 - jms_maxmsgs_trans1
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_trans1"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - JMS DiscOldMsgs Transacted"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,TX
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,RX
component[4]=wsDriver,m1,maxmsgbehavior/mqtt_maxmsgs_trans1.xml,RX_qos0
component[5]=wsDriver,m1,maxmsgbehavior/mqtt_maxmsgs_trans1.xml,RX_qos1
component[6]=wsDriver,m1,maxmsgbehavior/mqtt_maxmsgs_trans1.xml,RX_qos2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_maxmsgs_trans2
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_trans2"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - JMS DiscOldMsgs Transacted AsyncTrasactedSend"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,TX
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,RX
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_maxmsgs_trans3
#----------------------------------------------------
xml[${n}]="jms_maxmsgs_trans3"
timeouts[${n}]=80
scenario[${n}]="${xml[${n}]} - JMS DiscOldMsgs Transacted Receieves"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,TX
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,RX
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jms_maxmsgsDynamic_001
#----------------------------------------------------
xml[${n}]="jms_maxmsgsDynamic_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Dynamically updating MaxMessages value and MaxMessageBehavior"
component[1]=sync,m1
component[2]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Cons1
component[3]=jmsDriver,m1,maxmsgbehavior/${xml[${n}]}.xml,Prod1
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Test Case 99 - JMS_sharedsub_policyconfig_cleanup
#----------------------------------------------------
xml[${n}]="jms_policy_cleanup_for_MaxMessageBehavior"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - run policy_config.cli for the MaxMessages behavior bucket (internal LDAP)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupMM|-c|restpolicy_config.cli"
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
