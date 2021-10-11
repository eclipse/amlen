#!/bin/bash
 
#----------------------------------------------------------------------------
# run-cli.sh: Script for running cli scenarios
#
#
# The format of the input file is the same format as CLI set and delete (except without imaserver)
# This script can be call from runScenarios to create a new object or modify an existing object by using the imaserver's CLI.
#
# Support for running Bedrock command is done using the "bedrock" option type. See the example below
#
# Sample run-scenarios input file:
# #----------------------------------------------------
# # Scenario X
# #----------------------------------------------------
# xml[${n}]="policy_test_1"
# timeouts[${n}]=10
# scenario[${n}]="${xml[${n}]} Create Listener on port 16110 and verify"
# component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh,-o|../scripts/TestEndpoint16102.cli.cli|-s|clean"
# components[${n}]="${component[1]}"
# ((n+=1))
#
# Sample run-cli input file:
# #----------------------------------------------------
# # cli_section
# # |   expected return code (0-255, 'x' = don't check, '-' = non-zero and 'p' = pending delete (special case for delete where we check for "PendingAction = Delete" ))
# # v   v
# #----------------------------------------------------
# Setup 0 set ConnectionPolicy "Name=ConnPol19001" "PolicyType=Connection" "Description=ConnPol19001" "Protocol=JMS" "ClientAddress=${M1_IPv4_1},${M1_IPv6_2}"
# Setup 0 set MessagingPolicy "Name=PubSubMsgPol19001" "PolicyType=Messaging" "Description=Pub/Sub for LIS19001" "Topic=LIS19001_Topic" "Protocol=JMS" "MaxMessages=5000" "ActionList=Publish,Subscribe" "ClientAddress=${M1_IPv4_1},${M1_IPv6_2}"
# Setup 0 set MessageHub "Name=HUB1" "Description=HUB1"
# Setup 0 bedrock file list
# Setup 0 set Endpoint "Name=LIS19001" "SecurityPolicies=ConnPol19001,PubSubMsgPol19001" "Port=19001" "Protocol=JMS" "MessageHub=HUB1" "MaxMessageSize=4MB" "Interface=all" "Enabled=True"
# Clean 0 delete ConnectionPolicy "ConnPol19001"
# Clean 0 delete MessagingPolicy "PubSubMsgPol19001"
# Clean 0 delete MessageHub "HUB1"
# Clean 0 delete Endpoint "LIS19001"
#
# run-cli.sh has been updated to convert the old CLI commands to REST API requests. A1_REST=TRUE must be set in testEnv.sh
# You can also make REST request directly 
# rest 0 POST service/restart {"Service":"Server","Maintenance":"stop"}
# rest 0 POST configuration {"MessageHub":{"jrs_MessageHub":{}}}
# rest 0 GET configuration/MessageHub
# rest 0 DELETE configuration/MessageHub/jrs_MessageHub
# rest 0 PUT file ${M1_TESTROOT}/common/imaserver-crt.pem jmsimaserver-crt.pem
#
#
# TODO: config_item values with spaces are not verified correctly.
# TODO: Before doing the set, first do a get for the case where we are modifing an existing object.
#       We should then verify that config_items that were not specified were not changed.
#       Do we need a parameter to cause this script to fail if object already exist? 
#---------------------------------------------------------------------------
# 3/18/14 Change run-cli.sh support case insesitive CLI commands
# 4/07/15 Add support for Docker for calling python scripts
#         Change to use curl instead of python scripts
# 8/27/15 Add support for splitting MessagingPolicy 
#----------------------------------------------------------------------------
#set -x


function echolog {
    # if the log file is going to stdout don't echo twice
    if [[ "$CLI_LOG_FILE" == "/dev/stdout" ]] ; then
        echo "$@"
    else
        echo "$@" | tee -a $CLI_LOG_FILE
    fi
}


