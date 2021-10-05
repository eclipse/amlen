#!/bin/bash
#


# Helper functions

#------------------------------------------------------------------------------
# Print formatted trace statements with timestamps
#------------------------------------------------------------------------------
function trace {
    timestamp=`date +"%Y-%m-%d %H:%M:%S"`
    echo -e "[${timestamp}] $1\n" | tee -a ${LOG}
}


#------------------------------------------------------------------------------
# Run an imaserver rest command
# $1 = Expected RC
# $2 = REST action
# $3 = URI extension  (after /ima/v1/)
# $4+ = JSON payload
#------------------------------------------------------------------------------
function run_rest_command {
    #curlcmd="curl -sS --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45"
    #curlurl="http://${A1_HOST}:${A1_PORT}/ima/v1/"
    if [[ "$2" == "PUT" ]] ; then
        curldata="-X $2 -T ${@:4} ${curlurl}$3"
    elif [[ "$2" == "DELETE" || "$2" == "GET" ]] ; then
        curldata="-X $2 ${curlurl}$3"
    else
        curldata="-X $2 -d ${@:4} ${curlurl}$3"
    fi
    cmd="${curlcmd} -f $curldata"
    trace "${cmd}"
    reply=`${cmd}  2>&1`
    result=$?
    trace "${reply}"
    trace "RC=${result}\n"

    if [[ "$1" == "x" ]] ; then
        # ignore return code
        return
    elif [ ${result} -ne $1 ] ; then
        trace "FAILED: Expected RC: $1"
        if [ ${result} -ne 0 ] ; then
            trace "Retry without the -f option"
            cmd="${curlcmd} $curldata"
            reply=`${cmd}  2>&1`
            trace "${reply}"
        fi
        # abort
        exit 1
    fi
}

#------------------------------------------------------------------------------
# Loop and check the status of the specified server.
# $1 = timeout
# $2 = status
# rc=0 on Success
#------------------------------------------------------------------------------
function statusloop {
    rc=1
    elapsed=0
    timeout=$1
    expected=$2
    # Wait for Plugin status to be Running
    run_rest_command 0 GET service/status/Plugin
    while [ ${elapsed} -lt ${timeout} ] ; do
        echo -n "*"
        sleep 1
        reply=`${cmd}`
        status=`echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Plugin\"][\"Status\"]"`
        if [[ "${status}" == "${expected}" ]] ; then
            rc=0
            break
        fi
        elapsed=`expr ${elapsed} + 1`
    done
    trace "Status:\n${reply}"
    if [ ${rc} -ne 0 ] ; then
        trace "FAILED: ${expected} RC=${rc}"
		exit 1
    fi
}

#------------------------------------------------------------------------------
# Get list of plugins installed.
#------------------------------------------------------------------------------
function getPluginsInstalled {
    run_rest_command 0 GET configuration/Plugin
    pluginsInstalled=`echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Plugin\"]"`
    if [[ "${plugins}" =~ "${expected}" ]] ; then
        pluginInstalled="true"
    fi
}


