#!/bin/bash

#----------------------------------------------------
#  MQTTv3 scale test script
#
#   11/26/12 - Refactored ism-MQTT_ALL_Sample-runScenario01.sh into 01.sh
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTTv3 Connection workload for SoftLayer"
source ./svt_test.sh

#------------------------------------------
# Next line is required if you want to run "test transactions"
#------------------------------------------
export AUTOMATION_FRAMEWORK_SAVE_RC="/niagara/test/svt_cmqtt/.svt.af.runscenario.last.rc"

test_template_set_prefix "cmqtt_10_"

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

#------------------------------------
# The next variable can be used to speed up the 
# automation run by only randomly running some of the tests.
#------------------------------------
SVT_RANDOM_EXECUTION=1 ; #  2 means only 50 % of the tests will run, unless SVT_AT_VARIATION_SINGLE_LOOP=true set to 1 to make all tests run


declare -a test_array=(

    #---------------------------------------------------------
    # Test bucket SVT_01: Scale MQTTv3 connection (3 qos 0 messages to each connection - 3 Milllion total messages)
    #---------------------------------------------------------
    SVT_01a 1800 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_BURST=2|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01a: variation - same as Test bucket 1 , but now with qos 1 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01b 1800
        "SVT_AT_VARIATION_QOS=1|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_BURST=2|SVT_AT_VARIATION_PUBQOS=1|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01b: variation - same as Test bucket 1 , but now with qos 2 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01c 1800 
        "SVT_AT_VARIATION_QOS=2|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_BURST=2|SVT_AT_VARIATION_PUBQOS=2|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60"
        "Scale MQTTv3 connection workload"
    #---------------------------------------------------------
    # Test bucket SVT_01c: variation - same as Test bucket 1 , but now with qos 0, 100 msgs/client, in bursts of 10 every 20 sec
    #---------------------------------------------------------
    SVT_01d 3600 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # End of test array, do not place anything after this comment
    #---------------------------------------------------------
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
)
TEST_ARRAY_COLS=4


LAUNCH_SCALE_TEST_DESCRIPTION=

