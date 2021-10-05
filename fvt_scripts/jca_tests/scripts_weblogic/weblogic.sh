#!/bin/bash
#------------------------------------------------------------------------------
#
# weblogic.sh
# 
# Description:
#   This script is intended to be used to configure WebLogic application server
#   for automation.
#   This includes creating the J2C objects, deploying the applications, 
#   and resetting the server configuration.
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
# # Scenario 0 - setup weblogic
# #------------------------------------------------------
# xml[${n}]="scenario0"
# timeouts[${n}]=400
# scenario[${n}]="${xml[${n}]} - Configure WebLogic"
# component[1]=cAppDriverLog,m1,"-e|./weblogic.sh,-o|-a|setup"
# components[${n}]="${component[1]}"
# ((n+=1))
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Configure the Resource Adapter in WebLogic Application Server
#------------------------------------------------------------------------------
function configure_weblogic {
    # scp was.py, test.config, resource adapter and application to server
    echo "mkdir /WLS/" | ssh ${WLSAddress}
    
    # The app expects tests.properties to be in /WAS :(
    echo "mkdir /WAS/" | ssh ${WLSAddress}
    scp tests.properties ${WLSAddress}:/WAS/
    
    scp scripts_weblogic/*.py ${WLSAddress}:/WLS/
    scp config/*.config ${WLSAddress}:/WLS/
    scp ../lib/testTools_JCAtests.ear ${WLSAddress}:/WLS/

    scp ../lib/JMSTestDriver.jar ${WLSAddress}:/WLS/
    scp ${M1_IMA_SDK}/lib/ima.jmsra.rar ${WLSAddress}:/WLS/
    scp ../lib/ima.evilra.rar ${WLSAddress}:/WLS/

    echo "${WASPath}/common/bin/wlst.sh /WLS/wlst.py /WLS/test.config setup" | ssh ${WLSAddress}
    
    # Need to restart weblogic after configuring the resource adapter so that everything
    # is available in JNDI
    echo "${WASPath}/../user_projects/domains/base_domain/bin/stopWebLogic.sh < /dev/null &> /dev/null &" | ssh ${WLSAddress}

    # Wait a bit and make sure the server was stopped.
    # If it wasn't stopped after a minute, just kill the process.
    sleep 20
    pid=`ssh ${WLSAddress} ps -ef | grep weblogic | awk '{ print $2 }'`
    echo "WebLogic PID before kill: ${pid}"
    if [[ "${pid}" != "" ]] ; then
        ssh ${WLSAddress} kill -9 $pid
        result=`ssh ${WLSAddress} ps -ef | grep weblogic | awk '{ print $2 }'`
        echo "WebLogic PID after kill: ${result}"
    fi

    # Restart the server
    echo "/opt/Oracle/Middleware/user_projects/domains/base_domain/startWebLogic.sh < /dev/null &> /dev/null &" | ssh ${WLSAddress}

    # Give it some time to get up and running
    sleep 30
}

#------------------------------------------------------------------------------
# Cleanup the Resource Adapter configuration in WebLogic Application Server
#------------------------------------------------------------------------------
function cleanup_weblogic {
    echo "empty function"
    echo "${WASPath}/common/bin/wlst.sh /WLS/wlst.py /WLS/test.config cleanup" | ssh ${WLSAddress}
    
    # remove files that were scp'd to the application server host machine
    echo "rm -f /WLS/*.py /WLS/*.config /WLS/*.ear /WLS/*.jar /WAS/tests.properties *.xml ra.trace" | ssh ${WLSAddress}

    echo "rm -f /WLS/JMSTestDriver.jar" | ssh ${WLSAddress}
    
    sed -i -e s/${hostname}/M1_NAME/g tests.properties
    sed -i -e s/${A1_IPv4_1}/A1_IPv4_1/g config/test.config
    sed -i -e s/${A2_IPv4_1}/A2_IPv4_1/g config/test.config
}

#------------------------------------------------------------------------------
# Install enterprise application to WebLogic server
#------------------------------------------------------------------------------
function install_app {
    echo -e "\nInstalling app to $1 Application Server" >> $LOG
    echo "${WASPath}/common/bin/wlst.sh /WLS/install_app.py /WLS/test.config setup" | ssh ${WLSAddress}
}

#------------------------------------------------------------------------------
# Uninstall enterprise application from WebLogic Server
#------------------------------------------------------------------------------
function uninstall_app {
    echo -e "\nUninstalling app from $1 Application Server" >> $LOG
    echo "${WASPath}/common/bin/wlst.sh /WLS/install_app.py /WLS/test.config cleanup" | ssh ${WLSAddress}
}

function print_usage {
    echo './was -a <action> -f <logfile>'
    echo '    a )'
    echo '        action:       prepare, setup, cleanup, install, uninstall'
    echo '    f )'
    echo '        logfile:      Name of log file'
    echo ' '
    echo '  Use the following to setup WebLogic Application Server'
    echo '  Ex. ./weblogic.sh -a setup -f setup.log'
    exit 1
}

function parse_args {
    if [[ -z "${action}" ]]; then
        print_usage
    fi
    if [[ -z "$LOG" ]] ; then
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
        LOG=${OPTARG}
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

echo -e "Entering $0 with $# arguments: $@" >> $LOG

WLSAddress=`echo ${WASIP} | cut -d':' -f 1`
WLSPort=`echo ${WASIP} | cut -d':' -f 2`

hostname=${MQKEY}

total_fails=0
rc=0

ostype=`ssh root@${WLSAddress} uname`
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
	cp tests.uninitialized.properties tests.properties
    sed -i -e s/M1_NAME/${hostname}/g tests.properties
    sed -i -e s/A1_IPv4_1/${A1_IPv4_1}/g config/test.config
    sed -i -e s/A2_IPv4_1/${A2_IPv4_1}/g config/test.config
    sed -i -e s/APPSERVER/WLS/g tests.properties

    # Unpack the test application and replace the server values.
    unzip ../lib/testTools_JCAtests.ear -d ear/
    unzip ear/testTools_JCAtests.jar -d jar/
    rm ear/testTools_JCAtests.jar
    rm ../lib/testTools_JCAtests.ear
    cp config/ejb-jar.xml jar/META-INF/.
    sed -i -e s/A1_IPv4_1/${A1_IPv4_1}/g jar/META-INF/ejb-jar.xml
    sed -i -e s/A2_IPv4_1/${A2_IPv4_1}/g jar/META-INF/ejb-jar.xml
    cd jar/
    zip -r ../ear/testTools_JCAtests.jar *
    cd ../ear/
    zip -r ../../lib/testTools_JCAtests.ear *
    cd ../
    chmod +x ../lib/testTools_JCAtests.ear
    rm -rf jar/ ear/
    
    # Use /WAS directory for env file as its hardcoded in the test application
    echo "echo ${A1_HOST} > /WAS/env.file" | ssh ${WLSAddress}
    echo "echo ${A2_HOST} >> /WAS/env.file" | ssh ${WLSAddress}

    # Sleep because this goes too fast for run-scenarios to get the process ID
    sleep 4
    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Configure WebLogic
#------------------------------------------------------------------------------
elif [[ "${action}" = "setup" ]] ; then
    echo -e "\nSetting up WebLogic Server" >> $LOG
    configure_weblogic

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Install Application
#------------------------------------------------------------------------------
elif [[ "${action}" = "install" ]] ; then
    install_app

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Uninstall Application
#------------------------------------------------------------------------------
elif [[ "${action}" = "uninstall" ]] ; then
    uninstall_app

    if [[ $rc -gt 0 ]] ; then
        let total_fails+=1
    fi

#------------------------------------------------------------------------------
# Clean up WebLogic
#------------------------------------------------------------------------------
elif [[ "${action}" = "cleanup" ]] ; then
    echo -e "\nCleaning up WebLogic Server" >> $LOG
    cleanup_weblogic

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
    echo -e "\nweblogic.sh Test result is Failure! Number of failures: ${total_fails}" >> $LOG
else
    echo -e "\nweblogic.sh Test result is Success!" >> $LOG
fi
