#!/bin/bash
#-------------------------------------------------------------------------------
#  MQTTv3 Discard messages scale / load test script
#
#    2/19/14 - Created new test for Discard messages load and scale testing
#-------------------------------------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Discarded Messages QOS 0 test"
source ./svt_test.sh

#-------------------------------------------------------------------------------
# Next line is required if you want to run "test transactions"
#-------------------------------------------------------------------------------
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
export AUTOMATION_FRAMEWORK_TEST_PLAN_URL=https://changeme.jazzserver.example.com:9443/ccm/service/com.ibm.team.workitem.common.internal.rest.IAttachmentRestService/itemName/com.ibm.team.workitem.Attachment/10273

test_template_set_prefix "cmqtt_12_"

typeset -i n=0

#-------------------------------------------------------------------------------
# Function: do_run_sub_variation
#
# This function runs a subscription variation on the IBM MessageSight
# with support for > 1 M subscriptions . For > 999900 clients, subscription commands
# are executed in groups of 999900 each, followed by the balance/remainder of clients.
# 
# Note: All clients in a group need to complete their operations before starting the
# next group, because IBM MessageSight only supports up to 1M concurrently connected clients.
#-------------------------------------------------------------------------------
do_run_sub_variation (){  ############## Supports > 1 M subscriptions #############
    local num_clients=$1
    local workload=$2
    local description=$3
    local a_variation=$4
    local a_timeout=$5
    local my_description=""
    local my_variation=""
    local balance
    local remainder
    local counter 
    local a_command
    local per_client

    if [ -z "$a_timeout" ] ;then
        echo "`date`: ERROR:BUG:a_timeout not specified for do_run_sub_variation, all 5 args required."
        return 1;
    fi 
     
    let balance=$num_clients
    let remainder=($num_clients-999900);
    let counter=0
    while (($remainder>0)); do
        let per_client=(999900/${M_COUNT});
        a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"
        my_description="${description} - for total greater than 1M subs: execute variation for 999000 of $num_clients counter:$counter "
        my_variation="${a_variation}|SVT_AT_VARIATION_UNIQUE_CID=${counter}"
        test_template_add_test_single "$my_description" cAppDriver m1 "${a_command}" "${a_timeout}" "$my_variation" "TEST_RESULT: SUCCESS"
        let balance=$remainder
        let remainder=($balance-999900);
        let counter=$counter+1
    done
    let per_client=($balance/${M_COUNT});
    a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"
    my_description="${description} - execute variation for $balance of $num_clients counter:${counter} "
    my_variation="${a_variation}|SVT_AT_VARIATION_UNIQUE_CID=\\\${SVT_AT_VARIATION_UNIQUE_CID}${counter}"
    test_template_add_test_single "$my_description" cAppDriver m1 "${a_command}" "${a_timeout}" "$my_variation" "TEST_RESULT: SUCCESS"

    return 0;
}