launch_scale_test() {
    local workload=${1-"connections"}
#    local num_clients=${2-998000}
    local num_clients=${2-300}
    local additional_variation=${3-""}
    local additional_timeout=${4-0}
    local appliance_monitor="m1"; # appliance monitoring will run on this system by default.
    local per_client
    local a_variation=""
    local a_name=""
    local a_timeout=""
    local a_test_bucket=""
    local a_ha_test=""
    local repeat_count=0;
    local regexp

    local chance_of_test_running=$SVT_RANDOM_EXECUTION ;

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
        #let a_timeout=600
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

        if [ -n "$SVT_RANDOM_EXECUTION" ] ; then
            if ! svt_check_random_truth $chance_of_test_running ; then
                regexp="SVT_AT_VARIATION_SINGLE_LOOP=true"
                if [[ $a_variation =~ $regexp ]] ; then
                    echo "Continuing test because SVT_AT_VARIATION_SINGLE_LOOP=true. "
                else
                    echo "Skipping this test, it was not selected for Random Execution."
                    let j=$j+$TEST_ARRAY_COLS
                    continue;
                fi
            else 
                echo "Running test, it was selected for Random Execution, or has SVT_AT_VARIATION_SINGLE_LOOP=true."
            fi
        fi

        #if (($M_COUNT>=2)); then
            #appliance_monitor="m2"; # appliance monitoring will run on this system to distribute work more evenly
        #fi

        #regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        #if ! [[ $a_variation =~ $regexp ]] ; then
            #test_template_add_test_single "${a_name} - start primary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_primary_appliance|${workload}|1|${A1_IPv4_1}" 
	        #if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
                #test_template_add_test_single "${a_name} - start secondary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_secondary_appliance|${workload}|1|${A1_IPv4_1}|${A2_IPv4_1}" 
            #fi 
        #fi 

        #---------------------------------------
        # New way on 6.26.2013 - should be faster  - not working yet due to not cd'ing to cwd when ssh ing command
        #---------------------------------------
        #regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        #if ! [[ $a_variation =~ $regexp ]] ; then
            #test_template_add_test_single "${a_name} - start client monitoring " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_client|${workload}|1|" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
        #fi
        #---------------------------------------


        #------------------------------------------------
        # Added next 5 lines for RTC 34564 - in order to turn this off you must set it to false , default - on 
        #------------------------------------------------
        regexp="SVT_AT_VARIATION_RESOURCE_MONITOR=false"
        if ! [[ $a_variation =~ $regexp ]] ; then
            regexp="SVT_AT_VARIATION_TEST_ONLY=true"
            if ! [[ $a_variation =~ $regexp ]] ; then
                test_template_add_test_single "${a_name} - start \$SYS/ResourceMonitor/# topic monitoring  " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|resource_topic_monitor|10|1|${A1_IPv4_1}|${A2_IPv4_1}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
            fi
        fi


        regexp="SVT_AT_VARIATION_PUB_THROTTLE=true"
        if [[ $a_variation =~ $regexp ]] ; then
            test_template_add_test_single "${a_name} - start pub throttle  " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|pub_throttle|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
            
        fi



        if [ -n "$LAUNCH_SCALE_TEST_DESCRIPTION" ] ;then
            description="${a_name} - $LAUNCH_SCALE_TEST_DESCRIPTION"
        else
            description="${a_name} - run workload variation: ${a_variation} "
        fi

        test_template_add_test_single "$description" cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"
        #FIXME test_template_add_test_all_M_concurrent  "${a_name} - run workload variation: ${a_variation} " "${a_command}" "${a_timeout}" "${a_variation}" "TEST_RESULT: SUCCESS"


        regexp="(.*)(SVT_AT_VARIATION_REPEAT=)([0-9]+)(.*)"
        if [[ $a_variation =~ $regexp  ]] ; then
            repeat_count=${BASH_REMATCH[3]}
            let repeat_count=$repeat_count-1;
            while(($repeat_count>0)); do
                test_template_add_test_single  "${a_name} - run workload variation: ${a_variation} repetition: ${repeat_count}"  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"
                let repeat_count=$repeat_count-1;
            done
        fi


        regexp="SVT_AT_VARIATION_CLEANSESSION=false"
        if [[ $a_variation =~ $regexp ]] ; then
            echo "-----------------------------------------"
            echo "Cleanup - Automatically inserting another test to remove the durable subscriptions that were created by last test." 
            echo "TODO: if you need to override this behavior add another setting that can be checked"
            echo "-----------------------------------------"
            regexp="SVT_AT_VARIATION_TEST_ONLY=true"
            if ! [[ $a_variation =~ $regexp ]] ; then
                test_template_add_test_single  "${a_name} - performing automatic cleanup for durable subscriptions from previous variation. "  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_MSGCNT=0|SVT_AT_VARIATION_CLEANSESSION=true|SVT_AT_VARIATION_SYNC_CLIENTS=false" "TEST_RESULT: SUCCESS"
            fi

        fi

        #defect 36374 uncomment these lines and also run do_defect_36374 
        #for blar in ndog adog bdog cdog ddog edog fdog gdog hdog jdog kdog ldog mdog ndog odog pdog qdog rdog sdog tdog udog vdog wdog xdog ydog zdog akdog bkdog ckdog dkdog kedog kfdog kgdog khdog kdog kjdog kkdog kldog mkdog kndog kodog kpdog kqdog krdog ksdog ktdog kudog kvdog kwdog xkdog ykdog kzdog amdog bdog cdog ddog edog fdog gdog hdog dog jdog kdog ldog mdog ndog odog pdog qdog rdog sdog tdog udog vdog wdog xdog ydog zdog andog bndog cndog dndog nedog nfdog ngdog nhdog nndog njdog nkdog nldog mndog nndog nodog npdog nqdog nrdog nsdog ntdog nudog nvdog nwdog xndog yndog nzdog apdog bdog cdog ddog edog fdog gdog hdog dog jdog kdog ldog mdog ndog odog pdog qdog rdog sdog tdog udog vdog wdog xdog ydog zdog apdog bsdog csdog dsdog sedog sfdog sgdog shdog ssdog sjdog skdog sldog msdog sndog sodog spdog sqdog srdog ssdog stdog sudog svdog swdog xsdog ysdog szdog apdog bxdog csdog dsdog sedog sfdog sgdog shdog ssdog sjdog skdog sldog msdog sndog sodog spdog sqdog srdog ssdog stdog sudog svdog swdog xsdog ysdog szdog ; do
            #a_variation+="SVT_AT_VARIATION_UNIQUE=$blar|"
            #test_template_add_test_single  "${a_name} - run workload variation: ${a_variation} "  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"
        #done
        
    
	    if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
            regexp="SVT_AT_VARIATION_HA="
            if [[ $a_variation =~ $regexp ]] ; then

                #-------------------------------------------------
                # 6.27.13 - Basic ha test suite - 
                #-------------------------------------------------

#               test_template_add_test_single "SVT - HA test: stop Primary" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
#               test_template_add_test_single "SVT - HA test: start Standby" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
##                test_template_add_test_single "SVT - fail over primary" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|restartPrimary|-t|600" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "Test result is Success!"

##                test_template_add_test_sleep 600 ; # need to allow some time for existing 1 M client workload to reconnect and do messaging


##            else 
                echo "No ha testing scheduled at this time (A)"
            fi 
        else
            echo "No ha testing scheduled at this time (B)"
        fi


        regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        if ! [[ $a_variation =~ $regexp ]] ; then
            test_template_add_test_all_M_concurrent  "${a_name} - cleanup" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${workload}" 600 SVT_AUTOMATION_TEST_LOG_TAR=$a_test_bucket ""
            let j=$j+$TEST_ARRAY_COLS
        fi

    
        regexp="SVT_AT_VARIATION_SINGLE_LOOP=true"
        if [[ $a_variation =~ $regexp ]] ; then
            echo "Breaking after single loop since SVT_AT_VARIATION_SINGLE_LOOP=true"
            break;
        fi
    done
}


