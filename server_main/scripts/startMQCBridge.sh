#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

#set -x

DATE=`date`
echo "------------------- $DATE -------------------"

exec 200> /tmp/mqcbridge.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "MQ Connectivity process is already running" >&2
    exit 255
fi

CONFIG_FOLDER="${IMA_SVR_DATA_PATH}/data/config"
INSTALL_FOLDER="${IMA_SVR_INSTALL_PATH}"
LOG_FOLDER="${IMA_SVR_DATA_PATH}/diag/logs"
LOG_LEVEL="-5"
TRACE_FILE="mqctrace.log"
CFG_FILE="server.cfg"
TASKSET=""
while getopts ":c:i:l:L:t:s:" opt; do
#    if [[ $OPTARG =~ ^- ]]; then
#    
#    fi 
     case $opt in
        c)
            if [ -d "$OPTARG" ]; then
                CONFIG_FOLDER=`readlink -f $OPTARG`
            else
                echo "Configuration folder does not exist: $OPTARG" >&2
                exit 255
            fi
            ;;
        i)
            if [ -d "$OPTARG" ]; then
                INSTALL_FOLDER=`readlink -f $OPTARG`
            else
                echo "Installation folder does not exist: $OPTARG" >&2
                exit 255
            fi
            ;;
        l)
            LOG_LEVEL="-$OPTARG"
            ;;
        t)
            TRACE_FILE="$OPTARG"
            ;;
        s)
            TASKSET="taskset -c $OPTARG"
            ;;
        L)
            if [ -d "$OPTARG" ]; then
                LOG_FOLDER=`readlink -f $OPTARG`
            else
                echo "Log folder does not exist: $OPTARG" >&2
                exit 255
            fi
            ;;
        :)
            echo "Option -$OPTARG requires argument." >&2
            exit 255
            ;;
       \?)
            echo "Unknown option: -$OPTARG" >&2
            ;;
    esac
done

MQCINSTALLLOG="$LOG_FOLDER/mqclient_install.log"
reinstmqc=0
MQ_CONFIG_FOLDER="$CONFIG_FOLDER/mqm"

# Set environment variables that MQConnectivity will need
PATH=$INSTALL_FOLDER/mqclient/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH
export PATH
MQ_OVERRIDE_DATA_PATH=${IMA_SVR_DATA_PATH}/diag/mqclient
export MQ_OVERRIDE_DATA_PATH
MQ_INSTALLATION_PATH=$INSTALL_FOLDER/mqclient
export MQ_INSTALLATION_PATH

# Make sure the data path we are specifying exists
if [ ! -d $MQ_OVERRIDE_DATA_PATH ]
then
    echo Creating MQ data path $MQ_OVERRIDE_DATA_PATH >>$MQCINSTALLLOG 2>&1
    mkdir -p -m 770 $MQ_OVERRIDE_DATA_PATH >>$MQCINSTALLLOG 2>&1
fi

# Check for and create our mqclient.ini if one was not specified for us
if [ "${MQCLNTCF}" == "" ]
then
    MQ_INI_FILE="$MQ_CONFIG_FOLDER/mqclient.ini"

    if [ ! -f $MQ_INI_FILE ]
    then
        echo Creating $MQ_INI_FILE >>$MQCINSTALLLOG 2>&1
        mkdir -p -m 770 $MQ_CONFIG_FOLDER >> $MQCINSTALLLOG 2>&1
        cp $INSTALL_FOLDER/config/mqclient.ini $MQ_INI_FILE >> $MQCINSTALLLOG 2>&1
        rc=$?
        if [ $rc -ne 0 ] 
        then
            exit $rc
        fi
        sed -i 's%/var/mqm/\(.*\)%'$MQ_OVERRIDE_DATA_PATH'/\1%g' $MQ_INI_FILE
    fi

    export MQCLNTCF=$MQ_INI_FILE
fi

#mqclient gets very upset if $HOME is not specified... if it's not set, set it to
#whatever MQ_OVERRIDE_DATA_PATH is set to - whether or not it is already set as we
#don't want the MQ client using HOME for anything.
export HOME=${MQ_OVERRIDE_DATA_PATH}

#echo "$TASKSET $INSTALL_FOLDER/bin/mqcbridge -d -t $LOG_FOLDER/$TRACE_FILE $LOG_LEVEL $CONFIG_FOLDER/$CFG_FILE"
touch $LOG_FOLDER/mqcStartup.log > /dev/null 2>&1
$TASKSET $INSTALL_FOLDER/bin/mqcbridge -d -t $LOG_FOLDER/$TRACE_FILE $LOG_LEVEL $CONFIG_FOLDER/$CFG_FILE &
bpid=$!
echo "MQ Connectivity process created: Date=`date`   PID=$bpid"
wait $bpid  

rc=$?

${IMA_SVR_INSTALL_PATH}/bin/extractstackfromcore.sh
    
exit $rc

         
