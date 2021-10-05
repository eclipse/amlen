#! /bin/bash
#-------------------------------------------------------------------------------
#
# run-scenarios.sh
#
# Description:
#   This script is part of the Lion Automation Framework and is used to launch test subcontrollers on various machines.
#
# Input Parameter:
#   $1  -  required: the name of the file that contains the scenarios to run
#   $2  -  optional: a log file to which this script will direct its output.
#          If this option is not given, output will go to the default log file
#          in the directory where this script is invoked.
#
# Return Code:
#   0 - All test scenarios passed.
#   1 - One or more test scenarios failed.
#
#-------------------------------------------------------------------------------

# Define Server Status values
StatusRunning="Running"
StatusStopped=""
# Define Server StateDescription values
StateDescProduction="Running (production)"
StateDescMaintenance="Running (maintenance)"
StateDescStandby="Standby"

function logonly {
    echo "$@" >> "${logfile}"
}

#-------------------------------------------------------------------------------
function echolog {
    echo "$@" | tee -a "${logfile}"
}

#-------------------------------------------------------------------------------
# clean_up : will clean all the log and residual files from remote and local machine.
#    It is used when the logs are NOT collected.
#-------------------------------------------------------------------------------
function clean_up {
    echo

    # Process remote machine info for remote update script
    machine=1
    while (( machine <= M_COUNT ))
    do
        m_skip="M${machine}_SKIP"
        m_skip_val=$(eval echo \$${m_skip})
        if [[ "$m_skip_val" != "TRUE" ]] ; then
            # if M?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
            m_user="M${machine}_USER"
            m_userVal=$(eval echo \$${m_user})
            m_host="M${machine}_HOST"
            m_hostVal=$(eval echo \$${m_host})
            m_tcroot="M${machine}_TESTROOT"
            m_tcrootVal=$(eval echo \$${m_tcroot})
            m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"

            # Remove the remote update script that was created, any temporary files that were created, and log files
            ssh "${m_userVal}"@"${m_hostVal}" "cd ${m_testcasedirVal} ; rm -f ../bin/upd_all_cfg_remote.sh; rm -f temp_runcmd_*.rc; rm -f temp_*.sh; rm -f *_gen_*.xml; rm -f *.log"
        fi
        machine=$((machine + 1))
    done

    # Remove local log files and temporary files
    rm -f "${localTestcasePath}"/*.log
    rm -f "${localTestcasePath}"/temp_*.sh
    rm -f "${localTestcasePath}"/*_gen_*.xml

}

#-------------------------------------------------------------------------------
# clean_up_one : Used to remove the logs and residual files after the logs have
#    been collected on local and remote machine
# These values are all setup by the get_log_files and assumed accurate as is
#  	  m_user="M${machine}_USER"
#	  m_userVal=$(eval echo \$${m_user})
#	  m_host="M${machine}_HOST"
#	  m_hostVal=$(eval echo \$${m_host})
#	  m_tcroot="M${machine}_TESTROOT"
#	  m_tcrootVal=$(eval echo \$${m_tcroot})
#	  m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"
#--------------------------------------------------------------------------------
function clean_up_one {
    echo
    # Process remote machine info for remote update script
    # Remove the remote update script that was created, any temporary files that were created, and log files
    ssh "${m_userVal}"@"${m_hostVal}" "cd ${m_testcasedirVal} ; rm -f ../bin/upd_all_cfg_remote.sh; rm -f temp_runcmd_*.rc; rm -f temp_*.sh; rm -f *_gen_*.xml; rm -f *.log"

}

#-------------------------------------------------------------------------------
# print_error prints the input error msg $1, does some reporting and cleanup, and EXITS THE SCRIPT.
function print_error {
    echo
    echolog "$1"
    echolog "FAILED: Scenario ${scenario_number} - ${scenario[${scenario_number}]} RC=1"
    echo

    pass_all=1
    ((ttl_fail+=1))

    stopTest

    exit 1
}

#-------------------------------------------------------------------------------
function print_cmderror {
    print_error "Command \"$1\" failed."
}

#-------------------------------------------------------------------------------
function scpit {
    if ! scp "$1" "$2" > /dev/null
    then
        print_cmderror "scp $1 $2"
    fi
}

#-------------------------------------------------------------------------------
# Input Parameter:
#   $1  -  required: the name of the file that contains the scenarios to run
#   $2  -  optional: a log file to which this script will direct its output.
#          If this option is not given, output will go to the default log file
#          in the directory where this script is invoked.
function printUsage {
    echolog "Usage:  ${thiscmd} scenario_list [logfile]"
    echolog "  scenario_list - File that contains the list, descriptions of scenarios to execute."
    echolog "  logfile - File to receive execution output messages."
    echolog "            The default log file is: ${default_logfile}"
    echolog ""
}

#-------------------------------------------------------------------------------
# Sets the variables for a component
function parseComponent {

    TIMEOUT=0
    EXE_FILE=""
    XMLFILE=""
    TOOL_CFG=""
    OTHER_OPTS=""

    # Make sure ${xml[]} is assigned a value otherwise we will get syntex error when trying to copy and remove files generated using this value
    if [[ "${xml[${scenario_number}]}" == "" ]] ; then
        xml[${scenario_number}]="MissingXMLValue"
    fi

    saveIFS=$IFS
    IFS=',' && comp_array=($1)
    # We must restore IFS because bash uses it to do its parsing!
    IFS=$saveIFS

    TOOL=${comp_array[0]}

    #------------------------------------------------------------------------------------
    # If this is sleep, set the TIME variable
    #------------------------------------------------------------------------------------
    if [ "${TOOL}" == "sleep" ]
    then
        TIME=${comp_array[1]}
        return
    fi

    #-------------------------------------------------------------------------------
    # Figure out which machine this component should be started on
    # Verify that it does not exceed M_COUNT of ISMsetup.sh
    #-------------------------------------------------------------------------------
    MACHINE=${comp_array[1]}
    dut=${MACHINE:1}
    MACHINE=$( echo "${MACHINE}" | tr -s  '[:lower:]'  '[:upper:]' )

    if [ "${dut}" -gt "${M_COUNT}" ]
    then
        print_error "ERROR: Invalid machine id: ${MACHINE}, check ${scenario_list} and ISMsetup.sh: M_COUNT=${M_COUNT} for mismatch."
    fi

    LOGFILE=${xml[${scenario_number}]}-${TOOL}-${MACHINE}-${comp_idx}.log

    #-------------------------------------------------------------------------------
    # Get:  _USER, _HOST, _TESTROOT where this SubController Component will start
    #-------------------------------------------------------------------------------
    dutusr=${MACHINE}_USER
    dutip=${MACHINE}_HOST
    dutdir=${MACHINE}_TESTROOT
    USER=$(eval echo \$${dutusr})
    IP=$(eval echo \$${dutip})
    SSH_IP="ssh ${USER}@${IP} "
    SCRIPT_DIR=$(eval echo \$${dutdir})/scripts

    TESTCASE_DIR=$(eval echo \$${dutdir})/${localTestcaseDir}

    #------------------------------------------------------------------------------------
    # If this is testDriver set the TOOL_CFG or OTHER_OPTS, LOGFILE, and TIMEOUT variables
    #------------------------------------------------------------------------------------
    if [ "${TOOL}" == "testDriver" ]
    then
        # Save off the other options
        TOOL_CFG=${comp_array[2]}

        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${TOOL_CFG}-${MACHINE}-${comp_idx}.log

        if [ "${comp_array[3]}" != "" ]
        then
            OTHER_OPTS="-o ${comp_array[3]}"
        else
            OTHER_OPTS=
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi

        echo TIMEOUT is ${TIMEOUT}
        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a cAppDriver using No TOOL_CFG, use EXE_FILE and may use OTHER_OPTS.
    # LOGFILE is not checked for SUCCESS, generally this is for Sample Apps when don't own output
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "cAppDriver" ] || [ "${TOOL}" == "cAppDriverRC" ] || [ "${TOOL}" == "cAppDriverRConlyChk" ] || [ "${TOOL}" == "cAppDriverRConlyChkWait" ] || [ "${TOOL}" == "cAppDriverWait" ]
    then
        # Set EXE_FILE file, OTHER_OPTS, set TIMEOUT.  Build LOGFILE name.
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}

        EXE_FILE="$(echo "${exe_param}" | cut -d "${delimiter}" -f 2)"
        exe_name=$(basename "${EXE_FILE}")
        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"
        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi
        echo TIMEOUT is ${TIMEOUT}
        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a cAppDriverLog using No TOOL_CFG, use EXE_FILE and may use OTHER_OPTS.
    # LOGFILE is checked for SUCCESS, these are Test Apps where we own output written
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "cAppDriverLog" ] || [ "${TOOL}" == "cAppDriverLogWait" ] || [ "${TOOL}" == "cAppDriverLogEnd" ]
    then
        # Set EXE_FILE file, OTHER_OPTS, set TIMEOUT.  Build LOGFILE name.
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}

        EXE_FILE="$(echo "${exe_param}" | cut -d "${delimiter}" -f 2)"
        exe_name=$(basename ${EXE_FILE})
        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"
        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi
        echo TIMEOUT is ${TIMEOUT}
        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a cAppDriverCfg with a TOOL_CFG will NOT use OTHER_OPTS (generally),
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "cAppDriverCfg" ]
    then
        # Set TOOL_CFG file, build LOGFILE, clear OTHER_OPTS, set TIMEOUT
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}
        EXE_FILE="$(echo ${exe_param} | cut -d ${delimiter} -f 2)"
        exe_name=$(basename "${EXE_FILE}")

        exe_param=${comp_array[3]}
        delimiter=${exe_param:2:1}
        TOOL_CFG="$(echo ${exe_param} | cut -d ${delimiter} -f 2)"

        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[4]}"
        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi

        echo TIMEOUT is ${TIMEOUT}
        ENVVARS=${comp_array[5]}

    #------------------------------------------------------------------------------------
    # This is a subController with a TOOL_CFG may use OTHER_OPTS, MUST SET RS_IAM Env. Var.,
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "subController" ]
    then
        # Set TOOL_CFG file, build LOGFILE, clear OTHER_OPTS, set TIMEOUT
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}
        EXE_FILE="$(echo ${exe_param} | cut -d ${delimiter} -f 2)"
        exe_name=$(basename ${EXE_FILE})
        TOOL_CFG="$(echo ${exe_param} | cut -d ${delimiter} -f 3)"

        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"

        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi

        echo TIMEOUT is ${TIMEOUT}
        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a javaAppDriver using No TOOL_CFG, use EXE_FILE and may use OTHER_OPTS.
    # LOGFILE is not checked for SUCCESS, generally this is for Sample Apps when don't own output
    # For javaAppDriverRC the Return Code from the execution of exe_name is echoed to stdout can
    # then be checked by checking *.screenout.log
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "javaAppDriver" ] || [ "${TOOL}" == "javaAppDriverRC" ] || [ "${TOOL}" == "javaAppDriverLocalRC" ] || [ "${TOOL}" == "javaAppDriverWait" ]
    then
        # Set EXE_FILE file, OTHER_OPTS, set TIMEOUT.  Build LOGFILE name.
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}
        EXE_FILE="$(echo ${exe_param} | cut -d ${delimiter} -f 2)"
        exe_name=$(basename ${EXE_FILE})
        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"
        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi
        echo TIMEOUT is ${TIMEOUT}

        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a javaAppDriverLog using No TOOL_CFG, use EXE_FILE and may use OTHER_OPTS.
    # LOGFILE is checked for SUCCESS, these are Test Apps where we own output written
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "javaAppDriverLog" ] || [ "${TOOL}" == "javaAppDriverLogWait" ]
    then
        # Set EXE_FILE file, OTHER_OPTS, set TIMEOUT.  Build LOGFILE name.
        exe_param=${comp_array[2]}
        delimiter=${exe_param:2:1}
        EXE_FILE="$(echo ${exe_param} | cut -d ${delimiter} -f 2)"
        exe_name=$(basename ${EXE_FILE})
        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${exe_name}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"

        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi

        echo TIMEOUT is ${TIMEOUT}

        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # This is a javaAppDriverCfg with a TOOL_CFG will NOT use OTHER_OPTS (generally),
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "javaAppDriverCfg" ]
    then
        # Set TOOL_CFG file, build LOGFILE, clear OTHER_OPTS, set TIMEOUT
        TOOL_CFG=${comp_array[2]}
        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${TOOL_CFG}-${MACHINE}-${comp_idx}.log

        OTHER_OPTS="-o ${comp_array[3]}"
        if [ "${OTHER_OPTS}" == "-o " ]
        then
            OTHER_OPTS=""
        fi

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi
        echo TIMEOUT is ${TIMEOUT}

        ENVVARS=${comp_array[4]}

    #------------------------------------------------------------------------------------
    # If this is Sync Driver set the LOGFILE, and OTHER_OPTS variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "sync" ]
    then

        LOGFILE=${xml[${scenario_number}]}-${TOOL}-${MACHINE}.log

        if [ "${comp_array[2]}" != "" ]
        then
            OTHER_OPTS="-o ${comp_array[2]}"
        else
            OTHER_OPTS="-o -o_-p_${SYNCPORT}"
        fi

    #------------------------------------------------------------------------------------
    # If this is devtool, set the LLMD_PID variable
    #------------------------------------------------------------------------------------
    #        elif [ "${TOOL}" == "devtool" ]
    #        then
    #                 echo "Tool devtool = " ${TOOL}
    #
    #                LLMD_INDEX=${comp_array[2]}
    #                LLMD_PID=$(echo ${LLMD_PIDS} | cut -d ' ' -f ${LLMD_INDEX})

    #------------------------------------------------------------------------------------
    # If this is killcomponent, set the COMP_PID variable
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "killComponent" ]
    then
        COMP_INDEX=${comp_array[2]}
        CUENUM=${comp_array[3]}
        SOLUTION=${comp_array[4]}
        # list=( `echo ${PIDS}` ) this shell construction is deprecated
        # better to use "read"
        read -r -a list <<<"$PIDS"
        for p in "${list[@]}"
        do
            MY_PID=$(echo "${p}" | cut -d ':' -f 2)
            MY_COMP_ID=$(echo "${p}" | cut -d ':' -f 6)
            if [ "${MY_COMP_ID}" == "${COMP_INDEX}" ]
            then
                COMP_PID=${MY_PID}
        fi
        done

    #------------------------------------------------------------------------------------
    # If this is enableLDAP, set the ENABLE_LDAP_FILE and RUN_NUM variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "enableLDAP" ]
    then
        ENABLE_LDAP_FILE=${comp_array[2]}
        ENABLE_LDAP_LOG=$(echo "${ENABLE_LDAP_FILE}" | awk -F'/' '{ print $NF }').log
        RUN_NUM=${comp_array[3]}

    #------------------------------------------------------------------------------------
    # If this is disableLDAP, set the ENABLE_LDAP_FILE and RUN_NUM variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "disableLDAP" ]
    then
        ENABLE_LDAP_FILE=${comp_array[2]}
        ENABLE_LDAP_LOG=$(echo ${ENABLE_LDAP_FILE} | awk -F'/' '{ print $NF }').log
        RUN_NUM=${comp_array[3]}

    #------------------------------------------------------------------------------------
    # If this is purgeLDAPcert, set the PURGE_LDAPCERT_FILE and RUN_NUM variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "purgeLDAPcert" ]
    then
        PURGE_LDAPCERT_FILE=${comp_array[2]}
        PURGE_LDAPCERT_LOG=$(echo "${PURGE_LDAPCERT_FILE}" | awk -F'/' '{ print $NF }').log
        RUN_NUM=${comp_array[3]}

    #------------------------------------------------------------------------------------
    # If this is emptyLDAPcert, set the EMPTY_LDAPCERT_FILE and RUN_NUM variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "emptyLDAPcert" ]
    then
        EMPTY_LDAPCERT_FILE=${comp_array[2]}
        EMPTY_LDAPCERT_LOG=$(echo ${EMPTY_LDAPCERT_FILE} | awk -F'/' '{ print $NF }').log
        RUN_NUM=${comp_array[3]}

    #------------------------------------------------------------------------------------
    # If this is searchlogs, set the COMPARE_FILE and RUN_NUM variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "searchLogs" ] || [ "${TOOL}" == "searchLogsEnd" ]
    then
        COMPARE_FILE=${comp_array[2]}
        COMPARE_LOG=$(echo ${COMPARE_FILE} | awk -F'/' '{ print $NF }').log
        RUN_NUM=${comp_array[3]}

    #------------------------------------------------------------------------------------
    # If this is searchlogsNow, set the COMPARE_FILE variable
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "searchLogsNow" ]
    then
        COMPARE_FILE=${comp_array[2]}
        COMPARE_LOG=$(echo ${COMPARE_FILE} | awk -F'/' '{ print $NF }').log

    #------------------------------------------------------------------------------------
    # If this is runCommand, set the COMMAND and CUENUM variables
    #   SOLUTION is optional and refers to the SYNC Driver's namespace where 'Cues' are defined
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "runCommand" ]
    then
        COMMAND=${comp_array[2]}
        CUENUM=${comp_array[3]}
        SOLUTION=${comp_array[4]}
        ENVVARS=${comp_array[5]}

        if [ "${SOLUTION}" == "" ]
        then
            SOLUTION="tmrefcount"
        fi

    #------------------------------------------------------------------------------------
    # If this is runCommandNow, set the COMMAND variable
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "runCommandNow" ] || [ "${TOOL}" == "runCommandEnd" ] || [ "${TOOL}" == "runCommandLocal" ]
    then
        COMMAND=${comp_array[2]}
        ENVVARS=${comp_array[3]}

        #------------------------------------------------------------------------------------
        # If this is jmsDriver set the XMLFILE, CA_NAME, OTHER_OPTS, LOGFILE, and TIMEOUT variables
        #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "jmsDriver" ] || [ "${TOOL}" == "jmsDriverNocheck" ] || [ "${TOOL}" == "jmsDriverWait" ]
    then
        # Save off the other options
        XMLFILE=${comp_array[2]}
        NAME=${comp_array[3]}

        if [ "${comp_array[4]}" != "" ]
        then
            OTHER_OPTS="-o ${comp_array[4]}"
        else
            OTHER_OPTS=
        fi

        if [ "${comp_array[5]}" != "" ]
        then
            ENVVARS=${comp_array[5]}
        else
            ENVVARS=
        fi

        LOGFILE=$(basename ${XMLFILE})-${NAME}-${MACHINE}.log

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi
        echo TIMEOUT is ${TIMEOUT}

    #------------------------------------------------------------------------------------
    # If this is Web Sockets TestDriver set the XMLFILE, CA_NAME, OTHER_OPTS, LOGFILE, and TIMEOUT variables
    #------------------------------------------------------------------------------------
    elif [ "${TOOL}" == "wsDriver" ] || [ "${TOOL}" == "wsDriverNocheck" ] || [ "${TOOL}" == "wsDriverWait" ]
    then
        # Save off the other options
        XMLFILE=${comp_array[2]}
        NAME=${comp_array[3]}

        if [ "${comp_array[4]}" != "" ]
        then
            OTHER_OPTS="-o ${comp_array[4]}"
        else
            OTHER_OPTS=
        fi

        if [ "${comp_array[5]}" != "" ]
        then
            ENVVARS=${comp_array[5]}
        else
            ENVVARS=
        fi

        LOGFILE=$(basename ${XMLFILE})-${NAME}-${MACHINE}.log
        #LOGFILE=${xml[${scenario_number}]}-${TOOL}-${NAME}-${MACHINE}-${comp_idx}.log

        if [ "${timeouts[${scenario_number}]}" != "" ]
        then
            TIMEOUT=${timeouts[${scenario_number}]}
        else
            TIMEOUT=180
        fi

        echo "TIMEOUT is ${TIMEOUT}"
    fi
}

#---------------------------------------------------------------------------------------------------------------------------
# startComponent{}
# PID will contain one of there values which are used later on building the PID Lists (EXIPRING & NONEXPIRING)
#    - actual Process ID from the launched test tool element
#    - 'x' which means the start of this tool is delayed until the test completes
#    - "" which means this tool (ex. sleep) has already run to completion and does not need to be cleared later.
#    - '-1' Think this is a bad attempt using start.sh?
#---------------------------------------------------------------------------------------------------------------------------
function startComponent {

    if [ "${TOOL}" == "testDriver" ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -c ${TOOL_CFG} \"${ENVVARS}\" -f ${LOGFILE} \"${OTHER_OPTS}\")
        RES=$?

    elif [ "${TOOL}" == "cAppDriver" ] || [ "${TOOL}" == "cAppDriverRC" ] || [ "${TOOL}" == "cAppDriverRConlyChk" ] || [ "${TOOL}" == "cAppDriverRConlyChkWait" ] || [ "${TOOL}" == "cAppDriverWait" ]
    then
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...cAppDriver/cAppDriverRC:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE} \"${ENVVARS}\" -f ${LOGFILE} \"${OTHER_OPTS}\"
    fi
    # Call the start.sh script on the appropriate machine to start the driver
    PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE} \"${ENVVARS}\" -f ${LOGFILE} \"${OTHER_OPTS}\")
    RES=$?

    elif [ "${TOOL}" == "cAppDriverLog" ] || [ "${TOOL}" == "cAppDriverLogWait" ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE} \"${ENVVARS}\" -f ${LOGFILE} \"${OTHER_OPTS}\")
        RES=$?

    elif [ "${TOOL}" == "cAppDriverLogEnd" ]
    then
        PID=x
        RES=0

    elif [ "${TOOL}" == "cAppDriverCfg" ] || [ "${TOOL}" == "subController"  ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...cAppDriverCfg/subController:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR} -e ${EXE_FILE} -c ${TOOL_CFG} \"${ENVVARS}\" -f ${LOGFILE}  \"${OTHER_OPTS}\"
        fi
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR} -e ${EXE_FILE} -c ${TOOL_CFG} \"${ENVVARS}\" -f ${LOGFILE}  \"${OTHER_OPTS}\")
        RES=$?

    elif [ "${TOOL}" == "javaAppDriver" ] || [ "${TOOL}" == "javaAppDriverRC" ] || [ "${TOOL}" == "javaAppDriverWait" ]
    then
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...JavaAppDriver/JavaAppDriverRC:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE}   \"${ENVVARS}\"   -f ${LOGFILE} \"${OTHER_OPTS}\"
        fi
        # Call the start.sh script on the appropriate machine to start the driver
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE}   \"${ENVVARS}\"   -f ${LOGFILE} \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi

    elif [ "${TOOL}" == "javaAppDriverLocalRC" ]
    then
        echo "DEBUG...JavaAppDriverLocalRC: start.sh -t javaAppDriverRC -d ${TESTCASE_DIR} -e ${EXE_FILE} -f ${LOGFILE} ${OTHER_OPTS} ${ENVVARS}"
        # Call the start.sh script on the local machine to start the driver
        # Note: No quotes around ${OTHER_OPTS} and ${ENVVARS}
        PID=$(../scripts/start.sh -t javaAppDriverRC -d ${TESTCASE_DIR} -e ${EXE_FILE} -f ${LOGFILE} ${OTHER_OPTS} ${ENVVARS})
        RES=$?

    elif [ "${TOOL}" == "javaAppDriverLog" ] || [ "${TOOL}" == "javaAppDriverLogWait" ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...cAppDriverCfg/subController:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE}   \"${ENVVARS}\"   -f ${LOGFILE} \"${OTHER_OPTS}\"${OTHER_OPTS}\"
        fi
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE}   \"${ENVVARS}\"   -f ${LOGFILE} \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi

    elif [ "${TOOL}" == "javaAppDriverCfg" ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...cAppDriverCfg/subController:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -c ${TOOL_CFG}   \"${ENVVARS}\"  -f ${LOGFILE}  \"${OTHER_OPTS}\"
        fi
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -c ${TOOL_CFG}   \"${ENVVARS}\"  -f ${LOGFILE}  \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi

    elif [ "${TOOL}" == "sync" ]
    then
        # Call the start.sh script on the appropriate machine to start the synchronization server for driver sync
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...cAppDriverCfg/subController:"     ${SSH_IP}${SCRIPT_DIR}/start.sh -t test.llm.TestServer -d ${TESTCASE_DIR} -f ${LOGFILE} \"${OTHER_OPTS}\"
        fi
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t test.llm.TestServer -d ${TESTCASE_DIR} -f ${LOGFILE} \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi

    elif [ "${TOOL}" == "killComponent" ]
    then
        # Call the start.sh script on the appropriate machine to kill the specified component
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t ${TOOL} -d ${TESTCASE_DIR} -y ${COMP_PID} -q ${CUENUM} -i ${SOLUTION})
        RES=$?

    elif [ "${TOOL}" == "searchLogs" ]
    then
        # Call the start.sh script on the appropriate machine to search the logs
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t ${TOOL} -d ${TESTCASE_DIR} -m ${COMPARE_FILE} -w ${RUN_NUM})
        RES=$?

    elif [ "${TOOL}" == "searchLogsEnd" ]
    then
        PID=x
        RES=0
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...searchLogsEnd:  Adding to list of components to be run after all other components have completed."
            echo "DEBUG...PID=${PID}"
        fi

    elif [ "${TOOL}" == "searchLogsNow" ]
    then
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t ${TOOL} -d ${TESTCASE_DIR} -m ${COMPARE_FILE})
        RES=$?

    elif [ "${TOOL}" == "sleep" ]
    then
        sleep $TIME
        RES=$?

    elif [ "${TOOL}" == "runCommand" ]
    then
        # Call the start.sh script on the appropriate machine to run the command
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t ${TOOL} -d ${TESTCASE_DIR} -a ${COMMAND} -q ${CUENUM} -i ${SOLUTION} \"${ENVVARS}\")
        RES=$?

    elif [ "${TOOL}" == "runCommandNow" ]
    then
        # Call the start.sh script on the appropriate machine to run the command
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh -t ${TOOL} -d ${TESTCASE_DIR} -a ${COMMAND}  \"${ENVVARS}\")
        RES=$?

    elif [ "${TOOL}" == "runCommandEnd" ]
    then
        PID=x
        RES=0

    elif [ "${TOOL}" == "runCommandLocal" ]
    then
        #set -x
        # Call the start.sh script on the local machine to run the command
        PID=$(./scripts/start.sh -t runCommandNow -d ${TESTCASE_DIR} -a ${COMMAND}  \"${ENVVARS}\")
        RES=$?
        set +x
    elif [ "${TOOL}" == "jmsDriver" ] || [ "${TOOL}" == "jmsDriverNocheck" ] || [ "${TOOL}" == "jmsDriverWait" ]
    then
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...jmsDriver/jmsDriverNocheck:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR} -c ${XMLFILE} -n ${NAME} -f ${LOGFILE} \"${ENVVARS}\" \"${OTHER_OPTS}\"
        fi
        # Call the start.sh script on the appropriate machine to start the driver
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR} -c ${XMLFILE} -n ${NAME} -f ${LOGFILE} \"${ENVVARS}\" \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi


    elif [ "${TOOL}" == "wsDriver" ] || [ "${TOOL}" == "wsDriverNocheck" ] || [ "${TOOL}" == "wsDriverWait" ]
    then
        # Call the start.sh script on the appropriate machine to start the driver
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...startComponent...jmsDriver/jmsDriverNocheck:"     ${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -c ${XMLFILE} -n ${NAME} -f ${LOGFILE} \"${ENVVARS}\" \"${OTHER_OPTS}\"
        fi
        PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -c ${XMLFILE} -n ${NAME} -f ${LOGFILE} \"${ENVVARS}\" \"${OTHER_OPTS}\")
        RES=$?
        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...PID=${PID}"
        fi
    fi

    if [[ ${RES} -ne 0 ]]
    then
        echolog "Error in start.sh when starting ${TOOL}"
        # PID may contain and error message in this case
        print_error "$PID"
    fi

}

#-------------------------------------------------------------------------------
function sortPids {
    #set -x
    pidlist=( `echo $@` )
    #read -r -a pidlist <<<"$@"
    if [ ${#pidlist[@]} -eq 0 ]
    then
        expiring_process_list=( )
        nonexpiring_process_list=( )
    else
        NONEXPIRING=""
        EXPIRING=""
        for a in ${pidlist[@]}
        do
            if [[ $(echo ${a} | cut -d ':' -f 5) -eq 0 ]]
            then
                NONEXPIRING="${a} ${NONEXPIRING}"
            else
                EXPIRING="${a} ${EXPIRING}"
            fi
        done
        # expiring_process_list=( `echo ${EXPIRING}` )
        read -r -a expiring_process_list <<<"$EXPIRING"
        # nonexpiring_process_list=( `echo ${NONEXPIRING}` )
        read -r -a nonexpiring_process_list <<<"$NONEXPIRING"
    fi
}

#-------------------------------------------------------------------------------
function parsePids {

    saveIFS=$IFS
    IFS=':' && pid_array=($1)
    # We must restore IFS because bash uses it to do its parsing!
    IFS=$saveIFS

    TOOL=${pid_array[0]}
    PID=${pid_array[1]}
    USER=${pid_array[2]}
    IP=${pid_array[3]}
    TIMEOUT=${pid_array[4]}
    COMP_ID=${pid_array[5]}

    if [[ "${IP}" = "" ]]
    then
        SSH_IP=
    else
        SSH_IP="ssh ${USER}@${IP} "
    fi

    XMLFILE=${pid_array[6]}
    TOOL_CFG=${pid_array[7]}


}

#-------------------------------------------------------------------------------
function evaluateResults {

    if [  -n "$AUTOMATION_FRAMEWORK_TEST_PLAN_URL" ] ; then
        echo " ${xml[${scenario_number}]} TEST_PLAN_URL: $AUTOMATION_FRAMEWORK_TEST_PLAN_URL " >> $M1_TESTROOT/testplans.log
    fi

    if [[ ${result} -eq 0 && ${KILL_PID} -eq 0 ]] ; then
        if [ -n "$AUTOMATION_FRAMEWORK_SAVE_RC" ] ; then # OPTIONAL: User exported environment variable to save last rc
            # Give scripts the ability to dynamically check what the most recent rc was, by saving it to a user supplied filename
            echo 0 > $AUTOMATION_FRAMEWORK_SAVE_RC # Most recent test PASSED
        fi
        echolog "PASSED: Scenario ${scenario_number} - ${scenario[${scenario_number}]}"
        ((ttl_pass+=1))
        scenario_passed="true"
    else
        if [ -n "$AUTOMATION_FRAMEWORK_SAVE_RC" ] ; then # OPTIONAL: User exported environment variable to save last rc
            # Give scripts the ability to dynamically check what the most recent rc was, by saving it to a user supplied filename
            echo 1 > $AUTOMATION_FRAMEWORK_SAVE_RC # Most recent test FAILED
        fi
        echolog "FAILED: Scenario ${scenario_number} - ${scenario[${scenario_number}]}"
        pass_all=1
        ((ttl_fail+=1))
        scenario_passed="false"

        # Check to see if Server stopped running so that we can log a failed message at the end of the this suite of test
        local appliance=1
        while [ ${appliance} -le ${A_COUNT} ]
        do
            a_skip="A${appliance}_SKIP"
            a_skip_val=$(eval echo \$${a_skip})
            if [[ "$a_skip_val" != "TRUE" ]] ; then
                # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
                a_user="A${appliance}_USER"
                a_userVal[${appliance}]=$(eval echo \$${a_user})
                a_host="A${appliance}_HOST"
                a_hostVal[${appliance}]=$(eval echo \$${a_host})
                a_rest="A${appliance}_REST"
                a_restVal=$(eval echo \$${a_rest})
                a_port="A${appliance}_PORT"
                a_portVal=$(eval echo \$${a_port})
                cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
                reply=$($cmd)
                RC=$?
                StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")

                echo "$cmd" | tee -a $logfile
                if [ $RC -eq 0 ] ; then
                    #!!! Do not echo status reply to stdout! If it contains GVT characters it will cause STAF to crash!
                    # I found that if I put quotes around $reply echo command preserves the line feeds and if I don't it all gets printed on one line
                    # For the multiple server machines case it is easier to compare the multiple machines when each machine's replay is on one line
                    echo $reply >> $logfile
                else
                    echo "RC=$RC" | tee -a $logfile
                fi
                ##if [[ !("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                if [[ ! ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]]
                then
                    server_stopped_running="true"
                fi
            fi
            appliance=$(( appliance + 1 ))
        done
    fi
    echolog " "
}
#-------------------------------------------------------------------------------
function killProcesses {
    ##  THIS NEEDS TO BE REVIEWED AND EDITED...
    pidlist=( `echo $@` )
    # read -r -a pidlist <<<"$@"

    for p in ${pidlist[@]}
    do
        parsePids ${p}
        echo ${P}
        ## checking for process id is causing problem on cygwin so removing this check for now - jrs
        #		if [[ $(${SSH_IP}ps -ef | grep -v grep | grep -c " ${PID} ") -ne 0 ]]
        if [[ "${PID}" != "x" ]]
        then
            if [[ "${IP}" = "" ]]
            then
                echolog "Killing ${TOOL} process ${PID} on ${M1_host_ip} [$(date +"%Y/%m/%d %T:%3N %Z")]"
            else
                echolog "Killing ${TOOL} process ${PID} on ${IP} [$(date +"%Y/%m/%d %T:%3N %Z")]"
            fi

            if [ "${TOOL}" == "subController" ]
            then
                ${SSH_IP}kill -s SIGTERM ${PID}
            else
                ${SSH_IP}kill -9 ${PID}
            fi
        fi
    done

    #LAW: Kill any remaining firefox processes
    ostype=$(${SSH_IP}uname)
    case ${ostype} in
        CYGWIN*)
            PIDVAR=$(${SSH_IP}ps -efW | grep firefox)
            # PIDVAR=$(echo ${PIDVAR})
            PIDVAR="${PIDVAR}"
            PIDVAR=${PIDVAR#* }
            PIDVAR=${PIDVAR%% *}
            ##${SSH_IP}kill -9 ${PIDVAR};;
            if [ -n "${PIDVAR}" ]; then
                ${SSH_IP}taskkill /F /PID ${PIDVAR}
            fi
            ;;
        *)
            ${SSH_IP}killall -9 firefox
            ${SSH_IP}killall -9 firefox-bin;;
    esac
}

#-------------------------------------------------------------------------------
function check_for_cores {
    # You can not mix Angel with v1.2 so we only need to check A1
    if [[ "${A1_TYPE}" == "DOCKER" || "${A1_TYPE}" == "RPM" ]] ; then
        # !!!! Docker does not support list at this time
        # !!!! Cores in docker go to whatever the core pattern is set to
        core_found=false
        appliance=1
        while [ ${appliance} -le ${A_COUNT} ]
        do
            a_skip="A${appliance}_SKIP"
            a_skip_val=$(eval echo \$${a_skip})
            if [[ "${a_skip_val}" != "TRUE" ]] ; then
                # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
                a_user="A${appliance}_USER"
                a_userVal=$(eval echo \$${a_user})
                a_host="A${appliance}_HOST"
                a_hostVal=$(eval echo \$${a_host})

                aCorePath="/mnt/A${appliance}/messagesight/diag/cores"
                cmd="ssh ${a_userVal}@${a_hostVal} cd ${aCorePath} && ls bt.*"
                reply=`$cmd 2>/dev/null`
                RC=$?
                if [ $RC -eq 0 ] ; then
                    for item in ${reply} ; do
                        if [[ "${item:0:3}" == "bt." ]] ; then
                            core_found=true
                            break
                        fi
                    done
                fi
            fi
            appliance=$(( appliance + 1))
        done
    else # Old CLI case
        core_found=false
        appliance=1
        while [ ${appliance} -le ${A_COUNT} ]
        do
            a_skip="A${appliance}_SKIP"
            a_skip_val=$(eval echo \$${a_skip})
            if [[ "$a_skip_val" != "TRUE" ]] ; then
                # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
                a_user="A${appliance}_USER"
                a_userVal=$(eval echo \$${a_user})
                a_host="A${appliance}_HOST"
                a_hostVal=$(eval echo \$${a_host})

                #------------------------------------------------------------------
                # Get list of files
                #------------------------------------------------------------------

                cmd="ssh ${a_userVal}@${a_hostVal} advanced-pd-options list"
                reply=$($cmd)
                RC=$?
                if [ $RC -ne 0 ] ; then
                    echolog "${cmd} ReturnCode=$RC"
                    echolog ${reply}
                    echolog "FAILED: runScenarios - ${scenario_set_name}: advanced-pd-options list failed."
                else
                    #------------------------------------------------------------------
                    # Check for any core files
                    #------------------------------------------------------------------
                    for item in ${reply} ; do
                        if [[ "${item:0:3}" == "bt." || "${item}" =~ "core." ]] ; then
                            core_found=true
                            break
                        fi
                    done
                fi
            fi
            appliance=$(( appliance + 1 ))
        done
    fi

    if (( P_COUNT > 0 ))
    then
        core_proxy_found=false
        proxy=1
        while [ ${proxy} -le ${P_COUNT} ]
        do
            # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
            p_host="P${proxy}_HOST"
            p_hostVal=$(eval echo \$${p_host})

            proxyCorePath="/niagara/proxy"
            cmd="ssh ${p_hostVal} cd ${proxyCorePath} && ls bt.*"
            reply=$($cmd 2>/dev/null)
            RC=$?
            if [ $RC -eq 0 ] ; then
                for item in ${reply} ; do
                    if [[ "${item:0:3}" == "bt." ]] ; then
                        core_proxy_found=true
                        break
                    fi
                done
            fi
            proxy=$((proxy + 1))
        done
    fi

    if (( B_COUNT > 0 ))
    then
        core_bridge_found=false
        bridge=1
        while [ ${bridge} -le ${B_COUNT} ]
        do
            # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
            b_host="P${bridge}_HOST"
            b_hostVal=$(eval echo \$${p_host})
            bridgeCorePath="/var/imabridge/diag/cores"
            cmd="ssh ${p_hostVal} cd ${bridgeCorePath} && ls bt.*"
            reply=$($cmd 2>/dev/null)
            RC=$?
            if [ $RC -eq 0 ] ; then
                for item in ${reply} ; do
                    if [[ "${item:0:3}" == "bt." ]] ; then
                        core_bridge_found=true
                        break
                    fi
                done
            fi

            bridge=$(( bridge + 1))
        done
    fi
}

#-------------------------------------------------------------------------------
function get_log_files {
    #set -x
    if [[ "${RS_IAM}" == "" ]]
    then
        ##  Not running run-scenarios as subcontroller.
        ##  when running as a subcontroller, the log files do not need to be collected or NOT erased by subcontroller, let the Master run-scenarios.sh collect and erase
        ##  It's a timeing thing... just cause the subcontroller finished the rest of the test may not be...

        # Create a directory to store the log files
        if [ -e "${scenario_name}" ]
        then
            if [ -n "${scenario_name}" ]
            then
                rm -rf ${scenario_name:?}/*
            fi
        else
            mkdir -p ${scenario_name}
        fi
        ###########################################################
        # Copy ISM Server files to local directory
        # Handle appliances both V2.0 and later and V1.2
        ###########################################################
        appliance=1
        while [ ${appliance} -le ${A_COUNT} ]
        do
            a_skip="A${appliance}_SKIP"
            a_skip_val=$(eval echo \$${a_skip})
            if [[ "$a_skip_val" != "TRUE" ]] ; then
                # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
                a_user="A${appliance}_USER"
                a_userVal=$(eval echo \$${a_user})
                a_host="A${appliance}_HOST"
                a_hostVal=$(eval echo \$${a_host})
                a_srvcontid="A${appliance}_SRVCONTID"
                a_srvcontidVal=$(eval echo \$${a_srvcontid})
                m_user="${THIS}_USER"
                m_userVal=$(eval echo \$${m_user})
                # m_host="${THIS}_HOST"  ##Can't really use 'host' here like expect when in the KVM, use the IPv4_1 value and hope for the best :-)
                m_ipv4_1="${THIS}_IPv4_1"
                m_ipv4_1Val=$(eval echo \$${m_ipv4_1})
                # If m_ipv4 is really a ipv6 address for the case where automation forces all test are run using ipv6 addresses
                # convert to ipv4 address (this assumes the convention we are using where 10.10.3.211 and fc00::10:10:3:211)
                if [[ "$m_ipv4_1Val" == *:* ]] ; then
                    m_ipv4_1Val=${m_ipv4_1Val:6}
                    m_ipv4_1Val=${m_ipv4_1Val//:/.}
                fi
                m_tcroot="${THIS}_TESTROOT"
                m_tcrootVal=$(eval echo \$${m_tcroot})
                m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"

                a_type="A${appliance}_TYPE"
                a_typeVal=$(eval echo \$${a_type})
                if [[ "${a_typeVal}" == "DOCKER" || "${a_typeVal}" == "RPM" ]] ; then
                    # !!!! Docker does not support list at this time
                    # SCP all logs from server container host. Each container mounted it filesystem to the host filesystem under /mnt/A$/<containerID.
                    a_host_ip=$(echo ${a_hostVal} | cut -d' ' -f 1)
                    mkdir -p ${scenario_name}/A${appliance}/messagesight/diag/logs
                    scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/messagesight/diag/logs/* ${scenario_name}/A${appliance}/messagesight/diag/logs/.
                    mkdir -p ${scenario_name}/A${appliance}/messagesight/diag/mqclient
                    # disable collecting mqclient logs for now ...
                    # scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/messagesight/diag/mqclient/* ${scenario_name}/A${appliance}/messagesight/diag/mqclient/.
                    # delete rolled trace files
                    ssh ${m_userVal}@${a_host_ip} rm -f /mnt/A${appliance}/messagesight/diag/logs/imatrace_*.log.gz
                    mkdir -p ${scenario_name}/A${appliance}/messagesight/data
                    scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/messagesight/data/* ${scenario_name}/A${appliance}/messagesight/data/.
                    mkdir -p ${scenario_name}/A${appliance}/messagesight/diag/cores
                    #scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/messagesight/diag/cores/*.log ${scenario_name}/A${appliance}/messagesight/diag/cores/.
                    if [[ "${core_found}" == "true" ]] ; then
                        aCorePath="/mnt/A${appliance}/messagesight/diag/cores"
                        cmd="ssh ${m_userVal}@${a_host_ip} cd ${aCorePath} && ls bt.*"
                        reply=$($cmd 2>/dev/null)
                        RC=$?
                        if [ $RC -eq 0 ] ; then
                            for item in ${reply} ; do
                                if [[ "${item:0:3}" == "bt." ]] ; then
                                    echolog "FAILED: runScenarios - ${scenario_set_name}: ${scenario_name}_Core - A core file was found on A${appliance} after running ${scenario_name}."
                                    # MessageSight will compress file so we do not need to do this anymore
                                    #cmd="ssh ${m_userVal}@${a_host_ip} tar czf ${aCorePath}/${item}.tgz ${aCorePath}/${item}"
                                    #reply=$($cmd 2>/dev/null)
                                    scp ${m_userVal}@${a_host_ip}:${aCorePath}/${item} ${scenario_name}/A${appliance}/messagesight/diag/cores
                                    ssh ${m_userVal}@${a_host_ip} rm -f ${aCorePath}/${item}
                                    #ssh ${m_userVal}@${a_host_ip} rm -f ${aCorePath}/${item}.tgz
                                fi
                            done
                        fi
                    fi


                    #scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/cores/* ${scenario_name}/A${appliance}/cores/.
                    #ssh ${m_userVal}@${a_host_ip} rm -rf /mnt/A${appliance}/cores/messagesight/*

                    ssh ${a_host_ip} ip addr show > ${scenario_name}/A${appliance}/ipaddr.log
                    ssh ${a_host_ip} last > ${scenario_name}/A${appliance}/last.log
                    ssh ${a_host_ip} lastb > ${scenario_name}/A${appliance}/lastb.log
                    ssh ${a_host_ip} journalctl > ${scenario_name}/A${appliance}/journalctl.log

                    if [[ "${a_typeVal}" == "DOCKER" ]] ; then
                        ssh ${a_host_ip} docker logs -t ${a_srvcontidVal} >> ${scenario_name}/A${appliance}/docker.log
                        ssh ${a_host_ip} cat /var/log/messages | grep "docker:" >> ${scenario_name}/A${appliance}/messages.log
                    fi
                else
                    #------------------------------------------------------------------
                    # Get list of core files
                    #------------------------------------------------------------------
                    cmd="ssh ${a_userVal}@${a_hostVal} advanced-pd-options list"
                    reply=$($cmd)
                    RC=$?
                    echolog "${cmd} ReturnCode=$RC"
                    echolog ${reply}

                    if [ $RC -ne 0 ] ; then
                        echolog "FAILED: runScenarios - ${scenario_set_name}: advanced-pd-options list failed."
                    else
                        #------------------------------------------------------------------
                        # Get all core files that were returned
                        #------------------------------------------------------------------
                        # The core file name shows up twice in the result of the advanced-pd-options list command so we need to skip the second time
                        last_file_name=""
                        for item in ${reply} ; do
                            if [[ "${item:0:3}" == "bt." || "${item}" =~ core. ]] ; then
                                if [[ "${item}" != "${last_file_name}" ]] ; then
                                    #echo "item=${item}"
                                    last_file_name=${item}
                                    #------------------------------------------------------------------
                                    # get core file
                                    #------------------------------------------------------------------
                                    cmd="ssh ${a_userVal}@${a_hostVal} advanced-pd-options export ${item} scp://${m_userVal}@[${m_ipv4_1Val}]:${m_testcasedirVal}/${scenario_name}/${item}"
                                    # if filename does not end with .enc we assume it is an obfuscated file so we add .otx file extension
                                    if [[ "${item:${#item} - 4}" != ".enc" ]] ; then
                                        cmd="$cmd.otx"
                                    fi
                                    reply=$($cmd)
                                    RC=$?
                                    echolog "${cmd} ReturnCode=$RC"
                                    echolog ${reply}
                                    if [ $RC -ne 0 ] ; then
                                        echolog "FAILED: runScenarios - ${scenario_set_name}: advanced-pd-options export ${item} failed."
                                    else
                                        echolog "FAILED: runScenarios - ${scenario_set_name}: ${scenario_name}_Core - A core file was found on A${appliance} after running ${scenario_name}."
                                        #------------------------------------------------------------------
                                        # Delete core file
                                        #------------------------------------------------------------------
                                        cmd="ssh ${a_userVal}@${a_hostVal} advanced-pd-options delete ${item}"
                                        reply=$($cmd)
                                        RC=$?
                                        echolog "${cmd} ReturnCode=$RC"
                                        echolog ${reply}
                                        if [ $RC -ne 0 ] ; then
                                            echolog "FAILED: runScenarios - ${scenario_set_name}: advanced-pd-options delete ${item} failed."
                                        fi
                                    fi
                                fi
                            fi
                        done
                    fi

                    #------------------------------------------------------------------
                    # Get must-gather file
                    #------------------------------------------------------------------
                    # only get must-gather if there were test case failures or core files found
                    if [[ ${pass_all} != 0 || ${core_found} == "true" || ${includeMustGather} == "true" ]] ; then
                        # Copy appliance log files to directory
                        ssh ${a_userVal}@${a_hostVal} imaserver trace flush
                        ssh ${a_userVal}@${a_hostVal} platform must-gather must-gather.tgz
                        RC=$?
                        if [ $RC -ne 0 ]
                        then
                            echolog "FAILED: runScenarios - ${scenario_set_name}: must-gather failed on imaserver A${A_COUNT} ${a_hostVal}, ReturnCode = $RC"
                        fi
                        if ! ssh ${a_userVal}@${a_hostVal} file put must-gather.tgz scp://${m_userVal}@${m_ipv4_1Val}:${m_testcasedirVal}/${scenario_name}/${a_hostVal}.tgz
                        then
                            # must-gather failed, add StrictHostKeyChecking=no to ssh_config file and try again
                            echolog "must-gather failed... adding StrictHostKeyChecking no to config file in admin's home directory on imaserver A${A_COUNT} ${a_hostVal} and trying again"
                            echo "echo 'StrictHostKeyChecking no' > /home/admin/.ssh/config" | ssh ${a_userVal}@${a_hostVal} busybox
                            ssh ${a_userVal}@${a_hostVal} file put must-gather.tgz scp://${m_userVal}@${m_ipv4_1Val}:${m_testcasedirVal}/${scenario_name}/${a_hostVal}.tgz
                            RC=$?
                            if [ $RC -ne 0 ]
                            then
                                echolog "FAILED: runScenarios - ${scenario_set_name}: file_put - scp of must-gather failed on imaserver A${A_COUNT} ${a_hostVal}, RC = $RC"
                            fi
                        fi
                    fi

                fi # end if Docker
            fi #end if A?_SKIP
            appliance=$(( appliance + 1 ))
        done

        ################################################################
        # Copy IMA Proxy Server files to local directory
        # Collect logs from proxy machines
        ################################################################

        proxy=1

        while [ ${proxy} -le ${P_COUNT} ]
        do
            # if P?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
            p_host="P${proxy}_HOST"
            p_hostVal=$(eval echo \$${p_host})
            m_user="${THIS}_USER"
            m_userVal=$(eval echo \$${m_user})
            # m_host="${THIS}_HOST"  ##Can't really use 'host' here like expect when in the KVM, use the IPv4_1 value and hope for the best :-)
            m_ipv4_1="${THIS}_IPv4_1"
            m_ipv4_1Val=$(eval echo \$${m_ipv4_1})
            # If m_ipv4 is really a ipv6 address for the case where automation forces all test are run using ipv6 addresses
            # convert to ipv4 address (this assumes the convention we are using where 10.10.3.211 and fc00::10:10:3:211)
            if [[ "$m_ipv4_1Val" == *:* ]] ; then
                m_ipv4_1Val=${m_ipv4_1Val:6}
                m_ipv4_1Val=${m_ipv4_1Val//:/.}
            fi
            m_tcroot="${THIS}_TESTROOT"
            m_tcrootVal=$(eval echo \$${m_tcroot})
            m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"

            # There is not a P type we can check to see if we are docker or rpm
            # Check for either for bridge
            #if [[ "${p_typeVal}" == "DOCKER" || "${p_typeVal}" == "RPM" ]] ; then

            # !!!! Docker does not support list at this time
            # SCP all logs from host, proxy first.
            p_host_ip=$(echo ${p_hostVal} | cut -d' ' -f 1)
            mkdir -p ${scenario_name}/P${proxy}/imaproxy/logs
            scp -r ${p_host_ip}:/niagara/proxy/\*.txt ${scenario_name}/P${proxy}/imaproxy/logs/.
            scp -r ${p_host_ip}:/niagara/proxy/\*.log ${scenario_name}/P${proxy}/imaproxy/logs/.
            scp -r ${p_host_ip}:/niagara/proxy/trc\*.\* ${scenario_name}/P${proxy}/imaproxy/logs/.
            # remove proxy logs after copying them
            ssh root@${p_host_ip} "rm -f /niagara/proxy/trc.\*"
            ssh root@${p_host_ip} "rm -f /niagara/proxy/\*.txt"
            ssh root@${p_host_ip} "rm -f /niagara/proxy/\*.log"
            # probably won't get config since the test generally removes them
            mkdir -p ${scenario_name}/P${proxy}/imaproxy/config
            scp ${p_host_ip}:/niagara/proxy/config/\* ${scenario_name}/P${proxy}/imaproxy/config/.
            mkdir -p ${scenario_name}/P${proxy}/imaproxy/cores
            scp -r ${p_host_ip}:/niagara/proxy/bt.\* ${scenario_name}/P${proxy}/imaproxy/cores/.
            if [[ "${core_proxy_found}" == "true" ]] ; then
                proxyCorePath="/tmp/cores"
                cmd="ssh ${m_userVal}@${p_host_ip} cd ${proxyCorePath} && ls bt.*"
                reply=$($cmd 2>/dev/null)
                RC=$?
                if [ $RC -eq 0 ] ; then
                    for item in ${reply} ; do
                        if [[ "${item:0:3}" == "bt." ]] ; then
                            echolog "FAILED: runScenarios - ${scenario_set_name}: ${scenario_name}_Core - A core file was found from imaproxy on P${proxy} after running ${scenario_name}."
                            # MessageSight will compress file so we do not need to do this anymore
                            #cmd="ssh ${m_userVal}@${p_host_ip} tar czf ${proxyCorePath}/${item}.tgz ${proxyCorePath}/${item}"
                            #reply=$($cmd 2>/dev/null)
                            #scp ${m_userVal}@${p_host_ip}:${proxyCorePath}/${item} ${scenario_name}/P${proxy}/cores
                            # Leave the cores on the proxy server since they are small and we may want to use gdb to debug them
                            #ssh ${m_userVal}@${p_host_ip} rm -f ${proxyCorePath}/${item}
                            #ssh ${m_userVal}@${p_host_ip} rm -f ${proxyCorePath}/${item}.tgz
                            ssh ${m_userVal}@${p_host_ip} "gdb --batch -ex \"bt full\" /niagara/proxy/bin/imaproxy ${proxyCorePath}/${item} > ${proxyCorePath}/${item}_full_stack_gdb.out"
                            ssh ${m_userVal}@${p_host_ip} "gdb --batch -ex \"info thread\" /niagara/proxy/bin/imaproxy ${proxyCorePath}/${item} > ${proxyCorePath}/${item}_info_thread_gdb.out"
                            ssh ${m_userVal}@${p_host_ip} "gdb --batch -ex \"thread apply all backtrace full\" /niagara/proxy/bin/imaproxy ${proxyCorePath}/${item} > ${proxyCorePath}/${item}_thread_apply_all_gdb.out"
                        fi
                    done
                fi
                scp -r ${m_userVal}@${p_host_ip} "${proxyCorePath}/\*gdb.out $scenario_name/P${proxy}/imaproxy/diag/logs"
            fi

            ssh ${p_host_ip} ip addr show > ${scenario_name}/P${proxy}/ipaddr.log
            ssh ${p_host_ip} last > ${scenario_name}/P${proxy}/last.log
            ssh ${p_host_ip} lastb > ${scenario_name}/P${proxy}/lastb.log
            ssh ${p_host_ip} journalctl > ${scenario_name}/P${proxy}/journalctl.log
            proxy=$(( proxy + 1 ))
        done # End looping over proxies

        # Copy IMA Bridge Server files to local directory

        bridge=1

        while [ ${bridge} -le ${B_COUNT} ]
        do
            b_host="B${bridge}_HOST"
            b_hostVal=$(eval echo \$${b_host})
            m_user="${THIS}_USER"
            m_userVal=$(eval echo \$${m_user})
            # m_host="${THIS}_HOST"  ##Can't really use 'host' here like expect when in the KVM, use the IPv4_1 value and hope for the best :-)
            m_ipv4_1="${THIS}_IPv4_1"
            m_ipv4_1Val=$(eval echo \$${m_ipv4_1})
            # If m_ipv4 is really a ipv6 address for the case where automation forces all test are run using ipv6 addresses
            # convert to ipv4 address (this assumes the convention we are using where 10.10.3.211 and fc00::10:10:3:211)
            if [[ "$m_ipv4_1Val" == *:* ]] ; then
                m_ipv4_1Val=${m_ipv4_1Val:6}
                m_ipv4_1Val=${m_ipv4_1Val//:/.}
            fi
            m_tcroot="${THIS}_TESTROOT"
            m_tcrootVal=$(eval echo \$${m_tcroot})
            m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"

            # There is not a P type we can check to see if we are docker or rpm
            # Check for either for bridge
            #if [[ "${b_typeVal}" == "DOCKER" || "${b_typeVal}" == "RPM" ]] ; then

            # !!!! Docker does not support list at this time
            # SCP all logs from host, bridge first.
            b_host_ip=$(echo ${b_hostVal} | cut -d' ' -f 1)
            mkdir -p ${scenario_name}/B${bridge}/imabridge/logs
            scp -r ${b_host_ip}:/var/imabridge/diag/logs/\*.log ${scenario_name}/B${bridge}/imabridge/logs/.
            scp -r ${b_host_ip}:/var/imabridge/diag/logs/\*.trace ${scenario_name}/B${bridge}/imabridge/logs/.
            mkdir -p ${scenario_name}/B${bridge}/imabridge/config
            scp ${b_host_ip}:/var/imabridge/config/* ${scenario_name}/B${bridge}/imabridge/config/.
            mkdir -p ${scenario_name}/B${bridge}/imabridge/cores
            scp -r ${b_host_ip}:/var/imabridge/diag/cores/bt.\* ${scenario_name}/B${bridge}/imabridge/cores/.
            if [[ "${core_bridge_found}" == "true" ]] ; then
                bridgeCorePath="/var/imabridge/diag/cores"
                cmd="ssh ${m_userVal}@${b_host_ip} cd ${bridgeCorePath} && ls bt.*"
                reply=$($cmd 2>/dev/null)
                RC=$?
                if [ $RC -eq 0 ] ; then
                    for item in ${reply} ; do
                        if [[ "${item:0:3}" == "bt." ]] ; then
                            echolog "FAILED: runScenarios - ${scenario_set_name}: ${scenario_name}_Core - A core file was found from imabridge on B${bridge} after running ${scenario_name}."
                            # MessageSight will compress file so we do not need to do this anymore
                            #cmd="ssh ${m_userVal}@${b_host_ip} tar czf ${bridgeCorePath}/${item}.tgz ${bridgeCorePath}/${item}"
                            #reply=$($cmd 2>/dev/null)
                            #scp ${m_userVal}@${b_host_ip}:${bridgeCorePath}/${item} ${scenario_name}/B${bridge}/cores
                            # Leave the cores on the bridge server since they are small and we may want to use gdb to debug them
                            #ssh ${m_userVal}@${b_host_ip} rm -f ${bridgeCorePath}/${item}
                            #ssh ${m_userVal}@${b_host_ip} rm -f ${bridgeCorePath}/${item}.tgz
                            ssh ${m_userVal}@${b_host_ip} "gdb --batch -ex \"bt full\" /niagara/bridge/bin/imabridge ${bridgeCorePath}/${item} > ${bridgeCorePath}/${item}_full_stack_gdb.out"
                            ssh ${m_userVal}@${b_host_ip} "gdb --batch -ex \"info thread\" /niagara/bridge/bin/imabridge ${bridgeCorePath}/${item} > ${bridgeCorePath}/${item}_info_thread_gdb.out"
                            ssh ${m_userVal}@${b_host_ip} "gdb --batch -ex \"thread apply all backtrace full\" /niagara/bridge/bin/imabridge ${bridgeCorePath}/${item} > ${bridgeCorePath}/${item}_thread_apply_all_gdb.out"
                        fi
                    done
                    scp -r ${m_userVal}@${b_host_ip} "${bridgeCorePath}/\*gdb.out $scenario_name/B${bridge}/imabridge/diag/logs"
                fi
            fi
            #scp -r ${m_userVal}@${b_host_ip}:/mnt/B${bridge}/cores/* ${scenario_name}/B${bridge}/cores/.
            #ssh ${m_userVal}@${b_host_ip} rm -rf /mnt/B${bridge}/cores/messagesight/*

            ssh ${b_host_ip} ip addr show > ${scenario_name}/B${bridge}/ipaddr.log
            ssh ${b_host_ip} last > ${scenario_name}/B${bridge}/last.log
            ssh ${b_host_ip} lastb > ${scenario_name}/B${bridge}/lastb.log
            ssh ${b_host_ip} journalctl > ${scenario_name}/B${bridge}/journalctl.log
            bridge=$(( bridge + 1 ))
        done # end looping over bridges

        ## MOVE  local client's log files to local log directory
        LOGFILES=( `ls *.log javacore.*` )
        for l in ${LOGFILES[@]}
        do
            mv ${l} ${scenario_name}/
        done

        # Copy remote log files to the local log directory
        machine=1
        while [ ${machine} -le ${M_COUNT} ]
        do
            m_skip="M${machine}_SKIP"
            m_skip_val=$(eval echo \$${m_skip})
            if [[ "$m_skip_val" != "TRUE" ]] ; then
                # if M?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)

                if [ "M${machine}" == ${THIS} ]
                then

                    ## @Local machine, log files already moved to local log directory - clean up the residual files
                    #rm -f ${localTestcasePath}/*.log
                    rm -f ${localTestcasePath}/temp_*.sh
                    rm -f ${localTestcasePath}/*_gen_*.xml

                    ip addr show > ${localTestcasePath}/M${machine}_ipaddr.log
                    echo "" >> ${localTestcasePath}/M${machine}_ipaddr.log
                    echo "ping6 A1_IPv6_1 -I eth1 -c 1" >> ${localTestcasePath}/M${machine}_ipaddr.log
                    ping6 ${A1_IPv6_1} -I eth1 -c 1 >> ${localTestcasePath}/M${machine}_ipaddr.log
                else
                    ## @Remote Machine, copy files to local log directory and then 'clean_up' the residual files from from remote machine
                    m_user="M${machine}_USER"
                    m_userVal=$(eval echo \$${m_user})
                    m_host="M${machine}_HOST"
                    m_hostVal=$(eval echo \$${m_host})
                    m_tcroot="M${machine}_TESTROOT"
                    m_tcrootVal=$(eval echo \$${m_tcroot})
                    m_testcasedirVal="${m_tcrootVal}/${localTestcaseDir}"

                    ## Copy remote files to directory and clean-up if not running on the subcontroller
                    scp ${m_userVal}@${m_hostVal}:${m_testcasedirVal}/*.log ${scenario_name}/

                    clean_up_one;

                    ssh ${m_hostVal} ip addr show > ${localTestcasePath}/M${machine}_ipaddr.log
                    echo "" >> ${localTestcasePath}/M${machine}_ipaddr.log
                    echo "ping6 A1_IPv6_1 -I eth1 -c 1" >> ${localTestcasePath}/M${machine}_ipaddr.log
                    ssh ${m_hostVal} ping6 ${A1_IPv6_1} -I eth1 -c 1 >> ${localTestcasePath}/M${machine}_ipaddr.log
                fi
            fi

            machine=$(( machine + 1))
        done
    fi
}


#---------------------------------------------------------------------#
# The following variables must be set before calling this function.
# ${a_hostVal[${appliance}]}:${a_portVal}
# ${scenario_set_name}
# $StateDescProduction
# $StateDescStandby
#---------------------------------------------------------------------#
function rest_CleanStore {

    clean_store_successful="true"

    # CleanStore and restart imaserver
    cmd="curl -X POST ${curl_silent_options} ${curl_timeout_options} -d {\"Service\":\"Server\",\"CleanStore\":true} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/restart"
    echo "A${appliance}: $cmd" | tee -a $logfile
    reply=$($cmd)
    RC=$?
    echo "$reply" | tee -a $logfile
    echo "RC=$RC" | tee -a $logfile
    if [ $RC -ne 0 ] ; then
        echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:Server restart returned error."
        clean_store_successful="false"
        return

    else
        # wait for container to come up
        cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
        echo "A${appliance}: $cmd" | tee -a $logfile
        eclipse_s=0
        beginTime=$(date)
        while [ $eclipse_s -lt 120 ]
        do
            echo -n "*" | tee -a $logfile
            sleep 1
            reply=$($cmd)
            RC=$?
            # if not RC=0 no since checking reply
            if [ $RC -eq 0 ] ; then
                StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")
                ##if [[ ("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                if [[ ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
                    break
                fi
            fi
            endTime=$(date)
            # Caculate elapsed time
            calculateTime
        done
        echo "" | tee -a $logfile
        echo "StateDescription=$StateDescription" | tee -a $logfile

        # I found that if I put quotes around $reply echo command preserves the line feeds and if I don't it all gets printed on one line
        # For the multiple server machines case it is easier to compare the multiple machines when each machine's replay is on one line
        echo $reply >> $logfile
        echo "RC=$RC" | tee -a $logfile

        ##if [[ ("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
        if [[ ! ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
            echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:Service Status is not Running (production)."
            clean_store_successful="false"
        fi
    fi
}


#---------------------------------------------------------------------#
function verify_imaserver_running {

    local all_imaserver_running="true"
    local appliance=1
    while [ ${appliance} -le ${A_COUNT} ]
    do
        echo "" | tee -a $logfile
        echo ">>> Verifying A${appliance}:Servers is running" | tee -a $logfile
        a_skip="A${appliance}_SKIP"
        a_skip_val=$(eval echo \$${a_skip})
        if [[ "$a_skip_val" != "TRUE" ]] ; then
            # if A?_SKIP is TRUE then skip. (This give the test cases a way to skip this operation for a given machine)
            if [[ A${appliance}_SKIP_VERIFY_SERVER_RUNNING -ne 1 ]] ; then
                a_user="A${appliance}_USER"
                a_userVal[${appliance}]=$(eval echo \$${a_user})
                a_host="A${appliance}_HOST"
                a_hostVal[${appliance}]=$(eval echo \$${a_host})
                a_rest="A${appliance}_REST"
                a_restVal=$(eval echo \$${a_rest})
                a_port="A${appliance}_PORT"
                a_portVal=$(eval echo \$${a_port})
                a_type="A${appliance}_TYPE"
                a_typeVal=$(eval echo \$${a_type})
                a_srvcontid="A${appliance}_SRVCONTID"
                a_srvcontidVal=$(eval echo \$${a_srvcontid})

                # check status
                cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
                echo "A${appliance}: $cmd" | tee -a $logfile
                reply=$($cmd)
                RC=$?
                # I found that if I put quotes around $reply echo command preserves the line feeds and if I don't it all gets printed on one line
                # For the multiple server machines case it is easier to compare the multiple machines when each machine's replay is on one line
                echo $reply >> $logfile
                echo "RC=$RC" | tee -a $logfile

                if [ $RC -ne 0 ] ; then
                    # If Docker and status is not 0 container is probably not running so try to start
                    if [[ "${a_typeVal}" == "DOCKER" || "${a_typeVal}" == "RPM" ]] ; then
                        echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:Server may not be running. service/status returned RC=$RC"
                        # Set so that we collect logs even if all test pass after this
                        pass_all=1
                        # Try to restart Docker container
                        if [[ "${a_typeVal}" == "DOCKER" ]] ; then
                            cmd="ssh ${a_userVal[${appliance}]}@${a_hostVal[${appliance}]} docker ps -a"
                            echo "A${appliance}: $cmd" | tee -a $logfile
                            reply=$($cmd)
                            RC=$
                            echo "$reply" | tee -a $logfile
                            echo "RC=$RC" | tee -a $logfile
                            cmd="ssh ${a_userVal[${appliance}]}@${a_hostVal[${appliance}]} docker start ${a_srvcontidVal}"
                            echo "A${appliance}: $cmd" | tee -a $logfile
                            reply=$($cmd)
                            RC=$?
                        else
                            #!!! the name of this script will probably change
                            #cmd='ssh ${a_userVal[${appliance}]}@${a_hostVal[${appliance}]} "cd /opt/ibm/imaserver/bin;./startDockerMSServer.sh >/dev/null 2>&1 &"'
                            cmd='ssh ${a_userVal[${appliance}]}@${a_hostVal[${appliance}]} "systemctl start imaserver"'
                            echo "A${appliance}: $cmd" | tee -a $logfile
                            reply=$(eval ${cmd})
                            RC=$?
                        fi
                        echo "$reply" | tee -a $logfile
                        echo "RC=$RC" | tee -a $logfile

                        if [ $RC -ne 0 ] ; then
                            echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:Server start returned error."
                            # if start failed, we still want to get logs
                            all_imaserver_running="false"
                            # Failed to verify server running so check next Server
                            appliance=$(( appliance + 1))
                            continue
                        else
                            # wait for server to come up
                            eclipse_s=0
                            beginTime=$(date)
                            cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
                            echo "A${appliance}: $cmd" | tee -a $logfile
                            while [ $eclipse_s -lt 120 ] ; do
                                echo -n "*" | tee -a $logfile
                                sleep 1
                                reply=$($cmd 2>/dev/null)
                                RC=$?
                                # if not RC=0 no since checking reply
                                if [ $RC -eq 0 ] ; then
                                    StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                                    ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")

                                    ##if [[ ("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                                    if [[ ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" || "$StateDescription" =~ "$StateDescMaintenance") ]] ; then
                                        break
                                    fi
                                fi
                                endTime=$(date)
                                # Caculate elapsed time
                                calculateTime
                            done
                            echo "" | tee -a $logfile
                            echo "StateDescription=$StateDescription" | tee -a $logfile
                            echo $reply >> $logfile
                            echo "RC=$RC" | tee -a $logfile

                            if [ $RC -ne 0 ] ; then
                                echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:Issued $a_typeVal start and service/status returned RC=$RC."
                                # if status fails, we still want to get logs
                                all_imaserver_running="false"
                                # Failed to verify server running so check next Server
                                appliance=$(( appliance + 1))
                                continue
                            fi
                        fi
                    else
                        # If not DOCKER or RPM, not supported
                        echolog "FAILED: runScenarios - ${scenario_set_name}: ${a_hostVal[${appliance}]} A${appliance}:GET /ima/v1/service/status returned RC=$RC"
                        # if status fails, we still want to get logs
                        pass_all=1
                        get_log_files
                        exit 1
                    fi
                else ## RC=0 so service/status command was successful.
                    StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                    ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")
                    echo "StateDescription=$StateDescription" | tee -a $logfile
                fi

                #-------------------------------------------------------------------------------------------------------------
                # If we got here a server/status command was successfully run. If StateDescription is not Running (production)
                # or Standby we first try to switch to production and it this does not work we try to clean the store
                # ------------------------------------------------------------------------------------------------------------
                ##if [[ !("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                if [[ ! ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then

                    echolog ">>>Trying to restart A${appliance}:Server in production mode"
                    cmd="curl -X POST -s -S -d {\"Service\":\"Server\",\"Maintenance\":\"stop\"} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/restart"
                    echolog "A${appliance}: $cmd"
                    reply=$($cmd)
                    RC=$?
                    echolog "$reply"
                    echolog "RC=$RC"

                    cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
                    echolog "A${appliance}: $cmd"
                    eclipse_s=0
                    beginTime=$(date)
                    while [ $eclipse_s -lt 30 ] ; do
                        echo -n "*" | tee -a $logfile
                        sleep 1
                        reply=$($cmd 2>/dev/null)
                        RC=$?
                        # if not RC=0 no since checking reply
                        if [ $RC -eq 0 ] ; then
                            StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                            ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")
                            if [ $RC -eq 0 ] ; then
                                ##if [[ ("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                                if [[ ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
                                    break
                                fi
                            fi
                        fi
                        endTime=$(date)
                        # Caculate elapsed time
                        calculateTime
                    done
                    echo "" | tee -a $logfile
                    echo "StateDescription=$StateDescription" | tee -a $logfile
                    echo $reply >> $logfile
                    echolog "RC=$RC"

                    ##if [[ !("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                    if [[ ! ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
                        # If LDAP is enabled and we cannot connect to LDAP system will come up in Maintenance mode
                        echolog ">>>Try disabling A${appliance}:Server's LDAP"
                        cmd="curl -X POST -s -S -d {\"LDAP\":{\"Enabled\":false}} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/configuration"
                        echolog "A${appliance}: $cmd"
                        reply=$($cmd)
                        RC=$?
                        echolog "$reply"
                        echolog "RC=$RC"

                        echolog ">>>Trying to restart A${appliance}:Server in production mode again"
                        cmd="curl -X POST -s -S -d {\"Service\":\"Server\",\"Maintenance\":\"stop\"} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/restart"
                        echolog "A${appliance}: $cmd"
                        reply=$($cmd)
                        RC=$?
                        echolog "$reply"
                        echolog "RC=$RC"

                        cmd="curl -X GET ${curl_silent_options} ${curl_timeout_options} http://${a_hostVal[${appliance}]}:${a_portVal}/ima/v1/service/status"
                        echolog "A${appliance}: $cmd"
                        elapsed=0
                        while [ $elapsed -lt 30 ] ; do
                            echo -n "*" | tee -a $logfile
                            sleep 1
                            reply=$($cmd 2>/dev/null)
                            RC=$?
                            # if not RC=0 no since checking reply
                            if [ $RC -eq 0 ] ; then
                                StateDescription=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]")
                                ##Status=$(echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"Status\"]")
                                ##if [[ ("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                                if [[ ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
                                    break
                                fi
                            fi
                            elapsed=$(( elapsed + 1))
                        done
						echo "" | tee -a $logfile
                        echo "StateDescription=$StateDescription" | tee -a $logfile
                        echo $reply >> $logfile
                        echolog "RC=$RC"


                        ##if [[ !("$Status" =~ "$StatusRunning" && ( "$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby" ) ) ]]
                        if [[ ! ("$StateDescription" =~ "$StateDescProduction" || "$StateDescription" =~ "$StateDescStandby") ]] ; then
                            echolog ">>>A${appliance}:Server still not in production mode. Trying to restart Server with CleanStore"
                            rest_CleanStore
                            if [[ "$clean_store_successful" == "false" ]] ; then
                                all_imaserver_running="false"
                            fi
                        fi
                    fi
                fi
            fi # end if A?_SKIP
        fi
        appliance=$((appliance + 1))
    done

    if [[ "$all_imaserver_running" == "false" ]] ; then
        echolog ">>>Was not able to verify that all servers were running"
        pass_all=1
        # We still want to get logs
        get_log_files
        exit 1
    fi
}



#-------------------------------------------------------------------------------
function stopTest {

    beginTime=${saveBeginTime}
    endTime=$(date)
    echolog "End: ${endTime}"
    logonly " " >> ${logfile}

    #-------------------------------------------------------------------------------
    # Determine overall result
    #-------------------------------------------------------------------------------
    if [[ $server_stopped_running == "true" ]] ; then
        echolog "FAILED: runScenarios - ${scenario_set_name}: Server stopped running during execution."
    fi
    #total=${#scenario[*]}
    ((total=ttl_pass+ttl_fail+ttl_skip))
    echolog "Test Result of ${scenario_set_name}:  (Total:${total}   Pass:${ttl_pass}   Fail:${ttl_fail} Skip:${ttl_skip})"

    # Call the calculateTime function from commonFunctions.sh
    calculateTime

    echolog "Total time required for test execution - ${hr}:${min}:${sec}"
    logonly " "
    logonly "------------------------ End of ${scenario_set_name} ------------------------"
    logonly " " >> ${logfile}

    # Even if all test passed, there may still be a core file, so we need to check
    check_for_cores
    if [[ ${pass_all} == 0 && ${core_found} != "true" ]] ; then
        #result="PASS"

        if [[ $(getSavePassedLogs) == "on" ]] ; then
            # get_log_files() will clean up residual files and logs
            get_log_files
        else
            if [[ "${RS_IAM}" == "" ]]
            then
                ## If not running as subcontroller, then clean up.  Otherwise wait to let master Run-scenario.sh initiate the clean-up
                clean_up
            fi
        fi
    else
        #result="FAIL"
        get_log_files
    fi

    # kill any remaining processes
    #list=( `echo ${PIDS}` )
    read -r -a list <<<"$PIDS"
    killProcesses "${list[@]}"

}


#----------------------------------------------------------------------------------------------------------------------------------------------------------
# SIGINT_cleanup : when CTRL_C is set to interrupt processing, need to clean up the processes started, especially when run-scenarios was running
#   as a subcontroller  [ ex. ${IAM_RS}="subcontroller" is passed as parameter 3]
#
# NOTE:  when 'kill -9' is used to kill run-scenarios.sh THE SIGKILL SIGNAL CAN NOT BE TRAPPED, so NO cleanup processing can/will be done.
#   Be warned. Use SIGKILL as a last resort.
#----------------------------------------------------------------------------------------------------------------------------------------------------------
function SIGINT_cleanup {

    # kill any remaining processes
    stopTest
    #	list=( `echo ${PIDS}` )
    #	killProcesses "${list[@]}"
    exit
}

#-------------------------------------------------------------------------------
function waitOnPid {
    # You must have called parseComponent function, started the component and set PID prior to calling this function.
    # Sets KILL_PID=1,
    eclipse_s=0
    progressLine=1
    beginTime=$(date)
    while [ "${PID}" != "-1" ] && [ "${PID}" != "" ]; do
        echo -n "*"
        k=$(( eclipse_s/60 ))
        if [[ ${k} -ge ${progressLine} ]]; then
            ((progressLine+=1))
            echo ""
        fi

        RUNCMD=$(${SSH_IP}ps -ef | grep -v grep | cut -d : -f 1 | grep -c " ${PID} ")

        ## The above ps command sometime fails to find PID on cygwin.  Utill we have a better fix try a second time - jrs
        if [[ ${RUNCMD} -eq 0 && ${isCYGWIN} -eq 1 ]] ; then
                    RUNCMD=`${SSH_IP}ps -ef | grep -v grep | cut -d : -f 1 | grep -c " ${PID} "`
        fi

        if [[ ${RUNCMD} -ne 0 ]] ; then
            # Kill the process if the timeout has expired
            if [ ${eclipse_s} -ge ${TIMEOUT} ] ; then
                pstack ${PID} >> ${TOOL}_${PID}.pstack.log 2>&1
                pstack_rc=$?
                if [ ${pstack_rc} -eq 0 ]; then
                    echolog "pstack output stored in ${TOOL}_${PID}.pstack.log"
                else
                    echolog "pstack command failed ReturnCode=${pstack_rc} - trying gstack..."
                    # attempt gstack
                    gstack ${PID} >> ${TOOL}_${PID}.pstack.log 2>&1
                    gstack_rc=$?
                    if [ ${gstack_rc} -eq 0 ]; then
                        echolog "gstack output stored in ${TOOL}_${PID}.pstack.log"
                    else
                        echolog "gstack command failed ReturnCode=${gstack_rc} - no stack will be saved in ${TOOL}_${PID}.pstack."
                    fi
                fi

                echolog "Trying to kill PID ${PID}, ps shows:"
                ${SSH_IP}ps -ef | grep -v grep | grep " ${PID} " | tee -a ${logfile}

                # If Java PID generate javacore. Still need to do a regular kill however.
                if [[ "${TOOL}" =~ "java" || "${TOOL}" == "jmsDriver" || "${TOOL}" == "wsDriver" ]]
                then
                    ${SSH_IP}kill -3 ${PID}
                fi

                ${SSH_IP}kill -9 ${PID}
                kill_rc=$?
                echolog "-- $(date) --"
                KILL_PID=1
                if [[ ${kill_rc} -eq 0 ]] ; then
                    # Indicate that test failed so that we save the logs so we can debug the timeout
                    pass_all=1
                    wait_failed=1
                    echolog "TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${IP}"
                else
                    echolog "IGNORED - TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${IP}"
                fi
                PID="" # PID long longer exist (either we killed it or it when away by itself)
            fi
        else
            PID="" # PID completed before TIMEOUT
        fi

        sleep 1
        endTime=$(date)
        # Caculate elapsed time
        calculateTime
    done

    if [[ ${k} -lt ${progressLine} ]]; then
        echo ""
    fi
}


#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# BEGIN MAIN PATH
#-------------------------------------------------------------------------------
# Set some variables needed for the script
thiscmd=$0
DEBUG=OFF
trap SIGINT_cleanup SIGHUP SIGINT SIGTERM

localTestcasePath=$(pwd)
echo localTestcasePath="${localTestcasePath}"
#set -x
localTestcaseDir=$( basename ${localTestcasePath} )
echo localTestcaseDir="${localTestcaseDir}"

curl_silent_options="-f -s -S"
curl_timeout_options="--connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45"

# Source the commonFunctions.sh file to get access to common functions
source ../scripts/commonFunctions.sh
# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
    source  "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

#-------------------------------------------------------------------------------
# Validate input parameters and initialize output log file
#-------------------------------------------------------------------------------
if [[ $# -gt 3 || $# -lt 1 ]]; then
    err=1
else
    err=0
    scenario_list=$1
    # remove any path info in scenario_list's name
    scenario_name=$( basename ${scenario_list} )
    scenario_name=$(echo ${scenario_name} | cut -d '.' -f 1)
    default_logfile="${scenario_name}.log"
    logfile=${default_logfile}

    if [[ $# -eq 2 ]]; then
        logfile=$2
    fi

    if [[ -e ${logfile} ]]; then
        rm -f ${logfile}
    fi

    touch ${logfile}

    if [[ ! -w ${logfile} ]]; then
        print_error "ERROR: Unable to write to log file ${logfile}."
    fi

    # Run-scenarios is being started as a subController, set the Env Vars for subController
    if [[ $# -eq 3 ]]; then
        export ${3?}
    fi
   `echo ">>>>> RUN_SCENARIOS IS STARTING AS:" ${1} : ${2} : ${3} : ${RS_IAM} : >>${logfile}`
fi

if [[ ${err} -eq 1 ]] ; then
    echolog "Error in Input: ${thiscmd} $* RC=1"
    printUsage
    exit 1
fi


#-------------------------------------------------------------------------------
# Define the test case scenarios to be executed by this script and update the configuration files
#-------------------------------------------------------------------------------
if ! source ${scenario_list}
then
    print_error "Error sourcing ${localTestcasePath}/${scenario_list}"
fi

ostype=$(${SSH_IP}uname)
if [[ "${ostype}" =~ "CYGWIN" ]] ; then
    isCYGWIN=1
else
    isCYGWIN=0
fi

#-------------------------------------------------------------------------------
# Execute the test scenarios that were defined above.
#   'pass_all' is an indicator of whether or not all the tests executed by this script passed.
#    0 = pass; 1 = fail.
#-------------------------------------------------------------------------------
logonly  " "
logonly  "--------------------- Beginning of ${scenario_set_name} ---------------------"
echolog  " "
echolog  "SCENARIO: ${scenario_set_name}"
saveBeginTime=$(date)
echolog "Begin: ${saveBeginTime} (logfile: ${logfile})"

typeset -i  pass_all=0  ttl_fail=0  scenario_number=0  j=0 ttl_pass=0  ttl_skip=0

# Check to see if imaserver and mqconnectivity is running, if not restart
verify_imaserver_running

#
server_stopped_running="false"
end_and_cleanup="false"

#------------------------------------------------------------------------------------------------------
# Go through each scenario, and start the necessary components in order.
# There are 3 cases for starting
# 1. Start now and save PID and additional information in a list
# 2. Start now and wait for PID to complete
# 3. Ignore components that will be run at the end, they are handled later.
#------------------------------------------------------------------------------------------------------
while [[ $scenario_number -lt ${#scenario[*]} ]]
do
    if [[ "${end_and_cleanup}" == "true" && "${type[${scenario_number}]}" != "Cleanup" ]]
    then
         echolog "SKIPPED: Scenario ${scenario_number} - ${scenario[${scenario_number}]}"
        ((ttl_skip+=1))
    else
        if [[ "${type[${scenario_number}]}" == "Cleanup" ]] ; then
            # If type=Cleanup make sure that server is running so the cleanup can happen
            verify_imaserver_running
        fi

        # Initialize the result to passing
        result=0

        # display start timestamp
        echolog "-- $(date) --"
        if [[ "${type[${scenario_number}]}" != "" ]] ; then
            echolog "type=${type[${scenario_number}]}"
        fi

        #---------------------------------------------
        # Parse the list of components
        #---------------------------------------------
        component_array=( `echo ${components[${scenario_number}]}` )
        comp_idx=1
        KILL_PID=0
        wait_failed=0

        #------------------------------------------------------------------------------------
        # If $TIMEOUTMULTIPLIER is set, adjust timeout value (integer)
        #------------------------------------------------------------------------------------
        if [[ "$TIMEOUTMULTIPLIER" != "" && "${timeouts[${scenario_number}]}" != "" ]]
        then
            timeouts[${scenario_number}]=$(awk 'BEGIN {printf "%.0f\n", '${timeouts[${scenario_number}]}' * '${TIMEOUTMULTIPLIER}' }')
        fi

        for comp in ${component_array[@]}
        do
            PID=""
            # If cAppDriverLogWait, cAppDriverWait, javaAppDriverLogWait or javaAppDriverWait fails
            # we assume that there is no reason to start the remaining components
            if [[ ${wait_failed} == 0 ]] ; then
                # Set the variables for this component
                parseComponent ${comp}
                # Start the component and set the process id in PID
                startComponent

                # If the component started, save off the process id.  If not, then print an error message
                if [ "${PID}" == "-1" ] || [ "${PID}" == "" ]
                then
                    #           #  This use to be a longer/complicated IF-THEN-ELSE list, but I think it should only be there 3 commands are the only ones expected to NOT return a PID
                    #           #  Added runCommandNow to this list
                    if [ "${TOOL}" != "searchLogsEnd" ] && [ "${TOOL}" != "sleep" ] && [ "${TOOL}" != "runCommandEnd" ] && [ "${TOOL}" != "runCommandNow" ] && [ "${TOOL}" != "runCommandLocal" ] && [ "${TOOL}" != "enableLDAP" ] && [ "${TOOL}" != "disableLDAP" ] && [ "${TOOL}" != "purgeLDAPcert" ] && [ "${TOOL}" != "emptyLDAPcert" ]
                    then
                        #               if [ "${TOOL}" == "testDriver" ] || [ "${TOOL}" == "wsDriver" ] || [ "${TOOL}" == "wsDriverNocheck" ] || [ "${TOOL}" == "cAppDriver" ] || [ "${TOOL}" == "cAppDriverRC" ] || [ "${TOOL}" == "cAppDriverLog" ] || [ "${TOOL}" == "cAppDriverCfg" ] || [ "${TOOL}" == "javaAppDriver" ] || [ "${TOOL}" == "javaAppDriverRC" ] || [ "${TOOL}" == "javaAppDriverLog" ] || [ "${TOOL}" == "javaAppDriverCfg" ] || [ "${TOOL}" == "subController" ]
                        #                   then
                        print_error "${TOOL} ${TOOL_CFG} ${EXE_FILE} FAILED to start!"
                        #               fi
                    fi
                elif [ "${TOOL}" == "cAppDriverLogWait" ] || [ "${TOOL}" == "cAppDriverRConlyChkWait" ] || [ "${TOOL}" == "cAppDriverWait" ] || [ "${TOOL}" == "javaAppDriverLogWait" ] || [ "${TOOL}" == "javaAppDriverWait" ] || [ "${TOOL}" == "wsDriverWait" ] || [ "${TOOL}" == "jmsDriverWait" ] ; then
                    # Wait max of ${TIMEOUT} seconds for PID to go away before moving on to next component
                    echolog "Waiting for component ${comp} to complete..."
                    waitOnPid
                    # no need to wait on component_sleep since we waited on PID
                else
                    # Save the process ids and additional info into a space separated list
                    PIDS="${TOOL}:${PID}:${USER}:${IP}:${TIMEOUT}:${comp_idx}:${XMLFILE}:${TOOL_CFG} ${PIDS}"

                    # Sleep for ${component_sleep} seconds (or 3 seconds if it is not set) after starting each component
                    # to give it time to come up before starting the next component
                    if [ -z "${component_sleep}" ]
                    then
                        regex="End.*"
                        if [[ $comp =~ $regex ]] ; then
                            echolog " No need to sleep for component $comp"
                        else
                            sleep 3
                        fi
                    else
                        sleep ${component_sleep}
                    fi
                fi
                comp_idx=$(( comp_idx + 1))
            fi # end [ pass_all -eq 0 ]
        done # end loop through component array

        #-------------------------------------------------------------------------------
        # Watch to see if any processes that have a timeout set are still running.
        # If the timeout for a process has expired, then kill it
        #-------------------------------------------------------------------------------
        KILL_PID=0
        eclipse_s=0
        progressLine=1

        # list=( `echo ${PIDS}` )
        read -r -a list <<<"$PIDS"

        # Sort processes into a list of processes that have a timeout set, and a list of processes without a timeout
        sortPids "${list[@]}"

        if [ ${#expiring_process_list[@]} -ne 0 ]; then
           echolog "Waiting for scenario ${xml[${scenario_number}]} to complete..."
        fi

        beginTime=$(date)
        while [ ${#expiring_process_list[@]} -ne 0 ]
        do
            # Go through each process in the list and see if it is running, if so, keep it in the list
            # Processes in the list are in the format tool:pid:user:ip:timeout:xmlfile:name
            for p in "${expiring_process_list[@]}"
            do
                parsePids ${p}

                RUNCMD=$(${SSH_IP}ps -ef | grep -v grep | cut -d : -f 1 | grep -c " ${PID} ")

                ## The above ps command sometime fails to find PID on cygwin.  Utill we have a better fix try a second time - jrs
                if [[ ${RUNCMD} -eq 0 && ${isCYGWIN} -eq 1 ]] ; then
                    RUNCMD=$(${SSH_IP}ps -ef | grep -v grep | cut -d : -f 1 | grep -c " ${PID} ")
                fi

                if [[ ${RUNCMD} -ne 0 ]]
                then
                    # Kill the process if the timeout has expired; otherwise, add it to the list of still running processes
                    if [[ ${eclipse_s} -ge ${TIMEOUT} ]]
                    then
                        pstack ${PID} >> ${TOOL}_${PID}.pstack.log 2>&1
                        pstack_rc=$?
                        if [ ${pstack_rc} -eq 0 ]
                        then
                            echolog "pstack output stored in ${TOOL}_${PID}.pstack.log"
                        else
                            echolog "pstack command failed ReturnCode=${pstack_rc} - trying gstack..."
                            # attempt gstack
                            gstack ${PID} >> ${TOOL}_${PID}.pstack.log 2>&1
                            gstack_rc=$?
                            if [ ${gstack_rc} -eq 0 ]; then
                                echolog "gstack output stored in ${TOOL}_${PID}.pstack.log"
                            else
                                echolog "gstack command failed ReturnCode=${gstack_rc} - no stack will be saved in ${TOOL}_${PID}.pstack."
                            fi
                        fi
                        echolog "Trying to kill PID ${PID}, ps shows:"
                        ${SSH_IP}ps -ef | grep -v grep | grep " ${PID} " | tee -a ${logfile}
                        echolog "PIDS=${PIDS}"
                        s=0
                        for a in ${list[@]}
                        do
                            echo "pidlist[${s}] = '${a}'"
                            s=$(( s + 1))
                        done

                        # If Java PID generate javacore. Still need to do a regular kill however.
                        if [[ "${TOOL}" =~ "java" || "${TOOL}" == "jmsDriver" || "${TOOL}" == "wsDriver" ]]
                        then
                            ${SSH_IP}kill -3 ${PID}
                        fi

                        if [ "${TOOL}" == "subController" ]
                        then
                            ${SSH_IP}kill -s SIGTERM ${PID}
                        else
                            ${SSH_IP}kill -9 ${PID}
                        fi

                        kill_rc=$?
                        echolog "-- $(date) --"
                        KILL_PID=1
                        if [[ ${kill_rc} -eq 0 ]] ; then
                            # Indicate that at lease one test failed so that we save the logs so we can debug the timeout
                            pass_all=1
                            if [[ "${IP}" = "" ]]
                            then
                                echolog "TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${M1_host_ip}"
                            else
                                echolog "TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${IP}"
                            fi
                        else
                            if [[ "${IP}" = "" ]]
                            then
                                echolog "IGNORED - TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${M1_host_ip}"
                            else
                                echolog "IGNORED - TIMEOUT: Killing ${TOOL} ${XMLFILE} ${TOOL_CFG} process ${PID} on ${IP}"
                            fi
                        fi
                    else
                        new_list="${p} ${new_list}"
                    fi
                else
                    if [ "${DEBUG}" == "ON" ]
                    then
                        echo ""
                        echo "DEBUG...expiring_process_list: PID ${PID} is no longer running."
                        echo "DEBUG...RUNCMD=${RUNCMD}"
                        echo "DEBUG...p=${p}"
                    fi
                fi
            done # end loop through expiring process list

            echo -n "*"
            k=$(( eclipse_s/60 ))
            if [[ ${k} -ge ${progressLine} ]]; then
                ((progressLine+=1))
                echo ""
            fi

            sleep 1
            # expiring_process_list=( `echo ${new_list}` )
            read -r -a expiring_process_list <<<"${new_list}"
            new_list=
            endTime=$(date)
            # Caculate elapsed time
            calculateTime
        done # end while loop expiring process list

        if [[ ${k} -lt ${progressLine} ]]; then
            echo ""
        fi

        if [ "${DEBUG}" == "ON" ]
        then
            echo "DEBUG...expiring_process_list: No more components in list."
            echo "DEBUG...expiring_process_list=${expiring_process_list}"
            ps -ef | grep -v grep | grep java
        fi

        #--------------------------------------------------------
        # Kill any other processes that are still running
        #--------------------------------------------------------
        killProcesses "${nonexpiring_process_list[@]}"


        # Set the process list to be empty
        PIDS=


        # Set the LLMD process list to be empty
        LLMD_PIDS=


        #-----------------------------------------------------
        # Check for status of each driver execution, searchLogs, or runCommand
        # Return code of checkLog.sh:
        #   0 = all threads execution are expected
        #   1 = some thread execution aren not as expected
        #-----------------------------------------------------
        # For each testDriver or searchLogs component check the logfile
        # For each runCommand, check the rc file
        #-----------------------------------------------------
        comp_idx=1

        for comp in ${component_array[@]}
        do
            parseComponent ${comp}
            declare -i res=0
            if [ "${TOOL}" == "testDriver" ]
            then
               ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
               res=$?
               result=$(( result + res))

            #	elif [ "${TOOL}" == "cAppDriver" ] || [ "${TOOL}" == "cAppDriverCfg" ] || [ "${TOOL}" == "cAppDriverWait" ]
            #	then
            #	Preliminary Message to users writing automation using Dev. Samples - this is well known fact now
            #           echo "WARNING:  Don't own ${TOOL} output.  Can't check ${LOGFILE} for success, please use searchLogsEnd"

            elif [ "${TOOL}" == "cAppDriverRConlyChk"  ] || [ "${TOOL}" == "cAppDriverRConlyChkWait" ] || [ "${TOOL}" == "javaAppDriverRConlyChk" ]
            then
                SCREEN_OUTFILE=$(echo ${LOGFILE} | sed "s/\.log/\.screenout\.log/")

                ${SSH_IP}${SCRIPT_DIR}/checklog_AppDriverRC.sh ${TESTCASE_DIR}/${SCREEN_OUTFILE}
                res=$?
                result=$(( result + res ))

            elif [ "${TOOL}" == "cAppDriverLog" ] || [ "${TOOL}" == "cAppDriverLogWait" ]
            then
                ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
                res=$?
                result=$(( result + res ))

            elif [ "${TOOL}" == "cAppDriverLogEnd" ]
            then
                # Call the start.sh script on the appropriate machine to start the driver
                PID=$(${SSH_IP}${SCRIPT_DIR}/start.sh  -t ${TOOL} -d ${TESTCASE_DIR}  -e ${EXE_FILE} \"${ENVVARS}\" -f ${LOGFILE} \"${OTHER_OPTS}\")
                res=$?
                if [ $res -eq 0 ]
                then
                    echolog "Running component ${comp} and waiting to complete..."
                    waitOnPid
                    ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
                    res=$?
                fi
                result=$(( result + res ))
            elif [ "${TOOL}" == "subController"  ]
            then
                SCREEN_OUTFILE=$(echo ${LOGFILE} | sed "s/\.log/\.screenout\.log/")
                ${SSH_IP}${SCRIPT_DIR}/checkLog_subcontroller.sh ${TESTCASE_DIR}/${SCREEN_OUTFILE}
                res=$?
                result=$(( result + res ))
                #	elif [ "${TOOL}" == "javaAppDriver"  ] || [ "${TOOL}" == "javaAppDriverCfg" ] || [ "${TOOL}" == "javaAppDriverWait" ]
                #	then
                #	Preliminary Message to users writing automation using Dev. Samples - this is well known fact now
                #          echo "WARNING:  Don't own ${TOOL} output.  Can't check ${LOGFILE} for success, please use searchLogsEnd"

            elif [ "${TOOL}" == "javaAppDriverLog" ] || [ "${TOOL}" == "javaAppDriverLogWait" ]
            then
                ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
                res=$?
                result=$(( result + res ))

            elif [ "${TOOL}" == "jmsDriver" ] || [ "${TOOL}" == "jmsDriverWait" ]
            then
                if [ "${NAME}" == "ALL" ] ; then
                    ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
                    res=$?
                else
                    ${SSH_IP}${SCRIPT_DIR}/checkLog.sh ${TESTCASE_DIR}/${LOGFILE} ${NAME}
                    res=$?
                fi
                result=$(( result + res ))

            elif [ "${TOOL}" == "wsDriver" ] || [ "${TOOL}" == "wsDriverWait" ]
            then
                if [ "${NAME}" == "ALL" ] ; then
                    ${SSH_IP}${SCRIPT_DIR}/checkLog_noname.sh ${TESTCASE_DIR}/${LOGFILE}
                    res=$?
                else
                    ${SSH_IP}${SCRIPT_DIR}/checkLog.sh ${TESTCASE_DIR}/${LOGFILE} ${NAME}
                    res=$?
                fi
                result=$(( result + res ))

            elif [ "${TOOL}" == "searchLogs" ]
            then
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${COMPARE_LOG}) -eq 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${COMPARE_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "searchLogsNow" ]
            then
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${COMPARE_LOG}) -eq 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${COMPARE_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "enableLDAP" ]
            then
                echolog "Running enableLDAP..."
                ${SCRIPT_DIR}/enableLDAP.sh 1>${ENABLE_LDAP_LOG} 2>&1
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${ENABLE_LDAP_LOG}) -ne 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${ENABLE_LDAP_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "disableLDAP" ]
            then
                echolog "Running disableLDAP..."
                ${SCRIPT_DIR}/disableLDAP.sh 1>${ENABLE_LDAP_LOG} 2>&1
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${ENABLE_LDAP_LOG}) -ne 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${ENABLE_LDAP_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "purgeLDAPcert" ]
            then
                echolog "Running purgeLDAPcert..."
                ${SCRIPT_DIR}/purgeLDAPcert.sh 1>${PURGE_LDAPCERT_LOG} 2>&1
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${PURGE_LDAPCERT_LOG}) -ne 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${PURGE_LDAPCERT_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "emptyLDAPcert" ]
            then
                echolog "Running emptyLDAPcert..."
                ${SCRIPT_DIR}/emptyLDAPcert.sh 1>${EMPTY_LDAPCERT_LOG} 2>&1
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${EMPTY_LDAPCERT_LOG}) -ne 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${EMPTY_LDAPCERT_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "searchLogsEnd" ]
            then
                echolog "Running searchLogsEnd..."
                ${SSH_IP}${SCRIPT_DIR}/start.sh -t searchLogsEnd -d ${TESTCASE_DIR} -m ${COMPARE_FILE}
                if [ $(${SSH_IP}grep -c \"Log Search Result is Success\" ${TESTCASE_DIR}/${COMPARE_LOG}) -eq 0 ]
                then
                    echolog "There was a failure while searching the logs for this test. Check ${COMPARE_LOG} for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "runCommand" ]
            then
                if [ $(${SSH_IP}cat ${TESTCASE_DIR}/temp_runcmd_${CUENUM}.rc) -ne 0 ]
                then
                    echolog "There was a failure in runCommand ${COMMAND}. Check ${COMMAND}.${CUENUM}.log for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "runCommandNow" ]
            then
                if [ -f ${COMMAND} ]
                then
                    LOGNAME=$(echo ${COMMAND} | awk -F'/' '{ print $NF }')
                else
                    LOGNAME=${COMMAND}
                fi

                if [ $(${SSH_IP}cat ${TESTCASE_DIR}/${LOGNAME}.rc) -ne 0 ]
                then
                    echolog "There was a failure in runCommandNow ${COMMAND}. Check ${COMMAND}.log for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "runCommandEnd" ]
            then
                echolog "Running" ${SSH_IP}${SCRIPT_DIR}/start.sh -t runCommandEnd -d ${TESTCASE_DIR} -a ${COMMAND}  \"${ENVVARS}\"

                ${SSH_IP}${SCRIPT_DIR}/start.sh -t runCommandEnd -d ${TESTCASE_DIR} -a ${COMMAND} \"${ENVVARS}\"
                if [ $(${SSH_IP}cat ${TESTCASE_DIR}/${COMMAND}.rc) -ne 0 ]
                then
                    echolog "There was a failure in runCommandEnd ${COMMAND}. Check ${COMMAND}.log for more information"
                    result=$(( result + 1 ))
                fi
            elif [ "${TOOL}" == "runCommandLocal" ]
            then
                #set -x
                if [ $(cat ${TESTCASE_DIR}/${COMMAND}.rc) -ne 0 ]
                then
                    echolog "There was a failure in runCommandLocal ${COMMAND}. Check ${COMMAND}.log for more information"
                    result=$(( result + 1 ))
                fi
            fi

            if [ ${res} -eq 1 ] ; then
                echolog "There was a failure while searching the logs for this test. Check ${LOGFILE} for more information."
                echolog "${comp}"
            fi
            comp_idx=$(( comp_idx + 1))
        done # end of loop through componponents

        # Evaluate test results
        evaluateResults

        if [[ $result -ne 0 && "${type[${scenario_number}]}" == "EndOnFail" ]] ; then
            end_and_cleanup="true"
            echolog ">>> End and Cleanup"
        fi
    fi
    ((scenario_number+=1))

done # end loop through scenarios

stopTest
exit 0
