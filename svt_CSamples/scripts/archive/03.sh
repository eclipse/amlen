#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   11/26/12 - Refactored ism-MQTT_ALL_Sample-runScenario01.sh into 01.sh
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv3 Connection workload"
source ../scripts/commonFunctions.sh

test_template_set_prefix "cmqtt_03_"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#		m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#		
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController 
#		The configuration file to drive the test case using this controller.
#	<OTHER_OPTS>	is used when configuration file may be over kill,
#			parameters are passed as is and are processed by the subController.
#			However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#	component[x]=sync,<machineNumber_ismSetup>,
#	where:
#		<m1>			is the machine 1
#		<m2>			is the machine 2
#
#   Sleep:
#	component[x]=sleep,<seconds>
#	where:
#		<seconds>	is the number of additional seconds to wait before the next component is started.
#


declare -a test_array=(

    #---------------------------------------------------------
    # Test bucket SVT_01: 1 Million MQTTv3 connection unsecure (3 qos 0 messages to each connection - 3 Milllion total messages)
    #---------------------------------------------------------
    SVT_01 1800 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=3|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SUBSCRIBE_SYNC=true" 
        "1 Million MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01a: variation - same as Test bucket 1 , but now with qos 1 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01a 1800
        "SVT_AT_VARIATION_QOS=1|SVT_AT_VARIATION_MSGCNT=3|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SUBSCRIBE_SYNC=true" 
        "1 Million MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01b: variation - same as Test bucket 1 , but now with qos 2 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01b 1800 
        "SVT_AT_VARIATION_QOS=2|SVT_AT_VARIATION_MSGCNT=3|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SUBSCRIBE_SYNC=true"
        "1 Million MQTTv3 connection workload"
    #---------------------------------------------------------
    # Test bucket SVT_01c: variation - same as Test bucket 1 , but now with qos 0, 100 msgs/client, in bursts of 10 every 20 sec
    #---------------------------------------------------------
    SVT_01c 3600 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=100|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SUBSCRIBE_SYNC=true|SVT_AT_VARIATION_BURST=10" 
        "1 Million MQTTv3 connection workload"

    #---------------------------------------------------------
    # End of test array, do not place anything after this comment
    #---------------------------------------------------------
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
)
TEST_ARRAY_COLS=4

