#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="JMS SSL - 00"

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
 
# Replace M1_IPv6_1 with correct IP. When client has multiple IP address this is how we find the IP which will be use to send message to A1
echo "Before: M1_IPv6_1=${M1_IPv6_1}" | tee -a ${logfile}
# Replace M1_IPv6_1 with correct IP. When client has multiple IP address this is how we find the IP which will be use to send message to A1
REAL_M1_IPv6_1=`ping6 ${A1_IPv6_1} -I eth1 -c 1 | grep "PING" | awk -F' ' '{ print $4 }'`
echo "REAL_M1_IPv6_1=${REAL_M1_IPv6_1}" | tee -a ${logfile}
if [[ "$REAL_M1_IPv6_1" =~ .*:.* ]] ; then
   export M1_IPv6_1=${REAL_M1_IPv6_1}
fi   
echo "After: M1_IPv6_1=${M1_IPv6_1}" | tee -a ${logfile}


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
# Scenario 1 - jms_ssl_001
#----------------------------------------------------
xml[${n}]="jms_ssl_001"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test ability to create and send messages over an SSL connection"
component[1]=jmsDriver,m1,ssl/jms_ssl_001.xml,Component1
component[2]=jmsDriver,m1,ssl/jms_ssl_001.xml,Component2
component[3]=jmsDriver,m1,ssl/jms_ssl_001.xml,Component3
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jms_ssl_002
#----------------------------------------------------
xml[${n}]="jms_ssl_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test invalid usernames and passwords"
component[1]=jmsDriver,m1,ssl/jms_ssl_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Non-TLS version of Scenario 2 - jms_tlsDisabled_002
#----------------------------------------------------
xml[${n}]="jms_ssl_tlsDisabled_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test invalid usernames and passwords with a non-TLS security profile"
component[1]=jmsDriver,m1,ssl/jms_tlsDisabled_002.xml,ALL
components[${n}]="${component[1]}"
((n+=1))


if [[ "$A1_IPv6_1" =~ ":" ]] ; then

#----------------------------------------------------
# Scenario 3 - jms_ssl_003
# Skip on systems that don't support IPv6
#----------------------------------------------------
xml[${n}]="jms_ssl_003"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test messaging policies (IPv4)"
component[1]=jmsDriver,m1,ssl/jms_ssl_003.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

fi 

#----------------------------------------------------
# Scenario 4 - jms_ssl_004
#----------------------------------------------------
xml[${n}]="jms_ssl_004"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test messaging policies (IPv6)"
component[1]=jmsDriver,m1,ssl/jms_ssl_004.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - jms_ssl_005
#----------------------------------------------------
xml[${n}]="jms_ssl_005"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test complex messaging policy"
component[1]=jmsDriver,m1,ssl/jms_ssl_005.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - jms_ssl_006
#----------------------------------------------------
xml[${n}]="jms_ssl_006"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test invalid keystores and MQTT only endpoint"
component[1]=jmsDriver,m1,ssl/jms_ssl_006.xml,rmdr1
component[2]=jmsDriver,m1,ssl/jms_ssl_006.xml,rmdr2
component[3]=jmsDriver,m1,ssl/jms_ssl_006.xml,rmdr3
component[4]=jmsDriver,m1,ssl/jms_ssl_006.xml,rmdr4
component[5]=jmsDriver,m1,ssl/jms_ssl_006.xml,rmdr5
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - jms_ssl_007
#----------------------------------------------------
xml[${n}]="jms_ssl_007"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test Connection Policy with User"
component[1]=jmsDriver,m1,ssl/jms_ssl_007.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - jms_ssl_008
#----------------------------------------------------
xml[${n}]="jms_ssl_008"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test Policies without SSL"
component[1]=jmsDriver,m1,ssl/jms_ssl_008.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - jms_ssl_009
#----------------------------------------------------
xml[${n}]="jms_ssl_009"
timeouts[${n}]=40
scenario[${n}]="${xml[${n}]} - Test messaging policies with multiple clients"
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_009.xml,rmdr
component[3]=jmsDriver,m2,ssl/jms_ssl_009.xml,rmdr2
component[4]=jmsDriver,m1,ssl/jms_ssl_009.xml,rmdt
component[5]=jmsDriver,m2,ssl/jms_ssl_009.xml,rmdt2
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - jms_ssl_010
#----------------------------------------------------
xml[${n}]="jms_ssl_010"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test wildcard topics against messaging policies"
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_010.xml,rmdr
component[3]=jmsDriver,m2,ssl/jms_ssl_010.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - jms_ssl_011
#----------------------------------------------------
xml[${n}]="jms_ssl_011"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Variable Replacement in Messaging Policies (separate)"
component[1]=jmsDriver,m1,ssl/jms_ssl_011.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - jms_ssl_012
#----------------------------------------------------
xml[${n}]="jms_ssl_012"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Variable Replacement in Messaging Policies (combined)"
component[1]=jmsDriver,m1,ssl/jms_ssl_012.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - jms_ssl_013
#----------------------------------------------------
xml[${n}]="jms_ssl_013"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - UsePasswordAuthentication in SecurityProfile"
component[1]=jmsDriver,m1,ssl/jms_ssl_013.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - jms_ssl_014
#----------------------------------------------------
xml[${n}]="jms_ssl_014"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - UseClientCertificate in SecurityProfile"
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_014.xml,rmdr
component[3]=jmsDriver,m1,ssl/jms_ssl_014.xml,rmdr2
component[4]=jmsDriver,m1,ssl/jms_ssl_014.xml,rmdr3
component[5]=jmsDriver,m1,ssl/jms_ssl_014.xml,rmdt
components[${n}]="${component[1]} ${component[2]}  ${component[3]}  ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 15 - jms_ssl_015
#----------------------------------------------------
xml[${n}]="jms_ssl_015"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Tests for ClientAddress - positive and negative"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupM2|-c|restpolicy_config.cli"
component[2]=sync,m1
component[3]=jmsDriver,m1,ssl/jms_ssl_015.xml,consumer
component[4]=jmsDriver,m2,ssl/jms_ssl_015.xml,producer
component[5]=jmsDriver,m1,ssl/jms_ssl_015.xml,invalidIPClient
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

