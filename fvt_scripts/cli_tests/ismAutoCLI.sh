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

if [[ "${A1_REST}" == "TRUE" ]] ; then 

  # ======================================== 
  #  These tests are run on REST API systems
  # ========================================
 
  # First check ldaps configurability/function
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  ldaps/ism-cliLDAPS_Test-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  if [ "${FULLRUN}" == "TRUE" ] ; then
	  if [[ "${A1_LDAP}" == "FALSE" ]] ; then
	    RUNCMD="../scripts/run-scenarios.sh  policies/policy_m1ldap_setup_runScenarios.sh ${logfile}"
	    echo Running command: ${RUNCMD}
	    $RUNCMD
	    totalResults=$( expr ${totalResults} + $? )
	  fi
	
	  export component_sleep=0
	  RUNCMD="../scripts/run-scenarios.sh  policies/policy_validation_unpack_runScenarios.sh ${logfile}"
	  echo Running command: ${RUNCMD}
	  $RUNCMD
	  totalResults=$( expr ${totalResults} + $? )
	  unset component_sleep
	
	  export component_sleep=0
	  RUNCMD="../scripts/run-scenarios.sh  policy_tests/ptns_topic_auto/ptns_topic_auto_runScenarios.sh ${logfile}"
	  echo Running command: ${RUNCMD}
	  $RUNCMD
	  totalResults=$( expr ${totalResults} + $? )
	  unset component_sleep
	
	  export component_sleep=0
	  RUNCMD="../scripts/run-scenarios.sh  policy_tests/ptws_topic_auto/ptws_topic_auto_runScenarios.sh ${logfile}"
	  echo Running command: ${RUNCMD}
	  $RUNCMD
	  totalResults=$( expr ${totalResults} + $? )
	  unset component_sleep
  
	  export component_sleep=0
	  RUNCMD="../scripts/run-scenarios.sh  policy_tests/gvt_topic_mqtt_auto/gvt_topic_mqtt_auto_runScenarios.sh ${logfile}"
	  echo Running command: ${RUNCMD}
	  $RUNCMD
	  totalResults=$( expr ${totalResults} + $? )
	  unset component_sleep
	
	  export component_sleep=0
	  RUNCMD="../scripts/run-scenarios.sh  policy_tests/gvt_topic_jms_auto/gvt_topic_jms_auto_runScenarios.sh ${logfile}"
	  echo Running command: ${RUNCMD}
	  $RUNCMD
	  totalResults=$( expr ${totalResults} + $? )
	  unset component_sleep
	
	  if [[ "${A1_LDAP}" == "FALSE" ]] ; then
	    RUNCMD="../scripts/run-scenarios.sh  policies/policy_m1ldap_cleanup_runScenarios.sh ${logfile}"
	    echo Running command: ${RUNCMD}
	    $RUNCMD
	    totalResults=$( expr ${totalResults} + $? )
	  fi

  fi #end FULLRUN only.
  
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policies/policies_mmb/ism-cliPolicyMMBTest-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
  export component_sleep=0

  RUNCMD="../scripts/run-scenarios.sh  security/ima-CLI-SecurityAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  ep_flex/ep_flex_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_topics/ima-CRUD-TopicsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_queues/ima-CRUD-QueuesAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_subscriptions/ima-CRUD-SubscriptionsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_queues/ima-Stat-QueuesAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_subscriptions/ima-Stat-SubscriptionAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
  
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_topics/ima-Stat-TopicsAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

setSavePassedLogs on ${logfile}
  
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  must-gather/ima-must-gatherAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  export_import/ism-ExportImport-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

else # not DOCKER

# =========================================== 
#  These tests are run on non RESTAPI systems
# ===========================================

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  crud_security/ima-CRUD-SecurityAuto.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

if [ "${FULLRUN}" == "TRUE" ] ; then
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_security/ima-CRUD-SecurityTLSOptsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
fi

if [ "${FULLRUN}" == "TRUE" ] ; then
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  ism-cliDemoObjectsTest-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  ism-cliTest-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
fi
 
export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  rc_tests/ism-cliRcTest-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

if [ "${FULLRUN}" == "TRUE" ] ; then
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  help_tests/ism-cliHelpTest-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policies/policy_validation_unpack_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policy_tests/ptns_topic_auto/ptns_topic_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policy_tests/ptws_topic_auto/ptws_topic_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policy_tests/gvt_topic_mqtt_auto/gvt_topic_mqtt_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  policy_tests/gvt_topic_jms_auto/gvt_topic_jms_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
fi

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  policies/policies_dcn/ism-cliPolicyDCNTest-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  policies/policies_mmb/ism-cliPolicyMMBTest-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  stat_queues/ima-Stat-QueuesAuto-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  stat_topics/ima-Stat-TopicsAuto-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  security/ima-CLI-SecurityAuto-00.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

if [ "${FULLRUN}" == "TRUE" ] ; then
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_subscriptions/ima-Stat-SubscriptionAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  user_group_tests/ism-cliUserGroupTests-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  ep_flex/ep_flex_auto_runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  apdcore-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
fi

# These tests buckets include some test that are only run on development
export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  rc_tests/ism-cliLDAPTest-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

# These tests buckets include some test that are only run on development
export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  rc_tests/ism-cliLDAPS_Test-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

if [ "${FULLRUN}" == "TRUE" ] ; then
  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_memory/ima-Stat-MemoryAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_store/ima-Stat-StoreAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_misc/ima-Stat-MiscAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_dstmaprule/ima-Stat-DstMapRuleAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  stat_endpoint/ima-Stat-EndpointAuto-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  pending_delete/ima-pending-delete-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_subscriptions/ima-CRUD-SubscriptionsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_queues/ima-CRUD-QueuesAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_topics/ima-CRUD-TopicsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_monitoring/ima-CRUD-MonitoringAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  crud_mq/ima-CRUD-MQAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  endpoint_warnmsgs/ima-Endpoint-WarnMsgsAuto.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh  remote_syslog/ima-remote-syslog-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh advanced_pd/ismAutoConfigInit-runScenarios.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep

  export component_sleep=0
  RUNCMD="../scripts/run-scenarios.sh crud_snmp/ima-snmp-00.sh ${logfile}"
  echo Running command: ${RUNCMD}
  $RUNCMD
  totalResults=$( expr ${totalResults} + $? )
  unset component_sleep
fi

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  psk/ism-pskTests-runScenarios.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  cluster_config/ima-ClusterConfigAuto.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

# For now always run License test last.  
# It appears that this test restarts the server and it has not finished coming up when a test is run immediately after this one.
export component_sleep=0
RUNCMD="../scripts/run-scenarios.sh  licensed_usage/ima-LicensedUsageAuto.sh ${logfile}"
echo Running command: ${RUNCMD}
$RUNCMD
totalResults=$( expr ${totalResults} + $? )
unset component_sleep

fi # end if TYPE=DOCKER

# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE

exit ${totalResults}

