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

# Test Buckets that use the run-scenarios.sh framework:
# TODO!:  If you would like to save the logs regardless of the test bucket's outcome, setSavePassedLogs to 'on', default: off
# TODO!:  If you would like to control the time between every component start in your run-scenarios.sh target script,
#         setComponentSleep to 'the number of seconds to wait'.  default: 3 seconds.   
#  If you need a variable amount of time between component starts, in your run-scenarios.sh target script specify a component[#]=sleep,<number-of-seconds> .
setSavePassedLogs off ${logfile}
setComponentSleep 0 ${logfile}
totalResults=0
echo ' ==============================================================================='
echo ' H E Y ! ! !   In Automation this is called by ismAutoCluster_of2.sh.        ==='
echo ' ===  U S E  T H I S  O N L Y  -  F o r  D e b u g  P u r p o s e s          ==='
echo ' ===  Cause Janet says so,  only want one script per bucket for some reason. ==='
echo ' ==============================================================================='
export A1_HOSTNAME_OS_SHORT=`ssh $A1_USER@$A1_HOST "hostname -s"`
export A1_HOSTNAME_OS=`ssh $A1_USER@$A1_HOST "hostname -f"`
echo "A1_HOSTNAME_OS is $A1_HOSTNAME_OS, while A1_HOSTNAME_OS_SHORT is $A1_HOSTNAME_OS_SHORT"
if [[ "$A_COUNT" >  "1" ]]; then
    export A2_HOSTNAME_OS_SHORT=`ssh $A2_USER@$A2_HOST "hostname -s"`
    export A2_HOSTNAME_OS=`ssh $A2_USER@$A2_HOST "hostname -f"`
    echo "A2_HOSTNAME_OS is $A2_HOSTNAME_OS, while A2_HOSTNAME_OS_SHORT is $A2_HOSTNAME_OS_SHORT"
fi
if [[ "$A_COUNT" >  "2" ]]; then
    export A3_HOSTNAME_OS_SHORT=`ssh $A3_USER@$A3_HOST "hostname -s"`
    export A3_HOSTNAME_OS=`ssh $A3_USER@$A3_HOST "hostname -f"`
    echo "A3_HOSTNAME_OS is $A3_HOSTNAME_OS, while A3_HOSTNAME_OS_SHORT is $A3_HOSTNAME_OS_SHORT"
fi
if [[ "$A_COUNT" >  "3" ]]; then
    export A4_HOSTNAME_OS_SHORT=`ssh $A4_USER@$A4_HOST "hostname -s"`
    export A4_HOSTNAME_OS=`ssh $A4_USER@$A4_HOST "hostname -f"`
    echo "A4_HOSTNAME_OS is $A4_HOSTNAME_OS, while A4_HOSTNAME_OS_SHORT is $A4_HOSTNAME_OS_SHORT"
fi

#------------------------------------------------------------------------------
# Setup cluster for Common topic tree tests.
# Save results for setup always, since might be important for other tests debug. 
#------------------------------------------------------------------------------
#setSavePassedLogs on ${logfile}
#RUNCMD="../scripts/run-scenarios.sh  ../common_topic_tree/ism-clusterCTT_Setup.sh ${logfile}"
#echo Running command: ${RUNCMD}
#$RUNCMD
#totalResults=$( expr ${totalResults} + $? )

#setSavePassedLogs off ${logfile}

#------------------------------------------------------------------------------
# Run Basic Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
#setSavePassedLogs on ${logfile}
RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_Basic.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
setSavePassedLogs off ${logfile}

#------------------------------------------------------------------------------
# Run Cluster Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
#setSavePassedLogs on ${logfile}
RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_Cluster.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
setSavePassedLogs off ${logfile}


#------------------------------------------------------------------------------
# Run HA Setup Import/Export ClientSet tests.
#------------------------------------------------------------------------------
### setSavePassedLogs on ${logfile}
### RUNCMD="../scripts/run-scenarios.sh  client_set/ism-OrgMoveClientSet-01_HA.sh ${logfile}"
### echo Running command: ${RUNCMD}
### $RUNCMD
### totalResults=$( expr ${totalResults} + $? )
### setSavePassedLogs off ${logfile}


#------------------------------------------------------------------------------
# Teardown cluster config for Common topic tree tests.
#------------------------------------------------------------------------------
#RUNCMD="../scripts/run-scenarios.sh  ../common_topic_tree/ism-clusterCTT_Teardown.sh ${logfile}"
#echo Running command: ${RUNCMD}
#$RUNCMD
#totalResults=$( expr ${totalResults} + $? )

# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

# Clean up all the Paho persistence directories
../common/rmScratch.sh

exit ${totalResults}