do_init() {
    local appliance=${1-"${A1_HOST}"}
    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Notification that the test has started )
    #---------------------------------------------------------
    test_template_add_test_single "Scale MQTTv3 connection workload - notification" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|email_notifcation|na|1|${appliance}" 600 "" "TEST_RESULT: SUCCESS"
}

do_setup() {
    local appliance=${1-"${A1_IPv4_1}"}

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Virtual Nic setup on server)
    #---------------------------------------------------------
    test_template_add_test_single "Scale MQTTv3 connection workload - setup server for ${appliance}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|SERVER|1|${appliance}" 600 "SVT_ADMIN_IP=$A1_HOST" "TEST_RESULT: SUCCESS"

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (sysctl, Virtual Nic setup on client)
    #---------------------------------------------------------

    test_template_add_test_single "Scale MQTTv3 connection workload - setup client for ${appliance}" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|CLIENT|1|${appliance}" 600 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true|SVT_ADMIN_IP=$A1_HOST" "TEST_RESULT: SUCCESS"

}

num_clients=300
#num_clients=2400
#num_clients=12000
#num_clients=99000
#num_clients=29800

do_ha_sandbox_launch() {

    #launch_scale_test "connections" $num_clients "SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_HA=${A2_IPv4_1}|SVT_AT_VARIATION_QUICK_EXIT=true"

    echo "nothing in the ha sandbox"
}

do_ha_main_launch() {
	local num_clients=$num_clients;
    local ha_variation="SVT_AT_VARIATION_PUBRATE=0.008|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_HA=${A2_IPv4_1}|SVT_AT_VARIATION_QUICK_EXIT=true|SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT=120|SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_SUBONCON=1"
    
    # -- note this test really needs at least 1/2 hour if we want it to run to completion
    launch_scale_test "connections" $num_clients                      "${ha_variation}" 0



    num_clients=300 ; # less workload for final HA tests to make sure things run smoothly on client side.
#   num_clients=2490 ; # less workload for final HA tests to make sure things run smoothly on client side.
    #num_clients=24900 ; # less workload for final HA tests to make sure things run smoothly on client side.
    launch_scale_test "connections" $num_clients                      "${ha_variation}|SVT_AT_VARIATION_RESOURCE_MONITOR=false" 0 ; # RTC 34564
    launch_scale_test "connections_ldap" $num_clients                 "${ha_variation}" 1800
    launch_scale_test "connections_ldap_ssl" $num_clients             "${ha_variation}" 3600
    #launch_scale_test "connections_ltpa_ssl" $num_clients             "${ha_variation}|SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_PATTERN=fanin" 3600
    launch_scale_test "connections_ssl" $num_clients                  "${ha_variation}" 1800
    launch_scale_test "connections_client_certificate"      $num_clients   "${ha_variation}" 3600
    launch_scale_test "connections_client_certificate_ldap" $num_clients   "${ha_variation}" 1800
}


do_ha_tests() {
	#-----------------------------------
	# SVT HA tests
	#-----------------------------------
	if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
	
    	do_setup ${A2_IPv4_1} # make sure backup is setup too.
    
#       test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
        test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|setupHA|-p|1|-s|2|-i|4|-t|600" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "Test result is Success!"
	
    	#-----------------------------------
    	# Start my SVT HA enabled tests
    	#-----------------------------------
		do_ha_main_launch
	
    	#-----------------------------------
    	# End my SVT HA enabled tests
    	#-----------------------------------
	
#       test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
        test_template_add_test_single "SVT - disable HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|disableHA|-t|600" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "Test result is Success!"
	fi
}

#-----------------------------------
# Note there is also something that needs to be uncommented for 36374 
#-----------------------------------
do_defect_36374(){
    local num_clients=300
#   local num_clients=199800
    local variation

    variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=10|"
    variation+="SVT_AT_VARIATION_BURST=999|SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
    variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
    variation+="SVT_AT_VARIATION_TOPICCHANGE=1|"
    variation+="SVT_AT_VARIATION_UNIQUE=ad|"

    launch_scale_test "connections" $num_clients "$variation" 1800
}

