#!/bin/bash
#------------------------------------------------------------------------------
#
# was.sh
# 
# Description:
#   This script is intended to be used to configure Websphere Application
#   Server for automation.
#   This includes creating the J2C objects, deploying the applications, 
#   and resetting the server configuration.
#   It will eventually support this for all the different versions of WAS
#   that we will be testing in our automation environment (hopefully).
#
# Input:
#   $1 - required: -a action
#        Available actions include:
#          prepare
#          setup
#          cleanup
#
#   $2 - required: -f logfile
#
# Return Code:
#   0 - Pass
#   1 - Fail
#
#------------------------------------------------------------------------------
#
# Sample run-scenarios input file:
# #------------------------------------------------------
# # Scenario 0 - setup was profile
# #------------------------------------------------------
# xml[${n}]="scenario0"
# timeouts[${n}]=400
# scenario[${n}]="${xml[${n}]} - Configure WAS Profile"
# component[1]=cAppDriverLog,m1,"-e|./was.sh,-o|-a|setup"
# components[${n}]="${component[1]}"
# ((n+=1))
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# If type = was85 or type = was80
#   1. Make /WAS directory on WAS machine if it doesn't already exist
#   2. Send jython scripts, props file and app to WAS machine
#   3. Run the jython setup script
#------------------------------------------------------------------------------
function configure_was {
    echo -e "\nSetting up $1 Application Server" >> $WAS_LOG_FILE
    
    # scp was.py, test.config, resource adapter and application to server
    echo "mkdir /WAS/" | ssh ${WASAddress}
    scp scripts_was/*.py ${WASAddress}:/WAS/
    scp config/* ${WASAddress}:/WAS/
    scp tests.properties ${WASAddress}:/WAS/
    scp ../lib/testTools_JCAtests.ear ${WASAddress}:/WAS/

    # ... scp JMSTestDriver.jar to each node in cluster, if we might be in one?
    scp ${WASAddress}:/WAS/server.definition ./
    srvdefinition=`cat server.definition`
    if [[ "${srvdefinition}" =~ "SINGLE" ]] ; then
        echo "Not in a cluster" >> $WAS_LOG_FILE
        scp ../lib/JMSTestDriver.jar ${WASAddress}:/WAS/
        # Copy A1_HOST and A2_HOST to /WAS
        echo "echo ${A1_HOST} > /WAS/env.file" | ssh ${WASAddress}
        echo "echo ${A2_HOST} >> /WAS/env.file" | ssh ${WASAddress}
    else
        cluster=`echo $srvdefinition | cut -d';' -f 2-`
        IFS=$';' read -a array <<< $cluster
        for node in ${array} ; do
            scp ../lib/JMSTestDriver.jar ${node}:/WAS/
            # Copy A1_HOST and A2_HOST to /WAS
    		echo "echo ${A1_HOST} > /WAS/env.file" | ssh ${node}
    		echo "echo ${A2_HOST} >> /WAS/env.file" | ssh ${node}
        done
    fi
    
    # Run the jython script to configure the server and install the application
    # based on configuration in test.config
    echo "${WASPath}/bin/wsadmin${suffix} -lang jython -user admin -password admin -f ${CYGPATH}/WAS/was.py ${CYGPATH}/WAS/test.config setup 1" | ssh ${WASAddress}
}

#------------------------------------------------------------------------------
# Install enterprise application to WAS server
#------------------------------------------------------------------------------
function install_app_was {
    echo -e "\nInstalling app to $1 Application Server" >> $WAS_LOG_FILE
    echo "${WASPath}/bin/wsadmin${suffix} -lang jython -user admin -password admin -f ${CYGPATH}/WAS/install_app.py ${CYGPATH}/WAS/test.config setup 1" | ssh ${WASAddress}
}

#------------------------------------------------------------------------------
# Uninstall enterprise application from WAS server
#------------------------------------------------------------------------------
function uninstall_app_was {
    echo -e "\nUninstalling app from $1 Application Server" >> $WAS_LOG_FILE
    echo "${WASPath}/bin/wsadmin${suffix} -lang jython -user admin -password admin -f ${CYGPATH}/WAS/install_app.py ${CYGPATH}/WAS/test.config cleanup 1" | ssh ${WASAddress}
}

#------------------------------------------------------------------------------
# If type = was85 or type = was80
#   1. Run the jython cleanup script
#   2. Remove files that we sent to the WAS machine during setup
#------------------------------------------------------------------------------
function cleanup_was {
    echo -e "\nCleaning up $1 Application Server" >> $WAS_LOG_FILE
    # Run wsadmin script to cleanup application server configuration
    echo "${WASPath}/bin/wsadmin${suffix} -lang jython -user admin -password admin -f ${CYGPATH}/WAS/was.py ${CYGPATH}/WAS/test.config cleanup 1" | ssh ${WASAddress}
    # remove files that were scp'd to the application server host machine
    echo "rm -f /WAS/test.config" | ssh ${WASAddress}
    echo "rm -f /WAS/was.py" | ssh ${WASAddress}
    echo "rm -f /WAS/wsadminlib.py" | ssh ${WASAddress}
    echo "rm -f /WAS/install_app.py" | ssh ${WASAddress}
    echo "rm -f /WAS/testTools_JCAtests.ear" | ssh ${WASAddress}
    echo "rm -f /WAS/JMSTestDriver.jar" | ssh ${WASAddress}
    srvdefinition=`cat server.definition`
    if [[ "${srvdefinition}" =~ "SINGLE" ]] ; then
        echo "Not in a cluster" >> $WAS_LOG_FILE
        echo "rm -f /WAS/JMSTestDriver.jar" | ssh ${WASAddress}
    else
        cluster=`echo $srvdefinition | cut -d';' -f 2-`
        IFS=$';' read -a array <<< $cluster
        for node in ${array} ; do
            echo "rm -f /WAS/JMSTestDriver.jar" | ssh ${node}
        done
    fi
    sed -i -e s/${hostname}/M1_NAME/g setup/*.xml
    sed -i -e s/${hostname}/M1_NAME/g config/test.config
    sed -i -e s/${hostname}/M1_NAME/g tests.properties
    sed -i -e s/${A1_IPv4_1}/A1_IPv4_1/g config/test.config
}

#------------------------------------------------------------------------------
# If type = liberty:
#   1. scp resource adapter to AppServer
#   2. stop liberty profile
#   3. scp server.xml to AppServer
#   4. scp .ear file to AppServer
#   5. start AppServer
#------------------------------------------------------------------------------
function configure_liberty {
    echo -r "\nNot testing liberty just yet... Sorry" >> $WAS_LOG_FILE
    #echo -e "\nSetting up Liberty Profile V8.5.5 Application server" >> $WAS_LOG_FILE
    #sed -i -e s/M1_NAME/${hostname}/g ../common/wlp-server.xml
    # scp resource adapter
    #scp IMA_RA.rar root@${WASAddress}:${WASPath}/../../../
    # make sure liberty is stopped
    #echo "${WASPath}/../../../bin/server stop LIBERTY01" | ssh root@${WASAddress}
    # scp server.xml - variables should be replaced or we need to edit it?
    #scp ../common/wlp-server.xml root@${WASAddress}:${WASPath}/server.xml
    # scp .ear file
    #scp ../lib/testTools_JCAtests.ear root@${WASAddress}:${WASPath}/apps/
    # start server
    #echo "${WASPath}/../../../bin/server start LIBERTY01" | ssh root@${WASAddress}
}

#------------------------------------------------------------------------------
# If type = liberty
#------------------------------------------------------------------------------
function cleanup_liberty {
    echo -r "\nNot testing liberty just yet... Sorry" >> $WAS_LOG_FILE
    #echo -e "\nCleaning up Liberty Profile V8.5.5 Application server" >> $WAS_LOG_FILE
    # make sure liberty is stopped
    #echo "${WASPath}/../../../bin/server stop LIBERTY01" | ssh root@${WASAddress}
    # remove application
    #echo "rm -rf ${WASPath}/apps/testTools_JCAtests.ear" | ssh root@${WASAddress}
    # reset server.xml
    #scp ../common/wlp-server-basic.xml root@${WASAddress}:${WASPath}/server.xml
    # remove resource adapter
    #echo "rm -rf ${WASPath}/../../../IMA_RA.rar" | ssh root@${WASAddress}
    # start server
    #echo "${WASPath}/../../../bin/server start LIBERTY01" | ssh root@${WASAddress}
}

function print_usage {
    echo './was -a <action> -f <logfile>'
    echo '    a )'
    echo '        action:       prepare, setup, cleanup, install, uninstall'
    echo '    f )'
    echo '        logfile:      Name of log file'
    echo ' '
    echo '  Use the following to setup a was profile'
    echo '  Ex. ./was -a setup -f setup.log'
    exit 1
}

function parse_args {
    if [[ -z "${action}" ]]; then
        print_usage
    fi
    if [[ -z "$WAS_LOG_FILE" ]] ; then
        print_usage
    fi
    if [[ ${WASIP} = "" ]] ; then
        echo -e "\nWASIP must be defined in ISMsetup or testEnv"
        print_usage
    fi
    if [[ ${WASPath} = "" ]] ; then
        echo -e "\nWASPath must be defined in ISMsetup or testEnv"
        print_usage
    fi
    if [[ ${WASType} = "" ]] ; then
        echo -e "\nWASType must be defined in ISMsetup or testEnv"
        print_usage
    fi
}

#set -x
while getopts "a:f:" opt; do
    case $opt in
    a )
        action=${OPTARG}
        ;;
    f )
        WAS_LOG_FILE=${OPTARG}
        ;;
    * )
        exit 1
        ;;
    esac
done

if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

parse_args

echo -e "Entering $0 with $# arguments: $@" >> $WAS_LOG_FILE

n=0
for ip in $(echo ${WASClusterIP} | tr "," "\n")
do
    WASClusterIPs[$n]=$ip
    let n+=1
done

WASAddress=`echo ${WASIP} | cut -d':' -f 1`
WASPort=`echo ${WASIP} | cut -d':' -f 2`

hostname=${MQKEY}

total_fails=0
rc=0

ostype=`ssh root@${WASAddress} uname`
if [[ "${ostype}" =~ "CYGWIN" ]] ; then
	suffix=".bat"
    CYGPATH="C:/cygwin"
else
	suffix=".sh"
    CYGPATH=""
fi

#------------------------------------------------------------------------------
# Perform variable replacement
# This needs to be done before anything can be configured
#------------------------------------------------------------------------------
if [[ "${action}" = "prepare" ]] ; then
    sed -i -e s/M1_NAME/${hostname}/g config/test.config
    cp tests.uninitialized.properties tests.properties
    sed -i -e s/M1_NAME/${hostname}/g tests.properties
    sed -i -e s/APPSERVER/WAS/g tests.properties
    cp tests.properties tests.properties.replaced
    sed -i -e s/A1_IPv4_1/${A1_IPv4_1}/g config/test.config
    sed -i -e s/A2_IPv4_1/${A2_IPv4_1}/g config/test.config

    # Sleep because this goes too fast for run-scenarios to get the process ID
    sleep 4
    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Configure Application Server environment
#------------------------------------------------------------------------------
elif [[ "${action}" = "setup" ]] ; then
    if [[ "${WASType}" = "was85" ]] ; then
        configure_was "WAS V8.5.5"
    elif [[ "${WASType}" = "was80" ]] ; then
        configure_was "WAS V8.0"
    elif [[ "${WASType}" = "liberty" ]] ; then
        configure_liberty
    elif [[ "${WASType}" = "wasce" ]] ; then
        echo -e "\nSetting up WAS Community Edition Application server" >> $WAS_LOG_FILE
    fi

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Install Application
#------------------------------------------------------------------------------
elif [[ "${action}" = "install" ]] ; then
    if [[ "${WASType}" = "was85" ]] ; then
        install_app_was "WAS V8.5.5"
    elif [[ "${WASType}" = "was80" ]] ; then
        install_app_was "WAS V8.0"
    fi

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Uninstall Application
#------------------------------------------------------------------------------
elif [[ "${action}" = "uninstall" ]] ; then
    if [[ "${WASType}" = "was85" ]] ; then
        uninstall_app_was "WAS V8.5.5"
    elif [[ "${WASType}" = "was80" ]] ; then
        uninstall_app_was "WAS V8.0"
    fi

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Clean up Application Server environment
#------------------------------------------------------------------------------
elif [[ "${action}" = "cleanup" ]] ; then
    if [[ "${WASType}" = "was85" ]] ; then
        cleanup_was "WAS V8.5.5"
    elif [[ "${WASType}" = "was80" ]] ; then
        cleanup_was "WAS V8.0"
    elif [[ "${WASType}" = "liberty" ]] ; then
        cleanup_liberty
    elif [[ "${WASType}" = "wasce" ]] ; then
        echo -e "\nCleaning up WAS Community Edition Application server" >> $WAS_LOG_FILE
    fi

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Invalid action entered
#------------------------------------------------------------------------------
else
    print_usage
fi

if [ ${total_fails} -gt 0 ] ; then
    echo -e "\nwas.sh Test result is Failure! Number of failures: ${total_fails}" >> $WAS_LOG_FILE
else
    echo -e "\nwas.sh Test result is Success!" >> $WAS_LOG_FILE
fi
