#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------
scenario_set_name="SNMP Tests "

typeset -i n=0
declare -r server1=tcp://${A1_HOST}:16501
declare -r server2=tcp://${A1_HOST}:17001

#----------------------------------------------------
# Scenario 1 - SNMP_Tests_001
#----------------------------------------------------
xml[${n}]="SNMP_SetupSNMPTrapD_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Test 1 - SNMP TrapDaemon Setup"
component[1]=cAppDriverLog,m1,"-e|./ism-SNMP-StartSNMPTrapD.sh"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_Tests_001
#----------------------------------------------------
xml[${n}]="SNMP_Setup_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Test 1 - Policy Setup for IMAServer objects"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_Setup_001
#----------------------------------------------------
xml[${n}]="SNMP_Setup_001_Create_JMS_Subs"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Create JMS Durable Subscriptions"
component[1]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS1|-a|subscribe|-t|/SNMP/Test/durablesub1|-s|${server2}|-n|0|-b|-O|-v"
component[2]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS2|-a|subscribe|-t|/SNMP/Test/durablesub1|-s|${server2}|-n|0|-b|-O|-v"
component[3]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS3|-a|subscribe|-t|/SNMP/Test/durablesub1|-s|${server2}|-n|0|-b|-O|-v"
component[4]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS4|-a|subscribe|-t|/SNMP/Test/durablesub2|-s|${server2}|-n|0|-b|-O|-v"
component[5]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS5|-a|subscribe|-t|/SNMP/Test/durablesub2|-s|${server2}|-n|0|-b|-O|-v"
component[6]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|snmp-JMS6|-a|subscribe|-t|/SNMP/Test/durablesub2|-s|${server2}|-n|0|-b|-O|-v"
component[7]=searchLogsEnd,m1,verifydurablesub.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_Publish_001
#----------------------------------------------------
xml[${n}]="SNMP_Publish_001"
timeouts[${n}]=420
scenario[${n}]="${xml[${n}]} - Publish JMS Durable Subscriptions - Buffered"

component[1]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|p1|-a|publish|-t|/SNMP/Test/durablesub1|-s|${server2}|-n|35000|-r|-O|-v"
component[2]=javaAppDriver,m1,"-e|svt.jms.ism.JMSSample,-o|-i|p2|-a|publish|-t|/SNMP/Test/durablesub2|-s|${server2}|-n|35000|-r|-O|-v"
component[3]=searchLogsEnd,m1,verifypublishmsg.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_various verify steps 
#----------------------------------------------------

xml[${n}]="SNMP_verify_store_info"
scenario[${n}]="${xml[${n}]} - Verify Store Information (sometimes fails because size SNMP gets is slightly different from REST)"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyStoreInfo.sh"
component[2]=searchLogsEnd,m1,verifystore.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))


xml[${n}]="SNMP_verify_endpoint_info"
scenario[${n}]="${xml[${n}]} - Verify Endpoint Information"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyEndpointInfo.sh"
component[2]=searchLogsEnd,m1,verifyendpoint.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

xml[${n}]="SNMP_verify_memory_info"
scenario[${n}]="${xml[${n}]} - Verify Memory Information"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyMemoryInfo.sh"
component[2]=searchLogsEnd,m1,verifymemory.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

xml[${n}]="SNMP_verify_server_info"
scenario[${n}]="${xml[${n}]} - Verify Server Information"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyServerInfo.sh"
component[2]=searchLogsEnd,m1,verifyserver.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

xml[${n}]="SNMP_verify_harole_info"
scenario[${n}]="${xml[${n}]} - Verify HARole Information"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyHAInfo.sh"
component[2]=searchLogsEnd,m1,verifyha.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

xml[${n}]="SNMP_verify_General_info"
scenario[${n}]="${xml[${n}]} - Verify General Information"
timeouts[${n}]=240
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-VerifyGeneralInfo.sh"
component[2]=searchLogsEnd,m1,verifygeneral.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

xml[${n}]="SNMP_settest_info"
scenario[${n}]="${xml[${n}]} - Set SNMP Values"
timeouts[${n}]=100
component[1]=cAppDriverWait,m1,"-e|./ism-SNMP-SetValues.sh"
component[2]=searchLogsEnd,m1,verifysetsnmp.comparetest
components[${n}]="${component[1]} ${component[2]}"

((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_Cleanup_001
#----------------------------------------------------
xml[${n}]="SNMP_Cleanup_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Test 1 - Policy Cleanup for IMAServer objects"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - SNMP_Tests_001
#----------------------------------------------------
xml[${n}]="SNMP_KillSNMPTrapD_001"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Test 1 - SNMP TrapDaemon Cleanup"
component[1]=cAppDriverLog,m1,"-e|./ism-SNMP-KillSNMPTrapD.sh"
components[${n}]="${component[1]}"
((n+=1))