do_rtc_39574_retained_msgs(){
    local num_clients=${1-$num_clients}
    local operation=${2-""}
    local unique=${3-""}
    local type=${4-"connections"}
    local addnl_variation=${5-""}
    local variation

    if [ "$operation" == "pub" ]; then
        # Note: don't really need to pub 3 messages, only last one will be retained, but if needed one can verify that
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=3|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        variation+="SVT_AT_VARIATION_RETAINED_MSGS=true|"
        variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
        variation+="SVT_AT_VARIATION_PUBQOS=2|"
        variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        variation+="SVT_AT_VARIATION_UNIQUE_CID=$unique|"
    elif [ "$operation" == "sub" ]; then
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=1|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        variation+="SVT_AT_VARIATION_RETAINED_MSGS=true|"
        variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
        variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        variation+="SVT_AT_VARIATION_UNIQUE_CID=$unique|"
    elif [ "$operation" == "clean" ]; then #to clean up retained messages you must publish an empty payload SVT_AT_VARIATION_RETAINED_MSGS_CLEAN=true
        echo "ERROR: Invalid operation sent $operation. This could be supported in future. "
        echo "INFO: In order to clean up retained messages the publisher must publish to the same topic with an empty payload"
        echo "INFO: Currently this test does not yet support doing that."
        exit 1;
    else
        echo "ERROR: Invalid operation sent $operation"
        exit 1;
    fi
    variation+="$addnl_variation"
    launch_scale_test $type $num_clients "$variation" 1800
}

do_rtc_39574_durable_subs(){
    local num_clients=${1-$num_clients}
    local operation=${2-""}
    local unique=${3-""}
    local type=${4-"connections"}
    local addnl_variation=${5-""}
    local variation

    if [ "$operation" == "init" ]; then
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=0|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
        variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        variation+="SVT_AT_VARIATION_UNIQUE_CID=$unique|"
    elif [ "$operation" == "pub" ]; then
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=10|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
        variation+="SVT_AT_VARIATION_PUBQOS=2|"
        variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        variation+="SVT_AT_VARIATION_UNIQUE_CID=$unique|"
    elif [ "$operation" == "sub" ]; then
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=10|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
        variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        variation+="SVT_AT_VARIATION_UNIQUE_CID=$unique|"
        
    else
        echo "ERROR: Invalid operation sent $operation"
        exit 1;
    fi
    variation+="$addnl_variation"
    launch_scale_test $type $num_clients "$variation" 1800

}

#--------------------------------
# This test should hopefully find the max throughput that this client 
# combination is able to achieve for fanin case by stepping through various pub rates
#--------------------------------
do_stairstep_test(){
    local pattern=${1-"fanin"}
    local num_clients=300
#   local num_clients=199800
    local variation 
    local alternate_ip="10.10.3.20" ; # so publishers and subscribers will be using different nics

    variation="SVT_AT_VARIATION_PUBRATE=500|SVT_AT_VARIATION_MSGCNT=1000|"
    variation+="SVT_AT_VARIATION_PUBRATE_INCRA=100|"
    variation+="SVT_AT_VARIATION_PUBRATE_INCRB=100|"
    variation+="SVT_AT_VARIATION_PUBRATE_INCRC=60|"
    variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=0|"
    variation+="SVT_AT_VARIATION_TOPICCHANGE=1|"
    variation+="SVT_AT_VARIATION_PATTERN=${pattern}|"
    variation+="SVT_AT_VARIATION_PUB_THROTTLE=true|"
    variation+="SVT_AT_VARIATION_ALTERNATE_PRIMARY_IP=$alternate_ip|"
    launch_scale_test "connections" $num_clients "$variation" 1800

}


#--------------------------------
# This test uses the SVT_AT_VARIATION_REPEAT flag to connect 
# num_clients * SVT_AT_VARIATION_REPEAT times in quick succession.
# TODO: not working yet.
#--------------------------------
do_connection_blast(){
    local connection_type=${1-"connections"}
    local num_clients=${2-300}
    local iterations=${3-1}
    local addnl_variation=${4-""}
    variation="SVT_AT_VARIATION_PATTERN=connblast|"
    variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|"
    variation+="SVT_AT_VARIATION_QUICK_EXIT=true|"
    variation+="SVT_AT_VARIATION_REPEAT=${iterations}|"
    variation+="SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=120|SVT_AT_VARIATION_STATUS_UPDATE=120|${addnl_variation}"
    launch_scale_test ${connection_type} $num_clients "$variation" 1800
}

