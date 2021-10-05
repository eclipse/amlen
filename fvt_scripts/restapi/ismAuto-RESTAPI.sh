#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

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

## T0 allow XML files of TestDrivers to have variable replacement of NON_STANDARD Variables, 
##    must put the non-std vars into the appropriate source file for prepareTestEnvParameters.sh to build the SED cmds.

# 'Source' the Automated Env. Setup file, testEnv.sh,  if it exists 
#  otherwise a User's Manual run is assumed and ISmsetup file provides machine env. information.
if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  echo "export OrgMovePub=200" >> ../testEnv.sh
  echo "export OrgMoveSub=2000" >> ../testEnv.sh
  source ../testEnv.sh
  export ISM_AUTOMATION="TRUE"
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement 
  echo "export OrgMovePub=200" >> ../scripts/ISMsetup.sh
  echo "export OrgMoveSub=2000" >> ../scripts/ISMsetup.sh
  source ../scripts/ISMsetup.sh
  export ISM_AUTOMATION="FALSE"
fi  
echo "ISM_AUTOMATION is ${ISM_AUTOMATION}, where TRUE is Frank's automation environment, FALSE is private SVT execution."
../scripts/prepareTestEnvParameters.sh
# THE Above works in Local Test Env., however Frank has already called prepareReplaceCommon.sh. Try to call again.
../scripts/prepareReplaceCommon.sh

source ../scripts/commonFunctions.sh
export ISM_BUILDTYPE="production"
#source ../scripts/getImaVersion.sh
source ../scripts/getA_Hostnames.sh
source ../common/url.sh

#export A1_HOSTNAME_OS_SHORT=`ssh $A1_USER@$A1_HOST "hostname -s"`
#export A1_HOSTNAME_OS=`ssh $A1_USER@$A1_HOST "hostname -f"`
#echo "A1_HOSTNAME_OS is $A1_HOSTNAME_OS, while A1_HOSTNAME_OS_SHORT is $A1_HOSTNAME_OS_SHORT"
#if [[ "$A_COUNT" >  "1" ]]; then
#    export A2_HOSTNAME_OS_SHORT=`ssh $A2_USER@$A2_HOST "hostname -s"`
#    export A2_HOSTNAME_OS=`ssh $A2_USER@$A2_HOST "hostname -f"`
#    echo "A2_HOSTNAME_OS is $A2_HOSTNAME_OS, while A2_HOSTNAME_OS_SHORT is $A2_HOSTNAME_OS_SHORT"
#fi



# Test Buckets that use the run-scenarios.sh framework:
# TODO!:  If you would like to save the logs regardless of the test bucket's outcome, setSavePassedLogs to 'on', default: off
# TODO!:  If you would like to control the time between every component start in your run-scenarios.sh target script,
#         setComponentSleep to 'the number of seconds to wait'.  default: 3 seconds.   
#  If you need a variable amount of time between component starts, in your run-scenarios.sh target script specify a component[#]=sleep,<number-of-seconds> .
setSavePassedLogs off ${logfile}
setComponentSleep default ${logfile}
totalResults=0

# TODO!:  provide your run-scenarios target file(s), repeat as necessary, be sure to update the 'exit' command.
# Test Bucket Scenarios (group 1)
RUNCMD="../scripts/run-scenarios.sh  ism-RESTAPI-01.sh  ${logfile}"
echo Running command: ${RUNCMD}
#set -x
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
#set +x

## Test Bucket Scenarios (group 2)  Repeat as many groups necessary
logfile_2="${logfile}_2"
RUNCMD="../scripts/run-scenarios.sh  ism-RESTAPI-02.sh ${logfile_2}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )?

## Test Bucket Scenarios (group 3)  Repeat as many groups necessary
logfile_3="${logfile}_3"
RUNCMD="../scripts/run-scenarios.sh  ism-RESTAPI-03.sh ${logfile_3}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

exit ${totalResults}
