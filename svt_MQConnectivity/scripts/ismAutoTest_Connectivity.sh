#!/bin/bash
#
# Description:
#   This script executes IMA test bucket for:  [TODO!: fill in blank here and rename this file].
#
# Input Parameters:
#   $1  -  optional: a log file to which this script will direct its output.
#          If this option is not given, the execution result will be logged in
#          the default log file:  <thisFileName>.log
#
# All files required for this script should be copied from Build Server into this arrangement:
#  IMA Test Environment:  ../scripts, ../common, ../[your-testcase-dir]
#
# Environment setup required by the test cases launched by this script:
# 1. Modify ../scripts/ISMsetup.sh to match with your test machines setup.
#
# 2. Edit the TODO!: sections in this file to set execution directives
#    a.) Save the logs from all executions, pass or fail
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
  # Official IMA Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized IMASetup for THIS param, SSH environment file and Tag Replacement 
  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi  

source ../scripts/commonFunctions.sh

# Test Buckets that use the run-scenarios.sh framework:
# TODO!:  If you would like to save the logs regardless of the test bucket's outcome, setSavePassedLogs to 'on', default: off
# TODO!:  If you would like to control the time between every component start in your run-scenarios.sh target script,
#         setComponentSleep to 'the number of seconds to wait'.  default: 3 seconds.   
#  If you need a variable amount of time between component starts, in your run-scenarios.sh target script specify a component[#]=sleep,<number-of-seconds> .
setSavePassedLogs on ${logfile}
setComponentSleep default ${logfile}

# Script to call enablemq
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-enablemq.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run00MsgConResult=$?

# Script to call msgConversion_runScenario22
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario22.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run22MsgConResult=$?

# Script to call msgConversion_runScenario23
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario23.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run23MsgConResult=$?

# Script to call msgConversion_runScenario24
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario24.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run24MsgConResult=$?

# Script to call msgConversion_runScenario25
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario25.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run25MsgConResult=$?

# Script to call msgConversion_runScenario26
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario26.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run26MsgConResult=$?

# Script to call msgConversion_runScenario27
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario27.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run27MsgConResult=$?

# Script to call msgConversion_runScenario28
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario28.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run28MsgConResult=$?

# Script to call msgConversion_runScenario29
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario29.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run29MsgConResult=$?

# Script to call msgConversion_runScenario30
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario30.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run30MsgConResult=$?

# Script to call msgConversion_runScenario31
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario31.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run31MsgConResult=$?

# Script to call msgConversion_runScenario32
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario32.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run32MsgConResult=$?

# Script to call msgConversion_runScenario33
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario33.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run33MsgConResult=$?

# Script to call msgConversion_runScenario34
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario34.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run34MsgConResult=$?

# Script to call msgConversion_runScenario35
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario35.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run35MsgConResult=$?

# Script to call msgConversion_runScenario36
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario36.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run36MsgConResult=$?

# Script to call msgConversion_runScenario37
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario37.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run37MsgConResult=$?

# Script to call msgConversion_runScenario38
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario38.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run38MsgConResult=$?

# Script to call msgConversion_runScenario39
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario39.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run39MsgConResult=$?

# Script to call msgConversion_runScenario40
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario40.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run40MsgConResult=$?

# Script to call msgConversion_runScenario41
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario41.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run41MsgConResult=$?

# Script to call msgConversion_runScenario42
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario42.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run42MsgConResult=$?

# Script to call msgConversion_runScenario43
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario43.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run43MsgConResult=$?

# Script to call msgConversion_runScenario44
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario44.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run44MsgConResult=$?

# Script to call msgConversion_runScenario45
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario45.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run45MsgConResult=$?

# Script to call msgConversion_runScenario46
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario46.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run46MsgConResult=$?

# Script to call msgConversion_runScenario47
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario47.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run47MsgConResult=$?

# Script to call msgConversion_runScenario48
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario48.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run48MsgConResult=$?

# Script to call msgConversion_runScenario49
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario49.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run49MsgConResult=$?

# Script to call msgConversion_runScenario50
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario50.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run50MsgConResult=$?

# Script to call msgConversion_runScenario51
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario51.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run51MsgConResult=$?

# Script to call msgConversion_runScenario52
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario52.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run52MsgConResult=$?

# Script to call msgConversion_runScenario53
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario53.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run53MsgConResult=$?

# Script to call msgConversion_runScenario54
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario54.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run54MsgConResult=$?

# Script to call msgConversion_runScenario55
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario55.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run55MsgConResult=$?

# Script to call msgConversion_runScenario56
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario56.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run56MsgConResult=$?

# Script to call msgConversion_runScenario57
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario57.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run57MsgConResult=$?

# Script to call msgConversion_runScenario58
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario58.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run58MsgConResult=$?

# Script to call msgConversion_runScenario59
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario59.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run59MsgConResult=$?

# Script to call msgConversion_runScenario60
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario60.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run60MsgConResult=$?

# Script to call msgConversion_runScenario61
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario61.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run61MsgConResult=$?

# Script to call msgConversion_runScenario62
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario62.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run62MsgConResult=$?

# Script to call msgConversion_runScenario63
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario63.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run63MsgConResult=$?

# this scenario uses java to ssh commands to messagesight... needs to be ported to rest apis
# Script to call msgConversion_runScenario64
# RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario64.sh ${logfile}"
# echo Running command: ${RUNCMD}
# $RUNCMD
# run64MsgConResult=$?

# Script to call msgConversion_runScenario65
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario65.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run65MsgConResult=$?

# Script to call msgConversion_runScenario66
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario66.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run66MsgConResult=$?

# Script to call disablemq
RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-disablemq.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
run99MsgConResult=$?


# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

exit $( expr ${run00MsgConResult} + ${run22MsgConResult} + ${run23MsgConResult} + ${run24MsgConResult}  + ${run25MsgConResult} + ${run26MsgConResult} + ${run27MsgConResult}  + ${run28MsgConResult} + ${run29MsgConResult} + ${run30MsgConResult}  + ${run31MsgConResult} + ${run32MsgConResult} + ${run33MsgConResult}  + ${run34MsgConResult} + ${run35MsgConResult} + ${run36MsgConResult}  + ${run37MsgConResult} + ${run38MsgConResult} + ${run39MsgConResult}  + ${run40MsgConResult} + ${run41MsgConResult} + ${run42MsgConResult}  + ${run43MsgConResult} + ${run44MsgConResult} + ${run45MsgConResult}  + ${run46MsgConResult} + ${run47MsgConResult} + ${run48MsgConResult} + ${run49MsgConResult} + ${run50MsgConResult} + ${run51MsgConResult} + ${run52MsgConResult} + ${run53MsgConResult} + ${run54MsgConResult} + ${run55MsgConResult} + ${run56MsgConResult} + ${run57MsgConResult} + ${run58MsgConResult} + ${run59MsgConResult} + ${run60MsgConResult} + ${run61MsgConResult}  + ${run62MsgConResult}  + ${run63MsgConResult}  + ${run64MsgConResult}  + ${run65MsgConResult} + ${run66MsgConResult} + ${run99MsgConResult})