do_discard_msg_tests(){
    local num_clients=${1-100}
    local max_msgs=${2-5000}
    local max_msgs_multiplier=${3-2}
    local max_msgs_adder=${4-0}
    local qos=${5-0}
    local addnl_variation=${6-""}
    local workload=${7-"connections"}
    local a_timeout=${8-3600}
    local a_test_bucket="DiscardedMessages"
    local a_cleanup_bucket="cleanup"
    local a_name="$a_test_bucket num_clients:$num_clients max_msgs:$max_msgs "
    a_name+="max_msgs_multiplier=$max_msgs_multiplier max_msgs_adder=$max_msgs_adder qos:$qos "
    local a_variation="SVT_AT_VARIATION_PORT=17778|SVT_AT_VARIATION_KEEPALIVE=3600"
    local my_variation=""
    local my_description=""
    local my_command=""
    local my_pass="TEST_RESULT: SUCCESS"
    local a_endpoint="SVTcmqttDynamic"
    local a_msgpolicy="SVTcmqttDynamic"
    local per_client;
    local balance
    local remainder
    let per_client=($num_clients/${M_COUNT});
    local a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"
    local mmx
    let mmx=($max_msgs*$max_msgs_multiplier)
    local PubMsgs
    local MinOrder
    local MaxOrder
    let PubMsgs=($max_msgs_multiplier*${max_msgs}+${max_msgs_adder})
    let MinOrder=(${max_msgs_multiplier}*${max_msgs}+${max_msgs_adder}-${max_msgs})
    MaxOrder=$PubMsgs
    a_variation="$a_variation|$addnl_variation"

    #---------------------------------------------------------------------------
    # Start test transaction. All tests until end of transaction must pass or the test will be quickly aborted.
    #---------------------------------------------------------------------------
    my_description="${a_name} -Start a test transaction "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_start|na|1|${A1_IPv4_1}|${A2_IPv4_1}"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "$a_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Setup MaxMessages on the messaging policy to input value on Dynamic endpoint
    #---------------------------------------------------------------------------
    my_description="${a_name} - configure endpoint:$endpoint MsgPolicy:$msg_policy to MaxMessages: $max_msgs "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|dynamic_endpoint_17778|na|1|${A1_HOST}"
    my_variation="SVT_AT_VARIATION_17778_MAX_MSGS=${max_msgs}"
    test_template_add_test_single  "$my_description" cAppDriver m1 "$my_command" 600 "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Start 4 additional slow background subscribers per machine - 1 msg/sec (additional load generation)
    #---------------------------------------------------------------------------
    my_description="${a_name} - Start 4 additional slow background subscribers per machine - 1 msg per sec "
    my_variation="SVT_AT_VARIATION_MSGCNT=100000000000000|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
    my_variation+="SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_QUICK_EXIT=true|"
    my_variation+="SVT_AT_VARIATION_SUBRATE=1|"
    my_variation+="SVT_AT_VARIATION_ORDERMSG=2|SVT_AT_VARIATION_NO_TEST_CRITERIA=true|"
    my_variation+="SVT_AT_VARIATION_UNIQUE_CID=bc0|"
    my_variation+="SVT_AT_VARIATION_UNIQUE=background.load|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|4|1|${A1_IPv4_1}"
    test_template_add_test_single "$my_description" cAppDriver m1 "${my_command}" "${a_timeout}" "$my_variation" "$my_pass"


    #---------------------------------------------------------------------------
    # Start 4 additional fast background subscribers max msg/sec (additional load generation)
    #---------------------------------------------------------------------------
    my_description="${a_name} - start 4 additional fast background subscribers "
    my_variation="SVT_AT_VARIATION_MSGCNT=100000000000000|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
    my_variation+="SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_QUICK_EXIT=true|"
    my_variation+="SVT_AT_VARIATION_SUBRATE=0|"
    my_variation+="SVT_AT_VARIATION_ORDERMSG=2|SVT_AT_VARIATION_NO_TEST_CRITERIA=true|"
    my_variation+="SVT_AT_VARIATION_51598=true|"
    my_variation+="SVT_AT_VARIATION_UNIQUE_CID=bc1|"
    my_variation+="SVT_AT_VARIATION_UNIQUE=background.load|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|4|1|${A1_IPv4_1}"
    test_template_add_test_single "$my_description" cAppDriver m1 "${my_command}" "${a_timeout}" "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Start 4 additional fast background publishers (additional load generation)
    #---------------------------------------------------------------------------
    my_description="${a_name} - start 4 additional fast background publishers "
    my_variation="SVT_AT_VARIATION_MSGCNT=100000000000000|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
    my_variation+="SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_QUICK_EXIT=true|"
    my_variation+="SVT_AT_VARIATION_PUBRATE=0|"
    my_variation+="SVT_AT_VARIATION_ORDERMSG=2|SVT_AT_VARIATION_NO_TEST_CRITERIA=true|"
    my_variation+="SVT_AT_VARIATION_51598=true|"
    my_variation+="SVT_AT_VARIATION_UNIQUE_CID=bp1|"
    my_variation+="SVT_AT_VARIATION_UNIQUE=background.load|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|4|1|${A1_IPv4_1}"
    test_template_add_test_single "$my_description" cAppDriver m1 "${my_command}" "${a_timeout}" "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Start 20 fast background subscribers max msg/sec (additional load generation)
    #---------------------------------------------------------------------------
    my_description="${a_name} - start additional background testing and defect verification"
    my_variation="SVT_AT_VARIATION_MSGCNT=${PubMsgs}|"
    my_variation+="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true|"
    my_variation+="SVT_AT_VARIATION_PORT=17778|"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|51598_additional_test|na|1|${A1_IPv4_1}"
    test_template_add_test_single  "$my_description" cAppDriver m1 "${my_command}" "${a_timeout}" "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Establish durable subscriptions w/ support for > 1M subs
    #---------------------------------------------------------------------------
    my_description="${a_name} - establish durable subs "
    my_variation="SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_MSGCNT=0|"
    my_variation+="SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
    my_variation+="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    do_run_sub_variation $num_clients "$workload" "$my_description" "$my_variation" "${a_timeout}"

    #---------------------------------------------------------------------------
    # Publish PubMsgs a multiple of MaxMessages
    #---------------------------------------------------------------------------
    my_description="${a_name} - publish $PubMsgs msgs with MaxMessages set to $max_msgs "
    my_variation="SVT_AT_VARIATION_ORDERMSG=2|SVT_AT_VARIATION_CLEANSESSION=false|"
    my_variation+="SVT_AT_VARIATION_PUBQOS=$qos|SVT_AT_VARIATION_MSGCNT=${PubMsgs}|"
    my_variation+="SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|SVT_AT_VARIATION_PUBRATE=0|"
    my_variation+="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    test_template_add_test_single  "$my_description" cAppDriver m1 "${a_command}" "${a_timeout}" "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Print stats out before receive
    #---------------------------------------------------------------------------
    my_description="${a_name} - print out stats on appliance before receive"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|print_stats|na|1|${A1_HOST}"
    my_variation="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Start subscribers to receive discarded messages MaxMessages - x(multiplier) MaxMessages, and unsubscribe to clear durable subscription
    #---------------------------------------------------------------------------
    my_variation="SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_MSGCNT=${max_msgs}|" 
    my_variation+="SVT_AT_VARIATION_QOS=$qos|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
    my_variation+="SVT_AT_VARIATION_PCTMSGS=95|SVT_AT_VARIATION_ORDERMSG=2|"
    my_variation+="SVT_AT_VARIATION_ORDERMIN=${MinOrder}|SVT_AT_VARIATION_ORDERMAX=${MaxOrder}|"
    my_variation+="SVT_AT_VARIATION_VERIFYSTILLACTIVE=30|SVT_AT_VARIATION_UNSUBSCRIBE=1|"
    my_variation+="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true|"
    my_variation+="${a_variation}|SVT_AT_VARIATION_ALL_M=true"
    my_description="${a_name} - receive discarded messages ordered through $MinOrder to $MaxOrder in background "
    do_run_sub_variation $num_clients "$workload" "$my_description" "$my_variation" "${a_timeout}"

    #---------------------------------------------------------------------------
    # Print stats out after receive
    #---------------------------------------------------------------------------
    my_description="${a_name} - print out stats on appliance after receive "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|print_stats|na|1|${A1_HOST}"
    my_variation="SVT_AT_VARIATION_SKIP_LOG_PROCESSING=true|"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "$my_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Stop all processes  - including bg files 
    #---------------------------------------------------------------------------
    my_description="${a_name} - stop all processing "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|$workload"
    my_variation="SVT_AT_VARIATION_TT_PREFIX=$test_template_prefix"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "$my_variation" ""


    #---------------------------------------------------------------------------
    # Verify that all the SVT_TEST_CRITERIA results were successfully met
    #---------------------------------------------------------------------------
    my_description="${a_name} - verify results "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|verify_svt_criteria|${test_template_prefix}|1"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "" "$my_pass"

    #---------------------------------------------------------------------------
    # Cleanup subscriptions - only run when the transaction is failing 
    #---------------------------------------------------------------------------
    my_description="${a_name} - cleanup the subscriptions if needed during failing test transaction "
    my_variation="SVT_AT_VARIATION_CLEANSESSION=true|SVT_AT_VARIATION_MSGCNT=0|"
    my_variation+="SVT_AT_VARIATION_SKIP_PUBLISH=1|${a_variation}|"
    my_variation+="SVT_AT_VARIATION_RUN_ONLY_ON_TRANSACTION_FAILURE=true|"
    my_variation+="SVT_AT_VARIATION_ALL_M=true"
    do_run_sub_variation $num_clients "$workload" "$my_description" "$my_variation" "${a_timeout}"

    #---------------------------------------------------------------------------
    # End the test transaction
    #---------------------------------------------------------------------------
    my_description="${a_name} - End a test transaction "
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_end|na|1|${A1_IPv4_1}|${A2_IPv4_1}"
    test_template_add_test_single "$my_description" cAppDriver m1 "$my_command" 600 "$a_variation" "$my_pass"

    #---------------------------------------------------------------------------
    # Consolidate and capture all log files for this test transaction
    #---------------------------------------------------------------------------
    my_description="${a_name} - collect logs"
    my_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|collectlogs|$workload" 
    my_variation="SVT_AUTOMATION_TEST_LOG_TAR=${a_test_bucket}_${n}"

    test_template_add_test_all_M_concurrent "$my_description" "$my_command" 600 "$my_variation" ""


}

