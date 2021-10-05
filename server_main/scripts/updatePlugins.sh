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

LOGFILE=${IMA_SVR_DATA_PATH}/diag/logs/updatePlugins.log
touch ${LOGFILE}

DATE=`date`

echo "" >> ${LOGFILE} 2>&1 3>&1
echo "------------------------------------------" >> ${LOGFILE} 2>&1 3>&1
echo "Plugin update: Date: $DATE " >> ${LOGFILE} 2>&1 3>&1
echo "" >> ${LOGFILE} 2>&1 3>&1



exec 200> /tmp/imaplugin.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "IMA plugin process is running" >&2
    exit 255
fi

CONFIG_FOLDER=${IMA_SVR_DATA_PATH}/data/config
while getopts ":c:" opt; do
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

PLUGINS_DIR="$CONFIG_FOLDER/plugin/plugins"
STAGING_INSTALL_DIR="$CONFIG_FOLDER/plugin/staging/install"
STAGING_UNINSTALL_DIR="$CONFIG_FOLDER/plugin/staging/uninstall"
rc=0
if [ -d "$STAGING_UNINSTALL_DIR" ]; then
	for i in `find $STAGING_UNINSTALL_DIR -maxdepth 1 -mindepth 1 -type d`;
		do
		  name=$(basename $i)
		  rm -rf $PLUGINS_DIR/$name
		  if [ "$?" != "0" ]; then
			  rc=255
		  fi
		  rm -rf $i
		  if [ "$?" != "0" ]; then
			  rc=255
		  fi
		done       
else 
    echo "Plugin staging uninstall folder does not exist: $STAGING_UNINSTALL_DIR"
fi
if [ -d "$STAGING_INSTALL_DIR" ]; then
	for i in `find $STAGING_INSTALL_DIR -maxdepth 1 -mindepth 1 -type d`;
		do
		  cp -rf $i $PLUGINS_DIR/
		  if [ "$?" != "0" ]; then
			  rc=255
		  fi
		  rm -rf $i
		  if [ "$?" != "0" ]; then
			  rc=255
		  fi
		done       
else 
    echo "Plugin staging install folder does not exist: $STAGING_INSTALL_DIR"
fi
exit $rc
   
         