#--------------------------------
# RTC 39862, 39863, 39864, CommonName tester.
#--------------------------------
do_client_cert_with_unique_CommonName_for_each_client(){
    local my_clients=${1-$num_clients}
    local addnl_variation=${2-""}
	variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_SINGLE_LOOP=true|"
    variation+="SVT_AT_VARIATION_TOPICCHANGE=1|"
	variation+="SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=3|"
    variation+="$addnl_variation"

    launch_scale_test "connections_client_certificate_CN_fanout" $my_clients "$variation" 3600

	return 0;
}

#--------------------------------
# RTC 38061 - Load test security code
#--------------------------------
do_security_variations_with_dynamic_endpoint_17778(){
    local num_clients=${1-$num_clients}
    local iteration=0;

    for iteration in {0..11}; do
        variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER=60|"
        variation+="SVT_AT_VARIATION_PORT=17778|SVT_AT_VARIATION_SECURITY=$iteration|"
        variation+="SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=1|"
        variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|"
        launch_scale_test "connections_ssl" $num_clients "$variation" 3600
    done
}

do_rtc_39574() {
    local operation=${1-"init"}
    local num_clients=${2-$num_clients}
    local iteration=0;
    local addnl_variation=""

    if [ "$operation" == "sub" ] ;then
        addnl_variation=""
    else
        addnl_variation="SVT_AT_VARIATION_TEST_ONLY=true"
    fi
    if [ "$operation" == "pub" -o "$operation" == "sub" ] ;then
        do_rtc_39574_retained_msgs $num_clients $operation "rrrr" "connections_ssl" "$addnl_variation"
        do_rtc_39574_retained_msgs $num_clients $operation "ssss" "connections" "$addnl_variation"
        do_rtc_39574_retained_msgs $num_clients $operation "tttt" "connections_ldap" "$addnl_variation"
        do_rtc_39574_retained_msgs $num_clients $operation "uuuu" "connections_ldap_ssl" "$addnl_variation"
        do_rtc_39574_retained_msgs $num_clients $operation "vvvv" "connections_client_certificate" "$addnl_variation"
        #do_rtc_39574_retained_msgs $num_clients $operation "wwww" "connections_ltpa_ssl" "$addnl_variation"
        #do_rtc_39574_retained_msgs $num_clients $operation "xxxx" "connections_client_certificate_CN_fanout" "$addnl_variation"
    fi
    do_rtc_39574_durable_subs $num_clients $operation "aaaa" "connections_ssl" "$addnl_variation"
    do_rtc_39574_durable_subs $num_clients $operation "bbbb" "connections" "$addnl_variation"
    do_rtc_39574_durable_subs $num_clients $operation "cccc" "connections_ldap" "$addnl_variation"
    do_rtc_39574_durable_subs $num_clients $operation "dddd" "connections_ldap_ssl" "$addnl_variation"
    do_rtc_39574_durable_subs $num_clients $operation "eeee" "connections_client_certificate" "$addnl_variation"
    #do_rtc_39574 $num_clients $operation "ffff" "connections_ltpa_ssl" "$addnl_variation"
    #do_rtc_39574 $num_clients $operation "gggg" "connections_client_certificate_CN_fanout" "$addnl_variation"

    #--------------------------------------------------------
    # Also do manual setup
    #--------------------------------------------------------
    # 1. Setup Queue in MQ:  with runmqsc
    #  DEFINE QLOCAL ('TUYO') REPLACE

    # Also setup MQConnectivty Destination mapping rule MessageSight topic MIO -> Queue TUYO
    #  mqttsample_array -s 10.10.10.10:16102 -a publish  -t /MIO -q 2 -c false -v -m "howdy" -n 10
    #  mqttsample_array -s 10.10.10.10:16102 -a publish  -t /MIO -q 2 -c false -v -m "howdy" -n 10
    # Before code upgrade stop QM with endmqm -i SVTBRIDGE.QUEUE.MANAGER

    # After code upgrade verify receive of messages , 
    # Start MQ QM w/ strmqm
    # mquser@mar400 /opt/mqm_v7.5/samp/bin> ./amqsget TUYO

    # 2. Setup Queue MARCOS in messagesight
    # Buffer messages using -r flag
    # 
    # java svt.jms.ism.JMSSample -s tcp://10.10.10.10:16102 -r -a publish -q MARCOS -n 1000000 
    # verify recieve 1 msg
    # java svt.jms.ism.JMSSample -s tcp://10.10.10.10:16102  -a subscribe -q MARCOS -n 1
    # 
    #  after code upgrade verify receive rest of 999999 msgs
    # java svt.jms.ism.JMSSample -s tcp://10.10.10.10:16102  -a subscribe -q MARCOS -n 999999

}

