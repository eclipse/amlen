#!/bin/bash
curDir=$(dirname ${0})
curFile=$(basename ${0})
err=0
logfile=`echo ${curFile} | cut -d '.' -f 1`.log
if [[ $# -gt 1 ]] ; then
  err=1
elif [[ $# -eq 1 ]] ; then
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
if [[ -f "../testEnv.sh" ]]
then
  source ../testEnv.sh
else
  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi  

source ../scripts/commonFunctions.sh
setSavePassedLogs on ${logfile}
setComponentSleep 1 ${logfile}
totalResults=0

export TIMEOUTMULTIPLIER=4.0

############################################
# PRE-SETUP (enable HA) MUST BE RUN FIRST! #
# ... after deleting external LDAP!!!      #
############################################
RUNCMD="../scripts/run-scenarios.sh ha/ima-JCA-HA-presetup-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

##############################################
# SETUP MUST BE RUN ----->>> SECOND <<<----- #
##############################################
RUNCMD="../scripts/run-scenarios.sh  setup/ima-JCA-Setup-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

if [[ "${FULLRUN}" == "TRUE" ]] ; then
	############################################################
	# Basic Test Bucket -- because our framework is really bad #
	############################################################
	RUNCMD="../scripts/run-scenarios.sh  basic/ima-JCA-Basic-01.sh ${logfile}"
	echo Running command: ${RUNCMD}
	$RUNCMD
	totalResults=$( expr ${totalResults} + $? )
fi

##############################
# HIGH AVAILABILITY with JCA #
##############################
RUNCMD="../scripts/run-scenarios.sh  ha/ima-JCA-HA-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

#############################
# TEARDOWN MUST BE RUN LAST #
#############################
RUNCMD="../scripts/run-scenarios.sh  teardown/ima-JCA-Teardown.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

##############################################################
# Disable HA AFTER uninstalling app and deleting admin stuff #
##############################################################
RUNCMD="../scripts/run-scenarios.sh  ha/ima-JCA-HA-teardown-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )

RUNCMD="../scripts/forceResetHA.sh"
echo Running command: ${RUNCMD}
$RUNCMD

# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

exit ${totalResults}

