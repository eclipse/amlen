#!/bin/bash
#
# Description:
#   This script executes ISM test bucket for:  [TODO!: fill in blank here and rename this file].
#
# Input Parameters:
#   $1  -  optional: a log file to which this script will direct its output.
#          If this option is not given, the execution result will be logged in
#          the default log file:  <thisFileName>.log
#
# All files required for this script should be copied from Build Server into this arrangement:
#  ISM Test Environment:  ../scripts, ../common, ../[your-testcase-dir]
#
# Environment setup required by the test cases launched by this script:
# 1. Modify ../scripts/ISMsetup.sh to match with your test machines setup.
#
# 2. Edit the TODO!: sections in this file to set execution directives
#    b.) Need to allow more than default time between starting the components in run-scenarios
#    c.) Provide your run-scenarios target file(s)
#
# 3. Scripts and binaries in ../scripts, ../common, ../[your-testcase-dir] directories 
#    must have at least permissions '755'.
#
# Execution:
# 1. Go to the ../[your-testcase-dir] where the test case files reside,
#    then launch this script.
#
# Criteria for passing the test:
# 1. The actions being executed in all the threads/processes launched by 
#    runScenarios.sh target scripts are executed successfully.
#
# Return Code:
#  0 - Tests were executed.
#  1 - Tests could not be executed.
#
#-------------------------------------------------------------------------------
# set -x   # Enables tracing of script commands

curDir=$(dirname ${0})
curFile=$(basename ${0})
#-------------------------------------------------------------------#
# Input parameter validation                                        #
#-------------------------------------------------------------------#
err=0

## Default log file name
logfile=`echo ${curFile} | cut -d '.' -f 1`.log