do_rtc_39574_teardown(){ # run after doing a firmware migration.
    do_rtc_39574 "sub" 20000  # recieve all the retained and durable sub messages.
}

do_rtc_39574_setup(){ # run before doing a firmware migration
    do_rtc_39574 "init" 20000
    do_rtc_39574 "pub" 20000
}

do_defect_42237(){
    local num_clients=300
#   local num_clients=998000
    local iteration=0;
    local tmp;
    local myvar
    local addnl_variation=""

#   local num_clients=998000
    for myvar in {1..10} ; do
        for tmp in a b c d e f g h i j k l m n o p q r s t u v w x y z ; do
            operation="pub"
            do_rtc_39574_retained_msgs $num_clients $operation "${tmp}${myvar}" "connections" "$addnl_variation"
        done
    done

    return 0;

}

do_ha_test_9800_fanin(){ // Tested on 10.29.13 - passed  , 9800 publishers, 20 subscribers fan in , publishers publish 10 messages per sec, 10 messages each, did a failover in middle of test, 100% of messages received after failover.
    local num_clients=300
#   local num_clients=9800
    local addnl_variation=""
    local variation=""

#   num_clients=9800
    variation="SVT_AT_VARIATION_PATTERN=fanin|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_MSGCNT=10|"
    variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|SVT_AT_VARIATION_STATUS_UPDATE=1|"
    variation+="SVT_AT_VARIATION_VERIFYSTILLACTIVE=1800|"
    variation+="SVT_AT_VARIATION_HA2=${A2_IPv4_1}|SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT=1|"
    variation+="SVT_AT_VARIATION_PUBQOS=2|"
    variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
    variation+="SVT_AT_VARIATION_UNIQUE=T2_|"
    launch_scale_test "connections" $num_clients "$variation" 1800

    return 0;
}

do_rtc_43580_helper(){
    local num_clients=${1-$num_clients}
    local operation=${2-""}
    local unique=${3-""}
    local workload=${4-"connections"}
    local addnl_variation=${5-""}
    local a_timeout=${6-3600}
    local a_variation
    local per_client
    local a_name="Fill n Drain MQTTv3 - $operation "
    let per_client=($num_clients/${M_COUNT});
    local a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"

    if [ "$operation" == "init" ]; then
        a_variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=0|"
        a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        a_variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        a_variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
        a_variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        a_variation+="SVT_AT_VARIATION_SYNC_CLIENTS=true|"
    elif [ "$operation" == "pub" ]; then
        a_variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=10|"
        a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        a_variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        a_variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_SUBSCRIBE=1|"
        a_variation+="SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT=0|"  # May not be needed - trying to figure why publishing stops at 81 percent
        #a_variation+="SVT_AT_VARIATION_NODISCONNECT=1|"          # May not be needed - trying to figure why publishing stops at 81 percent
  #      a_variation+="SVT_AT_VARIATION_CLEANBEFORECONNECT=1|"     # May not be needed - trying to figure why publishing stops at 81 percent - this fixed it
        a_variation+="SVT_AT_VARIATION_RECONNECTWAIT=10|"        # May not be needed - trying to figure why publishing stops at 81 percent
        a_variation+="SVT_AT_VARIATION_PUBQOS=2|"
        a_variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        a_variation+="SVT_AT_VARIATION_ORDERMSG=1|"
        a_variation+="SVT_AT_VARIATION_SYNC_CLIENTS=true|"
    elif [ "$operation" == "sub" ]; then
        a_variation="SVT_AT_VARIATION_PATTERN=fanout|SVT_AT_VARIATION_PUBRATE=0|SVT_AT_VARIATION_MSGCNT=10|"
        a_variation+="SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_QOS=2|"
        a_variation+="SVT_AT_VARIATION_CLEANSESSION=false|"
        a_variation+="SVT_AT_VARIATION_TOPICCHANGE=1|SVT_AT_VARIATION_SKIP_PUBLISH=1|"
        a_variation+="SVT_AT_VARIATION_VERIFYPUBCOMPLETE=0|"
        a_variation+="SVT_AT_VARIATION_UNIQUE=$unique|"
        a_variation+="SVT_AT_VARIATION_ORDERMSG=1|"
        a_variation+="SVT_AT_VARIATION_SYNC_CLIENTS=true|"
    else
        echo "ERROR: Invalid operation sent $operation"
        exit 1;
    fi
    a_variation+="$addnl_variation"

    test_template_add_test_single  "${a_name} - run workload variation: ${a_variation} "  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"

}

