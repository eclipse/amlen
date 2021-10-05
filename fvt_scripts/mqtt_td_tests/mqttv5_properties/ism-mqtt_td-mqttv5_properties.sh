#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTTV5 Properties"

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
# Test Case 0 - mqtt_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - set policies for the mqttv5 properties bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupv5exp|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|mqttv5_cleanstart/policy_mqttv5.cli"

components[${n}]=" ${component[1]} ${component[2]} "

((n+=1))



#-------------------------------------------------------------
# Test Case  - MQTTv5 CONNECT Properties ReceiveMax 
#-------------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_connect_ReceiveMax"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 CONNECT Properties ReceiveMax"
timeouts[${n}]=90
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PubLowRM,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PubImplicitDefaultRM,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PubExplicitDefaultRM,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PubHighRM,-o_-l_9
#component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PubMaxRM,-o_-l_9,"-s|BITFLAG=-Djava.util.logging.config.file=/niagara/test/mqttv5trace.properties"
component[7]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SubReceiveMax,-o_-l_9
#component[8]=searchLogsEnd,m1,mqttv5_properties/${xml[${n}]}.comparetest

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}                 ${component[7]}"

((n+=1))

#-------------------------------------------------------------
# Test Case  - MQTTv5 CONNECT Properties Keep Alive 
#-------------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_connect_KeepAlive"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 CONNECT Properties Keep Alive"
timeouts[${n}]=45
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,KA_lessThanServerKeepAlive,-o_-l_9
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,KA_greaterThanServerKeepAlive,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,KA_ZERO,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,KA_MAX,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))


#-------------------------------------------------------------
# Test Case  - MQTTv5 CONNECT Properties SessionExpiryInterval 
#-------------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_connect_SEI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 CONNECT Properties Session Expiry Interval"
timeouts[${n}]=45
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SEI_lessThanPolicy,-o_-l_9
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SEI_MAXedToPolicy,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SEI_greaterThanPolicy,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]}  "

((n+=1))


#----------------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties Payload Format Indicator
#----------------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_PFI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties Payload Format Indicator"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PayloadFormatIndicator,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PFI_sub,-o_-l_9
components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))

#-------------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties MessageExpiryInterval
#-------------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_MEI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties Message Expiry Interval"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,MessageExpiryInterval,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,MEI_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties TopicAlias
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_TA"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties TopicAlias"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_PubMax,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_PubMax_sub,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_PubMax_wildcardsub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties ResponseTopic
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_RT"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties ResponseTopic"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,ResponseTopic,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,RT_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties CorrelationData
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_CD"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties CorrelationData"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,CorrelationData,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,CD_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties UserProperty
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_UP"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties UserProperty"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,UserProperty,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,UP_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties SubscriptionIdentifier
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_SI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties SubscriptionIdentifier"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SubscriptionIdentifier,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SI_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties ContentType
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_publish_CT"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties ContentType"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,ContentType,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,CT_sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties Payload Format Indicator
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_pubPropError_PFI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties Payload Format Indicator"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PFI_pub2fail,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,PFI_sub,-o_-l_9
component[4]=searchLogsEnd,m1,mqttv5_properties/${xml[${n}]}.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}  ${component[4]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties TopicAlias Errors
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_pubPropError_TA"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties TopicAlias Errors"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_PubGT_Max,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_Pub_Zero,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_Pub2DS,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,TopicAlias_Sub_DS,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Publish Properties SubscriptionIdentifiers Errors
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_pubPropError_SI"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Publish Properties SubscriptionIdentifiers Errors"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,SubscriptionIdentifierZero,-o_-l_9

components[${n}]="${component[1]} "

((n+=1))



#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Properties UserProperties
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subscribe_UP"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Properties User Property"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,UserProperty,-o_-l_9

components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 UNSubscribe Properties UserProperties
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_unsubscribe_UP"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Unsubscribe Properties User Property"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,UserProperty,-o_-l_9

components[${n}]="${component[1]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options NoLocal
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_NoLocal"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options - No Local"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,noLocal_Pub,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,noLocal_PubSub,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,noLocal_Sub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}  "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options NoLocal Error
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptionsError_NoLocal"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options Errors with No Local"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,noLocal_share,-o_-l_9
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,noLocal_sharedSub,-o_-l_9

components[${n}]="${component[1]} ${component[2]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainAsPublish
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_RetainAsPub"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options RetainAsPublish"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,pub,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0Topic,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0WC,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1Topic,-o_-l_9
component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1WC,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainAsPublish $share
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_RetainAsPub_share"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options RetainAsPublish on $share"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,pub,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0Topic,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0WC,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1Topic,-o_-l_9
component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1WC,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainAsPublish $SharedSubscription
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_RetainAsPub_sharedsub"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options RetainAsPublish on $SharedSubscription"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,pub,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0Topic,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv0WC,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1Topic,-o_-l_9
component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recv1WC,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainHandling
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_RetainHandling"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options - RetainHandling"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,RetainHandling_Pub,-o_-l_9
component[3]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recvRH0,-o_-l_9
component[4]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recvRH1,-o_-l_9
component[5]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recvRH2,-o_-l_9
component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,sharedSub0,-o_-l_9

components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "

((n+=1))


#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainHandling
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptions_RetainHandling.SimpleShare"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options - RetainHandling"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=sync,m1
component[2]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,RetainHandling_Pub,-o_-l_9
component[6]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,sharedSub0,-o_-l_9

components[${n}]="${component[1]} ${component[2]}  ${component[6]} "

###  temp for defect 211699  ###  
((n+=1))




#----------------------------------------------------------
# Test Case  - MQTTv5 Subscribe Options RetainHandling Errors
#----------------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="mqttV5_subOptionsError_RetainHandling"
scenario[${n}]="${xml[${n}]} - Test MQTTv5 Subscribe Options - RetainHandling Errors"
timeouts[${n}]=30
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,mqttv5_properties/${xml[${n}]}.xml,recvRH3,-o_-l_9

components[${n}]="${component[1]}  "

((n+=1))


#----------------------------------------------------
# Finally - policy_cleanup and gather logs.  
#----------------------------------------------------
xml[${n}]="mqtt_v5_policy_cleanup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup policies for v5 properties bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[2]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupv5exp|-c|mqttv5_cleanstart/policy_mqttv5.cli"
component[3]=cAppDriver,m1,"-e|../common/rmScratch.sh"

components[${n}]="${component[1]} ${component[2]} ${component[3]} "
#components[${n}]="                                 ${component[3]} "

((n+=1))