#----------------------------------------------------
# Scenario 16 - jms_ssl_016
#----------------------------------------------------
xml[${n}]="jms_ssl_016"
timeouts[${n}]=15
scenario[${n}]="${xml[${n}]} - Test update command on ConnectionPolicy (ClientID, UserID)"
component[1]=jmsDriver,m1,ssl/jms_ssl_016.xml,rmdx
components[${n}]="${component[1]}"
((n+=1))

# ----------------------------------------------------
# Scenario 17 - jms_ssl_017
# ----------------------------------------------------
xml[${n}]="jms_ssl_017"
timeouts[${n}]=15
scenario[${n}]="${xml[${n}]} - Test update command on ConnectionPolicy (GroupID)"
component[1]=jmsDriver,m1,ssl/jms_ssl_017.xml,rmdx
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 18 - jms_ssl_018
#----------------------------------------------------
xml[${n}]="jms_ssl_018"
timeouts[${n}]=15
scenario[${n}]="${xml[${n}]} - Test update command on ConnectionPolicy for nested grorups (GroupID)"
component[1]=jmsDriver,m1,ssl/jms_ssl_018.xml,rmdx
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_IPv6_1" =~ ":" ]] ; then

#----------------------------------------------------
# Scenario 21 - jms_ssl_021
# skip on systems that don't support IPv6
#----------------------------------------------------
xml[${n}]="jms_ssl_021"
timeouts[${n}]=15
scenario[${n}]="${xml[${n}]} - Test Client Address Ranges in ConnectionPolicy (IPv6) "
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_021.xml,su,,"-s|M1_IPv6_1=${M1_IPv6_1}"
component[3]=jmsDriver,m1,ssl/jms_ssl_021.xml,x,,"-s|M1_IPv6_1=${M1_IPv6_1}"
component[4]=jmsDriver,m1,ssl/jms_ssl_021.xml,td,,"-s|M1_IPv6_1=${M1_IPv6_1}"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

# ----------------------------------------------------
# Scenario 22 - jms_ssl_022
# skip on systems that don't support IPv6
# ----------------------------------------------------
xml[${n}]="jms_ssl_022"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test update on MessagingPolicy command (IPv6) "
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_022.xml,su,,"-s|M1_IPv6_1=${M1_IPv6_1}"
component[3]=jmsDriver,m1,ssl/jms_ssl_022.xml,x,,"-s|M1_IPv6_1=${M1_IPv6_1}"
component[4]=jmsDriver,m1,ssl/jms_ssl_022.xml,td,,"-s|M1_IPv6_1=${M1_IPv6_1}"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

fi 