do_rtc_43580(){
    local num_clients=${1-$num_clients}
    local type=${2-"connections"}
    local addnl_variation=${3-""}
    local unique=${4-"rtc43580"}
    local regexp
    local a_variation=""
    local a_name="Fill n Drain MQTTv3 "
    local a_test_bucket="SVTfilldrain_${n}"  ; # n is a global variable incremented in the test_template_ calls 
    local myfillp=""

    a_variation="SVT_AT_VARIATION_MSGFILE=/niagara/test/svt_cmqtt/BIGFILE|SVT_AT_VARIATION_86WAIT4COMPLETION=1|"
    a_variation+="$addnl_variation"

    #---------------------------------------------
    # Check inputs. (For pattern file creation)
    #---------------------------------------------
    regexp="(.*)(SVT_AT_VARIATION_FILL_PATTERN=)(['\''0-9\\]*)(.*)"
    if ! [[ $a_variation =~ $regexp ]] ; then
        echo "ERROR: do_rtc_43580: Must input SVT_AT_VARIATION_FILL_PATTERN= to properly specify test"
    else
        myfillp="${BASH_REMATCH[3]}"
    fi


    regexp="SVT_AT_VARIATION_FILL_PATTERN_SZ="
    if ! [[ $a_variation =~ $regexp ]] ; then
        echo "ERROR: do_rtc_43580: Must input SVT_AT_VARIATION_FILL_PATTERN_SZ= to properly specify test"
    fi

    #---------------------------------------------
    # Start test transaction. All tests until end of transaction must pass or the test will be quickly aborted.
    #---------------------------------------------
    test_template_add_test_single "${a_name} -Start a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_start|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #---------------------------------------------
    # The next test creates a pattern file that will be used to fill up the appliance to 85%. 
    # IMPORTANT: These variables MUST be passed in by the caller in a_variation.
    # 
    # For example below the input settings create a 1M message pattern file filled with all 0x55555
    # e.g. SVT_AT_VARIATION_FILL_PATTERN='\125' set pattern file to all 0x5
    # e.g. SVT_AT_VARIATION_FILL_PATTERN_SZ=1024
    #---------------------------------------------
    test_template_add_test_single "${a_name} - create ${myfillp} pattern file" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|pattern_file|/niagara/test/svt_cmqtt/BIGFILE|1|${appliance}" 600 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Cleanup any possible extraneous data that is on the appliance for unique subscription before starting test
    #--------------------------
    do_rtc_43580_helper $num_clients sub  ${unique} connections "$a_variation|SVT_AT_VARIATION_MSGCNT=0|SVT_AT_VARIATION_CLEANSESSION=true|"

    #--------------------------
    # Initialize unqiue subscriptions
    #--------------------------
    do_rtc_43580_helper $num_clients init ${unique} connections "$a_variation"

    #--------------------------
    # Start background Monitor for 85 % memory utilization
    #--------------------------
    test_template_add_test_single "${a_name} - start monitor for 85 percent mem utilization" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|svt_monitor_for_85_percent_mem_utilization|na|1|${A1_HOST}|${A2_HOST}" 120 "SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # (Fill in Background) Buffer messages on unique subsription until SIGTERM received or cleaned up below by cleanup step
    #--------------------------
    do_rtc_43580_helper $num_clients pub  ${unique} connections "$a_variation|SVT_AT_VARIATION_MSGCNT=1000|SVT_AT_VARIATION_QUICK_EXIT=true|"

    #--------------------------
    # Block until 85% utilzation confirmed 
    #--------------------------
    test_template_add_test_single "${a_name} - block until 85 percent memory utilization achieved " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|wait_for_85_percent_mem_utilization|na|1|${A1_HOST}|${A2_HOST}" 3600 "SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # make sure all processes are complete. Post Process logfiles to make sure they are collected later, do not create a tar yet.
    #--------------------------
    #test_template_add_test_all_M_concurrent  "${a_name} - cleanup stop publishing processes" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${type}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true|" ""
    test_template_add_test_all_M_concurrent  "${a_name} - cleanup and collect logs for fill operation" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${type}" 600 "" ""

    #--------------------------
    # (Drain) Receive all buffered messages
    #--------------------------
    do_rtc_43580_helper $num_clients sub  ${unique} connections "$a_variation|SVT_AT_VARIATION_MSGCNT=1000|SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_VERBOSE=1"

    #--------------------------
    # Verify all messages received.
    #--------------------------
    test_template_add_test_single "${a_name} - verify no buffered messages on subscriptions " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|rtc43580_check_zero_buffered_msgs|${unique}|1|${A1_HOST}|${A2_HOST}" 120 "SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # End the test transaction
    #--------------------------
    test_template_add_test_single "${a_name} - End a test transaction " cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|test_transaction_end|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true|$a_variation" "TEST_RESULT: SUCCESS"

    #--------------------------
    # Cleanup unique subscription before ending test
    #--------------------------
    do_rtc_43580_helper $num_clients sub  ${unique} connections "$a_variation|SVT_AT_VARIATION_MSGCNT=0|SVT_AT_VARIATION_CLEANSESSION=true|"

    #--------------------------
    # Consolidate and capture all log files for this test transaction
    #--------------------------

    test_template_add_test_all_M_concurrent  "${a_name} - collect logs" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${type}" 600 SVT_AUTOMATION_TEST_LOG_TAR=$a_test_bucket ""
        
}