function installPlugin {
    # See if the plug-in file exists
    FILE="${M1_IMA_SDK}/ImaTools/ImaPlugin/lib/${PLUGINFILE}"
    if [ -f $FILE ] ; then
        run_rest_command 0 POST configuration {\"TraceLevel\":\"9\"}
        # Need do first "file get" on the server
        run_rest_command 0 GET configuration/Plugin
        run_rest_command 0 PUT file/${PLUGINFILE} ${FILE}
        # Then tell the server to install the plugin
        run_rest_command 0 POST configuration \{\"Plugin\":\{\"${PLUGIN_NAME}\":\{\"File\":\"${PLUGINFILE}\"}}}
        # Then need to restart the plugin service
        run_rest_command 0 POST service/restart {\"Service\":\"Plugin\"}
        trace "Waiting for restart command to take effect."
        # Plugin shoul always be running after installing a plugin
        statusloop "40" "${RUNNING}"
        # GET plugin to see if pluin was installed
        getPluginsInstalled ${PLUGIN_NAME}
        if [[ "${pluginsInstalled}" =~ "${PLUGIN_NAME}" ]] ; then
            trace "Plugin ${PLUGIN_NAME} successfully installed."
        else
            trace "Plugin ${PLUGIN_NAME} failed to install."
            exit 1
        fi
        run_rest_command 0 POST configuration {\"TraceLevel\":\"5\,mqtt:9\,plugin:9\,tcp:9\"}
    else
        trace "File $FILE does not exist."
        exit 1
    fi
}

function deletePlugin {
    run_rest_command 0 POST configuration {\"TraceLevel\":\"9\"}
    run_rest_command 0 GET configuration/Plugin
    run_rest_command 0 DELETE configuration/Plugin/${PLUGIN_NAME}
    run_rest_command 0 POST service/restart {\"Service\":\"Plugin\"}
    # GET plugin to see if pluin was uninstalled
    getPluginsInstalled 
    trace "pluginsInstalled=${pluginsInstalled}"
    if [[ "${pluginsInstalled}" =~ "${PLUGIN_NAME}" ]] ; then
        trace "Plugin ${PLUGIN_NAME} failed to uninsall."
        exit 1
    else 
        trace "Plugin ${PLUGIN_NAME} successfully uninsalled."
    fi

    # Verify Plugin is still Running if the are plugins installed otherwise Plugin should be Stopped
    if [[ "${pluginsInstalled}" == "{}" ]] ; then
        statusloop "40" "${STOPPED}"
    else 
        statusloop "40" "${RUNNING}"
    fi

    run_rest_command 0 POST configuration {\"TraceLevel\":\"5\"}
}

function updatePlugin {
    # Updating config file
    if [ -f ${FILE} ] ; then
        # "file get" new config file to the server
        run_rest_command 0 PUT file/${PLUGIN_CONFIG} ${M1_TESTROOT}/plugin_tests/${PLUGIN_CONFIG}
        run_rest_command 0 POST configuration \{\"Plugin\":\{\"${PLUGIN_NAME}\":\{\"PropertiesFile\":\"${PLUGIN_CONFIG}\"}}}
        run_rest_command 0 GET configuration/Plugin/${PLUGIN_NAME}
        propertiesFile=`echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Plugin\"][\"${PLUGIN_NAME}\"][\"PropertiesFile\"]"`
        trace "propertiesFile=${propertiesFile}"
        if [[ "${propertiesFile}" == "${PLUGIN_CONFIG}" ]] ; then
            trace "Plugin ${PLUGIN_NAME} successfully updated."
        else
            trace "Plugin ${PLUGIN_NAME} updated config property ${PLUGIN_CONFIG} not found"
            exit 1
        fi
    fi
}


function print_usage {
    echo $0 '-a <action> -f <logfile> [-p <pluginfile>] [-n <plugin_name>]'
    echo '    a )'
    echo '        action:       install, delete'
    echo '    f )'
    echo '        logfile:      Name of log file'
    echo '    p )'
    echo '        pluginfile:   Name of the plugin zip file'
    echo '    n )'
    echo '        plugin_name:  Disply name of the plug-in'
    echo '    c )'
    echo '        pluginconfig: Name of plusin config file'
    echo ' '
    echo '  Use the following to install a plug-in:'
    echo '  Ex.' $0 '-a install -f installPlugin.log -p jsonplugin.zip'
    echo ' '
    echo '  Use the following to delete a plug-in:'
    echo '  Ex.' $0 '-a delete -f deletePlugin.log -n json_msg'
    echo ' '
    echo '  Use the following to update a plug-in configuration:'
    echo '  Ex.' $0 '-a update -f updatePlugin.log -n json_msg -c json.cfg'
    echo ' '
    exit 1
}

function parse_args {
    if [[ -z "${action}" ]]; then
        print_usage
    fi

    if [[ -z "${LOG}" ]] ; then
        print_usage
    fi

    if [[ "${action}" = "install" ]]; then
        if [[ -z "${PLUGINFILE}" ]] ; then
            print_usage
        fi
    fi    

    if [[ "${action}" = "delete" ]]; then
        if [[ -z "${PLUGIN_NAME}" ]] ; then
            print_usage
        fi
    fi    
    
    if [[ "${action}" = "update" ]]; then
        if [[ -z "${PLUGIN_NAME}" ]] || [[ -z "${PLUGIN_CONFIG}" ]] ; then
            print_usage
        fi
    fi
}

#
# Main body
#
if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
    source  ../scripts/ISMsetup.sh
else
    echo "neither testEnv.sh or ISMsetup.sh was used"
fi

while getopts "a:f:p:n:c:" opt; do
    case $opt in
    a )
        action=${OPTARG}
        ;;
    f )
        LOG=${OPTARG}
        ;;
    p )
        PLUGINFILE=${OPTARG}
        ;;
    n )
        PLUGIN_NAME=${OPTARG}
        ;;
    c )
        PLUGIN_CONFIG=${OPTARG}
        ;;
    * )
        exit 1
        ;;
    esac
done



    # Status messages version 2.0+
    RUNNING="Active"
    STOPPED="Inactive"
    PROP_FILE='"PropertiesFile": "jsonplugin.json"'
    curlcmd="curl -sS --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45"
    curlurl="http://${A1_HOST}:${A1_PORT}/ima/v1/"


    if [[ "${action}" = "install" ]]; then
    	installPlugin
    elif [[ "${action}" = "delete" ]]; then
    	deletePlugin
    elif [[ "${action}" = "update" ]]; then
        updatePlugin
    else
    	print_usage
    fi

    trace "${action} completed successfully"
