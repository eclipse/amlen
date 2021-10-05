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
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement 
  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi  

source ../scripts/commonFunctions.sh

os=$(uname -a | grep -v grep | grep Linux)
if [[ ${os} != "" ]] ; then
  ISLINUX="Yes"
else
  ISLINUX="No"
fi

# Test Buckets that use the run-scenarios.sh framework:
# TODO!:  If you would like to save the logs regardless of the test bucket's outcome, setSavePassedLogs to 'on', default: off
# TODO!:  If you would like to control the time between every component start in your run-scenarios.sh target script,
#         setComponentSleep to 'the number of seconds to wait'.  default: 3 seconds.   
#  If you need a variable amount of time between component starts, in your run-scenarios.sh target script specify a component[#]=sleep,<number-of-seconds> .
setSavePassedLogs off ${logfile}
setComponentSleep 1 ${logfile}
totalResults=0

export TIMEOUTMULTIPLIER=1.3

# TODO!:  provide your run-scenarios target file(s), repeat as necessary, be sure to update the 'exit' command.
# Test Bucket Scenarios (group 1)
RUNCMD="../scripts/run-scenarios.sh  adminobjs/ism-JMS-AdminObjs-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if [[ "${A1_LOCATION}" == "remote" ]] ; then
	# Test Bucket Scenarios (group 2 BASIC tests only) 
	RUNCMD="../scripts/run-scenarios.sh  msgdelivery/ismREMOTE-JMS-MsgDelivery.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
else 
	# Test Bucket Scenarios (group 2)
	RUNCMD="../scripts/run-scenarios.sh  msgdelivery/ism-JMS-MsgDelivery-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

#if [[ "${FULLRUN}" == "TRUE" ]] ; then
#	# Test Bucket Scenarios (group 3)
#	RUNCMD="../scripts/run-scenarios.sh  foreignmsg/ism-JMS-ForeignMsg-00.sh ${logfile}"
#	echo Running command: ${RUNCMD}
#	$RUNCMD
#	totalResults=$( expr ${totalResults} + $? )
#fi

# Test Bucket Scenarios (group 4)
RUNCMD="../scripts/run-scenarios.sh  deliverymode/ism-JMS-DeliveryMode-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if [[ "${A1_LOCATION}" == "remote" ]] ; then

	# Test Bucket Scenarios BASIC SCENARIOS ONLY (group 5)
	RUNCMD="../scripts/run-scenarios.sh  durablesub/ismREMOTE-JMS-DurableSub.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )

else 
	# Test Bucket Scenarios (group 5)
	RUNCMD="../scripts/run-scenarios.sh  durablesub/ism-JMS-DurableSub-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

if [[ "${A1_LOCATION}" == "remote" ]] ; then

	#  Skip the JMS SharedSubs bucket on Remote Servers.
	echo "  Skipping JMS Shared Subscriptions Bucket on this setup.  "

else 
	# Test Bucket Scenarios (group 6)
	RUNCMD="../scripts/run-scenarios.sh  sharedsub/ism-JMS-SharedSub-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

if [[ "${A1_LOCATION}" == "remote" ]] ; then

	# Test Bucket Scenarios (group 7) Basic tests only
	RUNCMD="../scripts/run-scenarios.sh  queues/ismREMOTE-JMS-Queues.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )

else 

	# Test Bucket Scenarios (group 7)
	RUNCMD="../scripts/run-scenarios.sh  queues/ism-JMS-Queues-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi


# Test Bucket Scenarios (group 8)
RUNCMD="../scripts/run-scenarios.sh  transactions/ism-JMS-Transactions-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if [[ "${FULLRUN}" == "FALSE" ]] ; then
    # Test Bucket Scenarios (group 9)
    RUNCMD="../scripts/run-scenarios.sh  nontd/ima-JMS-NonTD-00.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
fi

# Test Bucket Scenarios (group 10)
RUNCMD="../scripts/run-scenarios.sh  ssl/ism-JMS-SSL-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	# Test Bucket Scenarios (group 11)
	RUNCMD="../scripts/run-scenarios.sh  wildcard/ism-JMS-Wildcard-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
	
	# Test Bucket Scenarios (group 12)
	RUNCMD="../scripts/run-scenarios.sh  chainedcerts/ism-JMS-ChainedCert-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

## This bucket also does the cleanup of the things setup by ismAutoJMS-setup.sh
## Test run after this bucket need to do their own setup (including LDAP if needed).
if [[ "${A1_LOCATION}" == "remote" ]] ; then
	# Test Bucket Scenarios (group 14)
	RUNCMD="../scripts/run-scenarios.sh  maxmsgbehavior/ismREMOTE-JMS-MaxMsgBehavior.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
else 
	# Test Bucket Scenarios (group 14)
	RUNCMD="../scripts/run-scenarios.sh  maxmsgbehavior/ism-JMS-MaxMsgBehavior-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

if [ ${ISLINUX} = "Yes" ] ; then
# Test Bucket Scenarios (group 15)
RUNCMD="../scripts/run-scenarios.sh  ldap/ima-JMS-LDAP-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
fi 

#if [[ "${FULLRUN}" == "TRUE" ]] ; then
# Test Bucket Scenarios (group 16)
RUNCMD="../scripts/run-scenarios.sh  oauth/ima-JMS-OAuth-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
#fi

if [[ "${A1_LOCATION}" == "remote" ]] ; then

	#  Skip the JMS Expiry bucket on Remote Servers.
	echo "  Skipping JMS Expiry Bucket on this setup.  "

else 
	# Test Bucket Scenarios (group 17)
	RUNCMD="../scripts/run-scenarios.sh  expiry/ism-JMS-Expiry-00.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

# Test Bucket Scenarios (group 18)
RUNCMD="../scripts/run-scenarios.sh  admin_dynamic/ism-JMS-AdminDynamic-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

exit ${totalResults}