#----------------------------------------------------
# Scenario 23 - jms_ssl_023
#----------------------------------------------------
# Run the next 2 tests after main policy cleanup,
# because they reuse the same certificates
xml[${n}]="jms_ssl_023"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test creation, update and delete of certificate profiles"
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_023.xml,setup
component[3]=jmsDriver,m1,ssl/jms_ssl_023.xml,prodcons
component[4]=jmsDriver,m1,ssl/jms_ssl_023.xml,teardown
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

#----------------------------------------------------
# Scenario 24 - jms_ssl_024
#----------------------------------------------------
xml[${n}]="jms_ssl_024"
timeouts[${n}]=100
scenario[${n}]="${xml[${n}]} - Test creation, update and delete of security profiles"
component[1]=sync,m1
component[2]=jmsDriver,m1,ssl/jms_ssl_024.xml,setup
component[3]=jmsDriver,m1,ssl/jms_ssl_024.xml,prodcons1
component[4]=jmsDriver,m1,ssl/jms_ssl_024.xml,prodcons2
component[5]=jmsDriver,m1,ssl/jms_ssl_024.xml,prodcons3
component[6]=jmsDriver,m1,ssl/jms_ssl_024.xml,prodcons4
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

#----------------------------------------------------
# Test Case FIPS=True - turn FIPS on for some testing. 
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="jms_ssl_FIPSon"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Reconfigure for FIPS=True testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|FIPSon|-c|restpolicy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverStopRestart.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))



#----------------------------------------------------
# Test Case FIPS=True - turn FIPS on for some testing.
# Different tests are used for JVM's that support TLSv1.2 
# and JVM's that don't support TLSv1.2 
#----------------------------------------------------

if [ ${TLS12} = "Yes" ]
then

xml[${n}]="jms_ssl_FIPSTrue_001.xml"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test with FIPS=True, various Cipher suites and minimum protocols, using Java's that support TLSv1.2"
component[1]=jmsDriver,m1,ssl/jms_FIPSTrue_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

else
xml[${n}]="jms_ssl_FIPSTrue_oldJVM_001.xml"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test with FIPS=True, various Cipher suites and minimum protocols, using Java's that support TLSv1.2"
component[1]=jmsDriver,m1,ssl/jms_FIPSTrue_oldJVM_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

fi


#----------------------------------------------------
# Test Case FIPS=False - turn FIPS off 
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="jms_ssl_FIPSoff"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Turn FIPS off"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|FIPSoff|-c|restpolicy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverStopRestart.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case jms_SimpleFIPSFalse_001.xml
# JVM must support TLSv1.2 for this test.
# This test prints all the debug information for 
# handshaking, and ciphers. It was useful for 
# debugging on different JVM's. The variation used
# is in jms_FIPSTrue_001, and jms_FIPSFalse_001.xml, 
# (connection factory: cf2) 
# This only needs to be turned back on if we have a 
# problem and need a simpler testcase to see what
# is happening. Note the BITFLAG is set on this 
# test. 
#----------------------------------------------------

#xml[${n}]="jms_ssl_SimpleFIPSFalse_001.xml"
#timeouts[${n}]=20
#scenario[${n}]="${xml[${n}]} - Simple Test with FIPS=False, various Cipher suites and minimum protocols, using Java's that support TLSv1.2"
#component[1]=jmsDriver,m1,ssl/jms_SimpleFIPSFalse_001.xml,ALL,,"-s|BITFLAG=-Djavax.net.debug=all"
#components[${n}]="${component[1]}"
#((n+=1))


#----------------------------------------------------
# Test Case FIPS=True - turn FIPS on for some testing. 
# JVM must support TLSv1.2 for this test.
#----------------------------------------------------
if [ ${TLS12} = "Yes" ]
then

xml[${n}]="jms_ssl_FIPSFalse_001.xml"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - Test with FIPS=False, various Cipher suites and minimum protocols, using Java's that support TLSv1.2"
component[1]=jmsDriver,m1,ssl/jms_FIPSFalse_001.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

fi

#----------------------------------------------------
# Scenario 99 - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="jms_ssl_policy_cleanup"
timeouts[${n}]=190
scenario[${n}]="${xml[${n}]} - Cleanup in JMS SSL test bucket (internal LDAP)"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 99 - policy_cleanup M2
#----------------------------------------------------
# Clean up objects created for IP range test cases
# that use M2 variables, since BVT has no M2 defined.
# Note that the user TestUser is used by M2 but added and delteted in the setupuser/cleanupuser sections
xml[${n}]="jms_ssl_policy_cleanupM2"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Cleanup M2 specific admin objects"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupM2|-c|restpolicy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1  
    #----------------------------------------------------
    xml[${n}]="jms_ssl_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
