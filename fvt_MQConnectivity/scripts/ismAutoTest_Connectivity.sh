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
setSavePassedLogs off ${logfile}
setComponentSleep default ${logfile}
export TIMEOUTMULTIPLIER=1.5
totalResults=0


if [[ "${A1_LOCATION}" != "remote" ]] ; then
	#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	#----------------------------------------------------
	#  THIS SECTION IS THE FULL TEST SUITE THAT IS RUN WHEN
	#  THE SCRIPT IS EXECUTED WITH THE MESSAGESIGHT SERVER
	#  IN THE SAME DATACENTER AS THE CLIENTS AND THE QUEUE
	#  MANAGER.
	#  
	#  ALL NEW TESTS SHOULD BE ADDED HERE. NEW TESTS THAT
	#  DO NOT DO COMPLEX BEHAVIOR AND ONLY CONFIRM BASIC 
	#  FUNCTION SHOULD ALSO BE ADDED TO THE REMOTE SECTION
	#
	#----------------------------------------------------
	#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


    if [[ "$A1_LDAP" == "FALSE" ]] ; then
        RUNCMD="../scripts/run-scenarios.sh  mqconn_m1ldap_setup_runScenarios.sh ${logfile}"
        echo Running command: ${RUNCMD}
        $RUNCMD
        totalResults=$( expr ${totalResults} + $? )
    fi

    # Enable MQConnectivity Service 
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_enable-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Test Bucket Scenarios connectivity. 
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

setSavePassedLogs off ${logfile}

    # Script to call msgConversion_runScenarios
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenarios.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call jms_runScenario01
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_jms-runScenarios.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call rules-runScenarios
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_rules-runScenarios.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call restart_rules-runScenario02
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_restart_rules-runScenario02.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call restart_rules-runScenario04
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_restart_rules-runScenario04.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call restart_multi_rules-runScenario
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_restart_multi_rules-runScenario.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call ima_test_connectivity_boundaries01
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_boundaries01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Script to call ima_test_connectivity_boundaries01
    #RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_mqssl.sh ${logfile}"
    #echo Running command: ${RUNCMD}
    #$RUNCMD
    #totalResults=$( expr ${totalResults} + $? )

    # Script to call the final teardown 01
    RUNCMD="../scripts/run-scenarios.sh ima_test_final_teardown-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    # Disable MQConnectivity Service 
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_disable-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )

    if [[ "$A1_LDAP" == "FALSE" ]] ; then
        RUNCMD="../scripts/run-scenarios.sh  mqconn_m1ldap_cleanup_runScenarios.sh ${logfile}"
        echo Running command: ${RUNCMD}
        $RUNCMD
        totalResults=$( expr ${totalResults} + $? )
    fi

else
    # Test Bucket Scenarios (group 2 BASIC tests only) 
    #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    #----------------------------------------------------
    #  THIS SECTION IS FOR USE WHEN RUNNING CLIENTS AGAINST 
    #  A SERVER IN DIFFERENT DATACENTER. IN THAT CASE A BASIC
    #  TEST RUN TO VERIFY THE SERVER CONFIGURES AND RUNS
    #  IN THAT ENVIRONEMENT IS ALL THAT IS NEEDED. 
    #  
    #  COMPLEX SCENARIO TESTING SHOULD BE DONE ON SETUPS WHERE
    #  THE CLIENTS AND SERVERS ARE IN THE SAME DATACENTER. 
    # 
    #  DO NOT ADD EVERY TEST TO THIS SECTION! 
    #  
    #----------------------------------------------------
    #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    # We don't run most of these tests, in part because the time needed on EC2 and Azure is extremely long. 
    # The scripts chosen represent a minimal set of FVT tests to ensure there's nothing funny with these two platforms. 
    
    echo MQ Connectivity.  BASIC TESTS ONLY.  For use only on REMOTE SERVERS.
    
    if [[ "$A1_LDAP" == "FALSE" ]] ; then
        RUNCMD="../scripts/run-scenarios.sh  mqconn_m1ldap_setup_runScenarios.sh ${logfile}"
        echo Running command: ${RUNCMD}
        $RUNCMD
        totalResults=$( expr ${totalResults} + $? )
    fi
    
    # Enable MQConnectivity Service 
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_enable-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    #test basic connectivity
    # Test Bucket Scenarios (group 1) 
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    #test basic message conversion
    
    # Script to call msgConversion_runScenario01
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    # Script to call msgConversion_runScenario03
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_msgcon-runScenario03.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    #We do not run any JMS tests.
    
    #test basic rules processing
    
    # Script to call rules-runScenarios
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_rules-REMOTE-runScenarios.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    # We only need one restart test  
    
    # Script to call restart_multi_rules-runScenario
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_restart_multi_rules-runScenario.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    # Script to call ima_test_connectivity_boundaries01
    RUNCMD="../scripts/run-scenarios.sh ima_test_connectivity_boundaries01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    
    #We don't need to test tracing. 
    
    
    #test basic SSL. 
    
    # Script to call ima_test_connectivity_mqssl.sh
    #RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_mqssl.sh ${logfile}"
    #echo Running command: ${RUNCMD}
    #$RUNCMD
    #totalResults=$( expr ${totalResults} + $? )
    
    # Script to call the final teardown 01
    RUNCMD="../scripts/run-scenarios.sh ima_test_final_teardown-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    # Disable MQConnectivity Service 
    RUNCMD="../scripts/run-scenarios.sh ima_test_mqcon_disable-runScenario01.sh ${logfile}"
    echo Running command: ${RUNCMD}
    $RUNCMD
    totalResults=$( expr ${totalResults} + $? )
    
    if [[ "$A1_LDAP" == "FALSE" ]] ; then
        RUNCMD="../scripts/run-scenarios.sh  mqconn_m1ldap_cleanup_runScenarios.sh ${logfile}"
        echo Running command: ${RUNCMD}
        $RUNCMD
        totalResults=$( expr ${totalResults} + $? )
    fi

fi
    
# Do not alter these values, this resets them back to default values in case the next TC Batch fails to initialize the values
setComponentSleep default NONE
setSavePassedLogs off     NONE
    
exit ${totalResults}

