#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name=" Export, Import, Delete ClientSet functionality " 

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
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

if [[ "${A1_TYPE}" == "ESX" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
fi

#----------------------------------------------------
# Setup Scenario  - Configure ClusterMembership on the 2 servers. 
#----------------------------------------------------
#xml[${n}]="clusterCS_config_setup"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} -ClientSet -Set minimal required parameters for configuring a Cluster of 2 for Client Set testing "
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|cluster_manipulation/cluster_manipulation.cli|-a|1"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
#	components[${n}]="${component[1]} ${component[2]}"
#else	
#	component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1CM|-c|cluster_manipulation/cluster_manipulation.cli"
#	component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2CM|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
#fi	
#((n+=1))


#----------------------------------------------------
# Scenario 0 - Cleanup Client Set Delete #0
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp0"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - ClientSet Delete ALL: UncoupleServers,  Durable Subscriptions, MQTT connections, "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|4"
#
component[5]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[6]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A2|/var/messagesight/data/export/*"
component[7]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
component[8]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A4|/mnt/A4/messagesight/data/import/*"
#
component[10]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[11]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A2_IPv4_1}:${A2_PORT}"
component[12]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A3_IPv4_1}:${A3_PORT}"
component[13]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A4_IPv4_1}:${A4_PORT}"
# 
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]} ${component[10]} ${component[11]} ${component[12]} ${component[13]}  "

((n+=1))

#----------------------------------------------------
# Scenario 1 - Basic Client Set Org Move
#----------------------------------------------------
# A1_A2 = HA Pair 1 or Cluster1 Members
# A3_A4 = HA Pair 2 or Cluster2 Members
xml[${n}]="RESTAPI_OrgMove_ClientSetSetup"
xml1[${n}]="RESTAPI_OrgMove_ClientSetVerify_A1A2"
xml2[${n}]="RESTAPI_OrgMove_ClientSetVerify_A3A4"
xmlStay[${n}]="RESTAPI_OrgStay_ClientSetStream"
xmlExist[${n}]="RESTAPI_OrgExist_ClientSetStream"

xmlSync[${n}]="RESTAPI_Org_ClientSetSyncPoints"

timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} -  ClientSet - Export(A1) and Import(A3) Durable Subscriptions of MQTT clients "
component[1]=sync,m1
# OrgMove SETUP
component[2]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe0
component[3]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe1
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe2
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
   ###component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish0
component[8]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish2
component[9]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish3
component[10]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish4
   ###component[11]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive0
component[12]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[13]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
# OrgStay SETUP
component[14]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe0
component[15]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe1
component[16]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe2
component[17]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe3
component[18]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe4
component[19]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish0
component[20]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish1
component[21]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish2
component[22]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish3
component[23]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish4
# OrgExist SETUP
component[26]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe0
component[27]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe1
component[28]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe2
component[29]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe3
component[30]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe4
component[31]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish0
component[32]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish1
component[33]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish2
component[34]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish3
component[35]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish4
#

# Export(A1) here...
component[40]=sleep,2
component[41]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Export_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[42]=cAppDriverWait,m1,"-e|../restapi/scp_file.sh","-o|A1|/var/messagesight/data/export/*|A3|/mnt/A3/messagesight/data/import/"
component[43]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,export
# Setup needs to be BEFORE component[2]
component[45]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
component[46]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|2"
component[47]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|3"
component[48]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|4"
#
# Recv (A1)
# OrgMove Post Export
#component[50]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive0
component[51]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive1
component[52]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive2
component[53]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive3
component[54]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive4
#component[55]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish0
component[56]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish1
component[57]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish2
component[58]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish3
component[59]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish4
#
# Import(A3)
component[60]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Import_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[61]=sleep,4
component[62]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,import
#  CHANGED to be part of SETUP to be sure the DIR is clean before running tests (Pre Delete Export/Import)
component[63]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[64]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
#
# Recv(A3)
# OrgMove POST IMPORT
#component[70]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive0
component[71]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive1
component[72]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive2
component[73]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive3
component[74]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive4
#component[75]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish0
component[76]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish1
component[77]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish2
component[78]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish3
component[79]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish4
#

# Restart Servers, verify nothing new is in store, A3 has new info
# Delete ClientSet (A1)
component[80]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,delete
component[81]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteImEx_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"

#FULL ON
components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     ${component[40]} ${component[41]} ${component[42]} ${component[43]}     ${component[51]} ${component[52]} ${component[53]} ${component[54]}        ${component[56]} ${component[57]} ${component[58]} ${component[59]}         ${component[60]} ${component[61]} ${component[62]}        ${component[71]} ${component[72]} ${component[73]} ${component[74]}   ${component[76]} ${component[77]} ${component[78]} ${component[79]} ${component[80]} ${component[81]}   "

# Setup Only
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}    ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     "


###  OrgMove  Setup and Verify Only
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[2]}  ${component[3]}  ${component[4]}  ${component[5]}  ${component[6]}      ${component[8]}  ${component[9]}  ${component[10]}    ${component[12]} ${component[13]}     ${component[40]} ${component[41]} ${component[42]} ${component[43]}     ${component[51]} ${component[52]} ${component[53]} ${component[54]}   ${component[56]} ${component[57]} ${component[58]} ${component[59]}    ${component[60]} ${component[61]} ${component[62]}         ${component[71]} ${component[72]} ${component[73]} ${component[74]}   ${component[76]} ${component[77]} ${component[78]} ${component[79]} ${component[80]} ${component[81]}  "
###  OrgStay  Setup and Verify Only
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]} ${component[20]} ${component[21]} ${component[22]} ${component[23]}            ${component[40]} ${component[41]} ${component[42]} ${component[43]}                                                                                                                                                  ${component[60]} ${component[61]} ${component[62]}      "
###  OrgExist  Setup and Verify Only
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}            ${component[40]} ${component[41]} ${component[42]} ${component[43]}                                                                                                                                                  ${component[60]} ${component[61]} ${component[62]}      "

((n+=1))


#----------------------------------------------------
# Scenario 2 - Cleanup Client Set #1 (Basic)
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp1"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - ClientSet Delete ALL after Basic Import Export "
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
#component[2]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A3_IPv4_1}:${A3_PORT}"
#
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|4"
#
component[5]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[6]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A2|/var/messagesight/data/export/*"
component[7]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
component[8]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A4|/mnt/A4/messagesight/data/import/*"

# Setup Only
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}  "

((n+=1))

#----------------------------------------------------
# Scenario 3 - Cluster Client Set Org Move
#----------------------------------------------------
# A1_A2 = Cluster1 Members
# A3_A4 = Cluster2 Members
xml[${n}]="RESTAPI_OrgMove_ClientSetSetup"
xml1[${n}]="RESTAPI_OrgMove_ClientSetVerify_A1A2"
xml2[${n}]="RESTAPI_OrgMove_ClientSetVerify_A3A4"
xmlStay[${n}]="RESTAPI_OrgStay_ClientSetStream"
xmlExist[${n}]="RESTAPI_OrgExist_ClientSetStream"

xmlSync[${n}]="RESTAPI_Org_ClientSetSyncPoints"

timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} -  ClientSet - Export(A1_A2 Cluster) and Import(A3_A4 Cluster) Durable Subscriptions of MQTT clients "
component[1]=sync,m1
# OrgMove SETUP
component[2]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe0
component[3]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe1
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe2
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
   ###component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish0
component[8]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish2
component[9]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish3
component[10]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish4
   ###component[11]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive0
component[12]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[13]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
# OrgStay SETUP
component[14]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe0
component[15]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe1
component[16]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe2
component[17]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe3
component[18]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe4
component[19]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish0
component[20]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish1
component[21]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish2
component[22]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish3
component[23]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish4
# OrgExist SETUP
component[26]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe0
component[27]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe1
component[28]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe2
component[29]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe3
component[30]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe4
component[31]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish0
component[32]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish1
component[33]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish2
component[34]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish3
component[35]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish4
#

# Export(A1) here...
component[40]=sleep,2
component[41]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Export_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[42]=cAppDriverWait,m1,"-e|../restapi/scp_file.sh","-o|A1|/var/messagesight/data/export/*|A3|/mnt/A3/messagesight/data/import/"
component[43]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,export
# Setup needs to be BEFORE component[2]
component[45]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|CM1CS|-c|../restapi/test/restapi.cli|-a|1"
component[46]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|CM2CS|-c|../restapi/test/restapi.cli|-a|2"
component[47]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|CM3CS|-c|../restapi/test/restapi.cli|-a|3"
component[48]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|CM4CS|-c|../restapi/test/restapi.cli|-a|4"
#
# Recv (A1)
# OrgMove Post Export
#component[50]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive0
component[51]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive1
component[52]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive2
component[53]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive3
component[54]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive4
#component[55]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish0
component[56]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish1
component[57]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish2
component[58]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish3
component[59]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish4
#
# Import(A3)
component[60]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Import_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[61]=sleep,4
component[62]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,import
#  CHANGED to be part of SETUP to be sure the DIR is clean before running tests (Pre Delete Export/Import)
component[63]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[64]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
#
# Recv(A3)
# OrgMove POST IMPORT
#component[70]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive0
component[71]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive1
component[72]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive2
component[73]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive3
component[74]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive4
#component[75]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish0
component[76]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish1
component[77]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish2
component[78]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish3
component[79]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish4
#
# Delete ClientSet (A1)
component[80]=wsDriverWait,m1,client_set/${xmlSync[${n}]}.xml,delete
component[81]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteImEx_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"

# Restart Servers, verify nothing new is in store A1_A2, A3_A4 has new info
# Recv(A1), should be nothing there
# Delete ClientSet ALL (A1 and A3)

#FULL ON
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     ${component[40]} ${component[41]} ${component[42]} ${component[43]}     ${component[51]} ${component[52]} ${component[53]} ${component[54]}        ${component[56]} ${component[57]} ${component[58]} ${component[59]}         ${component[60]} ${component[61]} ${component[62]}        ${component[71]} ${component[72]} ${component[73]} ${component[74]}   ${component[76]} ${component[77]} ${component[78]} ${component[79]} ${component[80]} ${component[81]}  "

# Setup Only
components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  "
#${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}    ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     "

((n+=1))


#----------------------------------------------------
# Scenario 4 - Cleanup Client Set #2 Cluster
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp2"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - ClientSet Delete ALL after Clustered Import Export "
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
#component[2]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A2_IPv4_1}:${A2_PORT}"
#component[3]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A3_IPv4_1}:${A3_PORT}"
#component[4]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A4_IPv4_1}:${A4_PORT}"
#
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|4"
#
component[5]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[6]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A2|/var/messagesight/data/export/*"
component[7]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
component[8]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A4|/mnt/A4/messagesight/data/import/*"

# Setup Only
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}  "

((n+=1))



#----------------------------------------------------
# Scenario 5 - HA Client Set Org Move
#----------------------------------------------------
# A1_A2 = HA Pair 1 Members
# A3_A4 = HA Pair 2 Members
xml[${n}]="RESTAPI_OrgMove_ClientSetSetup"
xml1[${n}]="RESTAPI_OrgMove_ClientSetVerify_A1A2"
xml2[${n}]="RESTAPI_OrgMove_ClientSetVerify_A3A4"
xmlStay[${n}]="RESTAPI_OrgStay_ClientSetStream"
xmlExist[${n}]="RESTAPI_OrgExist_ClientSetStream"

xmlSync[${n}]="RESTAPI_Org_ClientSetSyncPoints"

timeouts[${n}]=900
scenario[${n}]="${xml[${n}]} -  ClientSet - Export(A1_A2 HA) and Import(A3_A4 HA) Durable Subscriptions of MQTT clients "
component[1]=sync,m1
# OrgMove SETUP
component[2]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe0
component[3]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe1
component[4]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe2
component[5]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe3
component[6]=wsDriver,m2,client_set/${xml[${n}]}.xml,subscribe4
   ###component[7]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish0
component[8]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish2
component[9]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish3
component[10]=wsDriver,m1,client_set/${xml[${n}]}.xml,publish4
   ###component[11]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive0
component[12]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive3
component[13]=wsDriver,m2,client_set/${xml[${n}]}.xml,receive4
# OrgStay SETUP
component[14]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe0
component[15]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe1
component[16]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe2
component[17]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe3
component[18]=wsDriver,m2,client_set/${xmlStay[${n}]}.xml,subscribe4
component[19]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish0
component[20]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish1
component[21]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish2
component[22]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish3
component[23]=wsDriver,m1,client_set/${xmlStay[${n}]}.xml,publish4
# OrgExist SETUP
component[26]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe0
component[27]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe1
component[28]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe2
component[29]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe3
component[30]=wsDriver,m2,client_set/${xmlExist[${n}]}.xml,subscribe4
component[31]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish0
component[32]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish1
component[33]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish2
component[34]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish3
component[35]=wsDriver,m1,client_set/${xmlExist[${n}]}.xml,publish4
#

# Export(A1) here...
component[40]=sleep,2
component[41]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Export_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[42]=cAppDriverWait,m1,"-e|../restapi/scp_file.sh","-o|A1|/var/messagesight/data/export/*|A3|/mnt/A3/messagesight/data/import/"
component[43]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,export
# Setup needs to be BEFORE component[2]
component[45]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|HA1CS|-c|../restapi/test/restapi.cli|-a|1"
component[46]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|HA2CS|-c|../restapi/test/restapi.cli|-a|2"
component[47]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|HA3CS|-c|../restapi/test/restapi.cli|-a|3"
component[48]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|HA4CS|-c|../restapi/test/restapi.cli|-a|4"
#
# Recv (A1)
# OrgMove Post Export
#component[50]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive0
component[51]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive1
component[52]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive2
component[53]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive3
component[54]=wsDriver,m2,client_set/${xml1[${n}]}.xml,receive4
#component[55]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish0
component[56]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish1
component[57]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish2
component[58]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish3
component[59]=wsDriver,m1,client_set/${xml1[${n}]}.xml,publish4
#
# Import(A3)
component[60]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-Import_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
component[61]=sleep,4
component[62]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,import
#  CHANGED to be part of SETUP to be sure the DIR is clean before running tests (Pre Delete Export/Import)
component[63]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[64]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
#
# Recv(A3)
# OrgMove POST IMPORT
#component[70]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive0
component[71]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive1
component[72]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive2
component[73]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive3
component[74]=wsDriver,m2,client_set/${xml2[${n}]}.xml,receive4
#component[75]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish0
component[76]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish1
component[77]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish2
component[78]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish3
component[79]=wsDriver,m1,client_set/${xml2[${n}]}.xml,publish4
#
# Delete ClientSet (A1)
component[80]=wsDriver,m1,client_set/${xmlSync[${n}]}.xml,delete
component[81]=cAppDriverRConlyChkWait,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteImEx_BasicClientSet.js","-s|IMA_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"

# Failover A1 verify nothing is resurrected, Failover A3 verify new data is persisted
# Recv(A1), should be nothing there
# Delete ClientSet ALL (A1 and A3)

#FULL ON
#components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}  ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     ${component[40]} ${component[41]} ${component[42]} ${component[43]}     ${component[51]} ${component[52]} ${component[53]} ${component[54]}        ${component[56]} ${component[57]} ${component[58]} ${component[59]}         ${component[60]} ${component[61]} ${component[62]}        ${component[71]} ${component[72]} ${component[73]} ${component[74]}   ${component[76]} ${component[77]} ${component[78]} ${component[79]} ${component[80]} ${component[81]}   "

# Setup Only
components[${n}]="${component[1]} ${component[64]} ${component[63]} ${component[45]} ${component[46]} ${component[47]} ${component[48]}  "
#${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}    ${component[8]} ${component[9]} ${component[10]}    ${component[12]} ${component[13]} ${component[14]} ${component[15]} ${component[16]} ${component[17]} ${component[18]} ${component[19]}   ${component[20]}  ${component[21]}  ${component[22]}  ${component[23]} ${component[24]} ${component[25]} ${component[26]} ${component[27]} ${component[28]} ${component[29]} ${component[30]} ${component[31]} ${component[32]} ${component[33]} ${component[34]} ${component[35]}     "

#((n+=1))


#----------------------------------------------------
# Scenario 6 - Cleanup Client Set #3 (HA)
#----------------------------------------------------
# A1 = ClusterMember1
# A2 = ClusterMember2
xml[${n}]="restapi-ClientSetCleanUp3"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} -  ClientSet Delete ALL after HA Import Export "
#component[1]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A1_IPv4_1}:${A1_PORT}"
#component[2]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A2_IPv4_1}:${A2_PORT}"
#component[3]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A3_IPv4_1}:${A3_PORT}"
#component[4]=cAppDriverRConlyChk,m1,"-e|mocha","-o|../restapi/test/clientset/restapi-DeleteAllClientSet.js","-s|THIS_AdminEndpoint=${A4_IPv4_1}:${A4_PORT}"
#
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|3"
component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|../restapi/test/restapi.cli|-a|4"
#
component[5]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A1|/var/messagesight/data/export/*"
component[6]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A2|/var/messagesight/data/export/*"
component[7]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A3|/mnt/A3/messagesight/data/import/*"
component[8]=cAppDriver,m1,"-e|../restapi/rm_file.sh","-o|A4|/mnt/A4/messagesight/data/import/*"

# Setup Only
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}  "

#((n+=1))



#----------------------------------------------------
# Teardown Scenario  - Reset ClusterMembership Defaults
#----------------------------------------------------
#xml[${n}]="clusterCs_config_cleanup"
#timeouts[${n}]=30
#scenario[${n}]="${xml[${n}]} - Reset ClusterMembership configuration in DeleteClientSet tests"
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|cluster_manipulation/cluster_manipulation.cli|-a|1"
#component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|cluster_manipulation/cluster_manipulation.cli|-a|2"
#
#components[${n}]="${component[1]} ${component[2]} "
#((n+=1))

