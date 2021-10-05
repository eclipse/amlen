#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   11/26/12 - Refactored ism-MQTT_ALL_Sample-runScenario01.sh into 01.sh
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQ and HA Automated Resilience test"
source ./svt_test.sh

#------------------------------------------
# Next line is required if you want to run "test transactions"
#------------------------------------------
export AUTOMATION_FRAMEWORK_SAVE_RC="/niagara/test/svt_cmqtt/.svt.af.runscenario.last.rc"

#-------------------------------------------------------------------------------
# Test Case Design / Plan
#
# The next export is required if you want to specify the location of the test plan or design
#
# Note: this export can be exported once for a plan that covers all tests for this file
# or often during test execution to specify different plans for each individual test.
# Results are stored in $M1_TESTROOT/testplans.log , and links displayed during post 
# processing.
#-------------------------------------------------------------------------------
export AUTOMATION_FRAMEWORK_TEST_PLAN_URL=https://changeme.jazzserver.example.com:9443/ccm/service/com.ibm.team.workitem.common.internal.rest.IAttachmentRestService/itemName/com.ibm.team.workitem.Attachment/9735

test_template_set_prefix "cmqtt_15_"

typeset -i n=0

do_ha_mq_tests(){
    local addnl_variation=${1-""}
    local a_test_bucket="MQHAResilience"
    local a_cleanup_bucket="cleanup"
    local a_name="$a_test_bucket "
    local a_variation="SVT_AT_VARIATION_QUICK_EXIT=true"

    a_variation="$a_variation|$addnl_variation"

    #---------------------------------------------
    # Start test transaction. All tests until end of transaction must pass or the test will be quickly aborted.
    #---------------------------------------------
    test_template_add_test_single "${a_name} -Start a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_start|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #---------------------------------------------
    # Cleanup any extraneous logs that may have existed from a previous failed run
    #---------------------------------------------
    test_template_add_test_single "${a_name} - cleanup any extraneous logs" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|47726" 600 SVT_AUTOMATION_TEST_LOG_TAR=${a_cleanup_bucket}_${n} ""

    #------------------------
    # Setup HA
    #------------------------
    
    test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

    #---------------------------------------------
    # Make sure we are in a valid HA config.
    #---------------------------------------------
    test_template_add_test_single "SVT - verify HA both running" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|verifyBothRunning|1|${A1_HOST}|${A2_HOST}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Setup MQ : MQ Server (Define Queues...)
    #--------------------------
    test_template_add_test_single "${a_name} - setup Websphere MQ " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_setup|SETUP|1|${A1_HOST}|${A2_HOST}" 120 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Enable Destination Mapping Rules 
    #--------------------------
    test_template_add_test_single "${a_name} - enable DMRs" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_dmr_control|ENABLE|1|${A1_HOST}|${A2_HOST}" 120 "$a_variation" "TEST_RESULT: SUCCESS"

    #---------------------------------------------
    # Cleanup any extraneous subscription data.
    #---------------------------------------------
    test_template_add_test_single "${a_name} - cleanup MQ Looped Durable Subscribers" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|SUB|1|${A1_IPv4_1}|${A2_IPv4_1}" 120 "SVT_AT_VARIATION_47726_NUMMSG=0|$a_variation|SVT_AT_VARIATION_QUICK_EXIT=false" "SVT_TEST_RESULT: SUCCESS"

    #--------------------------
    # Start MQ loop background Qos 2 subscribers
    #--------------------------
    test_template_add_test_single "${a_name} - start MQ Looped Subscribers" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|SUB|1|${A1_IPv4_1}|${A2_IPv4_1}" 120 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Start MQ loop background Qos 2 publishers
    #--------------------------
    test_template_add_test_single "${a_name} - start MQ Looped Publishers" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|PUB|1|${A1_IPv4_1}|${A2_IPv4_1}" 120 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"

    #------------------------------------   
    # The number of failure injections is controlled by the list of numbers in the next for loop.
    # 
    #  Example 1: for v in 1 4 20 21 45 ; do 
    #           This means that there will be 5 separate failure injections after 1 4 20 ... to 45 messages are received by all subscribers
    # 
    #  Example 2: for v in 100 200 400 500 1000 2000 2100 4500 4900 ; do 
    #           This means that there will be 9 separate failure injections after 100 200 400 ... to 4900 messages are received by all subscribers
    #------------------------------------   
    let injectcount=0;
    for v in 500 900 1400 2000 2400 4500 ; do
        
        #--------------------------
        # Block until at least ${v}xx messages recieved by all subs
        #--------------------------
        test_template_add_test_single "${a_name} - block until ${v} messages received by MQ Connectivity subscriber loops " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_wait_msg_count|${v}|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

        #--------------------------
        # Start one of the different failure operations
        #--------------------------
        declare -a fail_injection_array=(
            "STOP"
            "DEVICE"
            "ENDMQ"
            "STOP"
            "POWER"
            "STOP"
            "STOP"
            "STOP"
            "STOP"
            "STOP"
            "STOP"
            "STOP"
            NULL NULL NULL NULL
            NULL NULL NULL NULL
            NULL NULL NULL NULL
            #-----------------------------------------------------------------------
            # - Additional failure injects - pending when all current defects solved
            #-----------------------------------------------------------------------
            # PENDING : Power off primary : defect 48155
            # PENDING : Power off standby
            # PENDING : Stop standby : 
            # PENDING : Stop QMGR / start QMGR
            # PENDING : Disable / Enable DMRs
        );
       
        if [ "${fail_injection_array[$injectcount]}" == "STOP" ] ;then
            test_template_add_test_single "SVT - verify HA both running" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|verifyBothRunning|1|${A1_HOST}|${A2_HOST}" 1800 "$a_variation" "TEST_RESULT: SUCCESS"
            test_template_add_test_single "SVT - stop primary" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST}" 600 "$a_variation" "TEST_RESULT: SUCCESS"
            test_template_add_test_single "SVT - start standby" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST}" 600 "$a_variation" "TEST_RESULT: SUCCESS"
        elif [ "${fail_injection_array[$injectcount]}" == "DEVICE" ] ;then
            test_template_add_test_single "SVT - verify HA both running" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|verifyBothRunning|1|${A1_HOST}|${A2_HOST}" 1800 "$a_variation" "TEST_RESULT: SUCCESS"
            test_template_add_test_single "SVT - device restart primary" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|deviceRestartPrimary|1|${A1_HOST}|${A2_HOST}" 600 "$a_variation" "TEST_RESULT: SUCCESS"
        elif [ "${fail_injection_array[$injectcount]}" == "POWER" ] ;then
            test_template_add_test_single "SVT - verify HA both running" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|verifyBothRunning|1|${A1_HOST}|${A2_HOST}" 1800 "$a_variation" "TEST_RESULT: SUCCESS"
            test_template_add_test_single "SVT - power cycle primary" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|powerCyclePrimary|1|${A1_HOST}|${A2_HOST}" 600 "$a_variation" "TEST_RESULT: SUCCESS"
        elif [ "${fail_injection_array[$injectcount]}" == "ENDMQ" ] ;then
            test_template_add_test_single "${a_name} - cycle Websphere MQ w/ endmqm and strmqm " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_cycle_mq|na|1|${A1_HOST}|${A2_HOST}" 120 "$a_variation" "TEST_RESULT: SUCCESS"
        else
            injectcounter=0;
        fi

        let injectcount=$injectcount+1
    done
    
    #--------------------------
    # Block until all 6000 messages recieved by all subs
    #--------------------------
    test_template_add_test_single "${a_name} - block until 6000 messages received by MQ Connectivity subscriber loops " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_wait_msg_count|6000|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify all pubs and subs complete
    #--------------------------
    test_template_add_test_single "${a_name} - wait for all pubs and subs to complete" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|WAIT|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify results, check for duplicate messages
    #--------------------------
    test_template_add_test_single "${a_name} - verify results and no duplicate messages. " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_verify_result|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"


    #--------------------------
    # End the test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - End a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_end|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Cleanup MQ loop durable subscribers - wait for the operation to complete (SVT_AT_VARIATION_QUICK_EXIT=false)
    #--------------------------
    test_template_add_test_single "${a_name} - cleanup any extraneous processes" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|na" 600 SVT_AT_VARIATION_QUICK_EXIT=true ""
    test_template_add_test_single "${a_name} - cleanup MQ Looped Durable Subscribers" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|SUB|1|${A1_IPv4_1}|${A2_IPv4_1}" 120 "SVT_AT_VARIATION_47726_NUMMSG=0|$a_variation|SVT_AT_VARIATION_QUICK_EXIT=false" "SVT_TEST_RESULT: SUCCESS"

    #--------------------------
    # Disable Destination Mapping Rules so cleanup can be done.
    #--------------------------
    test_template_add_test_single "${a_name} - disable DMRs (required) to cleanup MQ queues" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_dmr_control|DISABLE|1|${A1_HOST}|${A2_HOST}" 120 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Cleanup MQ : MQ Server (Delete Queues...)
    #--------------------------
    test_template_add_test_single "${a_name} - cleanup Websphere MQ " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_setup|CLEAN|1|${A1_HOST}|${A2_HOST}" 120 "$a_variation" "TEST_RESULT: SUCCESS"


    #------------------------
    # Disable HA
    #------------------------
    test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Consolidate and capture all log files for this test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - collect logs" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|47726" 600 SVT_AUTOMATION_TEST_LOG_TAR=${a_test_bucket}_${n} ""
        
}


#------------------------
# Execute MQ / HA resilience test
#------------------------
do_ha_mq_tests

