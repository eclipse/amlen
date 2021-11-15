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


exec 200> /tmp/imaplugin.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "IMA plugin process is already running" >&2
    exit 255
fi

CONFIG_FOLDER=${IMA_SVR_DATA_PATH}/data/config
MAXHEAP="512"
VMARG=""
DBG_ENDPOINT=""
LOCAL_IP=""
LOCAL_PORT=""
INSTALL_FOLDER="${IMA_SVR_INSTALL_PATH}"
LOG_FOLDER="${IMA_SVR_DATA_PATH}/diag/logs"
TASKSET=""
while getopts ":c:x:v:d:a:p:i:l:s:" opt; do
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
            if [ -d "$OPTARG" ]; then
                LOG_FOLDER=`readlink -f $OPTARG`
            else
                echo "Log folder does not exist: $OPTARG" >&2
                exit 255
            fi
            ;;
        s)
            TASKSET="taskset -c $OPTARG"
            ;;
        x)
            MAX_HEAP="$OPTARG"
            ;;
        v)
            VMARG="$OPTARG"
            ;;
        d)
            DBG_ENDPOINT="$OPTARG"
            ;;
        a)
            LOCAL_IP="$OPTARG"
            ;;
        p)
            LOCAL_PORT="$OPTARG"
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


JAR_FILE=$INSTALL_FOLDER/jars/imaPlugin.jar
BOOT_CP=-Xbootclasspath/p:$JAR_FILE
DUMP_FOLDER="$LOG_FOLDER/plugin"
mkdir -p $DUMP_FOLDER
DUMP_PARAM=-Xdump:java:defaults:file=$DUMP_FOLDER/%pid/javacore-%seq.txt
SM=-Djava.security.manager=default
MS=-Xms64m
MX="-Xmx${MAXHEAP}m"
OUT_LOG="$LOG_FOLDER/imapluginout.log"
ERR_LOG="$LOG_FOLDER/imapluginerr.log"

DEBUG_OPT=""
if [ "$DBG_ENDPOINT" != "" ]; then
   DEBUG_OPT="-Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=$DBG_ENDPOINT" 
fi 

IP_OPT=""
PORT_OPT=""
if [ "$LOCAL_IP" != "" ]; then
     IP_OPT="-i $LOCAL_IP"
fi 

if [ "$LOCAL_PORT" != "" ]; then
     PORT_OPT="-p $LOCAL_PORT"
fi 
export IMA_CONFIG_DIR=$CONFIG_FOLDER/

# Use Java if we shipped it
if [ -d "${IMA_SVR_INSTALL_PATH}/ibm-java-x86_64-80" ]
then
    export JAVA_HOME="${IMA_SVR_INSTALL_PATH}/ibm-java-x86_64-80"
    export JAVA=${IMA_SVR_INSTALL_PATH}/ibm-java-x86_64-80/jre/bin/java
fi

#echo "$TASKSET $JAVA $SM $DUMP_PARAM $BOOT_CP $MS $MX  $DEBUG_OPT $VMARG -jar $JAR_FILE -d $IP_OPT $PORT_OPT >$OUT_LOG 2>$ERR_LOG"
$TASKSET $JAVA $SM $DUMP_PARAM $BOOT_CP $MS $MX $DEBUG_OPT $VMARG -jar $JAR_FILE -d $IP_OPT $PORT_OPT >$OUT_LOG 2>$ERR_LOG &
jpid=$!
echo "Plugin process created with pid=$jpid"
wait $jpid  
rc=$?
#${IMA_SVR_INSTALL_PATH}/bin/compressjavacores.sh
   
exit $rc

         
