#!/bin/bash
######################################################
###              JBOSS w/ IMA FVT                  ###
######################################################
#
# jboss.sh
#
# Description:
#   This script is intended to be used to configure JBOSS Application Server 7.x
#   for use in IBM MessageSight FVT automation.
#
# Input:
#  $1 [required] -a <action>
#  $2 [required] -f <logfile>
#
# Return:
#   0 - success
#   1 - failure


function print_usage {
	echo './jboss.sh -a <action> -f <logfile> -r <resource-adapter> -e <app>' 1>&2 | tee -a $LOG
	echo '   a)' 1>&2 | tee -a $LOG
	echo '     action: installapp, installra, setup, cleanup' 1>&2 | tee -a $LOG
	echo '   f)' 1>&2 | tee -a $LOG
	echo '     logfile: Name of log file' 1>&2 | tee -a $LOG
	echo ' ' 1>&2 | tee -a $LOG
	echo 'example:   ./jboss.sh -a setup -f /tmp/setup.log' 1>&2 | tee -a $LOG
    echo 'example:   ./jboss.sh -a installra -f /tmp/ra.log -r /tmp/ima.jmsra.rar' 1>&2 | tee -a $LOG
    echo 'example:   ./jboss.sh -a installapp -f /tmp/app.log -e /tmp/mycoolapp.ear' 1>&2 | tee -a $LOG
	exit 1
}

function check_args {
	if [[ -z "${action}" ]] ; then
		print_usage
	fi
	if [[ -z "$LOG" ]] ; then
		print_usage
	fi
	if [[ ${IP} = "" ]] ; then
		echo -e "\nIP (WASIP) must be defined in ISMsetup or testEnv"
	fi
	if [[ ${DIR} = "" ]] ; then
		echo -e "\nDIR (WASPath) must be defined  in ISMsetup or testEnv"
	fi
}

function install_ra {
    if [[ -z "${RA}" ]] ; then
        echo "Error: Resource Adapter was not specified"
        print_usage
    fi

    RAname=`echo ${RA} | sed 's/\//\ /g' | awk '{print $NF}'`
    echo "Installing the RA ${RAname}"

    # Delete any existing
    #          [ra]
    #          [ra].dodeploy
    #          [ra].deployed
    #          [ra].deploying
    #          [ra].failed
    #          [ra].skipdeploy
    #          [ra].isundeploying
    #          [ra].undeployed
    #          [ra].pending

    echo "rm -f ${DIR}/standalone/deployments/${RAname}*" | ssh ${IP}
    scp "${RA}" "${IP}:/${DIR}/standalone/deployments/."
    echo "touch ${DIR}/standalone/deployments/${RAname}.dodeploy" | ssh ${IP}
}

function install_app {
    if [[ -z "${APP}" ]] ; then
        echo "Error: J2E App was not specified"
        print_usage
    fi

    APPname=`echo ${APP} | sed 's/\//\ /g' | awk '{print $NF}'`
    echo "Installing the APP ${APPname}"

    # Delete any existing
    #          [app]
    #          [app].dodeploy
    #          [app].deployed
    #          [app].deploying
    #          [app].failed
    #          [app].skipdeploy
    #          [app].isundeploying
    #          [app].undeployed
    #          [app].pending

    echo "rm -f ${DIR}/standalone/deployments/${APPname}*" | ssh ${IP}
    scp "${APP}" "${IP}:/${DIR}/standalone/deployments/."
    echo "touch ${DIR}/standalone/deployments/${APPname}.dodeploy" | ssh ${IP}
}


############################################################
###                    "main"                            ###
############################################################

#set -x
while getopts "a:f:r:e:" opt; do
	case $opt in
	a )
		action=${OPTARG}
		;;
	f )
		LOG=${OPTARG}
        echo "jboss.sh begin" > $LOG
		;;
    r )
        RA=${OPTARG}
        ;;
    e )
        APP=${OPTARG}
        ;;
	* )
		print_usage
		exit 1
		;;
	esac
done

if [[ -f "../testEnv.sh" ]] ; then
	source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
	source ../scripts/ISMsetup.sh
else
	echo "Warning: No ../testEnv.sh or ../scripts/ISMsetup.sh found"
fi

check_args

echo -e "Entering action $0 with $# arguments: $@" | tee -a $LOG

IP=`echo ${WASIP} | cut -d: -f1`
PORT=`echo ${WASIP} |  cut -d : -f2`
HOST=${MQKEY}
DIR=$WASPath

echo "log: $LOG"

echo "DIR:         $DIR"  | tee -a $LOG
echo "IP:          $IP"   | tee -a $LOG
echo "PORT:        $PORT" | tee -a $LOG
echo "HOST:        $HOST" | tee -a $LOG
echo "APP:         $APP"  | tee -a $LOG
echo "RA:          $RA"   | tee -a $LOG

if [[ "${action}" =~ "setup" ]] ; then
    echo "doing setup"
elif [[ "${action}" =~ "teardown" ]] ; then
    echo "doing teardown"
elif [[ "${action}" =~ "installra" ]] ; then
    install_ra
elif [[ "${action}" =~ "installapp" ]] ; then
    install_app
else
    echo "Error: action '${action}' not supported"
    print_usage
fi

rc=0
