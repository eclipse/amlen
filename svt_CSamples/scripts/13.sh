#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   4/11/14 - Addeded shared subscription tests
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv3 SharedSubs Scale Workload"
source ./svt_test.sh

#------------------------------------------
# Next line is required if you want to run "test transactions"
#------------------------------------------
export AUTOMATION_FRAMEWORK_SAVE_RC="/niagara/test/svt_cmqtt/.svt.af.runscenario.last.rc"

test_template_set_prefix "cmqtt_13_"

typeset -i n=0

#---------------------------------------------
# Shared subscription tests
#---------------------------------------------
do_my_shared_sub_pattern(){
    local num_clients=${1-999900}
    local addnl_variation=${2-""}
    local a_description=${3-""}
    local a_timeout=${4-3600}
    local a_workload=${5-"connections"}
    local per_client 
    local a_command
    local a_variation=""
    local a_test_bucket=""

    a_test_bucket="NonDurableSharedSubs_${n}" ; # n is a global variable incremented in the test_template_ calls

    a_variation="SVT_AT_VARIATION_PATTERN=fanout_shared|SVT_AT_VARIATION_MSGCNT=100|"
    a_variation+="SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_SYNC_CLIENTS=true|"
    a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_STATUS_UPDATE=1|"
    a_variation+="SVT_AT_VARIATION_QOS=0|"
    a_variation+="SVT_AT_VARIATION_CLEANSESSION=true|"
    a_variation+="SVT_AT_VARIATION_UNIQUE=sharedsub_fan_out_1|"
    a_variation+="SVT_AT_VARIATION_ORDERMSG=2|"
    a_variation+="SVT_AT_VARIATION_SHAREDSUB=scale|"
    a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|"
    a_variation+="SVT_AT_VARIATION_SHAREDSUB_MULTI=scale|"
    a_variation+="SVT_AT_VARIATION_HA2=${A2_IPv4_1}|"
    a_variation+="$addnl_variation"

    let per_client=($num_clients/${M_COUNT});
    a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${a_workload}|${per_client}|1|${A1_IPv4_1}"
    test_template_add_test_single "$a_description" cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"

    a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${a_workload}"
    test_template_add_test_all_M_concurrent  "Cleanup - collect logs" "$a_command" 600 "SVT_AUTOMATION_TEST_LOG_TAR=$a_test_bucket" ""

    return 0;
}

durable_shared_sub_step1(){
    local num_clients=${1-20}
    local addnl_variation=${2-""}
    local description=${3-"Establish durable subs "}
    local qos=${4-0}
    local variation=""
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=0|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=0|"
    variation+="SVT_AT_VARIATION_HA2=|"
    variation+="SVT_AT_VARIATION_QOS=${qos}|SVT_AT_VARIATION_PUBQOS=${qos}|SVT_AT_VARIATION_CLEANSESSION=false|" 
    variation+="SVT_AT_VARIATION_SKIP_PUBLISH=1|"
    variation+="$addnl_variation"
    do_my_shared_sub_pattern $num_clients "$variation" "$description for qos: $qos"
}

durable_shared_sub_step2(){
    local num_clients=${1-20}
    local addnl_variation=${2-""}
    local description=${3-"Publish to durable subs"}
    local qos=${4-0}
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=100|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=1000|"
    variation+="SVT_AT_VARIATION_HA2=|"
    variation+="SVT_AT_VARIATION_QOS=${qos}|SVT_AT_VARIATION_PUBQOS=${qos}|SVT_AT_VARIATION_CLEANSESSION=false|" 
    variation+="SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
    variation+="$addnl_variation"
    do_my_shared_sub_pattern $num_clients "$variation" "$description for qos: $qos"
}

durable_shared_sub_step3(){
    local num_clients=${1-20}
    local addnl_variation=${2-""}
    local description=${3-"Receive msgs on durable subs"}
    local qos=${4-0}
    local variation=""
    variation+="SVT_AT_VARIATION_MSGCNT=1000|"
    variation+="SVT_AT_VARIATION_PCTMSGS=100|"
    variation+="SVT_AT_VARIATION_UNSUBSCRIBE=1|"
    variation+="$addnl_variation"
    durable_shared_sub_step1 $num_clients "$variation" "$description" $qos

}

durable_shared_sub_tests(){
    local num_clients=${1-80}
    local msg_cnt=${2-1000000}
    local qos=${3-0}
    local addnl_variation=${4-""}
    local a_test_bucket=""
    local my_description
    local my_variation
    local my_command

    a_test_bucket="DurableSharedSubs_${n}" ; # n is a global variable incremented in the test_template_ calls
    
    my_variation="SVT_AT_VARIATION_SYNC_CLIENTS=false"
    my_variation+="$addnl_variation"
    my_description="Durable shared subs - Establish durable subs"
    durable_shared_sub_step1 $num_clients "$my_variation" "$my_description" $qos
    my_variation="SVT_AT_VARIATION_MSGCNT=${msg_cnt}|SVT_AT_VARIATION_PCTMSGS=10"
    my_variation+="$addnl_variation"
    my_description="Durable shared subs - Publish to durable subs"
    durable_shared_sub_step2 $num_clients "$my_variation" "$my_description" $qos
    my_variation="SVT_AT_VARIATION_MSGCNT=${msg_cnt}|SVT_AT_VARIATION_PCTMSGS=90"
    my_variation+="$addnl_variation"
    my_description="Durable shared subs - Receive msgs on durable subs"
    durable_shared_sub_step3 $num_clients "$my_variation" "$my_description" $qos

    #---------------------------------------------------------------------------
    # Consolidate and capture all log files for this test transaction
    #---------------------------------------------------------------------------
    my_description="Durable shared subs - cleanup and collect logs"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|connections"
    my_variation="SVT_AUTOMATION_TEST_LOG_TAR=${a_test_bucket}"
    test_template_add_test_all_M_concurrent  "$my_description" "$my_command" 600 "$my_variation" ""

}