launch_scale_test() {
    local workload=${1-"connections"}
    local num_clients=${2-998000}
    local additional_variation=${3-""}
    local additional_timeout=${4-0}
    local appliance_monitor="m1"; # appliance monitoring will run on this system by default.
    local per_client
    local a_variation=""
    local a_name=""
    local a_timeout=""
    local a_test_bucket=""
    local a_ha_test=""
    local regexp

#    echo "------------1--------------"
#    echo "workload is $workload"
#    echo "------------3--------------"
#    echo $3
#    echo $additional_variation
#    echo "------------3--------------"
    
    let per_client=($num_clients/${M_COUNT});
    a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"
    let j=0
    while [ "${test_array[${j}]}" != "NULL" ] ; do
        #echo "j is $j ${test_array[$j]} ${test_array[$j+1]} ${test_array[$j+2]} ${test_array[$j+3]}"
        let a_timeout=(${test_array[$j+1]}+${additional_timeout});
        a_test_bucket="${test_array[$j]}_${n}"  ; # n is a global variable incremented in the test_template_ calls 
        a_variation="${test_array[$j+2]}"
        if [ -n "$additional_variation" ] ; then
            a_variation+="|$additional_variation"
        fi
        a_name="${test_array[$j+3]}  - run ${per_client} ${workload} per client "       

        echo "a_test_bucket is $a_test_bucket"
        echo "a_timeout is $a_timeout"
        echo "a_variation is $a_variation"
        echo "a_name is $a_name"

        if (($M_COUNT>=2)); then
            appliance_monitor="m2"; # appliance monitoring will run on this system to distribute work more evenly
        fi
        test_template_add_test_single "${a_name} - start primary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_primary_appliance|${workload}|1|${A1_HOST}" 
	    if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
            test_template_add_test_single "${a_name} - start secondary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_secondary_appliance|${workload}|1|${A1_HOST}|${A2_HOST}" 
        fi 

        #FIXME test_template_add_test_all_M_concurrent "${a_name} - start client monitoring " "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_client|${workload}|1|" 

        #---------------------------------------
        # New way on 6.26.2013 - should be faster  - not working yet due to not cd'ing to cwd when ssh ing command
        #---------------------------------------
        test_template_add_test_single "${a_name} - start client monitoring " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_client|${workload}|1|" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
        #---------------------------------------


        #------------------------------------------------
        # Added next 5 lines for RTC 34564 - in order to turn this off you must set it to false , default - on 
        #------------------------------------------------
        regexp="SVT_AT_VARIATION_RESOURCE_MONITOR=false"
        if ! [[ $a_variation =~ $regexp ]] ; then
            test_template_add_test_single "${a_name} - start \$SYS/ResourceMonitor/# topic monitoring  " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|resource_topic_monitor|10|1|${A1_IPv4_1}|${A2_IPv4_1}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
        fi

        test_template_add_test_all_M_concurrent  "${a_name} - run workload variation: ${a_variation} " "${a_command}" "${a_timeout}" "${a_variation}" "TEST_RESULT: SUCCESS"

	    if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
            regexp="SVT_AT_VARIATION_HA="
            if [[ $a_variation =~ $regexp ]] ; then

                #-------------------------------------------------
                # 6.27.13 - Basic ha test suite - 
                #-------------------------------------------------
                test_template_add_test_single "SVT - HA test: stop Primary" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|stopPrimary|-f|/dev/stdout|" 600 "" "Test result is Success"
#
                test_template_add_test_single "SVT - HA test: start Standby" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|startStandby|-f|/dev/stdout|" 600 "" "Test result is Success"

                test_template_add_test_sleep 600 ; # need to allow some time for existing 1 M client workload to reconnect and do messaging

                #-------------------------------------------------
                # 6.27.13 - Basic ha test suite - add 10 random HA failover tests, sleep 300 seconds after each test to allow processing
                # to occur such as receiving messages and reconnecting. Types of failovers currently are limited to DEVICE , KILL, STOP 
                #
                # 6.27.13 - Current problem with below is going into split brain state..... (on kill primary ... i think
                #-------------------------------------------------

                #. ./ha_test.sh ${A1_IPv4_1} ${A2_IPv4_1}
                #svt_ha_clr_random_test
                #for v in {0..9} ; do
                    #svt_ha_gen_random_test $v
                    #a_ha_test=`svt_ha_get_random_test $v`
#
                    #test_template_add_test_single "SVT - HighAvailability test - run random test variation: ${a_ha_test}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_ha_run_random_test|$v|1|${A1_IPv4_1}|${A2_IPv4_1}|" 3600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
                    #regexp="PRIMARY"
                    #if [[ $a_ha_test =~ $regexp ]] ;then
                        ##-----------------------------------------
                        ## If there was a failover more time is needed
                        ##-----------------------------------------
                        #test_template_add_test_sleep 60 ; # need to allow some time for existing 1 M client workload to reconnect and do messaging
                    #fi
                #done
                

                #-------------------------------------------------
                # 6.26.13 - Basic ha test suite - w/ haFunctions.sh  - this worked.
                #-------------------------------------------------

                #test_template_add_test_single "SVT - HA test: stop Primary" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|stopPrimary|-f|/dev/stdout|" 600 "" "Test result is Success"
#
                #test_template_add_test_single "SVT - HA test: start Standby" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|startStandby|-f|/dev/stdout|" 600 "" "Test result is Success"

                #-------------------------------------------------
                # 6.26.13 - Basic ha test suite -  not working either
                #-------------------------------------------------
                #export SVT_AT_VARIATION_HA_TESTCASE="svt_ha_test_X_disrupt_server_on_Y KILL PRIMARY"
                #test_template_add_test_single "${a_name} - HA test: ${SVT_AT_VARIATION_HA_TESTCASE}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|ha_test|killprimary|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

                #-------------------------------------------------
                # 6.26.13 - Basic ha test suite - not working either because of spaces
                #-------------------------------------------------
                #a_ha_test="svt_ha_test_X_disrupt_server_on_Y KILL PRIMARY"
                #test_template_add_test_single "${a_name} - HA test: ${a_ha_test}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|ha_test|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "SVT_AT_VARIATION_HA_TESTCASE=\"${a_ha_test}\"|SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
#
##                a_ha_test="svt_ha_test_X_disrupt_server_on_Y STOP PRIMARY"
##                a_ha_test="svt_ha_test_X_disrupt_server_on_Y DEVICE PRIMARY"
###
#                a_ha_test="svt_ha_test_X_disrupt_server_on_Y KILL STANDBY"
            
                #-------------------------------------------------
                # 6.26.13 - The following randomized testing is not working because it is done in a subshell (no_name_00)
                #-------------------------------------------------
                #. ./ha_test.sh ${A1_IPv4_1} ${A2_IPv4_1} 600
                #{  svt_ha_print_tests |grep -v NETWORK | grep -v SPLITBRAIN |  \
                #grep -v SIGSEGV;   svt_ha_print_tests |grep -E 'NETWORK|SPLITBRAIN' | \
                #sort -R | head -5 ; } | sort -R | head -10 | while read a_ha_test; do 
                        ##echo $a_ha_test ;
                        #test_template_add_test_single "${a_name} - HA test: ${a_ha_test}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|ha_test|${a_ha_test}|1|${A1_IPv4_1}|${A2_IPv4_1}" 3600 "SVT_AT_VARIATION_QUICK_EXIT=\"${a_ha_test}\"|SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
                #done

            else 
                echo "No ha testing scheduled at this time (A)"
            fi 
        else
            echo "No ha testing scheduled at this time (B)"
        fi

        test_template_add_test_all_M_concurrent  "${a_name} - cleanup" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${workload}" 600 SVT_AUTOMATION_TEST_LOG_TAR=$a_test_bucket ""
        let j=$j+$TEST_ARRAY_COLS

        regexp="SVT_AT_VARIATION_SINGLE_LOOP=true"
        if [[ $a_variation =~ $regexp ]] ; then
            echo "Breaking after single loop since SVT_AT_VARIATION_SINGLE_LOOP=true"
            break;
        fi
    done
}


