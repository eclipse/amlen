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

test_template_set_prefix "cmqtt_18_"

typeset -i n=0

do_my_tests(){

    a_variation=""

    #---------------------------------------------
    # Start test transaction. All tests until end of transaction must pass or the test will be quickly aborted.
    #---------------------------------------------
    test_template_add_test_single "${a_name} -Start a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_start|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # End the test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - End a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_end|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "$a_variation" "TEST_RESULT: SUCCESS"

        
}


#------------------------
# Execute MQ / HA resilience test
#------------------------
do_my_tests

