#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   11/26/12 - Refactored ism-MQTT_ALL_Sample-runScenario01.sh into 01.sh
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv3 Store Record workload with retained messages "
source ./svt_test.sh

#------------------------------------------
# Next line is required if you want to run "test transactions"
#------------------------------------------
export AUTOMATION_FRAMEWORK_SAVE_RC="/niagara/test/svt_cmqtt/.svt.af.runscenario.last.rc"

test_template_set_prefix "cmqtt_17_"

typeset -i n=0

if [ "${A1_HOST_OS:0:4}" != "CCI_" ] ;then
  num_clients=100000 # number of clients to use for this test
  num_msg=99999999999 # will never send all of these.
else
#  num_clients=60000 # number of clients to use for this test
  num_clients=600 # number of clients to use for this test
  num_msg=99999999999 # will never send all of these.
fi

do_store_record_tests(){
    local addnl_variation=${1-""}
    local a_test_bucket="StoreRecordLimitRetained"
    local a_cleanup_bucket="cleanup"
    local a_name="$a_test_bucket "
    local a_variation

    a_variation="$addnl_variation"

    #---------------------------------------------
    # Start test transaction. All tests until end of transaction must pass or the test will be quickly aborted.
    #---------------------------------------------
    test_template_add_test_single "${a_name} -Start a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_start|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #---------------------------------------------
    # Cleanup any extraneous logs that may have existed from a previous failed run
    #---------------------------------------------
    test_template_add_test_single "${a_name} - cleanup any extraneous logs" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|47726" 600 SVT_AUTOMATION_TEST_LOG_TAR=${a_cleanup_bucket}_${n} ""

    #--------------------------
    # Verify pre-condition - 0 % store is used in pool 1
    #--------------------------
    test_template_add_test_single "${a_name} - verify precondition that zero percent is used on disk or a clean store is required to run this test" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|wait_disk|1|${A1_HOST}|${A2_HOST}" 180 "$a_variation|SVT_AT_VARIATION_69925_VAL=75|SVT_AT_VARIATION_69925_OP=>=" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify pre-condition - no pre-existing retained messages, note if this fails a clean store is needed to continue with the test
    #--------------------------
    test_template_add_test_single "${a_name} - verify precondition that 0 retained messages exist on /svt_cmqtt_17_retain/ or a clean store is required to run this test" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|verify_retained|1|${A1_HOST}|${A2_HOST}" 180 "$a_variation|SVT_AT_VARIATION_69925_VAL=0|SVT_AT_VARIATION_69925_OP===" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Check current store stats
    #--------------------------
    test_template_add_test_single "${a_name} - print stats before operation" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|print_store_stats|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Start background publishing retained messages to fill up store pool 1
    #--------------------------
    a_variation="SVT_AT_VARIATION_QUICK_EXIT=true|"
    a_variation+="SVT_AT_VARIATION_PATTERN=fanin|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=$num_msg|"
    a_variation+="SVT_AT_VARIATION_PUBQOS=2|"
    #a_variation+="SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=60|"
    a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
    a_variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
    a_variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
    a_variation+="SVT_AT_VARIATION_RETAINED_MSGS=true|"
    a_variation+="SVT_AT_VARIATION_UNIQUE=svt_cmqtt_17_retain|"
    a_variation+="SVT_AT_VARIATION_RESOURCE_MONITOR=false|"
    a_variation+="SVT_AT_VARIATION_TOPIC_MULTI_TEST=$num_msg|"
    a_variation+="SVT_AT_VARIATION_WAITFORCOMPLETEMODE=1|"
    a_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=1|"
    a_variation+="SVT_AT_VARIATION_TEST_ONLY=true|"
    a_variation+="$addnl_variation"
    export SVT_LAUNCH_SCALE_TEST_DESCRIPTION="${a_name} - start ${num_clients} background retained publishers to fill up store pool one memory to fifty percent "
    svt_common_launch_scale_test "connections" $num_clients "$a_variation" 0
    
    #--------------------------
    # Block until >= 49 % store is achieved in pool 1
    #--------------------------
    test_template_add_test_single "${a_name} - block until <= 49 percent memory free" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|wait_disk|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation|SVT_AT_VARIATION_69925_VAL=49|SVT_AT_VARIATION_69925_OP=<=" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify stable for 10 minutes , memory , retained messages etc.. should all stop growing.
    #--------------------------
#   test_template_add_test_single "${a_name} - verify system remains stable for 10 minutes" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|verify_stable|1|${A1_HOST}|${A2_HOST}" 1800 "$a_variation|SVT_AT_VARIATION_69925_VAL=10" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Stop background publishers
    #--------------------------
    test_template_add_test_single "${a_name} - stop background retained publishers - cleanup any extraneous processes" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|na" 600 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true" ""

    #--------------------------
    # Read all retained messages out?
    #--------------------------
    #test_template_add_test_single "${a_name} - wait for all pubs and subs to complete" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726|WAIT|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify results?
    #--------------------------
    #test_template_add_test_single "${a_name} - verify results and no duplicate messages. " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_47726_verify_result|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "SVT_AT_VARIATION_47726_NUMMSG=6000|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Check current store stats
    #--------------------------
    test_template_add_test_single "${a_name} - print stats before cleaning out" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|print_store_stats|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Create NULLMSG file - retained messages cleaned up by writing "" message to same topic
    #--------------------------
    test_template_add_test_single "${a_name} - create NULLMSG file to use for cleaning up retained messages" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|create_null_msg|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Clean out all the retained messages
    # Start background publishing retained empty messages to free  store pool 1
    #--------------------------
    a_variation="SVT_AT_VARIATION_QUICK_EXIT=true|"
    a_variation+="SVT_AT_VARIATION_PATTERN=fanin|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=$num_msg|"
    a_variation+="SVT_AT_VARIATION_PUBQOS=0|"
    #a_variation+="SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=60|"
    a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=0|"
    a_variation+="SVT_AT_VARIATION_CLEANSESSION=true|"
    a_variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
    a_variation+="SVT_AT_VARIATION_RETAINED_MSGS=true|"
    a_variation+="SVT_AT_VARIATION_UNIQUE=svt_cmqtt_17_retain|"
    a_variation+="SVT_AT_VARIATION_RESOURCE_MONITOR=false|"
    a_variation+="SVT_AT_VARIATION_TOPIC_MULTI_TEST=$num_msg|"
    a_variation+="SVT_AT_VARIATION_WAITFORCOMPLETEMODE=1|"
    a_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=1|"
    a_variation+="SVT_AT_VARIATION_TEST_ONLY=true|"
    a_variation+="SVT_AT_VARIATION_MSGFILE=NULLMSG|"
    a_variation+="SVT_AT_VARIATION_69925_VAL=LIMIT_NUM_MSG|" # Need to make publishers stop when accomplished goal. See SVT.pattern.sh for logic
    a_variation+="$addnl_variation"
    export SVT_LAUNCH_SCALE_TEST_DESCRIPTION="${a_name} - start ${num_clients} background retained publishers to free up disk memory back to TBD zero percent" ; # note if TBD not zero when started... need to handle TODO
    svt_common_launch_scale_test "connections" $num_clients "$a_variation" 0

    #--------------------------
    # Block until <= 1 % store is achieved in pool 1
    #--------------------------
    test_template_add_test_single "${a_name} - block until 1 percent memory usage on disk" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|verify_retained|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation|SVT_AT_VARIATION_69925_VAL=0|SVT_AT_VARIATION_69925_OP===" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Check current store stats
    #--------------------------
    test_template_add_test_single "${a_name} - print stats after first chance cleaner" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|print_store_stats|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Stop background publishers
    #--------------------------
    test_template_add_test_single "${a_name} - stop background retained publishers - cleanup any extraneous processes" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|na" 600 "SVT_AT_VARIATION_QUICK_EXIT=true|SVT_AT_VARIATION_ALL_M=true" ""

    #--------------------------
    # Check current store stats
    #--------------------------
    test_template_add_test_single "${a_name} - print stats before last chance cleaner" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|print_store_stats|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Verify results? -  no more retained messages
    #--------------------------
    test_template_add_test_single "${a_name} - block until 0 retained messages left on /svt_cmqtt_17_retain/# TODO make all messages match that pattern" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|69925|verify_retained|1|${A1_HOST}|${A2_HOST}" 3600 "$a_variation|SVT_AT_VARIATION_69925_VAL=0|SVT_AT_VARIATION_69925_OP===|SVT_AT_VARIATION_69925_CLEAN_STRAGGLERS=true" "TEST_RESULT: SUCCESS"

    #--------------------------
    # End the test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - End a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_end|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Consolidate and capture all log files for this test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - collect logs" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|47726" 600 SVT_AUTOMATION_TEST_LOG_TAR=${a_test_bucket}_${n} ""
        
}


#------------------------
# Execute 
#------------------------
do_store_record_tests