do_init() {
    local appliance=${1-"${A1_IPv4_1}"}
    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Notification that the test has started )
    #---------------------------------------------------------
    test_template_add_test_single "1 Million MQTTv3 unsecure connection workload - notification" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|email_notifcation|na|1|${appliance}" 600 "" "TEST_RESULT: SUCCESS"
}

do_setup() {
    local appliance=${1-"${A1_HOST}"}

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Virtual Nic setup on server)
    #---------------------------------------------------------
    test_template_add_test_single "1 Million MQTTv3 unsecure connection workload - setup server for ${appliance}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|SERVER|1|${appliance}" 300 "" "TEST_RESULT: SUCCESS"

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (sysctl, Virtual Nic setup on client)
    #---------------------------------------------------------
    #FIXME test_template_add_test_all_M_concurrent "1 Million MQTTv3 unsecure connection workload - setup client for ${appliance}" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|CLIENT|1|${appliance}" 600 "" "TEST_RESULT: SUCCESS"

    test_template_add_test_single "1 Million MQTTv3 unsecure connection workload - setup client for ${appliance}" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|CLIENT|1|${appliance}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"

}



num_clients=998000
#num_clients=29800

do_ha_sandbox_launch() {

    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_HA=${A2_IPv4_1}|SVT_AT_VARIATION_QUICK_EXIT=true"

    echo "nothing in the ha sandbox"
}