#-------------------------------------
# Fill / Drain tests (RTC 43580)
#-------------------------------------
launch_fill_drain_tests(){
    local num_clients=${1-300}
    local addnl_variation=""

    addnl_variation="SVT_AT_VARIATION_FILL_PATTERN='\125'|SVT_AT_VARIATION_FILL_PATTERN_SZ=1024|SVT_AT_VARIATION_SUBONCON=1|"
    addnl_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=0|" #- Fixes not filling up issue RTC 44385
    do_rtc_43580 $num_clients connections "$addnl_variation"


    addnl_variation="SVT_AT_VARIATION_FILL_PATTERN='\252'|SVT_AT_VARIATION_FILL_PATTERN_SZ=1024|SVT_AT_VARIATION_SUBONCON=1|"
    addnl_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=0|" #- Fixes not filling up issue RTC 44385
    do_rtc_43580 $num_clients connections "$addnl_variation"

}



do_sandbox_launch() {
    local num_clients=100
    local iteration=0;
    local tmp;
    local myvar
    local addnl_variation=""
    local variation=""

    #-----------------------------------------
    # Note: Below the SVT_AT_VARIATION_SUBONCON value is not required, but it will inherently better synchronize the subscribers before reception starts during Drain phase.
    #-----------------------------------------
    addnl_variation="SVT_AT_VARIATION_FILL_PATTERN='\125'|SVT_AT_VARIATION_FILL_PATTERN_SZ=1024|"
    addnl_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=1|"
#    addnl_variation="SVT_AT_VARIATION_FILL_PATTERN='\125'|SVT_AT_VARIATION_FILL_PATTERN_SZ=1024|SVT_AT_VARIATION_SUBONCON=1|"
#    addnl_variation+="SVT_AT_VARIATION_86WAIT4COMPLETION=0|" #- Fixes not filling up issue RTC 44385
    echo "addnl_variation is $addnl_variation"
    do_rtc_43580 600 connections "$addnl_variation"
#   do_rtc_43580 11000 connections "$addnl_variation"

    return 0;
    
}

do_standalone_tests() {
    local num_clients=$num_clients;

    launch_scale_test "connections" $num_clients                      "" 0
#    launch_fill_drain_tests 1100 "SVT_PF_CRITERIA_ORDERING=log.*"    # New - added for RTC 43580
    launch_scale_test "connections" $num_clients                      "SVT_AT_VARIATION_RESOURCE_MONITOR=true" 0 ; # RTC 34564
    launch_scale_test "connections_ldap" $num_clients                 "" 1800
    launch_scale_test "connections_ldap_ssl" $num_clients             "" 3600
    #launch_scale_test "connections_ltpa_ssl" $num_clients             "SVT_AT_VARIATION_SINGLE_LOOP=true|SVT_AT_VARIATION_PATTERN=fanin" 3600
    launch_scale_test "connections_ssl" $num_clients                  "" 1800
    # - This may cause out of memory ( RTC 35918) on the appliance, and also on the client systems.
    # num_clients=998000;  launch_scale_test "connections_client_certificate" $num_clients   "SVT_AT_VARIATION_PUBRATE=0.05" 1800

    num_clients=300; # try to fix RTC 35918 with less clients
#   num_clients=2490; # try to fix RTC 35918 with less clients
    #num_clients=14900; # try to fix RTC 35918 with less clients
    launch_scale_test "connections_client_certificate" $num_clients   "" 3600
    launch_scale_test "connections_client_certificate_ldap" $num_clients   "" 1800
#    do_client_cert_with_unique_CommonName_for_each_client 95000 ; # 100K clients with unique CommonName
    do_client_cert_with_unique_CommonName_for_each_client $num_clients ; # 2.5K clients with unique CommonName
}

do_softlayer_init(){
    test_template_add_test_single "${a_name} - Setup for softlayer" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|softlayer_setup|na|1|${A1_HOST}:${A1_PORT}|${A2_HOST}:${A2_PORT}" 600 "" "TEST_RESULT: SUCCESS"

}


do_softlayer_init
do_init
do_setup ${A1_IPv4_1}
#do_sandbox_launch
do_standalone_tests
do_ha_tests