function setObject {
    #----------------------------------------------------------------------------
    # Set Object (used for singleton objects only)
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_params}"
    echolog "${cmd}"
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC" 
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        if [[ "$expected_rc" == "0" ]] ; then
            if [ $RC -ne 0 ] ; then
                let num_fails+=1
            else
                # if this is not a request for help then verify object got set
                if [[ "${cli_object_type_lc}" != "help" && "${cli_params,,}" != "help" ]] ; then 
                    # parse object name 
                    object=${cli_object_type%%=*}
                    cmd="${sshcmd} imaserver get ${object}"
                    echolog "${cmd}"
                    reply=`$cmd`
                    RC=$?
                    echo ${reply} >> $CLI_LOG_FILE
                    echolog "RC=$RC"
                    if [ $RC -ne 0 ] ; then
                        let num_fails+=1
                    else
                        # Remove space character before and after '=' character. Do in 2 steps in case there is nothing after the =
                        reply=${reply// =/=}
                        reply=${reply//= /=}
                        # Remove trailing spaces
                        reply="${reply%"${reply##*[![:space:]]}"}"
                        reply_lc="${reply,,}"							
                        if [[ "${reply_lc}" =~ "${cli_object_type_lc}" ]] ; then
                            echo "Verified \"${cli_object_type}\"" >> $CLI_LOG_FILE
                        else
                            echo "Failure: Expected \"${cli_object_type}\" got \"${reply}\"" >> $CLI_LOG_FILE
                            echo "Failure: Expected \"${cli_object_type_lc}\" got \"${reply_lc}\"" >> $CLI_LOG_FILE
                            let num_fails+=1
                        fi
                    fi
                fi
            fi
            if [ ${num_fails} -gt 0 ] ; then 
                echolog ">>> set Failed"
            else
                echolog ">>> set Passed"
            fi
        else
            echolog "Expected RC=$expected_rc"
            if [ $RC -ne $expected_rc ] ; then
                let num_fails+=1
                echolog ">>> RC compare Failed"
            else
                echolog ">>> RC compare Passed"
            fi
        fi
    fi
}


function createUpdateObject {
    #----------------------------------------------------------------------------
    # create or update Object
    #----------------------------------------------------------------------------
    
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_params}"
    echolog "${cmd}" 
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC" 
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        if [[ "$expected_rc" == "0" ]] ; then
            if [ $RC -ne 0 ] ; then
                let num_fails+=1
            else
                # if this is not a request for help or TopicMonitor then verify object parameters got set
                if [[ "${cli_object_type_lc}" != "help" && "${cli_params,,}" != "help" ]] ; then 
                    if [[ "${cli_object_type_lc}" == "topicmonitor" ]] ; then 
                        #------------------------------------------------------------------
                        # Verify TopicString got created
                        #------------------------------------------------------------------
                        topic_string=${cli_params##*=}
                        topic_string=${topic_string%%\"}
                        cmd="${sshcmd} imaserver list ${cli_object_type}"
                        echolog "${cmd}"
                        reply=`$cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        if [ $RC -ne 0 ] ; then
                            let num_fails+=1
                        else
                            #------------------------------------------------------------------
                            # Verify TopicString exist
                            #------------------------------------------------------------------
                            let item_found=0
                            for reply_item in ${reply} ; do
                                if [[ "${reply_item}" == "${topic_string}" ]] ; then
                                    let item_found=1
                                    break
                                fi
                            done

                            if [[ "${item_found}" -eq "1" ]] ; then
                                echo "Verified \"${topic_string}\"" 
                            else
                                echo "Failure: Could not find \"${topic_string}\" in list" 
                                let num_fails+=1
                            fi
                        fi    
                    else  # Not "TopicMonitor"
                        #------------------------------------------------------------------
                        # Verify all parameters specified above got created
                        #------------------------------------------------------------------
                        cli_name=""
                        for item in ${cli_params} ; do
                           item_name=${item%%=*}
                            item_name=${item_name##\"}
                            item_name_lc="${item_name,,}"
                            if [[ "${item_name_lc}" == "name" ]] ; then
                                cli_name=$item
                                break
                            elif [[ "${item_name_lc}" == "host" ]] && [[ "${cli_object_type_lc}" == "snmptrapsubscriber" ]] ; then
                                cli_name=$item
                                break
                            fi
                        done
                        cmd="${sshcmd} imaserver show ${cli_object_type} ${cli_name}"
                        echolog "${cmd}"
                        reply=`$cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        if [ $RC -ne 0 ] ; then
                            let num_fails+=1
                        else
                            #------------------------------------------------------------------
                            # Verify all config_items were set
                            #------------------------------------------------------------------
                            # Remove space character before and after '=' character
                            reply=${reply// = /=}
               
                            # For each config_item=value verify that it is in reply
                            for item in ${cli_params[@]} ; do  
                                # remove quotes around item
                                item=${item%%\"}
                                item=${item##\"}
                                item_name=${item%%=*}
                                item_name_lc="${item_name,,}"
                                item_value=${item##*=}

                                if [[ ! ( "${item_name_lc}" == "password" ) ]] && [[ ! ( "${item_name_lc}" == "bindpassword" ) ]] ; then 

                                    # For each config_item=value pair in cli_params verify there is a match reply
                                    let item_found=0
                                    for reply_item in ${reply} ; do
                                        reply_item_name=${reply_item%%=*}
                                        reply_item_name_lc="${item_name,,}"
                                        reply_item_value=${reply_item##*=}
 
                                        if [[ "${reply_item_name_lc}" == "${item_name_lc}" ]] ; then
                                            if [[ "${reply_item_value}" == "${item_value}" ]] ; then
                                                let item_found=1
                                                break
                                            fi
                                        fi
                                    done
 
                                    if [[ "${item_found}" -eq "1" ]] ; then
                                        echo "Verified \"${item}\"" >> $CLI_LOG_FILE
                                    else
                                        echo "Failure: Could not get \"${item}\"" >> $CLI_LOG_FILE
                                        let num_fails+=1
                                    fi

                                fi
                            done
                        fi
                    fi
                fi
            fi
            if [ ${num_fails} -gt 0 ] ; then 
                echolog ">>> ${cli_createUpdate} Failed"
            else
                echolog ">>> ${cli_createUpdate} Passed"
            fi
        else
            echolog "Expected RC=$expected_rc"
            if [ $RC -ne $expected_rc ] ; then
                let num_fails+=1
                echolog ">>> RC compare Failed"
            else
                echolog ">>> RC compare Passed"
            fi
        fi
    fi
}


function deleteObject {
#----------------------------------------------------------------------------
# Delete Object
#----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_params}"
    echolog "${cmd}"
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        if [[ "$expected_rc" == "p" || "$expected_rc" == "0" ]] ; then
            if [ $RC -ne 0 ] ; then
                let num_fails+=1
            else
                # if this is not a request for help or TopicMonitor then verify object got deleted
                if [[ "${cli_object_type_lc}" != "help" && "${cli_params,,}" != "help" ]] ; then 
                    if [[ "${cli_object_type}" == "TopicMonitor" ]] ; then 
                        #------------------------------------------------------------------
                        # Verify TopicString got deleted
                        #------------------------------------------------------------------
                        topic_string=${cli_params##*=}
                        topic_string=${topic_string%%\"}
                        echolog "> This \"list ${cli_object_type}\" request should either fail with a non zero RC or ${topic_string} should not be in list "
                        cmd="${sshcmd} imaserver list ${cli_object_type}"
                        echolog "${cmd}"
                        reply=`$cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        let item_found=0
                        if [ $RC -eq 0 ] ; then
                            #------------------------------------------------------------------
                            # If list is returned verify TopicString exist
                            #------------------------------------------------------------------
                            let item_found=0
                            for reply_item in ${reply} ; do
                                if [[ "${reply_item}" == "${topic_string}" ]] ; then
                                    let item_found=1
                                    break
                               fi
                            done

                            if [[ "${item_found}" -eq "0" ]] ; then
                                echo "Verified \"${topic_string}\" deleted" 
                            else
                                echo "Failure: Found \"${topic_string}\" in list" 
                                let num_fails+=1
                            fi
                        fi    
                    elif [[ "${cli_object_type}" == "Subscription" ]] ; then 
                        #------------------------------------------------------------------
                        # Verify that the subscription has been deleted
                        #-------------------------------------------------------------
                        cmd="${sshcmd} imaserver stat ${cli_object_type} ${cli_params}"
                        echolog "${cmd}"
                        reply=`$cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        if [ $RC -eq 0 ] ; then
                            echo "Failure: RC=0 for ${cmd}" 
                            let num_fails+=1
                        fi
		      else # Not "TopicMonitor", not "Subscription"
                        #------------------------------------------------------------------
                        # Show Object by Name (this should fail)
                        #------------------------------------------------------------------
                        if [ "${A1_REST}" == "TRUE" ] ; then
                            cli_name=""
                            for item in ${cli_params} ; do
                                item_name=${item%%=*}
                                item_name=${item_name##\"}
                                item_name_lc="${item_name,,}"

                                if [[ "${item_name_lc}" == "name" ]] ; then
                                    cli_name=$item
                                    break
                                elif [[ "${item_name_lc}" == "host" ]] && [[ "${cli_object_type_lc}" == "snmptrapsubscriber" ]] ; then
                                    cli_name=$item
                                    break
                                fi
                            done
                            cmd="${sshcmd} imaserver show ${cli_object_type} ${cli_name}"
                        fi

                        echolog "${cmd}"
                        reply=`$timecmd $cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        if [[ "$expected_rc" == "p" ]] ; then
                            echolog "> This \"show ${cli_object_type}\" request should pass with status of pending delete"
                            if [[ ! (${reply} =~ "PendingAction = Delete") ]] ; then
                                let num_fails+=1
                            fi 
                        else
                            echolog "> This \"show ${cli_object_type}\" request should fail with a non zero RC"
                            if [ $RC -eq 0 ] ; then
                                let num_fails+=1
                            fi   
                        fi  
                    fi    
                fi
            fi
            if [ ${num_fails} -gt 0 ] ; then 
                echolog ">>> delete Failed"
            else
                echolog ">>> delete Passed"
            fi
        else
            echolog "Expected RC=$expected_rc"
            if [ $RC -ne $expected_rc ] ; then
                let num_fails+=1
                echolog ">>> RC compare Failed"
            else
                echolog ">>> RC compare Passed"
            fi
        fi
    fi
}


function applyObject {
    #----------------------------------------------------------------------------
    # Apply Object
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_name}"
    echolog "${cmd}" 
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        if [[ "$expected_rc" == "0" ]] ; then
            if [ $RC -ne 0 ] ; then
                let num_fails+=1
            # TODO: Is there a way to verify from the CLI
            fi
            if [ ${num_fails} -gt 0 ] ; then 
                echolog ">>> apply Failed"
            else
                echolog ">>> apply Passed"
            fi
        else
            echolog "Expected RC=$expected_rc"
            if [ $RC -ne $expected_rc ] ; then
                let num_fails+=1
                echolog ">>> RC compare Failed"
            else
                echolog ">>> RC compare Passed"
            fi
        fi    
    fi
}


function resetObject {
    #----------------------------------------------------------------------------
    # Reset Object - Deletes objects except for ones that start with "Demo" and "AdminDefault"
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd

    if [[ "${cli_object_type}" == "ALL" || "${cli_object_type}" == "all" ]] ; then
        if [ "${A1_LDAP}" == "FALSE" ] ; then
        # DOCKER does not support messaging user and group commands so we must use LDAP
            objects=" Endpoint SecurityProfile CertificateProfile ConnectionPolicy MessagingPolicy LTPAProfile MessageHub DestinationMappingRule QueueManagerConnection Queue WebUser"
        else
            objects=" Endpoint SecurityProfile CertificateProfile ConnectionPolicy MessagingPolicy LTPAProfile MessageHub DestinationMappingRule QueueManagerConnection Queue user group WebUser"
        fi
    else
        objects=${cli_object_type}
    fi
        
    for cli_object_type in ${objects} ; do
        let reset_failed=0
        #------------------------------------------------------------------
        # Get all instances of Object Type
        #------------------------------------------------------------------
        echolog " "
        if [[ "${cli_object_type}" == "WebUser" ]] ; then
            cmd="${sshcmd} imaserver user list Type=WebUI"
            echolog "${cmd}"
            listReply=`$timecmd $cmd | grep UserID`	
        elif [[ "${cli_object_type}" == "user" ]] ; then
            cmd="${sshcmd} imaserver ${cli_object_type} list Type=Messaging"
            echolog "${cmd}"
            listReply=`$timecmd $cmd | grep UserID`
        elif [[ "${cli_object_type}" == "group" ]] ; then
            cmd="${sshcmd} imaserver ${cli_object_type} list Type=Messaging"
            echolog "${cmd}"
            listReply=`$timecmd $cmd | grep GroupID`
        else
            cmd="${sshcmd} imaserver list ${cli_object_type}"
            echolog "${cmd}"
            listReply=`$timecmd $cmd`
        fi
        listRC=$?
        echo ${listReply} >> $CLI_LOG_FILE
        echolog "list RC=$listRC"
        if [[ "${listReply}" == "" && ${listRC} -ne 0 ]] ; then
            echolog "It is okay for the list action to fail if there are no objects."
        fi

        if [[ ${listReply} != "" ]] ; then
            #------------------------------------------------------------------
            # Remove all objects that were returned
            #------------------------------------------------------------------
            for item in ${listReply} ; do
                if [[ "${item:0:4}" != "Demo" && "${item}" != "UserID:" && "${item}" != "GroupID:" && "${item:0:12}" != "AdminDefault" && ! ("${item}" == "admin" && "${cli_object_type}" == "WebUser") ]] ; then 
                    #------------------------------------------------------------------
                    # Delete object
                    #------------------------------------------------------------------
                    # DestinationMappingRules need to be disabled before they can be deleted
                    if [[ ${cli_object_type} == "DestinationMappingRule" ]] ; then
                        cmd="${sshcmd} imaserver update ${cli_object_type} Name=${item} Enabled=false"
                        echolog "${cmd}"
                        reply=`$timecmd $cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                    fi

                    if [[ "${cli_object_type}" == "WebUser" ]] ; then
                         cmd="${sshcmd} imaserver user delete UserID=${item} Type=WebUI"
                    elif [[ "${cli_object_type}" == "user" ]] ; then
                        cmd="${sshcmd} imaserver ${cli_object_type} delete UserID=${item} Type=Messaging"
                    elif [[ "${cli_object_type}" == "group" ]] ; then
                        cmd="${sshcmd} imaserver ${cli_object_type} delete GroupID=${item}"
                    else
                        cmd="${sshcmd} imaserver delete ${cli_object_type} Name=${item}"
                    fi
                    echolog "${cmd}"
                    reply=`$timecmd $cmd`
                    RC=$?
                    echo ${reply} >> $CLI_LOG_FILE
                    echolog "RC=$RC"
                    if [ $RC -ne 0 ] ; then
                        #let num_fails+=1
			if [[ "${cli_object_type}" == "WebUser" && "${item}" == "admin" ]] ; then
			    echo "${item} is default system user." >> $CLI_LOG_FILE
			else
                            let reset_failed=1
                            echolog ">> Failed delete"
			fi
                    else
                        #------------------------------------------------------------------
                        # Get Object by Name (this should fail)
                        #------------------------------------------------------------------
                        echo "> This get object request should fail with RC=1" >> $CLI_LOG_FILE
                        if [[ "${cli_object_type}" == "user" ]] ; then
                            cmd="${sshcmd} imaserver ${cli_object_type} list Type=Messaging UserID=${item}"
                        elif [[ "${cli_object_type}" == "group" ]] ; then
                            cmd="${sshcmd} imaserver ${cli_object_type} list Type=Messaging GroupID=${item}"
                        else
                            cmd="${sshcmd} imaserver show ${cli_object_type} Name=${item}"
                        fi
                        echolog "${cmd}"
                        reply=`$timecmd $cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        if [ $RC -eq 0 ] ; then
                            let reset_failed=1
                            echolog ">> Failed show"
                        fi
                    fi 
                else
                    if [[ "${item}" != "UserID:" && "${item}" != "GroupID:" ]] ; then
                        echolog "Skipping ${item}"
                    fi
                fi
            done # Loop through object types

            if [ ${reset_failed} -gt 0 ] ; then
                let num_fails+=1 
            fi
        fi    

        if [ ${reset_failed} -gt 0 ] ; then
            echolog ">>> reset ${cli_object_type} Failed"
        else
            echolog ">>> reset ${cli_object_type} Passed"
        fi
    done # Loop through list of objects
}


function stopstart {
    #----------------------------------------------------------------------------
    # CLI stop and start 
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_options}"
    echolog "${cmd}"
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"
    if [[ "$expected_rc" == "0" ]] ; then
        if [ $RC -ne 0 ] ; then
	    let num_fails+=1
           echolog ">>> ${cli_action} ${cli_options} command Failed"
        else
           echolog ">>> ${cli_action} ${cli_options} command Passed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog ">>> RC compare Failed"
        else
            echolog ">>> RC compare Passed"
        fi
    fi
    if [ $RC -eq 0 ] ; then
        if [[ "${cli_options_lc}" != "help" ]] ; then 
            timeout=90
            elapsed=0 
            if [[ ${cli_options} == "" ]] ; then
                cmd="${sshcmd} status imaserver"
            else
                cmd="${sshcmd} status ${cli_options}"
            fi
            while [ $elapsed -lt $timeout ] ; do
                echo -n "*" >> $CLI_LOG_FILE
                sleep 1
                reply=`$cmd`
                if [[ "$cli_action_lc" == "stop" && "$reply" == "Status = Stopped" ]] ; then
                    echo " $reply" >> $CLI_LOG_FILE
                    break
                elif [[ "$cli_action_lc" == "start" && "$reply" =~ "Status = Running" ]] ; then
                    echo " $reply" >> $CLI_LOG_FILE
                    break
                fi
                elapsed=`expr $elapsed + 1`
            done
            if [ $elapsed -ge $timeout ] ; then
                let num_fails+=1
                echolog ">>> ${cli_action} ${cli_options} Failed to ${cli_action}"
            fi
        fi
    fi
}

function statCmd {
    #----------------------------------------------------------------------------
    # CLI stat
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_options}"
    echolog "${cmd}"
    if [[ "${cli_object_type}" != "help" && "${cli_options}" != "help" ]] ; then 
        reply=`$cmd > statOutput.csv`
    else
        reply=`$timecmd $cmd`    
    fi
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC" 
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog ">>> RC compare Failed"
        else
            echolog ">>> RC compare Passed"
        fi
    fi
}

function passthru {
    #----------------------------------------------------------------------------
    # CLI passthru
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd
    echolog " "
    cmd="${sshcmd} imaserver ${cli_action} ${cli_object_type} ${cli_params}"
    echolog "${cmd}" 
    reply=`$timecmd $cmd`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC" 
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog ">>> RC compare Failed"
        else
            echolog ">>> RC compare Passed"
        fi
    fi
}

function bedrockPassthru {
    #----------------------------------------------------------------------------
    # CLI passthru
    #----------------------------------------------------------------------------
    let num_fails=0
    local cmd="${sshcmd}"
    echolog " "

    for item in ${bedrock_cmd[@]} ; do
        # remove quotes around item
        item=${item%%\"}
        item=${item##\"}
        if [[ "${item%%=*}" == "BedrockOut" ]] ; then
            bedrockOutFile=${item##*=}
        else
            cmd="${cmd} ${item}"
        fi
    done

    echolog "${cmd}"    

    if [ -n "$bedrockOutFile" ] ; then
        reply=`$timecmd $cmd  2>&1 | tee ${bedrockOutFile}`
    else
        reply=`$timecmd $cmd`
    fi
    
    RC=$?

    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog ">>> RC compare Failed"
        else
            echolog ">>> RC compare Passed"
        fi
    fi
}

function getthensetObject {
    # Check to see if a new value was requested
    if [[ "$cli_object_type" != "" ]] ; then
        # Save original expected_rc and set to 0. We expect this get to always pass.
        original_expected_rc=$expected_rc
        expected_rc=0
        # Call get and then use returned value to do a set
        cli_object_type=${cli_object_type_get}
        cli_object_type_lc=${cli_object_type,,}
        cli_action="get"
        passthru
        # Get value from reply
        # Remove space character before and after '=' character. Do in 2 steps in case there is nothing after the =
        reply=${reply// =/=}
        original_value=${reply//= /=}

        # We can only continure if we were able to get initial value
        if [[ $num_fails -eq 0 ]] ; then
            cli_action="set"
            # restore expected_rc
            expected_rc=$original_expected_rc
            cli_object_type=${cli_object_type_set}
            cli_object_type_lc=${cli_object_type,,}
            setObject
            let total_num_fails=$num_fails
      
            # Set back to original value
            cli_object_type=${original_value%* }
            cli_object_type_lc=${cli_object_type,,}

            # We expect this set to always pass
            expected_rc=0
            cli_action="set"
            setObject
            let num_fails+=$total_num_fails
        else
            echolog ">>> Failed: getthenset could not get initial value."
        fi
    else
        echolog ">>> Failed: getthenset requires a value to set."
    fi
}

###############################################################################################
# The following functions are used to configure an external LDAP located on the client machine.
# When the Appliance A1_LDAP=FALSE the CLI user and group commands will configure LDAP instead
# of the local configuration.
###############################################################################################
### LDAP init ###
function ldap_init {
ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: o=IBM
objectclass: organization
o: IBM

EOF

ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=Manager,o=IBM
objectclass: organizationalRole
cn: Manager

EOF

ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: ou=MessageSight,o=IBM
ou: MessageSight
description: Root entry for External LDAP Testing
objectClass: top
objectClass: organizationalUnit

EOF

ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: ou=groups,ou=MessageSight,o=IBM
ou: groups
objectClass: top
objectClass: organizationalUnit

EOF

ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: ou=users,ou=MessageSight,o=IBM
ou: users
objectClass: top
objectClass: organizationalUnit

EOF
RC=$?
if [ "$RC" -eq "68" ]; then
    echo "RC: another RC=68!"
    echo "Set RC=0"
    RC=0
fi
}

### LDAP add user ###
function ldap_adduser {
reply=`$timecmd ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: uid=$USERID,ou=users,ou=MessageSight,o=IBM
uid: $USERID
objectClass: inetOrgPerson
objectClass: organizationalPerson
objectClass: person
objectClass: top
sn: $USERID
cn: $USERID
userPassword: $PASSWORD

EOF`
RC=$?
if [ "$RC" -eq "68" ]; then
    echo "RC: another RC=68!"
    echo "Set RC=0"
    RC=0
fi
echo ${reply} >> $CLI_LOG_FILE

# Can not just lowercase userid for mail because GVT characters fail case ldapadd fo fail
#mail: ${USERID,,}@test.ibm.com
}

### LDAP add group ###
function ldap_addgroup {
reply=`$timecmd ldapadd -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=$GROUP,ou=groups,ou=MessageSight,o=IBM
objectClass: groupOfNames
objectClass: top
member: uid=dummy
cn: $GROUP

EOF`
RC=$?
if [ "$RC" -eq "68" ]; then
    echo "RC: another RC=68!"
    echo "Set RC=0"
    RC=0
fi
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP add user member ###
function ldap_addusermember {
reply=`$timecmd ldapmodify -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=$GROUPMEMBERSHIP,ou=groups,ou=MessageSight,o=IBM
changetype: modify
add: member
member: uid=$USERID,ou=users,ou=MessageSight,o=IBM

EOF`
RC=$?
if [ "$RC" -eq "68" ]; then
    echo "RC: another RC=68!"
    echo "Set RC=0"
    RC=0
fi
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP add group member ###
function ldap_addgroupmember {
reply=`$timecmd ldapmodify -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=$GROUPMEMBERSHIP,ou=groups,ou=MessageSight,o=IBM
changetype: modify
add: member
member: cn=$GROUP,ou=groups,ou=MessageSight,o=IBM

EOF`
RC=$?
if [ "$RC" -eq "68" ]; then
    echo "RC: another RC=68!"
    echo "Set RC=0"
    RC=0
fi
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP remove user member ###
function ldap_removeusermember {
reply=`$timecmd ldapmodify -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=$GROUPMEMBERSHIP,ou=groups,ou=MessageSight,o=IBM
changetype: modify
delete: member
member: uid=$USERID,ou=users,ou=MessageSight,o=IBM

EOF`
RC=$?
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP remove group member ###
function ldap_removegroupmember {
reply=`$timecmd ldapmodify -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
dn: cn=$GROUPMEMBERSHIP,ou=groups,ou=MessageSight,o=IBM
changetype: modify
delete: member
member: cn=$GROUP,ou=groups,ou=MessageSight,o=IBM

EOF`
RC=$?
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP delete user ###
function ldap_deleteuser {
reply=`$timecmd ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
uid=$USERID,ou=users,ou=MessageSight,o=IBM

EOF`
RC=$?
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP delete group ###
function ldap_deletegroup {
reply=`$timecmd ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
cn=$GROUP,ou=groups,ou=MessageSight,o=IBM

EOF`
RC=$?
echo ${reply} >> $CLI_LOG_FILE
}

### LDAP clean ###
function ldap_clean {
ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
ou=groups,ou=MessageSight,o=IBM

EOF

ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
ou=users,ou=MessageSight,o=IBM

EOF

ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
ou=MessageSight,o=IBM

EOF

ldapdelete -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
cn=Manager,o=IBM

EOF

ldapdelete -r -x -c -D "cn=Manager,o=IBM" -w secret <<EOF
o=IBM

EOF
RC=$?
}

function cli_params_string2array {
    # We parse the cli_params string into an array of name=value elements.
    # Bedrock cli optionally uses quotes around the entire "name=value".
    # If there are spaces in the value you must place quotes around the "name=some value". 
    # If quotes are not used, spaces are used to seperate name=value elements.
    # If the value contains the quote character it must be escaped with a second quote (name=another""value).
    i=0
    j=0
    first=true
    unset cli_params_array
    while [ $i -lt ${#cli_params} ]
    do
        # First we concatenate the 
        if [[ ! ( "$first" == "true" && "${cli_params:$i:1}" == " " ) ]] ; then
            # If end=" " and this is a space do not include character
            if [[ !( "$end" == " " && "${cli_params:$i:1}" == " " ) ]] ; then
                cli_params_array[$j]="${cli_params_array[$j]}${cli_params:$i:1}"
            fi
        fi

        # Then we parse into an array of name=value elements
        if [[ "$first" == "true" ]] ; then
            # If the first character is a quote then the last character will be a quote otherwise it will be a space
            if [[ "${cli_params:$i:1}" == "\"" ]] ; then
                end="\""
            else
                end=" "
            fi
            first="false"
        # if not the first character then check to see if it is the last character
        elif [[ "${cli_params:$i:1}" == "${end}" ]] ; then
            # if this characters and next character is quote this is not last character 
            if [[ "${cli_params:$i:2}" == "\"\"" ]] ; then
                #cli_params_array[$j]="${cli_params_array[$j]}${cli_params:$i:1}"
                # throw away extra quote
                i=`expr $i + 1`
            else
                j=`expr $j + 1`
                first="true"
            fi
        fi

        i=`expr $i + 1`
    done

  ##echo ">>>array_size=${#cli_params_array[@]}"
  #k=0
  #while [[ $k -lt ${#cli_params_array[@]} ]] ; do
  #    echo ">>>\"${cli_params_array[$k]}\""
  #    k=`expr $k + 1`
  #done
}


function get_value {
    # get value of a name=value item in $cli_params
    # $1 name
    # results placed in $VALUE
    i=0
    VALUE=""
    while [[ $i -le ${#cli_params_array[@]} ]] ; do
        item=${cli_params_array[$i]}
        # remove quotes around item
        item=${item%%\"}
        item=${item##\"}
        item_name=${item%%=*}
        if [[ "${item_name,,}" == "$1" ]] ; then
           VALUE=${item##*=}
           break
         fi
        i=`expr $i + 1`
    done

    # The following character must be escapted for LDAP - ,\"#+<>;
    VALUE=${VALUE//\\/\\\\}
    VALUE=${VALUE//\"/\\\"}
    VALUE=${VALUE//\#/\\\#}
    VALUE=${VALUE//\+/\\\+}
    VALUE=${VALUE//\</\\\<}
    VALUE=${VALUE//\>/\\\>}
    VALUE=${VALUE//\;/\\\;}
    VALUE=${VALUE//\,/\\,}
}

function ldap_config {
    local ldap_fails=0
    echolog "[ldap_config] ${cli_action} ${cli_verb} ${cli_params}"
    cli_params_string2array

    if [[ "${cli_action}" == "user" ]] ; then
        #Get UserID
        get_value "userid"
        USERID=$VALUE 
        #echo "USERID=$VALUE"           
        case ${cli_verb} in
            'add')
                if [[ "${USERID}" == "" ]] ; then
                    echolog ">>> Failed: UserID is required."
                else
                    #Get password
                    get_value "password"
                    PASSWORD=$VALUE
                    #echo "PASSWORD=$VALUE" 
                    ldap_adduser

                    if [[ $RC -ne 0 ]] ; then
	                 let ldap_fails+=1
                        echo "ldap_add returned $RC" >> $CLI_LOG_FILE
                    fi
                    #Check for one or more GroupMembership and add user if found
                    for item in $cli_params ; do
                        # remove quotes around item
                        item=${item%%\"}
                        item=${item##\"}
                        item_name=${item%%=*}
                        if [[ "${item_name,,}" == "groupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_addusermember
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        fi
                    done
                fi
            ;;

            'delete')
                if [[ "${USERID}" == "" ]] ; then
                    echolog ">>> Failed: UserID is required."
                else
                    ldap_deleteuser
                    if [[ $RC -ne 0 ]] ; then
                        let ldap_fails+=1
                        echo "ldapdelete returned $RC" >> $CLI_LOG_FILE
                    fi
                fi    
            ;;

            'list')
                if [[ "${USERID}" == "" ]] ; then
                    CN=""
                else
                    CN="uid=${USERID},"
                fi
                reply=`ldapsearch -xLLL -b "${CN}ou=users,ou=MessageSight,o=ibm"`
                RC=$?
                echo ${reply} >> $CLI_LOG_FILE
                if [[ $RC -ne 0 ]] ; then
                    let ldap_fails+=1
                    echo "ldap_search returned $RC" >> $CLI_LOG_FILE
                fi
            ;;

            'edit')
                if [[ "${USERID}" == "" ]] ; then
                    echolog ">>> Failed: UserID is required."
                else
                    #Check for one or more AddGroupMembership or RemoveGroupMembership and add or remove user if found
                    for item in $cli_params ; do
                        # remove quotes around item
                        item=${item%%\"}
                        item=${item##\"}
                        item_name=${item%%=*}
                        if [[ "${item_name,,}" == "addgroupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_addusermember
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        elif [[ "${item_name,,}" == "removegroupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_removeusermember                           
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        fi
                    done
                fi
            ;;

            # These are special case verbs used to initilize and clean the LDAP (they are not supported by CLI)
            'init') 
                ldap_init
                if [[ $RC -ne 0 ]] ; then
                    let ldap_fails+=1
                    echo "ldap init script returned $RC" >> $CLI_LOG_FILE
                fi
            ;;

            'clean')
                ldap_clean
                if [[ $RC -ne 0 ]] ; then
                    let ldap_fails+=1
                    echo "ldap clean script returned $RC" >> $CLI_LOG_FILE
                fi
            ;;

        esac

    elif [[ "${cli_action}" == "group" ]] ; then
        #Get GROUP
        get_value "groupid"
        GROUP=$VALUE

        case ${cli_verb} in
            'add')
                if [[ "${GROUP}" == "" ]] ; then
                    echolog ">>> Failed: GroupID is required."
                else
                    ldap_addgroup
                    if [[ $RC -ne 0 ]] ; then
	                 let ldap_fails+=1
                        echo "ldapadd returned $RC" >> $CLI_LOG_FILE
                    fi
                    #Check for one or more GroupMembership and add group if found
                    for item in $cli_params ; do
                        # remove quotes around item
                        item=${item%%\"}
                        item=${item##\"}
                        item_name=${item%%=*}
                        if [[ "${item_name,,}" == "groupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_addgroupmember
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        fi
                    done
                fi
            ;;

            'delete')
                if [[ "${GROUP}" == "" ]] ; then
                    echolog ">>> Failed: GroupID is required."
                else
                    ldap_deletegroup
                    if [[ $RC -ne 0 ]] ; then
                        let ldap_fails+=1
                        echo "ldapdelete returned $RC" >> $CLI_LOG_FILE
                    fi
                fi
             ;;

            'list')
                if [[ "${GROUP}" == "" ]] ; then
                    CN=""
                else
                    CN="cn=${GROUP},"
                fi
                reply=`ldapsearch -xLLL -b "${CN}ou=groups,ou=MessageSight,o=ibm"`
                RC=$?
                echo ${reply} >> $CLI_LOG_FILE
                if [[ $RC -ne 0 ]] ; then
                    let ldap_fails+=1
                    echo "ldap_search returned $RC" >> $CLI_LOG_FILE
                fi
            ;;

            'edit')
                if [[ "${GROUP}" == "" ]] ; then
                    echolog ">>> Failed: GroupID is required."
                else
                    #Check for one or more AddGroupMembership or RemoveGroupMembership and add or remove group if found
                    for item in $cli_params ; do
                        # remove quotes around item
                        item=${item%%\"}
                        item=${item##\"}
                        item_name=${item%%=*}
                        if [[ "${item_name,,}" == "addgroupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_addgroupmember
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        elif [[ "${item_name,,}" == "removegroupmembership" ]] ; then
                            GROUPMEMBERSHIP=${item##*=}
                            ldap_removegroupmember
                            if [[ $RC -ne 0 ]] ; then
                                let ldap_fails+=1
                                echo "ldap_modify returned $RC" >> $CLI_LOG_FILE
                            fi
                        fi
                    done
                fi
            ;;

        esac

    else
        echolog ">>> Failed: Invalid   Valid option are add, modify, delete and list"
    fi

    # To make LDAP compatible when the CLI behavior we only return RC=1 for failures
    if [[ "ldap_fails" -ne 0 ]] ; then
        RC=1
    else
        rc=0
    fi

    echolog "RC=$RC" 

    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog -e ">>> ${cli_action} ${cli_verb} Failed\n"
        else
            echolog -e ">>> ${cli_action} ${cli_verb} Passed\n"
        fi
    fi
}


function reformatCliParams_json {
#set -x
    # Refomat cli_params by make sure object name is first and remove "name="
    # When building rest_payload there is no need to put single quot around sting, bash does the automatically (why?)
    rest_name=""
    BEGIN_JSON="\\{"
    rest_params=${BEGIN_JSON}
    for item in ${cli_params} ; do
     #echo "item=$item" 
     ## if there is a equal character in item assume that this item does not belong to previous value and handle as normal
      if [[ "${item}" =~ "=" ]] ; then  
        # remove quots
        item=${item%%\"}
        item=${item##\"}
        # get name
        item_name=${item%%=*}
        item_name_lc="${item_name,,}"
        # get value       
        item_value=${item:${#item_name}+1}  
        if [[ "${item_name_lc}" == "name" || "${item_name_lc}" == "host" || "${item_name_lc}" == "snmptrapsubscriber" ]] ; then
            rest_name="${item_value}"

        #!!! If MessagingPolicy we try to use the DestinationType to rename to the new Policy name 
        #    However if this command is an update and does not include DestinationType we will do this renaming later
        elif [[ ${rest_object_type} == "MessagingPolicy" && "${item_name_lc}" == "destinationtype" ]] ; then
            case ${item_value,,} in
              'topic' )
                rest_object_type="TopicPolicy"
                ;;
              'queue' )
                rest_object_type="QueuePolicy"
                ;;
              'subscription' )
                rest_object_type="SubscriptionPolicy"
                ;;
              *)
                echolog "DestinationType of ${item_value} is invalid"
                ;;
 
            esac
        else
            if [[ "${rest_params}" != "${BEGIN_JSON}" ]] ; then
                rest_params="${rest_params},"
            fi
            # don't put quots around value when "Type":"Number"
            if [[ ${item_value} =~ ^[0-9]+$ && 
                ( "${item_name_lc}" == "tracemessagedata" ||
                  "${item_name_lc}" == "tracebackup" ||
                  "${item_name_lc}" == "tracebackupcount" ||
                  "${item_name_lc}" == "pluginremotedebugport" ||
                  "${item_name_lc}" == "pluginmaxheapsize" ||
                  "${item_name_lc}" == "fixdndpointinterface" ||
                  "${item_name_lc}" == "port" ||
                  "${item_name_lc}" == "maxlogsize" ||
                  "${item_name_lc}" == "rotationcount" ||
                  #"${item_name_lc}" == "maxmessagesize" ||
                  "${item_name_lc}" == "maxsendsize" ||
                  "${item_name_lc}" == "maxmessages" ||
                  "${item_name_lc}" == "ruletype" ||
                  "${item_name_lc}" == "groupcachetimeout" ||
                  "${item_name_lc}" == "timeout" ||
                  "${item_name_lc}" == "cachetimeout" ||
                  "${item_name_lc}" == "maxconnections" ||
                  "${item_name_lc}" == "discoverytimeout" ||
                  "${item_name_lc}" == "heartbeattimeout" ||
                  "${item_name_lc}" == "timeout" ||
                  "${item_name_lc}" == "maxlogsize" ) ]]
                then
                rest_params="${rest_params}\\\"${item_name}\\\":${item_value}" 

            # force true and false to lower case and don't put quots around booleans
            elif [[ ${item_value,,} == "true" || ${item_value,,} == "false" ]] ; then
                rest_params="${rest_params}\\\"${item_name}\\\":${item_value,,}"

            #!!! If Endpoint we must move MessagingPolicies into the new policy names
            elif [[ "${rest_object_type}" == "Endpoint" && "${item_name_lc}" == "messagingpolicies" ]] ; then  
                # Separate MessagingPolicies into the 3 new types
                echo ">>>>item_value=${item_value}" 
                saveIFS=$IFS
                IFS=$','
                rest_TopicPolicies=""
                rest_QueuePolicies=""
                rest_SubscriptionPolicies=""
                # We assume there are no spaces in item_vaue                
                for policy_name in ${item_value} ; do
                    #echo ">>>>policy_name=\"$policy_name\""
                    save_rest_object_type=${rest_object_type}
                    IFS=$saveIFS
                    # If DemoMessagingPolicy rename to DemoTopicPolicy and assign to TopicPolicies   
                    if [[ ${policy_name} == "DemoMessagingPolicy" ]] ; then
                        if [[ "${rest_TopicPolicies}" == "" ]] ; then
                            rest_TopicPolicies="DemoTopicPolicy"
                        else
                            rest_TopicPolicies="${rest_TopicPolicies},DemoTopicPolicy"
                        fi
                        echo ">>>>Policy${policy_name} renamed to DemoTopicPolicy and assigned to TopicPolicies"
                    else 
                        rest_object_type="MessagingPolicy"
                        cli_params="Name=${policy_name}"
                        get_policy_type
                        IFS=$','
                        case ${rest_policy_type} in
                          'TopicPolicy' )
                            if [[ "${rest_TopicPolicies}" == "" ]] ; then
                                rest_TopicPolicies="${policy_name}"
                            else
                                rest_TopicPolicies="${rest_TopicPolicies},${policy_name}"
                            fi
                            ;;
                          'QueuePolicy' )
                            if [[ "${rest_QueuePolicies}" == "" ]] ; then
                                rest_QueuePolicies="${policy_name}"
                            else
                                rest_QueuePolicies="${rest_QueuePolicies},${policy_name}"
                            fi
                            ;;
                          'SubscriptionPolicy' )
                            if [[ "${rest_SubscriptionPolicies}" == "" ]] ; then
                                rest_SubscriptionPolicies="${policy_name}"
                            else
                                rest_SubscriptionPolicies="${rest_SubscriptionPolicies},${policy_name}"
                            fi
                            ;;
                          *)
                            echolog "Policy Type of \"${rest_policy_type}\" is invalid in Policy ${policy_name}"
                            ;;
                        esac
                    fi
                done
                rest_params="${rest_params}\\\"TopicPolicies\\\":\\\"${rest_TopicPolicies}\\\",\\\"QueuePolicies\\\":\\\"${rest_QueuePolicies}\\\",\\\"SubscriptionPolicies\\\":\\\"${rest_SubscriptionPolicies}\\\""
                rest_object_type=${save_rest_object_type}
                IFS=$saveIFS
                rest_object_type="Endpoint"

            #!!! If MessagingPolicy we must change Destination to Topic, Queue or Subscription
            elif [[ (${rest_object_type} == "MessagingPolicy" || ${rest_object_type} == "TopicPolicy"|| ${rest_object_type} == "QueuePolicy" || ${rest_object_type} == "SubscriptionPolicy") && "${item_name_lc}" == "destination" ]] ; then
                # scan back through the parameters to find DestinationType to determine how to change Destination
                DT_found="false"
                for item2 in ${cli_params} ; do
                    # remove quots
                    item2=${item2%%\"}
                    item2=${item2##\"}
                    # get name
                    item_name2=${item2%%=*}
                    item_name2_lc="${item_name2,,}"
                    # get value       
                    item_value2=${item2:${#item_name2}+1}
                    # Before RESTAPIs you were not allowed to change DestinationType.

                    if [[ "${item_name2_lc}" == "destinationtype" ]] ; then
                         DT_found="true"
                         case ${item_value2,,} in
                          'topic' )
                            item_name="Topic"
                            ;;
                          'queue' )
                            item_name="Queue"
                            ;;
                          'subscription' )
                            item_name="Subscription"
                            ;;
                          *)
                            echolog "Destination of ${item_value2} is invalid"
                            ;;
                        esac
                    fi
                done

                if [[ "${DT_found}" == "false" && "${cli_action}" == "update" ]] ; then

                    saved_item_value=$item_value
                    get_policy_type
                    item_value=$saved_item_value

                    rest_object_type="${rest_policy_type}"
                    case ${rest_policy_type} in
                      'TopicPolicy' )
                        item_name="Topic"
                        ;;
                      'QueuePolicy' )
                        item_name="Queue"
                        ;;
                      'SubscriptionPolicy' )
                        item_name="Subscription"
                        ;;
                      *)
                        echolog "DestinationType not provided and ${rest_policy_type} was not found"
                        ;;
                    esac
                fi
  
                rest_params="${rest_params}\\\"${item_name}\\\":\\\"${item_value}\\\""
            else
                rest_params="${rest_params}\\\"${item_name}\\\":\\\"${item_value}\\\""
            fi
        fi
      else #Assume that item belongs to the previous value
        # remove ending quot
        rest_params=${rest_params%%\\\"}
        # append item
        rest_params="${rest_params}\ ${item%%\"}\\\""
      fi 
      #echo "rest_params=$rest_params"
    done # end of for item in ${cli_params}

    #!!! If MessagingPolicy still exist this was probably a update and did not include DestinationType
    #    We must determine if Topic, Queue or Subscription and change rest_object_type
    if [[ "${rest_object_type}" == "MessagingPolicy" && "${cli_action}" == "update" ]] ; then
        get_policy_type
        if [[ "${rest_policy_type}" != "" ]] ; then
            rest_object_type="${rest_policy_type}"
        else
            echolog ">>>>Failed: Not able to convert CLI because command does not contain DestinationType and MessagingPolicy ${item_name} was not found."
            rest_params=""
            let num_fails+=1
        fi  
    fi

    if [[ "${rest_params}" == "" ]] ; then
        rest_payload=""
    else
        # We must escape parentheses so that eval does not return an error
        rest_params=${rest_params//\(/\\\(}
        rest_params=${rest_params//\)/\\\)}

        rest_params="${rest_params}}"
    
        if [[ "${rest_object_type}" == "" ]] ; then
            rest_payload="${rest_params}"
        elif [[ "${rest_name}" == "" ]] ; then
            rest_payload="\\{\\\"$rest_object_type\\\":${rest_params}}"
        else
            rest_payload="\\{\\\"$rest_object_type\\\":\\{\\\"${rest_name}\\\":${rest_params}}}"
        fi
    fi
}

function reformatCliParams_json_TopicMonitor {
#set -x
    # While the json is in an array formation this code only support a single item in the array
    # When building rest_payload there is no need to put single quot around sting, bash does the automatically (why?)
    rest_name=""
    rest_params=""
    for item in ${cli_params} ; do
        ## if there is a equal character in item assume that this item does not belong to previous value and handle as normal
        if [[ "${item}" =~ "=" ]] ; then  
            # remove quots
            item=${item%%\"}
            item=${item##\"}
            # get name
            item_name=${item%%=*}
            item_name_lc="${item_name,,}"
            # get value       
            item_value=${item:${#item_name}+1}
            # Only add TopicString items to the array (There shouldn't be any others)
            if [[ "${item_name_lc}" == "topicstring" ]] ; then
                if [[ "${rest_params}" != "" ]] ; then
                    rest_params="${rest_params},"
                fi
                rest_params="${rest_params}\\\"${item_value}\\\""
            fi
        else #Assume that item belongs to the previous value
            # remove ending quot
            rest_params=${rest_params%%\\\"}
            # append item
            rest_params="${rest_params}\ ${item%%\"}\\\""
        fi 
    done

    # We must escape parentheses so that eval does not return an error
    rest_params=${rest_params//\(/\\\(}
    rest_params=${rest_params//\)/\\\)}

    if [[ "${rest_object_type}" == "" ]] ; then
        rest_payload="${rest_params}"
    else
        rest_payload="\\{\\\"$rest_object_type\\\":[${rest_params}]}"
    fi
#set +x
}

function reformatCliParams_json_TrustedCertificate {
#set -x
    # While the json is in an array format, this code only support a single item in the array
    # When building rest_payload there is no need to put single quot around sting, bash does the automatically (why?)
    rest_name=""
    rest_params=""
    for item in ${cli_params} ; do
        ## if there is a equal character in item assume that this item does not belong to previous value and handle as normal
        if [[ "${item}" =~ "=" ]] ; then  
            # remove quots
            item=${item%%\"}
            item=${item##\"}
            # get name
            item_name=${item%%=*}
            item_name_lc="${item_name,,}"
            # get value       
            item_value=${item:${#item_name}+1}

            if [[ "${rest_params}" != "" ]] ; then
                rest_params="${rest_params},"
            fi

            # force booleans to lower case
            if [[ ${item_value,,} == "true" || ${item_value,,} == "false" ]] ; then
                rest_params="${rest_params}\\\"${item_name}\\\":${item_value,,}"
            else
                rest_params="${rest_params}\\\"${item_name}\\\":\\\"${item_value}\\\""
            fi

        else #Assume that item belongs to the previous value
            # remove ending quot
            rest_params=${rest_params%%\\\"}
            # append item
            rest_params="${rest_params}\ ${item%%\"}\\\""
        fi 
    done

    # We must escape parentheses so that eval does not return an error
    rest_params=${rest_params//\(/\\\(}
    rest_params=${rest_params//\)/\\\)}

    if [[ "${rest_object_type}" == "" ]] ; then
        rest_payload="${rest_params}"
    else
        rest_payload="\\{\\\"$rest_object_type\\\":[\\{${rest_params}}]}"
    fi
#set +x
}

function reformatCliParams_json_Stop {
#set -x
    # When building rest_payload there is no need to put single quot around sting, bash does the automatically (why?)
    force="false"
    rest_payload=""
    for item in ${cli_params} ; do
        # remove quots
        item=${item%%\"}
        item=${item##\"}
        # get service and check for force
        if [[ "${item,,}" == "imaserver" || "${item,,}" == "mqconnectivity" ]] ; then
            rest_payload="\\{\\\"Service\\\":\\\"${item}\\\""
        elif [[ ${item,,} == "force" ]] ; then
            force="true"
        fi
    done

    # If rest_payload is not set default to imaserver
    if [[ ${rest_payload} == "" ]] ; then
        rest_payload="\\{\\\"Service\\\":\\\"imaserver\\\""
    fi

    # We must escape parentheses so that eval does not return an error
    rest_payload=${rest_payload//\(/\\\(}
    rest_payload=${rest_payload//\)/\\\)}

    if [[ "${force}" == "true" ]] ; then
        rest_payload="${rest_payload},\\\"Force\\\":true}"
    else
        rest_payload="${rest_payload}}"
    fi
    
#set +x
}


function reformatCliParams_uri {
#set -x
    # Refomat cli_params for REST GET
    rest_name_1=$1
    rest_name_2=$2
#set +x
    rest_value_1=""
    rest_value_2=""
    rest_uri_extension="${rest_object_type}"
    rest_filter=""
    for item in ${cli_params} ; do
        # remove quots
        item=${item%%\"}
        item=${item##\"}
         # get name
        item_name=${item%%=*}
        item_name_lc="${item_name,,}"
        # get value
        item_value=${item:${#item_name}+1}
        # replace select characters with uri escape represention
        item_value=${item_value//%/%25}
        item_value=${item_value//#/%23}
        item_value=${item_value//$/%24}
        item_value=${item_value//&/%26}
        item_value=${item_value//\?/%3F}

            if [[ "${item_name_lc}" == "${rest_name_1,,}" ]] ; then
                rest_value_1="${item_value}"
            elif [[ "${item_name_lc}" == "${rest_name_2,,}" ]] ; then
                rest_value_2="${item_value}"
            else
                if [[ "${rest_filter}" != "" ]] ; then
                    rest_filter="${rest_filter}&"
                fi
                if [[ ${item_value,,} == "true" || ${item_value,,} == "false" ]] ; then
                    # force true and false to lower case
                    rest_filter="${rest_filter}${item_name}=${item_value,,}"
                else
                    rest_filter="${rest_filter}${item}"
                fi
            fi
    done
    
    if [[ "${rest_value_1}" != "" ]] ; then
        if [[ "${rest_value_2}" == "" ]] ; then
            rest_uri_extension="${rest_uri_extension}/${rest_value_1}"
        else
            rest_uri_extension="${rest_uri_extension}/${rest_value_1}/${rest_value_2}"
        fi
    fi
    if [[ "${rest_filter}" != "" ]] ; then
        rest_uri_extension="${rest_uri_extension}?${rest_filter}"
    fi
}


function rest_verifyRC {
    if [[ "$expected_rc" == "x" ]] ; then
        # Do not check RC
        echolog ">>> RC was not checked - Passed"
    elif [[ "$expected_rc" == "-" ]] ; then
        # Non-zero RC expected
        if [ $RC -ne 0 ] ; then
            # Non-zero RC expected
            echolog ">>> RC checked for non-zero - Passed"
        else
	     let num_fails+=1
            echolog ">>> RC checked for non-zero - Failed"
        fi
    else
        echolog "Expected RC=$expected_rc"
        if [ $RC -ne $expected_rc ] ; then
            let num_fails+=1
            echolog ">>> RC compare Failed"
        else
            echolog ">>> RC compare Passed"
        fi
    fi
}


function get_policy_type {
   # This assumes that there is only one Policy type with this name
   rest_policy_type=
   if [[ "${rest_object_type}" == "MessagingPolicy" ]] ; then
       # We don't know which MessagingPolicy type this is so we must try all 3
       rest_policy_type=
       rest_object_type_list="TopicPolicy QueuePolicy SubscriptionPolicy"
       for rest_object_type in ${rest_object_type_list} ; do
          #cli_params=${line[@]:4}
          rest_type="configuration"
          reformatCliParams_uri name
          #echo ">>>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
          rest_get_silent
          if [[ $RC -eq 0 ]] ; then
              rest_policy_type=${rest_object_type}
              break
          fi
       done
       echo ">>>>Policy${cli_params} rest_policy_type=${rest_policy_type}"
       rest_object_type="MessagingPolicy"
    else
       echo ">>>>Cannot use get_policy_type for a ${rest_policy_type} policy."
    fi
}

function rest_get {
    #cmd="${python_path}/get.py -s ${adminPort} ${rest_object_type} ${rest_name}"
    cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}/${rest_uri_extension}"
    echolog "${cmd}"
    reply=`${cmd}`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    if [[ "${rest_type}" == "monitor" ]] ; then
        echo ${reply} > "reply.json"
    fi
    echolog "RC=$RC"
    rest_verifyRC
}


function rest_get_noverify {
    cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}/${rest_uri_extension}"
    echo "${cmd}" 
    reply=`${cmd}`
    RC=$?
    echo ${reply}
    echo "RC=$RC"
}


function rest_get_silent {
    cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}/${rest_uri_extension}"
    $cmd 1>/dev/null 2>&1
    RC=$?
}


function rest_put_file {
    #cmd="${python_path}/put.py -s ${adminPort} ${rest_object_type} ${rest_name}"
    cmd="curl -X PUT ${curl_silent_options} ${curl_timeout_options} -T ${rest_src_name} ${USERPW} http://${adminPort}${BASE_URI}${rest_object_type}/${rest_dst_name}"
    echolog "${cmd}"
    reply=`${cmd}`
    RC=$?
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"

    rest_verifyRC
}

function rest_post {

  if [[ "${rest_payload}" != "" ]] ; then
    # At this point the only "${" characters should belong to MessageSight variables (and not automation variables).
    # We must escape them so that eval does not try to replace them and must also escape the '{' character
    rest_payload=${rest_payload//\${/\\\$\\\{};
    # We also need to escape "$SYS"
    rest_payload=${rest_payload//\$SYS/\\\$SYS};
    # We must also escape & 
    rest_payload=${rest_payload//\&/\\\&};
   
    # We must also escape the ( in certain cases
    #rest_payload=${rest_payload//Team(imaclient/\Team\\(imaclient}

    #cmd="${python_path}/post.py -s '${rest_payload}'"
    cmd="curl -X POST ${curl_silent_options} ${curl_timeout_options} -d ${rest_payload} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}"
    echolog ${cmd}

    # We need to use eval so that bash can handle the space character
#set -x
    reply=$(eval ${cmd})
    RC=$?
#set +x
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"

    rest_verifyRC

    if [[ "$expected_rc" == "0" ]] ; then
        if [ $RC -ne 0 ] ; then
            # Did not get the expected RC=0
            echolog " >>> Expected to Pass so running command again without -f to try to get more information."
            cmd="curl -X POST -s -S ${curl_timeout_options} -d ${rest_payload} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}"
            reply=$(eval ${cmd})
            RC=$?
            echolog ${reply} 
            echolog "RC=$RC"
        else
            # Do a get so that we can use to debug.  We do not verify that the get matches the item specified in the post.
            if [[ ${rest_type} == "configuration" ]] ; then
                if [[ "${rest_object_type}" == "" ]] ; then
                    # Must be singleton, get name (string before :) and remove braces and quotes
                    rest_uri_extension=${rest_payload%%\\\":*}
                    rest_uri_extension=${rest_uri_extension##*\"}
                elif [[ "${rest_name}" == "" ]] ; then
                    # No name so just use object type 
                    rest_uri_extension="${rest_object_type}"              
                else
                    # Names need to replace select characters with uri escape represention
                    rest_name=${rest_name//%/%25}
                    rest_name=${rest_name//#/%23}
                    rest_name=${rest_name//$/%24}
                    rest_name=${rest_name//&/%26}
                    rest_name=${rest_name//\?/%3F}
 
                    rest_uri_extension="${rest_object_type}/${rest_name}"              
                fi

                rest_get_noverify

                if [ $RC -ne 0 ] ; then
                    echolog ">>>>Warning: Could not GET object after doing a POST."
                #    let num_fails+=1
                fi  
            fi
        fi
    fi
    
  else
  # Bridge RESTAPI Allows POST with NULL Payloads
  # OMITTED:  -d ${rest_payload}  from the 2 POST CURL URIs since there is NO payload
  
    cmd="curl -X POST ${curl_silent_options} ${curl_timeout_options}  ${USERPW} http://${adminPort}${BASE_URI}${rest_type}"
    echolog ${cmd}

    # We need to use eval so that bash can handle the space character
#set -x
    reply=$(eval ${cmd})
    RC=$?
#set +x
    echo ${reply} >> $CLI_LOG_FILE
    echolog "RC=$RC"

    rest_verifyRC

    if [[ "$expected_rc" == "0" ]] ; then
        if [ $RC -ne 0 ] ; then
            # Did not get the expected RC=0
            echolog " >>> Expected to Pass so running command again without -f to try to get more information."
            cmd="curl -X POST -s -S ${curl_timeout_options}  ${USERPW} http://${adminPort}${BASE_URI}${rest_type}"
            reply=$(eval ${cmd})
            RC=$?
            echolog ${reply} 
            echolog "RC=$RC"
        else
            # Do a get so that we can use to debug.  We do not verify that the get matches the item specified in the post.
            if [[ ${rest_type} == "configuration" ]] ; then
                if [[ "${rest_object_type}" == "" ]] ; then
                    # Must be singleton, get name (string before :) and remove braces and quotes
                    rest_uri_extension=${rest_payload%%\\\":*}
                    rest_uri_extension=${rest_uri_extension##*\"}
                elif [[ "${rest_name}" == "" ]] ; then
                    # No name so just use object type 
                    rest_uri_extension="${rest_object_type}"              
                else
                    # Names need to replace select characters with uri escape represention
                    rest_name=${rest_name//%/%25}
                    rest_name=${rest_name//#/%23}
                    rest_name=${rest_name//$/%24}
                    rest_name=${rest_name//&/%26}
                    rest_name=${rest_name//\?/%3F}
 
                    rest_uri_extension="${rest_object_type}/${rest_name}"              
                fi

                rest_get_noverify

                if [ $RC -ne 0 ] ; then
                    echolog ">>>>Warning: Could not GET object after doing a POST."
                #    let num_fails+=1
                fi  
            fi
        fi
    fi
  fi
}


function rest_delete {
    #cmd="${python_path}/delete.py -s ${adminPort} ${rest_object_type} ${rest_name}"
    cmd="curl -X DELETE ${curl_silent_options} ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}/${rest_uri_extension}"
    echolog "${cmd}"
    reply=`${cmd}`
    RC=$?
    echolog ${reply}
    echolog "RC=$RC"

    rest_verifyRC

    if [[ "$expected_rc" == "0" && "$RC" != "0" ]] ; then
        # Did not get the expected RC=0
        echolog " >>> Expected to Pass so running command again without -f to try to get more information."
        cmd="curl -X DELETE -s -S ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}${rest_type}/${rest_uri_extension}"
        reply=`${cmd}`
        RC=$?
        echolog ${reply}
        echolog "RC=$RC"
    fi
}


function run_action {
            echolog -e "\n${line}"
            # Replace ISMsetup variables with actual values (e.g. ${M1_IPv4_1} gets changed 10.10.1.211)
            # We must escape the open brace on JSON but not on replacement variables so that eval does not remove them
            # we assume the JSON braces will always be followed by quot and replacement variables will not
            line=${line//\{\"/\\\{\"}
            # We must also escape the quotes so that eval does not remove them
            line=${line//\"/\\\"}
            # We must also escape parentheses so that eval does not return an error
            line=${line//\(/\\\(}
            line=${line//\)/\\\)}
            # We must also escape the open brace in the MessageSight variables
            line=${line//\$\{UserID\}/\$\\\{UserID\}}
            line=${line//\$\{CommonName\}/\$\\\{CommonName\}}
            line=${line//\$\{ClientID\}/\$\\\{ClientID\}}
            line=${line//\$\{GroupID\}/\$\\\{GroupID\}}
            # We must also escape & 
            line=${line//\&/\\\&}
            # Replace all {SPACE}, and {COMMA} with spaces and commas
            line=${line//\{SPACE\}/ }
            line=${line//\{COMMA\}/\\\,}

            line=($(eval echo ${line}))

            if [[ ${#line[@]} -lt 3 ]] ; then
                echolog "Failure: Problem parsing input file: ${CLI_INPUT_FILE}"
                echo "Each line must contain minimum of 3 item: <cli_section> <expected_rc> <cli_action>"
                echo "\"${line[@]}\""
                exit 1
            fi

            expected_rc=${line[1]}
            # only a positive integer or 'x', '-' or 'p' are allowed.
            if [[ ! ("$expected_rc" =~ ^[0-9]+$ || "$expected_rc" =~ ^[-xp]$) ]] ; then
                echolog "Failure: Problem parsing input file: ${CLI_INPUT_FILE}"
                echo "expected_rc (second item on line) must be a positive integer, 'x', '-' or 'p'"
                echo "\"${line[@]}\""
                exit 1
            fi
            cli_action=${line[2]}

            # Call the apropriate function based on the first string in cli_action
            cli_action_lc="${cli_action,,}"
            case ${cli_action} in

              'set')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=""
                    cli_params=${line[@]:3}
                    reformatCliParams_json

                    rest_type="configuration"
                    echo ">>>${rest_type} ${cli_action} object=${rest_object_type} name=${rest_name} payload=${rest_payload}"
                    rest_post
                else
                    cli_object_type=${line[3]}
                    cli_object_type_lc="${cli_object_type,,}"
                    cli_params=${line[@]:4}

                    #echo ">>>set cli_object_type=$cli_object_type cli_params=$cli_params"
                    #echo ">>>set cli_object_type_lc=$cli_object_type_lc"
                    setObject
                fi
                ;; 

              'create' | 'update' )

                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=${line[3]}
                    cli_params=${line[@]:4}
                    if [[ "${rest_object_type}" == "TopicMonitor" ]] ; then
                        reformatCliParams_json_TopicMonitor 
                    else
                        reformatCliParams_json
                    fi

                    rest_type="configuration"
                    echo ">>>${rest_type} ${cli_action} object=${rest_object_type} name=${rest_name} payload=${rest_payload}"
                    rest_post
                else

                    cli_object_type=${line[3]}
                    cli_object_type_lc="${cli_object_type,,}"
                    cli_params=${line[@]:4}

                    #echo ">>>$cli_action object=$cli_object_type params=$cli_params"
                    createUpdateObject
                fi
                ;;

              'delete')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=${line[3]}
                    if [[ ${rest_object_type} == "LDAP" ]] ; then
                        # REST does not support DELETE LDAP so we must do POST LDAP disable
                        rest_type="configuration"
                        cli_params="Enabled=false"
                        reformatCliParams_json
                        rest_post
                    else
                        if [[ ${#line[@]} -gt 3 ]] ; then
                            cli_params=${line[@]:4}
                            #!!! If MessagingPolicy we must first determine if Topic, Queue or Subscription
                            if [[ "${rest_object_type}" == "MessagingPolicy" && "${cli_params}" != "" ]] ; then
                                get_policy_type
                                rest_object_type="${rest_policy_type}"
                                if [[ "$rest_object_type" == "" ]] ; then
                                    echolog ">>>>Warning: Before we can DELETE a MessagingPolicy we must first do GETs to determine the new policy type and a policy by this name was not found."
                                    RC=1
                                    rest_verifyRC
                                fi
                            fi 

                            if [[ "$rest_object_type" != "" ]] ; then

                                if [[ "${rest_object_type}" == "Subscription" ]] ; then
                                    rest_type="service"
                                    reformatCliParams_uri ClientID SubName
                                elif [[ "${rest_object_type}" == "TrustedCertificate" ]] ; then
                                    rest_type="configuration"
                                    reformatCliParams_uri SecurityProfileName TrustedCertificate
                                elif [[ "${rest_object_type}" == "TopicMonitor" ]] ; then
                                    rest_type="configuration"
                                    reformatCliParams_uri TopicString
                                else
                                    rest_type="configuration"
                                    reformatCliParams_uri name
                                fi

                                echo ">>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
                                rest_delete
                            fi
                        fi
                    fi
                else # old CLI
                    cli_object_type=${line[3]}
                    cli_object_type_lc="${cli_object_type,,}"
                    cli_params=${line[@]:4}
     
                    #echo ">>>delete object=$cli_object_type params=$cli_params" 
                   deleteObject
                fi 
                ;;
    
              'apply')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    if [[ "${line[@]:4}" =~ "TrustedCertificate=" ]] ; then
                        rest_object_type="TrustedCertificate"
                        cli_params=${line[@]:4}
                        reformatCliParams_json_TrustedCertificate

                        rest_type="configuration"
                        echo ">>>${rest_type} ${cli_action} object=${rest_object_type} name=${rest_name} payload=${rest_payload}"
                        rest_post
                    else
                        echo "${line[@]}"
                        echo ">>>Warning: \"${cli_action} ${line[@]:3}\" is not supported for REST." >> $CLI_LOG_FILE
                    fi
                else
                    cli_object_type=${line[3]}
                    cli_name=${line[@]:4}
   
                    #echo ">>>import object=$cli_object_type name=$cli_name"
                    applyObject
                fi
                ;;
    		
              'reset')
                cli_object_type=${line[3]}
    
                #echo ">>>reset object=$cli_object_type"
                resetObject
                ;;

              'stat')

                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=${line[3]}
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_params=${line[@]:4}
                        reformatCliParams_uri
                    else
                        rest_uri_extension=
                    fi
                    rest_type="monitor"

                    echo ">>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
                    rest_get
                else 
                    cli_object_type=${line[3]}
                    cli_options=${line[@]:4}

                    #echo ">>>stat object=$cli_object_type"
                    statCmd
                fi   
                ;;

              'start' | 'stop')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    cli_params=${line[@]:3}
#set -x
                    if [[ ${cli_action} == "start" || ( ${cli_action} == "stop" && ${cli_params} =~ "force" ) ]] ; then
                        #use docker start $A1_SRVCONTID
#                        a_srvcontid="A${APPLIANCE}_SRVCONTID"
#                        a_srvcontidVal=$(eval echo \$${a_srvcontid})
                        cmd="${sshcmd} docker ${cli_action} ${a_srvcontidVal}"
                        echolog "${cmd}"
                        reply=`$timecmd $cmd`
                        RC=$?
                        echo ${reply} >> $CLI_LOG_FILE
                        echolog "RC=$RC"
                        rest_verifyRC

                    elif [[ ${cli_action} == "stop" ]] ; then
                        reformatCliParams_json_Stop
                        rest_type="service/stop"
                        echo ">>>${rest_type} ${cli_action} object=${rest_object_type} name=${rest_name} payload=${rest_payload}"
                        rest_post

                    else
                        echo "${line[@]}"
                        echo ">>>Failed: \"${cli_action}\" is not supported for REST yet." >> $CLI_LOG_FILE
                    fi
                else 
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_options=${line[@]:3}
                        cli_options_lc="${cli_options,,}"
                    else
                        cli_options=
                        cli_options_lc=
                    fi
 
                    #echo ">>>{$cli_action} options=$cli_options"
                    stopstart
                fi
#set +x
                ;; 		

              'group' | 'user')
                # Get Type
                for item in ${line[@]:4} ; do
                    # remove quotes around item
                    item=${item%%\"}
                    item=${item##\"}
                    item_name=${item%%=*}
                    item_name=${item_name##\"}
                    if [[ "${item_name,,}" == "type" ]] ; then
                        TYPE=${item##*=}
                        break
                    fi
                done

                cli_verb=${line[3]}
                if [[ ("${A1_LDAP}" == "FALSE") &&
                      (("${cli_action_lc}" == "user" && "${TYPE,,}" == "messaging") || 
                       ("${cli_action_lc}" == "group" && "${cli_verb,,}" != "list") ||
                       ("${cli_action_lc}" == "group" && "${cli_verb,,}" == "list" && "${TYPE,,}" == "messaging")) ]]
                then
                   # Configure LDAP
                    cli_params=${line[@]:4}
                    #echo ">>>LDAP action=${cli_action} verb=$cli_verb params=$cli_params"
                    ldap_config
                else 
                    cli_object_type=${line[3]}
                    if [[ ${#line[@]} -gt 4 ]] ; then
                        cli_params=${line[@]:4}
                    else
                        cli_params=
                    fi

                    #echo ">>>passthru object_type=$cli_object_type" params=${cli_params} 
                    passthru
                fi
                ;;	


              'get' | 'list' | 'show' | 'status' )

                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=${line[3]}

                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_params=${line[4]}
                        #!!! If MessagingPolicy we must first determine if Topic, Queue or Subscription
                        if [[ "${rest_object_type}" == "MessagingPolicy" ]] ; then
                            get_policy_type
                            rest_object_type="${rest_policy_type}"
                        fi
                        reformatCliParams_uri Name
                    else
                        rest_name=
                    fi
                    if [[ "${cli_action}" == "status" ]] ; then
                        rest_type="service/status" 
                    else
                        rest_type="configuration"
                    fi

                    echo ">>>${cli_action}  type=${rest_type} uri_extension=${rest_uri_extension}"
                    rest_get

                else # old cli command 
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_object_type=${line[3]}
                        cli_object_type_lc="${cli_object_type,,}"
                        cli_params=${line[@]:4}
                    else
                        cli_object_type=
                        cli_object_type_lc=
                        cli_params=
                    fi
   
                    #echo ">>>${cli_action} object=${cli_object_type} params=$cli_params"
                    passthru
                fi   
                ;;		


              'backup' | 'harole' | 'restore' | 'runmode' | 'trace' | 'test' | 'clean' | 'commit' | 'rollback' | 'forget' | 'purge' )
                if [ "${A1_REST}" == "TRUE" ] ; then
                    echo "${line[@]}"
                    echo ">>>Failed: \"${cli_action}\" is not supported for REST yet." >> $CLI_LOG_FILE
                else    
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_object_type=${line[3]}
                        cli_object_type_lc="${cli_object_type,,}"
                        cli_params=${line[@]:4}
                    else
                        cli_object_type=
                        cli_object_type_lc=
                        cli_params=
                    fi

                    #echo ">>>${cli_action} object=${cli_object_type} params=$cli_params"
                    passthru
                fi
                ;;


              'sleep')
                cli_options=${line[3]}
                #echo ">>>${cli_action} options=$cli_options"
                sleep $cli_options    
                RC=$?
                echolog "${cmd} RC=$RC"
                echo "sleep $cli_options RC=$RC" >> $CLI_LOG_FILE 
                if [ $RC -ne 0 ] ; then
                    let num_fails+=1
                    echo ">>> ${cli_action} ${cli_options} Failed" >> $CLI_LOG_FILE
                else
                    echo ">>> ${cli_action} ${cli_options} Passed" >> $CLI_LOG_FILE
                fi  
                ;;
             
              'bedrock')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    if [[ "${line[3]}" == "file" && "${line[4]}" == "get" ]] ; then
                        rest_object_type="file"
                        rest_src_name=${line[5]##*:}
                        if [[ ${#line[@]} -gt 5 ]] ; then
                             rest_dst_name=${line[6]}
                        else
                             rest_dst_name=
                        fi

                        echo ">>>${cli_action} object=${rest_object_type} rest_src_name=${rest_src_name} rest_dst_name=${rest_dst_name}"
                        rest_put_file
                    else
                        echo ">>>${line[@]}" >> $CLI_LOG_FILE
                        echo ">>>${line[3]}" >> $CLI_LOG_FILE
                        echo ">>>Failed: \"${cli_action}\" is not supported for REST yet." >> $CLI_LOG_FILE
                    fi
                else
                    bedrock_cmd=${line[@]:3}
 
                    #echo ">>>bedrock bedrock_cmd=$bedrock_cmd"
                    bedrockPassthru 
                fi
                ;;


              'getthenset')
                if [ "${A1_REST}" == "TRUE" ] ; then
                    echo "${line[@]}"
                    echo ">>>Failed: \"${cli_action}\" is not supported for REST yet." >> $CLI_LOG_FILE
                else
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        cli_object_type_get=${line[3]}
                        cli_params=""
                        if [[ ${#line[@]} -gt 4 ]] ; then
                            cli_object_type_set=${line[@]:4}
                        else
                            cli_object_type_set=
                        fi
                    else
                        cli_object_type_get=
                    fi

                    echo ">>>${cli_action} object_get=$cli_object_type_get object_set=$cli_object_type_set" 
                    getthensetObject
                fi
                ;;	


              'GET' )
                if [ "${A1_REST}" == "TRUE" ] ; then

                    uri=${line[3]}
                    rest_type=${uri%%/*}
                    rest_uri_extension=${uri:${#rest_type}+1}

                    echo ">>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
                    rest_get
                else 
                    echo ">>> A1_REST in not set to True, skipping ${line[@]}"
                fi   
                ;;


              'PUT' )
                if [ "${A1_REST}" == "TRUE" ] ; then
                    if [[ "${line[3]}" == "file" ]] ; then
                        rest_object_type="file"
                        rest_src_name=${line[4]}
                        if [[ ${#line[@]} -gt 4 ]] ; then
                             rest_dst_name=${line[5]}
                        else
                             rest_dst_name=
                        fi

                        echo ">>>${cli_action} object=${rest_object_type} rest_src_name=${rest_src_name} rest_dst_name=${rest_dst_name}"
                        rest_put_file
  
                        # echo ">>>${cli_action} object=${rest_object_type} name=${rest_name}"
                        # rest_put_file
                    else
                        echo "${line[@]}"
                        echo ">>>Failed: \"${line[3]}\" is not supported for REST yet." >> $CLI_LOG_FILE
                    fi
                else 
                    echo ">>> A1_REST in not set to True, skipping ${line[@]}"
                fi   
                ;;


              'POST' )
                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_type=${line[3]} 
                    rest_payload=${line[@]:4}                    

                    echo ">>>${cli_action} payload=${rest_payload}"

                    # We must escape parentheses so that eval does not return an error
#set -x
                    rest_payload=${rest_payload//\{\"/\\\{\"}
                    rest_payload=${rest_payload//\"/\\\"}
                    rest_payload=${rest_payload//\ /\\\ }
                    # We must also escape parens 
    				rest_payload=${rest_payload//\(/\\\(}
    				rest_payload=${rest_payload//\)/\\\)}
#set +x
                    # We need to extract rest_object_type and rest_name because it will be needed by GET that is run after POST
                    rest_object_type=${rest_payload%%\\\":*}
                    rest_name=${rest_payload:${#rest_object_type}+2}
                    rest_object_type=${rest_object_type##*\"}
                    # check to see it payload contains a name
                    if [[ "${rest_name:2}" =~ ":\{" ]] ; then
                        rest_name=${rest_name%%\\\":*}
                        rest_name=${rest_name##*\"}
                    else
                        rest_name=""
                    fi

                    echo ">>>object=${rest_object_type} name=${rest_name}"

                    rest_post
                else
                    echo ">>> A1_REST in not set to True, skipping ${line[@]}"
                fi
                ;;

              'DELETE' )
                if [ "${A1_REST}" == "TRUE" ] ; then

                    uri=${line[3]}
                    rest_type=${uri%%/*}
                    rest_uri_extension=${uri:${#rest_type}+1}

                    echo ">>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
                    rest_delete
                else
                    echo ">>> A1_REST in not set to True, skipping ${line[@]}"
                fi 
                ;;

              'STAT' )
                if [ "${A1_REST}" == "TRUE" ] ; then
                    rest_object_type=${line[3]}
                    if [[ ${#line[@]} -gt 3 ]] ; then
                        rest_name=${line[4]}
                    else
                        rest_name=
                    fi
                    rest_type="monitor"

                    echo ">>>${cli_action} type=${rest_type} uri_extension=${rest_uri_extension}"
                    rest_get
                else
                    echo ">>> A1_REST in not set to True, skipping ${line[@]}"
                fi 
                ;;


	

              *)
                echolog "${cmd} RC=$RC"
                echo ">>>cli_action \"Failed: ${cli_action}\" is not valid." >> $CLI_LOG_FILE
                let num_fails+=1
                ;;
 
            esac
}

#----------------------------------------------------------------------------
# Print Usage
#----------------------------------------------------------------------------
printUsage(){

echo "Usage: run-cli.sh -f <CLI_LOG_FILE> [-a <A_number> | -p <Pnumber> | -b <Bnumber>] [-s <cli_section>] [-c <CLI_INPUT_FILE> | -i <SINGLE_LINE>] [-u <USER:PASSWORD>]"
echo "  CLI_LOG_FILE:   The name of the log file (normally automatically provided by run-scenarios)"
echo "  A_number:       The IMASERVER where the CLI commands are sent.  (default=1)"
echo "  P_number:       The IMAPROXY  where the CLI commands are sent.  (default=1)"
echo "  B_number:       The IMABRIDGE where the CLI commands are sent.  (default=1)"
echo "  cli_section:    The secion of the CLI_INPUT_FILE to execute. (default=ALL)"
echo "  CLI_INPUT_FILE: The name of the file that has the objects to create.  Objects must be in the correct format"
echo "  SINGLE_LINE:    A single line in the same format as in the CLI_INPUT_FILE."
echo "  USER:PASSWORD : Authorization Credentials used by Bridge."
echo "    Notes: stdout goes to CLI_LOG_FILE and errout goes to CLI_LOG_FILE.screenout.log"
echo "           You can only specify either -i or -c, but not both."
echo "           You can only specify either -a or -p, but not both."
}

#----------------------------------------------------------------------------
# Main script starts here
#----------------------------------------------------------------------------
# Check input
if [[ ($# -lt 4) || ($# -gt 10) ]] ; then
	printUsage
	exit 1
fi

#----------------------------------------------------------------------------
# Source the ISMsetup file to get access to information for the remote machine
#----------------------------------------------------------------------------
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi
#----------------------------------------------------------------------------
# Source the commonFunctions file
#----------------------------------------------------------------------------
source ../scripts/commonFunctions.sh

python_path="${M1_IMA_SDK}/python"
curl_silent_options="-f -s -S"
curl_timeout_options="--connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45"

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
APPLIANCE=1
IMAPROXY=""
IMABRIDGE=""
SECTION="ALL"
USERPW=""

echo "Entering $0 with $# arguments: $@"

while getopts "f:c:a:p:b:s:i:u:" option ${OPTIONS}
  do
    #echo "option=${option}  OPTARG=${OPTARG}"
    case ${option} in
    f ) CLI_LOG_FILE=${OPTARG}
        ;;
    c ) CLI_INPUT_FILE=${OPTARG}
        ;;
    a ) APPLIANCE=${OPTARG}
        ;;
    p ) IMAPROXY=${OPTARG}
        ;;
    b ) IMABRIDGE=${OPTARG}
        ;;
    s ) SECTION=${OPTARG}
        ;;
    i ) COMMAND=${OPTARG}
        ;;
    u ) USERPW=${OPTARG}
        ;;
    esac	
  done

#echo "COMMAND=$COMMAND"

if [[ -n "${USERPW}" ]]; then
    USERPW=" -u ${USERPW} "
fi

if [[ -n "${IMABRIDGE}" ]]; then
# Commands will be sent to Bridge or Proxy RESTAPI interface
    a_user="B${IMABRIDGE}_USER"
    a_host="B${IMABRIDGE}_HOST"
    a_port="B${IMABRIDGE}_BRIDGEPORT"
    a_srvcontid="B${IMABRIDGE}_BRIDGECONTID"
    BASE_URI="/admin/"
    
elif [[ -n "${IMAPROXY}" ]]; then
# Commands will be sent to Bridge or Proxy RESTAPI interface
    a_user="P${IMAPROXY}_USER"
    a_host="P${IMAPROXY}_HOST"
    a_port="P${IMAPROXY}_PORT"
    a_srvcontid="P${IMAPROXY}_BRIDGECONTID"
    BASE_URI="/admin/"
else 

# Default is to Assume APPLIANCE is passed
    a_user="A${APPLIANCE}_USER"
    a_host="A${APPLIANCE}_HOST"
    a_port="A${APPLIANCE}_PORT"
    a_srvcontid="A${APPLIANCE}_SRVCONTID"
    BASE_URI="/ima/v1/"
fi

#a_user="A${APPLIANCE}_USER"
a_userVal=$(eval echo \$${a_user})
#a_host="A${APPLIANCE}_HOST"
a_hostVal=$(eval echo \$${a_host})
if [[ "${a_userVal}" == "" ]] ; then
#    echolog "Failure: A${APPLIANCE}_USER is not defined in ISMsetup.sh or testEnv.sh"
    echolog "Failure: ${a_user} is not defined in ISMsetup.sh or testEnv.sh"
    exit 1
fi
if [[ "${a_hostVal}" == "" ]] ; then
#    echolog "Failure: A${APPLIANCE}_HOST is not defined in ISMsetup.sh or testEnv.sh"
    echolog "Failure: ${a_host} is not defined in ISMsetup.sh or testEnv.sh"
    exit 1
fi
sshcmd="ssh ${a_userVal}@${a_hostVal}"
timecmd="time -f'%E'"

#a_port="A${APPLIANCE}_PORT"
a_portVal=$(eval echo \$${a_port})
if [[ "${A1_REST}" == "TRUE" && "${a_portVal}" == "" ]] ; then
    echolog "Failure: A${APPLIANCE}_PORT is not defined in ISMsetup.sh or testEnv.sh"
    exit 1
fi
adminPort="${a_hostVal}:${a_portVal}"

a_srvcontidVal=$(eval echo \$${a_srvcontid})
if [[ "${a_srvcontidVal}" == "" ]] ; then
    echolog "WARNING: ${a_srvcontid} is not defined in ISMsetup.sh or testEnv.sh, IGNORE if it is an RPM Instance"
#    exit 1
fi


if [[ "$COMMAND" != "" ]] ; then
#set -x
    let num_fails=0
    line=${COMMAND}

    run_action

    if [[ $num_fails -ne 0 ]] ; then
        exit 1
    else
        exit 0
    fi
else

    let total_fails=0

    # Load input file into an array
    # (Do a file read did not work with the ssh statement inside the while loop.)
    saveIFS=$IFS
    IFS=$'\n'
    filecontent=( `cat "${CLI_INPUT_FILE}"` )

    # Check to see if the cli input file read failed
    if [ $? != 0 ]
    then
	echolog "Failure: Problem reading input file: ${CLI_INPUT_FILE}"
	echo "Compare files in directory: " `ls -al`
	exit 1
    fi
    # We must restore IFS because bash uses it to do its parsing!
    IFS=$saveIFS

    beginTime=$(date)

    foundSectionCount=0


    # Step through the array and call the apropriate function based on the first string in cli_action 
    for line in "${filecontent[@]}"
    do
        # Skip out any comments
        if [[ "${line:0:1}" != "#" ]] ; then
            let num_fails=0

            if [[ "${line%% *}" == "${SECTION}" || "${SECTION}" == "ALL" ]] ; then 
                let foundSectionCount+=1

                run_action

                let total_fails+=${num_fails}
            
                # If CLI command fail and we expected it to pass check status of imaserver    
                if [[ "${num_fails}" != "0" ]] ; then
                    # Get status of imaserver
                    if [ "${A1_REST}" == "TRUE" ] ; then
                        cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} ${USERPW} http://${adminPort}${BASE_URI}service/status"
                        reply=`$cmd`
                        StateDescription=`echo $reply | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`
                        ##Status=`echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]"`
                    else
                        cmd="${sshcmd} status imaserver"
                        StateDescription=`$timecmd $cmd`
                    fi

                    ##if [[ !("$Status" =~ "Running" && "$StateDescription" =~ "Running (production)") ]] ; then
                    if [[ ! ("$StateDescription" =~ "Status = Running (production)") ]] ; then
                        echolog ">>>   ${cmd}"
                        echolog ">>>   ${reply}"
                        reply=`date`
                        echolog ">>>   ${reply}"
                    fi
                fi
            fi
        fi
    done

    # Log time it took to execute tests
    endTime=$(date)
    # Caculate elapsed time
    calculateTime
    echo -e "\nTotal time required for test execution - ${hr}:${min}:${sec}" >> $CLI_LOG_FILE
    if [ ${foundSectionCount} -gt 0 ]
    then
    	echo -e "\nrun-cli.sh Found ${foundSectionCount} copies of ${SECTION}" >>$CLI_LOG_FILE 
    else
	echo -e "\nrun-cli.sh DID NOT FIND ANY copies of ${SECTION}" >>$CLI_LOG_FILE 
    fi
    if [ ${total_fails} -gt 0 ]
    then
	echo -e "\nrun-cli.sh Test result is Failure! Number of failures: ${total_fails}" >> $CLI_LOG_FILE
    else
	echo -e "\nrun-cli.sh Test result is Success!" >> $CLI_LOG_FILE
    fi

fi