do_ha_main_launch() {
    local ha_variation="SVT_AT_VARIATION_PUBRATE=0.008|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_HA=${A2_IPv4_1}|SVT_AT_VARIATION_QUICK_EXIT=true|SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT=120|SVT_AT_VARIATION_SINGLE_LOOP=true"

    launch_scale_test "connections" $num_clients                      "${ha_variation}" 0
    launch_scale_test "connections" $num_clients                      "${ha_variation}|SVT_AT_VARIATION_RESOURCE_MONITOR=false" 0 ; # RTC 34564
	
	num_clients=100000 ;
    launch_scale_test "connections_ldap" $num_clients                 "${ha_variation}" 1800
    launch_scale_test "connections_ldap_ssl" $num_clients             "${ha_variation}" 3600
    launch_scale_test "connections_ssl" $num_clients                  "${ha_variation}" 1800
    launch_scale_test "connections_client_certificate" $num_clients   "${ha_variation} " 1800
}


do_ha_tests() {
	#-----------------------------------
	# SVT HA tests
	#-----------------------------------
	if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
	
    	do_setup ${A2_HOST} # make sure backup is setup too.
    
       	test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|setupHA|-f|/dev/stdout|" 600 "" "Test result is Success"
	
    	#-----------------------------------
    	# Start my SVT HA enabled tests
    	#-----------------------------------
		do_ha_main_launch
	
    	#-----------------------------------
    	# End my SVT HA enabled tests
    	#-----------------------------------
	
       	test_template_add_test_single "SVT - cleanup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|disableHA|-f|/dev/stdout" 600 "" "Test result is Success"
	
	fi
}

do_sandbox_launch() {
    local num_clients=998000

    launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=100000|SVT_AT_VARIATION_BURST=10" 115000

    #--------------------------------------

    #--------------------------------------
    # Next test - defect 34597
    #--------------------------------------
    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=100"

    #--------------------------------------
    # Next test - not sure - will it recreate the memory leak with SVT_AT_VARIATION_SUBSCRIBE_SYNC=
    # The memory leak seemed to happen while the clients were taking a long time to connect due to heavy load.
    # by not syncing before starting to publish a heavy load is created which causes the rest of the non-connected
    # clients to take longer to connect which is when the extreme loss of memory was observed ~ 1 GB of memory lost every 5 seconds.
    #--------------------------------------
    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=100|SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=15|SVT_AT_VARIATION_SUBSCRIBE_SYNC="
    
    #--------------------------------------
    # Next test worked well. Created 1 M tall message bumps every 20 seconds.
    #--------------------------------------
    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=100|SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=5|SVT_AT_VARIATION_BURST=10" 300
    
    #--------------------------------------
    # Next test worked well. Created 3 M tall message bumps every 90 seconds. Messaging part alone takes 15 minutes or so.
    #--------------------------------------
    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0.01|SVT_AT_VARIATION_MSGCNT=500|SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=5|SVT_AT_VARIATION_BURST=50" 1800
    
}

do_standalone_tests() {
    launch_scale_test "connections" $num_clients                      " " 0
    launch_scale_test "connections" $num_clients                      "SVT_AT_VARIATION_RESOURCE_MONITOR=true" 0 ; # RTC 34564
    launch_scale_test "connections_ldap" $num_clients                 " " 1800
    launch_scale_test "connections_ldap_ssl" $num_clients             " " 3600
    launch_scale_test "connections_ssl" $num_clients                  " " 1800
    launch_scale_test "connections_client_certificate" $num_clients   " " 1800
}

do_init
do_setup ${A1_HOST}
do_standalone_tests
do_ha_tests


#do_sandbox_launch