declare -a myarray=( 
#---------------------------------------------------------------------------
#connections multiplier maxmsg   adder   qos  pubtype
#                                             0 : 20 topics
#      N                                      1 :  N topics
#---------------------------------------------------------------------------
       1        2        1        0        0        0
       1        2        1        0        0        0
       1        2        1        0        1        0
       1        2        1        0        2        0
       2   100000        1        0        0        0
       5        2        1        0        1        0
       9        2        1        0        2        0
      19        2        1        0        1        0
  999900        2       10        0        0        0
 1999800        9       10        1        0        0
  999900        2       10        1        1        0
      10        1  1000000     1323        0        0
       1        1 20000000   999380        0        0
#-----------------------------------------------------
# Rest of tests may be too stressful for SVT automation infrastructure, although,
# these tests all passed on my own hardware. On SVT automation systems they slowed
# the network and caused problems w/ FVT tests . Long term solution would be to 
# separate FVT / SVT tests on different switches and/or vlans
#-----------------------------------------------------
  999900        2       10       -1        2        0
 1999800        5       10       -1        1        0
 1999800        3       10        3        2        0
  100000     9998        2        1        0        0
  100000      100        3        0        0        0
  100000     9900        4        1        0        0
  100000     8888        5        2        0        0
  100000     2222        7        3        0        0
  100000      333       10        1        0        0
  100000      344       15       -1        0        0
  100000      222       20        5        0        0
  100000       13       25       10        0        0
  100000      124       50       -2        0        0
  100000        2       51        1        1        0
  100000        2       52       -9        2        0
  100000       12      100        1        0        0
   10000        9      500        0        0        0
   10000        8     1000        0        0        0
   10000        2     1250        0        0        0
   10000        2     2000        0        0        0
   10000        2     5000     4999        0        0
    1000        2   100000        1        0        0
     100        2   500000     9999        0        0
      10        1  1000000     1323        0        0
    NULL     NULL     NULL     NULL     NULL     NULL
    NULL     NULL     NULL     NULL     NULL     NULL
    NULL     NULL     NULL     NULL     NULL     NULL
    NULL     NULL     NULL     NULL     NULL     NULL
#####################################################
#   NULL     NULL     NULL     NULL     NULL        0
#     50       20   500000    51323        0        0
#   NULL     NULL     NULL     NULL     NULL        0
#   NULL     NULL     NULL     NULL     NULL        0
#   NULL     NULL     NULL     NULL     NULL        0
#6999300        2       10       31        2        0
#   NULL     NULL     NULL     NULL     NULL        0
#7999200        2       10       31        2        0 #bt.tcpiop, bt.reconnect(out of nvdimm)
#   NULL     NULL     NULL     NULL     NULL        0
#8999100        2       10       31        2        0
#9999000        2       10       31        2        0 #bt.tcpiop, bt.reconnec (out of nvdimm)
#   NULL     NULL     NULL     NULL     NULL        0
#4999500        2       10       31        2        0
#   NULL     NULL     NULL     NULL     NULL        0
#9999000        2       10       31        2        0 #bt.tcpiop, bt.reconnect (out of nvdimm)
#   NULL     NULL     NULL     NULL     NULL        0
#1999800        2       10       31        0        0
#   NULL     NULL     NULL     NULL     NULL        0
#####################################################
);

