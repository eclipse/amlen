#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
# Testcase Problem Log:  (why is this testcase failing)
# 2.25.13 - defect 26243: CLI: imaserver create DestinationMappingRule fails with...
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 05"
httpSubDir="svt_atelm/web/html"

typeset -i n=0
typeset -i svt=0

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

# For these tests, source the file that sets up the URL prefix
source ../common/url.sh

echo "M1_BROWSER_LIST=${M1_BROWSER_LIST}"

if [[ "${M1_BROWSER_LIST}" == *firefox* ]]
then
    #----------------------------------------------------
    # JavaScript MQTT Websocket Test Cases 
    # RTC task 18579 - lowered number of clients to get testcase to pass more reliably. Previously 1, 75, 150
    # This new number of 1,10,25 will still test the end to end functionality, and ISM, but will not be pushing the limits of what
    # the client side can do to the point where frequent failures occurr.
    #----------------------------------------------------
    for svt in 1 10 25 
    do
        
        #-------------------------------------------
        #  Assert that n < 10 or this script is brokent.
        #-------------------------------------------
        if (($n>9)); then
                echo "ERROR: Assertion failed: This script does not support n>9. below atelm_05_0 needs to be fixed."
                exit 1;
        fi

        echo "Adding JavaScript MQTT Websocket test cases for $svt clients"

        xml[${n}]="atelm_05_0${n}"
	test_template_initialize_test "${xml[${n}]}"
        if [ "$svt" == "1" ] ;then
            let minutes=1;
            action="automotive";
            scenario[${n}]="${xml[${n}]} - ${svt} JavaScript Users Basic Run, MQ Connectivity, MQ, and WAS End to End Auotomotive Scenario $minutes mins"
        else
            let minutes=5;
            let car_minutes=${minutes}+4; # Cars start a few minutes (4) before users and need to continue running when users are running
            #action="automotive-unlock";
            action="automotive";
            scenario[${n}]="${xml[${n}]} - ${svt} JavaScript Users and Cars, MQ Connectivity, MQ, and WAS End to End Auotomotive Scenario $minutes mins"
        fi

        let messages=${minutes}*3*${svt}
        let timeouts[${n}]=${minutes}*60+${svt}/100+360

        #-------------------------------------
        # JavaScript Users Configuration
        #-------------------------------------
        component[1]=cAppDriver,m1,"-e|firefox","-o|-new-window|${URL_M1_IPv4}/${httpSubDir}/user_scale.html?&client_count=${svt}&action=$action&logfile=${xml[${n}]}-user_scale-M1-6.log&test_minutes=${minutes}&verbosity=9&start=true","-s-DISPLAY=localhost:1"

        #-------------------------------------
        # Add compare test for non-standard filename... I probably could switch this to log to the firefox.screenout.log file and not have to do this 
        #-------------------------------------
        test_template_add_to_comparetest_full ${xml[${n}]} 1 1 "${xml[${n}]}-user_scale-M1-6.log" "SVTUserScale Success" 

        #-------------------------------------
        # searchLog Configuration
        #-------------------------------------
        component[2]=searchLogsEnd,m1,atelm_05_0${n}_m1.comparetest

        #----------------------------------------------------
        # Webshere MQ and Application Server (WAS) Configuration
        # TODO: stop WAS, stop/start MQ, start MQ - still seeing 
        #issue where WAS/MQ get into some strange state where it doesn't work.
        #----------------------------------------------------
        #/opt/IBM/WebSphere/AppServer/profiles/AppSrv01/bin/stopServer.sh server1
        #sudo -u mquser /var/mqm/configureMQ.sh 
        #/opt/IBM/WebSphere/AppServer/profiles/AppSrv01/bin/startServer.sh server1

        #----------------------------------------------------
        # MQConnectivity configuration
        #
        # 10.15.12 Observation: had to add sleeps to ensure proper 
        # ordering of commands below to get MQConnectivity configured.
        # There is also a bug that mqconnectivity_stop doesnt ever finish -James will work on fixing (working around) that in run-cli.sh
        # Once this all is working the sleeps can be taken out and change AppDriverLog back to cAppDriverLogWait
        #----------------------------------------------------
        component[3]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|mqconnectivity_stop|-c|policy_config.cli"
        component[4]=sleep,15
        component[5]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|mqconnectivity_start|-c|policy_config.cli"
        component[6]=sleep,60
        component[7]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|mqconnectivity_automotive_user_scale|-c|policy_config.cli"

        #-------------------------------------
        # Cars Configuration
        #-------------------------------------

        component[8]=javaAppDriver,m1,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|${svt}|0|${car_minutes}|2"
        #component[9]=sleep,${svt}
        component[9]=sleep,5

        #component[11]=runCommandEnd,m1,svt-metrics.sh,"-s|SVT_TEXT=finished__|SVT_FILE=ISM_AutomotiveTelematics_JavaMQTT_${n}-javaAppDriver-svt.scale.vehicle.SVTVehicleScale-M1-1.screenout.log"

        if [ "$svt" == "1" ] ;then
            #-----------------------------
            # Basic Run - no cars, just users
            #-----------------------------
	    test_template_runorder="3 4 5 6 7 1 2"
        else
            #-----------------------------
            # Full Run - with cars.
            #-----------------------------
            echo "Cars not implemented yet"
	    test_template_runorder="8 9 3 4 5 6 7 1 2"
        fi

	echo "test_template_runorder set to $test_template_runorder"

	test_template_finalize_test

        ((n+=1))
    done

fi
