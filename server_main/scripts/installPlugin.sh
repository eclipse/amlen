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

LOGFILE=${IMA_SVR_DATA_PATH}/diag/logs/installPlugins.log
touch ${LOGFILE}

DATE=`date`

echo "" >> ${LOGFILE} 2>&1 3>&1
echo "------------------------------------------" >> ${LOGFILE} 2>&1 3>&1
echo "Plugin install: Date: $DATE " >> ${LOGFILE} 2>&1 3>&1
echo "" >> ${LOGFILE} 2>&1 3>&1


JAVA=${IMA_SVR_INSTALL_PATH}/ibm-java-x86_64-80/jre/bin/java
$JAVA -version >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [ "$rc" != "0" ]; then
   echo "Unable to invoke java command"
   exit 255
fi


exec 200> /tmp/imapluginInstall.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "Plugin install is already in process" >&2
    exit 255
fi

CONFIG_FOLDER=${IMA_SVR_DATA_PATH}/data/config
ZIP_FILE=""
PROPS_FILE=""
NAME=""
ACTION="test"
OVERWRITE=""
MAXHEAP="512"
VMARG=""
DBG_ENDPOINT=""
INSTALL_FOLDER="${IMA_SVR_INSTALL_PATH}"
while getopts ":z:n:p:x:v:d:c:I:toirj" opt; do
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
        I)
            if [ -d "$OPTARG" ]; then
                INSTALL_FOLDER=`readlink -f $OPTARG`
            else
                echo "Installation folder does not exist: $OPTARG" >&2
                exit 255
            fi
            ;;
        z)
            ZIP_FILE="$OPTARG"
            ;;
        p)
            PROPS_FILE="$OPTARG"
            ;;
        n)
            NAME="$OPTARG"
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
        o)
            OVERWRITE="true"
            ;;
        i)
            ACTION="Install"
            ;;
        r)
            ACTION="Remove"
            ;;
        t)
            ACTION="test"
            ;;
        :)
            echo "Option -$OPTARG requires argument." >&2
            exit 255
            ;;    
		\?)
			echo "Unknown option: -$OPTARG" >&2
			exit 255
			;;
	esac			
		
		
done


JAR_FILE=$INSTALL_FOLDER/jars/imaPlugin.jar
BOOT_CP=-Xbootclasspath/p:$JAR_FILE
DUMP_FOLDER="$LOG_FOLDER/plugin"
DUMP_PARAM=-Xdump:java:defaults:file=$DUMP_FOLDER/%pid/javacore-%seq.txt
SM=-Djava.security.manager=default
MS=-Xms64m
MX="-Xmx${MAXHEAP}m"
DEBUG_OPT=""
if [ "$DBG_ENDPOINT" != "" ]; then
   DEBUG_OPT="-Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=$DBG_ENDPOINT" 
fi 

OPT1=""
OPT2=""
OPT3=""
OPT4=""
OPT5=""
if [ "$ZIP_FILE" != "" ]; then
     OPT1="Zip=$ZIP_FILE"
fi 

if [ "$PROPS_FILE" != "" ]; then
     OPT2="propertiesFile=$PROPS_FILE"
fi 

if [ "$NAME" != "" ]; then
     OPT3="Name=$NAME"
fi 

if [ "$OVERWRITE" != "" ]; then
     OPT4="allowOverwrite=$OVERWRITE"
fi 

export IMA_CONFIG_DIR=$CONFIG_FOLDER/
if [ ! -d $CONFIG_FOLDER/plugin/staging/install ]; then
    mkdir -p $CONFIG_FOLDER/plugin/staging/install
fi
if [ ! -d $CONFIG_FOLDER/plugin/staging/uninstall ]; then
    mkdir -p $CONFIG_FOLDER/plugin/staging/uninstall
fi
if [ ! -d $CONFIG_FOLDER/plugin/plugins ]; then
    mkdir -p $CONFIG_FOLDER/plugin/plugins
fi

CLASS=com.ibm.ima.plugin.impl.ImaPluginInstaller
echo "" >> ${LOGFILE} 2>&1 3>&1
echo "Invoke command" >> ${LOGFILE} 2>&1 3>&1
echo "$JAVA $SM $DUMP_PARAM $BOOT_CP $MS $MX $DEBUG_OPT $VMARG -cp $JAR_FILE $CLASS $ACTION $OPT1 $OPT2 $OPT3 $OPT4 $OPT5" >> ${LOGFILE} 2>&1 3>&1
echo "" >> ${LOGFILE} 2>&1 3>&1

PID=$$
TMP_LOG_FILE="/tmp/installPlugin.$PID.log"


$JAVA $SM $DUMP_PARAM $BOOT_CP $MS $MX $DEBUG_OPT $VMARG $CLASS $ACTION $OPT1 $OPT2 $OPT3 $OPT4 $OPT5 >> ${TMP_LOG_FILE} 2>&1 3>&1
rc=$?

cat ${TMP_LOG_FILE} >> ${LOGFILE}  2>&1 3>&1
cat ${TMP_LOG_FILE} 
rm -f ${TMP_LOG_FILE}

exit $rc
   
         