if [[ $# -gt 1 ]] ; then
  err=1
elif [[ $# -eq 1 ]] ; then
  ## Append test result to the user specified log file
  logfile=$1
fi

rm -f ${logfile}

if [[ ${err} -eq 1 ]] ; then
   echo 
   echo Error in Input: ${0} $@ | tee -a ${logfile}
   echo "Usage:  " ${0} "  [logfile]" | tee -a ${logfile}
   echo
   echo "  where:" | tee -a ${logfile}
   echo
   echo "  logfile:  where to redirect the execution result to. By default, it goes to ${0}.log " | tee -a ${logfile}
   echo
   exit 1
fi

# 'Source' the Automated Env. Setup file, testEnv.sh,  if it exists 
#  otherwise a User's Manual run is assumed and ISmsetup file provides machine env. information.
if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement 
  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi  

source ../scripts/commonFunctions.sh

bash ../jms_td_tests/HA/updateAllowSingleNIC.sh
bash create_xml_includes.sh

# create a bunch of xml copies because getting unique filenames for the same xml is no fun
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_001a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_001b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_002a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_002b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_003a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_003b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_004a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_004b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_005a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_005b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_006a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_006b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_007a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_007b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_008a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_008b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_009a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_009b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_010a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_010b.xml
cp common_topic_tree/testmqtt_clusterCTT_001.xml cluster_cli/testmqtt_clusterCLI_011a.xml
cp common_topic_tree/testmqtt_clusterCTT_002.xml cluster_cli/testmqtt_clusterCLI_011b.xml

# Test Buckets that use the run-scenarios.sh framework:
# TODO!:  If you would like to save the logs regardless of the test bucket's outcome, setSavePassedLogs to 'on', default: off
# TODO!:  If you would like to control the time between every component start in your run-scenarios.sh target script,
#         setComponentSleep to 'the number of seconds to wait'.  default: 3 seconds.   
#  If you need a variable amount of time between component starts, in your run-scenarios.sh target script specify a component[#]=sleep,<number-of-seconds> .
setSavePassedLogs off ${logfile}
setComponentSleep 0 ${logfile}
totalResults=0

if ! [[ "${A1_HOST_OS}" =~ "EC2" ]] && ! [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  #------------------------------------------------------------------------------
  # Setup cluster
  #------------------------------------------------------------------------------
  RUNCMD="../scripts/run-scenarios.sh  retained_messages/ism-clusterRM_Setup.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Cluster Retained Messages Tests
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  retained_messages/ism-Cluster_RetainedMessages.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Tear down cluster
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  retained_messages/ism-clusterRM_Teardown.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
fi

#------------------------------------------------------------------------------
# Setup cluster
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  flow_control/ism-cluster_SetupFC.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )


#------------------------------------------------------------------------------
# Cluster Flow Control Tests
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  flow_control/ism-Cluster-FlowControl-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Tear down cluster
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  flow_control/ism-cluster_TeardownFC.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )


#------------------------------------------------------------------------------
# Setup cluster
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  cluster_manipulation/ism-cluster_SetupCM.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Cluster Join Leave tests.
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  cluster_manipulation/ism-Cluster-Manipulation-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Tear down cluster
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  cluster_manipulation/ism-cluster_TeardownCM.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Setup cluster
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  reliable/ism-cluster_SetupR.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Cluster Reliable Messaging Tests
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  reliable/ism-Cluster-Reliable-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Teardown cluster
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  reliable/ism-cluster_TeardownR.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Setup cluster Admin Remove Member
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  remove_member/ism-cluster_SetupAdmRm.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Cluster Admin Remove Member Tests
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  remove_member/ism-Cluster-AdmRm-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )


#------------------------------------------------------------------------------
# Tear down cluster - teardown included in the test itself to avoid test failures due to ending up in maintenance mode
#------------------------------------------------------------------------------
#
#RUNCMD="../scripts/run-scenarios.sh  remove_member/ism-cluster_TeardownAdmRm.sh ${logfile}"
#echo Running command: ${RUNCMD}
#$RUNCMD
#totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Setup cluster HA for failover tests
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  cluster_HA/ism-cluster_SetupClusterHA.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#enable when defects resolve (3-10-16)
#------------------------------------------------------------------------------
# Cluster HA failover tests
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  cluster_HA/ism-Cluster_HA-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Tear down cluster HA
#------------------------------------------------------------------------------

RUNCMD="../scripts/run-scenarios.sh  cluster_HA/ism-cluster_TeardownClusterHA.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if ! [[ "${A1_HOST_OS}" =~ "EC2" ]] && ! [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  #------------------------------------------------------------------------------
  # Cluster CLI Tests
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  cluster_cli/ism-Cluster-CLI-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Teardown cluster
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  cluster_cli/ism-cluster_TeardownCLI.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
fi

rm -rf cluster_cli/testmqtt*a.xml
rm -rf cluster_cli/testmqtt*b.xml

if ! [[ "${A1_HOST_OS}" =~ "EC2" ]] && ! [[ "${A1_HOST_OS}" =~ "MSA" ]] ; then
  #------------------------------------------------------------------------------
  # Setup cluster
  #------------------------------------------------------------------------------
  RUNCMD="../scripts/run-scenarios.sh  retained_messages_HA/ism-clusterRMHA_Setup.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Cluster Retained HA Messages Tests
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  retained_messages_HA/ism-Cluster_RetainedMessagesHA.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Tear down cluster
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  retained_messages_HA/ism-clusterRMHA_Teardown.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )

  #------------------------------------------------------------------------------
  # Cluster HA Part 1
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  basic_clusterHA/ism-Cluster-BasicHA-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  
  #------------------------------------------------------------------------------
  # Cluster HA Part 2
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  basic_clusterHA/ism-Cluster-BasicHA-01.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  
  #------------------------------------------------------------------------------
  # Cluster HA Part 3
  #------------------------------------------------------------------------------

  RUNCMD="../scripts/run-scenarios.sh  basic_clusterHA/ism-Cluster-BasicHA-02.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
fi


#------------------------------------------------------------------------------
# OrgMove ClientSet need HOSTNAMES in Env.
#------------------------------------------------------------------------------
source ../scripts/getA_Hostnames.sh

#------------------------------------------------------------------------------
# Run Basic Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
 RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_Basic.sh ${logfile}"
 echo Running command: ${RUNCMD}
 $RUNCMD
 totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Run Cluster Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
 RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_Cluster.sh ${logfile}"
 echo Running command: ${RUNCMD}
 $RUNCMD
 totalResults=$( expr ${totalResults} + $? )

#------------------------------------------------------------------------------
# Run HA Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_HA.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )




# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

# Clean up all the Paho persistence directories
../common/rmScratch.sh

exit ${totalResults}