COLWIDTH=6

GUARANTEED_TESTS=12 ; # first 13 tests will run
RANDOM_TRUTH=1000000000     ; # rest of tests 13 - N will each have a 1 in 1000000000 chance of running

let v=0;
let t=1;
while [ "${myarray[$v]}" != "NULL" ]; do
    if (($t>$GUARANTEED_TESTS));  then
        if svt_check_random_truth $RANDOM_TRUTH ; then
            echo "This test $t was randomly selected to run"
        else
            echo "This test $t was skipped"
            let t=$t+1
            let v=$v+COLWIDTH;
            continue;
        fi
    else
        echo "This test $t is part of base GUARANTEED_TESTS "
    fi
    #---------------------------------------------------------------------------
    # Need to lower the number of connections as the number of maxmessages go up or we will blow things away
    #---------------------------------------------------------------------------
    let Pubmsgs=(${myarray[1+$v]}*${myarray[2+$v]}+${myarray[3+$v]})
    echo "Pubmsgs= ${myarray[1+$v]} * ${myarray[2+$v]} + ${myarray[3+$v]}  = ${Pubmsgs} "
    let MinOrder=(${myarray[1+$v]}*${myarray[2+$v]}+${myarray[3+$v]}-${myarray[2+$v]})
    echo "MinOrder  = $MinOrder "
    echo "MaxOrder  = $Pubmsgs (Pubmsgs)"
    let totalbytes=(${myarray[$v]}*32*${Pubmsgs}*$M_COUNT)
    echo "Test Parameters: throughput:$totalbytes with conncount:${myarray[$v]}, "
    echo " ... continued: msgsize:32 and maxmsgarray: ${myarray[2+$v]} multiplier=${myarray[1+$v]} adder=${myarray[3+$v]}"
    if (($MinOrder<0)); then
        #---------------------------------------------------------------------------
        # Skip test - parameter issue.
        #---------------------------------------------------------------------------
        echo "ERROR: Invalid Test Parameter: Skipping Test: do_discard_msg_tests ${myarray[$v]} ${myarray[2+$v]} ${myarray[1+$v]} ${myarray[3+$v]} ${myarray[4+$v]}  "
        echo "ERROR: Invalid Test Parameter: MinOrder is < 0 : $MinOrder " 
    elif (($Pubmsgs<0)); then
        #---------------------------------------------------------------------------
        # Skip test - parameter issue.
        #---------------------------------------------------------------------------
        echo "ERROR: Invalid Test Parameter: Skipping Test: do_discard_msg_tests ${myarray[$v]} ${myarray[2+$v]} ${myarray[1+$v]} ${myarray[3+$v]} ${myarray[4+$v]}  "
        echo "ERROR: Invalid Test Parameter: Pubmsgs is < 0 : $Pubmsgs " 
    else 
        #---------------------------------------------------------------------------
        # Execute test
        #---------------------------------------------------------------------------
        echo "Test: do_discard_msg_tests ${myarray[$v]} ${myarray[2+$v]} ${myarray[1+$v]} ${myarray[3+$v]} ${myarray[4+$v]}  "
        do_discard_msg_tests ${myarray[$v]} ${myarray[2+$v]} ${myarray[1+$v]} ${myarray[3+$v]} ${myarray[4+$v]}
        
    fi 
    let t=$t+1
    let v=$v+COLWIDTH;
done
