#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="ClusterSetup - Reliable Messaging - 00"

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
# Setup Scenario  - Configure ClusterMembership on the 3 servers. 
#----------------------------------------------------
xml[${n}]="clusterReliable_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Set minimal required parameters for configuring a Cluster of 2 for reliable channel tests "
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1|-c|reliable/reliable.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2|-c|reliable/reliable.cli|-a|2"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3|-c|reliable/reliable.cli|-a|3"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
	components[${n}]="${component[1]} ${component[2]} ${component[3]}"
else
  	component[4]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC1CM|-c|reliable/reliable.cli"
	component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC2CM|-c|reliable/reliable.cli|-a|2"
	component[6]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupC3CM|-c|reliable/reliable.cli|-a|3"
  	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
fi
((n+=1))

#----------------------------------------------------
# Scenario 1 - createCluster
#----------------------------------------------------
# A1 = Running
xml[${n}]="createClusterReliable"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - create cluster CLUSTERRELIABLE with 3 members"
if [[ "${A1_HOST_OS}" =~ "EC2" ]] || [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  # If we are running in EC2 we don't have multicast support so use discovery server list
  component[1]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|1|-n|${MQKEY}_ClusterReliable|-b|2|3|-v|2|-ca|${A1_IPv4_INTERNAL_1}|-ma|${A1_IPv4_INTERNAL_1}"
  component[2]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|2|-n|${MQKEY}_ClusterReliable|-b|1|3|-v|2|-ca|${A2_IPv4_INTERNAL_1}|-ma|${A2_IPv4_INTERNAL_1}"
  component[3]=cAppDriverLogWait,m1,"-e|../scripts/cluster.py,-o|-a|addClusterMember|-m|3|-n|${MQKEY}_ClusterReliable|-b|1|2|-v|2|-ca|${A3_IPv4_INTERNAL_1}|-ma|${A3_IPv4_INTERNAL_1}"
  components[${n}]="${component[1]} ${component[2]} ${component[3]}"
else
  component[1]=cAppDriverLog,m1,"-e|../scripts/cluster.py,-o|-a|createCluster|-n|${MQKEY}_ClusterReliable|-l|1|2|3|-v|2"
  components[${n}]="${component[1]}"
fi
((n+=1))