launch_shared_sub_tests(){
    local addnl_variation=${1-""}
    local description=""
    local qos=${2-0}
    local variation

    if (($qos<1)); then
        #-------------------------------------------------------------------------
        #
        # Variable Fanout test. on single machine: 999800 / 60 = 16 K unique shared subs, 60 subscribers each
        # 600 messages (10% of total) published to each unique shared sub. Verify that order is correct.
        # Verify that at least 9% of 100 (9) messages are received by each shared sub.
        #
        # Note: on 10 M_COUNT machines, 199960 unique shared subs are created, so the number of
        # machines in the test environment impacts the number of unqiue shared subs, and also the number
        # of messages published to each shared sub.
        #
        #-------------------------------------------------------------------------
        description="Shared Subscription Fanout: scale test thousands
    of individual shared subscriptions"
        variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
        variation+="SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS=10|SVT_AT_VARIATION_PCTMSGS=9"
        do_my_shared_sub_pattern 199800   "$variation|$addnl_variation" "$description"
    fi 

    #----------------------------------------------------
    # Fanin test - 499.8K publish 1 message fanin to 20 Shared subs with M_COUNT consumers each
    #----------------------------------------------------
    description="Shared Subscription Fanin: scale 999.8K publish 1
message fanin to 20 Shared subs with M_COUNT consumers each"
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=90|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=1|"
    variation+="SVT_AT_VARIATION_HA2=|"
    do_my_shared_sub_pattern 499800  "$variation|$addnl_variation" "$description"

    #----------------------------------------------------
    # Fanin test - 499.8K publish 1 message fanin to 20 Shared subs with M_COUNT consumers each  - verbose output
    #----------------------------------------------------
    description="Shared Subscription Fanin: scale 499.8K publish 1
message fanin to 20 Shared subs with M_COUNT consumers each  - verbose output"
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=90|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=1|"
    variation+="SVT_AT_VARIATION_HA2=|"
    variation+="SVT_AT_VARIATION_VERBOSE_PUB=1|SVT_AT_VARIATION_VERBOSE_SUB=1"
    do_my_shared_sub_pattern 499800  "$variation|$addnl_variation" "$description"

    #----------------------------------------------------
    # Fanin test - 5M messages fanin: 499.8K publishers publish 10 message each to 20 Shared subs with M_COUNT consumers each
    #----------------------------------------------------
    description="Shared Subscription Fanin: scale 10M messages fanin:
999.8K publishers publish 10 message each to 20 Shared subs with M_COUNT consumers each"
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=90|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=10|"
    variation+="SVT_AT_VARIATION_HA2=|"
    do_my_shared_sub_pattern 499800   "$variation|$addnl_variation" "$description"

    #----------------------------------------------------
    # Fanin test - 50M messages fanin: 499K publishers publish 100 message each to 20 Shared subs with M_COUNT consumers each
    #----------------------------------------------------
    description="Shared Subscription Fanin: scale 100M messages fanin:
999.8K publishers publish 100 message each to 20 Shared subs with M_COUNT consumers each"
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=90|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=100|"
    variation+="SVT_AT_VARIATION_HA2=|"
    do_my_shared_sub_pattern 499800  "$variation|$addnl_variation" "$description"

    #----------------------------------------------------
    # Fanin test - 50M messages fanin: 50 publishers publish 1 M message each to 20 Shared subs with M_COUNT consumers each
    #----------------------------------------------------
    description="Shared Subscription Fanin: 50M messages fanin: 50 publishers publish 1 M message each to 20 Shared subs with M_COUNT consumers each"
    variation="SVT_AT_VARIATION_TOPIC_MULTI_TEST=scale|SVT_AT_VARIATION_ORDERMSG=2|"
    variation+="SVT_AT_VARIATION_PCTMSGS=90|"
    variation+="SVT_AT_VARIATION_PATTERN=fanin_shared|"
    variation+="SVT_AT_VARIATION_MSGCNT=1000000|"
    variation+="SVT_AT_VARIATION_HA2=|"
    do_my_shared_sub_pattern 50  "$variation|$addnl_variation" "$description"


    durable_shared_sub_tests 80 500000 0
    durable_shared_sub_tests 50000 2 1
    durable_shared_sub_tests 500000 2 2

    #----------------------------------------------------
    # next one can be quite stressful, but it is also unpredictable. Probably best to run manually for now if needed
    #----------------------------------------------------
    #durable_shared_sub_tests 100 5 2 "SVT_AT_VARIATION_INJECT_DISCONNECT=1|SVT_AT_VARIATION_INJECT_DISCONNECTX=1"

}

#-------------------------------------
# Currently no HA tests with shared subs have been added into Automation
#-------------------------------------
do_standalone_tests() { 
    launch_shared_sub_tests
}


#-------------------------------------
# Next line only operates in sandbox environment to prevent accidental file deletes
#-------------------------------------
svt_do_automatic_backup 

#-------------------------------------
# Note: this testcase assumes testcase 10.sh with "do_init" 
# and "do_setup" has already been run, if that is not the case,
# you must do that first.
#-------------------------------------
do_standalone_tests

