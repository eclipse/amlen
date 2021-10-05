#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the MQTT test driver
#    driven automated tests.
#		03/05/2012 - LAW - Initial Creation
#       04/25/2012 - MLC - Updated to MQTT test driver
#
#----------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="MQTT LTPA via WSTestDriver"

typeset -i n=0
typeset -i nnIBM=0
typeset -i nn16=0

java -version 1>out.log 2>&1
cat out.log
nnIBM=$(grep IBM out.log | wc -l)
nn16=$(grep "1\.6\.0" out.log | wc -l)
nn18=$(grep "1\.8\.0" out.log | wc -l)
echo "nnIBM" $nnIBM >> out.log
echo "nn16" $nn16 >> out.log
echo "nn18" $nn18 >> out.log
IBM16="No"
 if [[ $nnIBM -gt 0 ]] ; then
   TLS12="Yes"
   if [[ $nn16 -gt 0 ]] ; then
     IBM16="Yes"
   else
     IBM16="No"
   fi
 else
   if [[ $nn16 -gt 0 ]] ; then
     TLS12="No"
   elif [[ $nn18 -gt 0 ]] ; then
     TLS12="No"
   else
     TLS12="Yes"
   fi
 fi
echo "TLS12" $TLS12 >> out.log


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
# Test Case 0 - Configure for LTPA
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="setupLTPA"
scenario[${n}]="${xml[${n}]} - Configure for LTPA"
timeouts[${n}]=80
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupLTPA|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverRestart.sh"
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

#----------------------------------------------------
# Test Case 1 - LTPA Connect with user authorization
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa01"
scenario[${n}]="${xml[${n}]} - LTPA Connect with user authorization"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 2 - LTPA Connect with group authorization
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa02"
scenario[${n}]="${xml[${n}]} - LTPA Connect with group authorization"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 3 - LTPA Connect with indirect group authorization
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa03"
scenario[${n}]="${xml[${n}]} - LTPA Connect with indirect group authorization"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case x - Update UserIdMap
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="updateLTPA"
scenario[${n}]="${xml[${n}]} - Update UserIdMap"
timeouts[${n}]=80
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|updateLTPA|-c|policy_config.cli"
#component[2]=cAppDriver,m1,"-e|../common/serverRestart.sh"
#components[${n}]="${component[1]} ${component[2]} "
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case x - LTPA Connect with mail UserIdMap
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpaMail"
scenario[${n}]="${xml[${n}]} - LTPA Connect mail UserIdMap"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case x - LTPA Connect Fail with mail UserIdMap
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpaMail02"
scenario[${n}]="${xml[${n}]} - LTPA Connect Fail mail UserIdMap"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case x - Reset UserIdMap
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="resetLTPA"
scenario[${n}]="${xml[${n}]} - Reset UserIdMap"
timeouts[${n}]=80
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resetLTPAuserIDMap|-c|policy_config.cli"
#component[2]=cAppDriver,m1,"-e|../common/serverRestart.sh"
#components[${n}]="${component[1]} ${component[2]} "
components[${n}]="${component[1]}"

((n+=1))

#----------------------------------------------------
# Test Case 4 - LTPA Connect with indirect group authorization
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa04"
scenario[${n}]="${xml[${n}]} - LTPA Connect with modified/bad ltpa token"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

# This test does not successfully run on virtual machines and all of our automation machines are virtual
#if [ ${FULLRUN} = "TRUE" ]
#then
##----------------------------------------------------
## Test Case 5 - test that token timeout terminates connection
##----------------------------------------------------
## The prefix of the XML configuration file for the driver
#xml[${n}]="testmqtt_ltpa05"
#scenario[${n}]="${xml[${n}]} - LTPA token timeout disconects client"
#timeouts[${n}]=760
## Set up the components for the test in the order they should start
#component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace|-c|policy_config.cli"
#component[2]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
#components[${n}]="${component[1]} ${component[2]} "
#((n+=1))
#fi

#----------------------------------------------------
# Test Case 6 - LTPA Connect with user authorization and TLSEnabled=False
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa01_notls"
scenario[${n}]="${xml[${n}]} - LTPA Connect with user authorization"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 7 - LTPA Connect with indirect group authorization
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa04_notls"
scenario[${n}]="${xml[${n}]} - LTPA Connect with modified/bad ltpa token and TLSEnable=False"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 8 - turn FIPS on and repeat test 1
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="FIPSon"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Reconfigure for FIPS"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|FIPSon|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../common/serverRestart.sh"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
if [ ${nn18} -eq 0 ]
then
# Test Case 9 - repeat test 1 with FIPS
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="testmqtt_ltpa01ff"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - LTPA Connect with user authorization and FIPS fails if we don't specify TLSv1.2"
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
fi

if [ ${TLS12} = "Yes" ]
then
#----------------------------------------------------
# Test Case 9a - repeat test 1 with FIPS
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="testmqtt_ltpa01fa"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - LTPA Connect with user authorization and FIPS, specify enabledCipherSuites"
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

if [ ${IBM16} = "No" ]
then
#----------------------------------------------------
# Test Case 9b - repeat test 1 with FIPS
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.

xml[${n}]="testmqtt_ltpa01f"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - LTPA Connect with user authorization and FIPS"
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 10 - repeat test 2 with FIPS
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa02f"
scenario[${n}]="${xml[${n}]} - LTPA Connect with group authorization and FIPS"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 11 - repeat test 3 with FIPS
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
xml[${n}]="testmqtt_ltpa03f"
scenario[${n}]="${xml[${n}]} - LTPA Connect with indirect group authorization and FIPS"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=wsDriver,m1,ltpa/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]} "

((n+=1))
fi
fi
#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="policy_cleanup"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Cleanup for mqtt ltpa test bucket "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|resettrace|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|FIPSoff|-c|policy_config.cli"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupLTPA|-c|policy_config.cli"
component[4]=cAppDriver,m1,"-e|../common/serverRestart.sh"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
