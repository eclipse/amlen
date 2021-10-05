#!/bin/bash
#----------------------------------------------------
#  MQTTv3 scale test script - load qos 1 clean=false with 100K msg/sec bursts with 100K connections 10 minutes
#
#   9/24/14 - Created
#
#----------------------------------------------------

source ./svt_test.sh
scenario_set_name="MQTTv3 Scale Test workload qos 1 clean false with 100K msg per sec bursts with 100K connections 10 minutes"
MY_SCENARIO=50
typeset -i n=0

num_client=99000
num_msg=240 # approximately 10 minutes of test runtime.
#num_msg=20 #

svt_common_init_ha_test $MY_SCENARIO

a_variation="SVT_AT_VARIATION_QOS=1|SVT_AT_VARIATION_MSGCNT=$num_msg|SVT_AT_VARIATION_PUBRATE=0.04|"
a_variation+="SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_CLEANSESSION=false|"
a_variation+="SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_PUBQOS=1|"
a_variation+="SVT_AT_VARIATION_VERIFYSTILLACTIVE=60|"
a_variation+="SVT_AT_VARIATION_FAILOVER=|" # must be STOP, DEVICE, POWER , FORCE,  etc... to inject failover during test
a_variation+="SVT_AT_VARIATION_FAILOVER_SLEEP=|" # must be set to seconds to sleep after a failover to allow messaging to resume
a_variation+="SVT_AT_VARIATION_WAITFORCOMPLETEMODE=1|SVT_AT_VARIATION_86WAIT4COMPLETION=1|"
a_variation+="SVT_AT_VARIATION_HA=${A2_IPv4_1}|SVT_AT_VARIATION_QUICK_EXIT=true|"
a_variation+="SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT=120|SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_SUBONCON=1|"
a_variation+="SVT_AT_VARIATION_RESOURCE_MONITOR=false|"
a_variation+="SVT_AT_VARIATION_NOW=${SVT_AT_SCENARIO_8}|"  # required when using SVT_AT_VARIATION_RESUME=true flag
a_variation+="SVT_AT_VARIATION_RESUME=true|" # flag tells svt_common_launch_scale_test to resume the test when SVT_AT_VARIATION_QUICK_EXIT=true was used

export SVT_LAUNCH_SCALE_TEST_DESCRIPTION="unspecified"
svt_common_launch_scale_test "connections" $num_client "$a_variation" 0

