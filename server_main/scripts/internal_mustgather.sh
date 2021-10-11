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

IMACFGDIR=${IMA_SVR_DATA_PATH}/data/config

WORKING_DIR=$1
if [ -z $WORKING_DIR ]
then
	WORKING_DIR=${IMA_SVR_DATA_PATH}/diag/cores
fi
mkdir -p ${WORKING_DIR} > /dev/null 2>&1
WORKING_DIR="$(cd $WORKING_DIR; pwd)"

PREFIX=$2
if [ -z $PREFIX ]
then
	PREFIX=container
fi

IMASERVER_SOURCE=$(dirname $(rpm -q --filesbypkg IBMWIoTPMessageGatewayServer | grep lib64 | head -n 1  | awk '{print $2}') 2> /dev/null)
if [ -z $IMASERVER_SOURCE ]
then
	IMASERVER_SOURCE=${IMA_SVR_INSTALL_PATH}
fi

WEBUI_SOURCE=$(dirname $(dirname $(rpm -q --filesbypkg IBMWIoTPMessageGatewayWebUI | grep "bin/server$" | head -n 1 | awk '{print $2}') 2> /dev/null) 2> /dev/null)
if [ ! -z $WEBUI_SOURCE ]
then
	ISWEBUI=1
	JAVABINARY=$(dirname $(rpm -q --filesbypkg IBMWIoTPMessageGatewayWebUI 2> /dev/null | grep "bin/java$" | head -n 1 | awk '{print $2}') 2> /dev/null)
else
	ISWEBUI=0
fi

FILES_TO_COMPRESS=
FILES_TO_CLEANUP=

COUNT=1

if [ "$PREFIX" == "container" ]
then
echo "====================================="
echo " General info section"
echo "====================================="
echo "${COUNT}. uname -a"
uname -a 2> /dev/null
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. locale"
locale 2> /dev/null
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. date"
date 2> /dev/null
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. df -h --exclude-type=nfs4 --exclude-type=nfs3"
df -h --exclude-type=nfs4 --exclude-type=nfs3 2> /dev/null
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. hostname"
hostname 2>&1
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. free"
free 2>&1
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. du -sxm --exclude=/proc /*"
du -sxm --exclude=/proc /* 2>&1
COUNT=$(($COUNT+1))
echo "-------------------------------------"
fi

NTPSTAT=$(which ntpstat > /dev/null 2>&1)
if [ $? -eq 0 ]
then
	echo "-------------------------------------"
	echo "${COUNT}. ntpstat"
	ntpstat 2>&1
	COUNT=$(($COUNT+1))
	echo "-------------------------------------"
fi

echo "====================================="
echo " Process and file descriptor section"
echo "====================================="
echo "${COUNT}. ulimit -a"
echo "ulimit -a" | /bin/bash 2>&1
COUNT=$(($COUNT+1))
echo "-------------------------------------"
echo "${COUNT}. ps -axef"
ps -axef 2>&1
COUNT=$(($COUNT+1))
echo "-------------------------------------"

echo "${COUNT}. top"
if [ -f ~/.toprc ]
then
	mv ~/.toprc ~/.toprc.ima.bak
fi
echo "RCfile for \"top with windows\" \
Id:a, Mode_altscr=0, Mode_irixps=1, Delay_time=1.000, Curwin=0\
Def	fieldscur=AEHIOQTWKNMbcdfgJplrsuvyzX\
	winflags=97081, sortindx=10, maxtasks=0\
	summclr=6, msgsclr=2, headclr=3, taskclr=2\
Job	fieldscur=ABcefgjlrstuvyzMKNHIWOPQDX\
	winflags=62777, sortindx=0, maxtasks=0\
	summclr=6, msgsclr=6, headclr=7, taskclr=6\
Mem	fieldscur=ANOPQRSTUVbcdefgjlmyzWHIKX\
	winflags=62777, sortindx=13, maxtasks=0\
	summclr=5, msgsclr=5, headclr=4, taskclr=5\
Usr	fieldscur=ABDECGfhijlopqrstuvyzMKNWX\
	winflags=62777, sortindx=4, maxtasks=0\
	summclr=3, msgsclr=3, headclr=2, taskclr=3" > ~/.toprc
top -b -n1
if [ -f ~/.toprc.ima.bak ]
then
	mv ~/.toprc.ima.bak ~/.toprc
fi
COUNT=$(($COUNT+1)) 
echo "-------------------------------------"

IOSTAT=$(which iostat > /dev/null 2>&1)
if [ $? -eq 0 ]
then
	echo "-------------------------------------"
	echo "${COUNT}. iostat 1 5"
	iostat 1 5 2>&1
	COUNT=$(($COUNT+1))
	echo "-------------------------------------"
fi

PERF=$(which perf > /dev/null 2>&1)
if [ $? -eq 0 ]
then
	perf record -o $WORKING_DIR/perf.data -ag -- sleep 5 > /dev/null 2>&1
	perf report -i $WORKING_DIR/perf.data > ${WORKING_DIR}/${PREFIX}_perf.report 2>&1 3>&1
	rm $WORKING_DIR/perf.data 2> /dev/null
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_perf.report"
	FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_perf.report"
fi

if [ "$PREFIX" == "container" ]
then
	tar -czf ${WORKING_DIR}/${PREFIX}_varlog.tar.gz /var/log --ignore-failed-read > /dev/null 2>&1
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_varlog.tar.gz"
	FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_varlog.tar.gz"
 
	tar -czf ${WORKING_DIR}/${PREFIX}_etc.tar.gz /etc --ignore-failed-read > /dev/null 2>&1
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_etc.tar.gz"
	FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_etc.tar.gz"
fi

env > ${WORKING_DIR}/${PREFIX}_environment_vars.txt 2>&1
FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_environment_vars.txt"
FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_environment_vars.txt"

GET_WEBUI_INFO=0
GET_IMASERVER_INFO=0
if [ $ISWEBUI -eq 0 ]
then
	GET_IMASERVER_INFO=1
else
	GET_WEBUI_INFO=1
fi
if [ "$PREFIX" != "container" ]
then
	GET_IMASERVER_INFO=1
    if [ -f ${IMA_WEBUI_INSTALL_PATH}/bin/startWebUI.sh ]
    then
	    GET_WEBUI_INFO=1
    fi
fi


if [ $GET_IMASERVER_INFO -eq 1 ]
then
	tar -czf ${WORKING_DIR}/${PREFIX}_binaries.tar.gz ${IMASERVER_SOURCE}/bin ${IMASERVER_SOURCE}/lib64 --ignore-failed-read > /dev/null 2>&1
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_binaries.tar.gz"
	FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_binaries.tar.gz"
	${IMASERVER_SOURCE}/bin/extractstackfromcore.sh ${IMASERVER_SOURCE}
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ${IMA_SVR_DATA_PATH}/diag/cores/messagesight_stack.*"
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ${IMA_SVR_DATA_PATH}/data"
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ${IMA_SVR_DATA_PATH}/diag/logs"
	
	for f in imaserver icu_gettext mqcbridge
	do
		OUT=$(ldd ${IMASERVER_SOURCE}/bin/${f} 2>&1)
		if [ $? -eq 0 ]
		then
			echo "${COUNT}. ldd on $f"
			echo "$OUT"
			COUNT=$(($COUNT+1)) 
			echo "-------------------------------------"
			echo "${COUNT}. version check on $f"
			strings ${IMASERVER_SOURCE}/bin/$f | grep version_string 2> /dev/null
			COUNT=$(($COUNT+1))
			echo "-------------------------------------"
		fi		
	done
	for f in ${IMASERVER_SOURCE}/lib64/*
	do
		OUT=$(ldd $f 2>&1)
		if [ $? -eq 0 ]
		then
			echo "${COUNT}. ldd on $f"
			echo "$OUT"
			COUNT=$(($COUNT+1)) 
			echo "-------------------------------------"
		fi
	done
	
    PID=$(ps -ef | grep "imaserver -d" | grep -v grep | tail -n 1 | awk '{print $2}')
    if [ ! -z $PID ]
    then
        echo "${COUNT}. pmap -p imaserver"
        pmap -p $PID 2>&1
        COUNT=$(($COUNT+1))
        echo "-------------------------------------"

        echo "${COUNT}. gstack imaserver"
        gstack $PID 2>&1
        COUNT=$(($COUNT+1))
        echo "-------------------------------------"

        # imaserver is up, so flush the trace
        ADMIN_PORT=$(cat ${IMACFGDIR}/server_dynamic.json 2>/dev/null | python3 -c 'import json,sys;obj=json.load(sys.stdin);print(obj["AdminEndpoint"]["AdminEndpoint"]["Port"]);' 2>/dev/null)
		if [ -z $ADMIN_PORT ]
		then
			ADMIN_PORT=9089
		fi
        ADMIN_HOST=$(cat ${IMACFGDIR}/server_dynamic.json 2>/dev/null | python3 -c 'import json,sys;obj=json.load(sys.stdin);print(obj["AdminEndpoint"]["AdminEndpoint"]["Interface"]);' 2>/dev/null)
		if [ -z $ADMIN_HOST ] || [ "$ADMIN_HOST" == "All" ]
		then
			ADMIN_HOST="127.0.0.1"
		fi
		curl -H "Content-Type: application/json" -X POST --data "{ }" http://${ADMIN_HOST}:${ADMIN_PORT}/ima/v1/service/trace/flush > /dev/null 2>&1
    fi

    PID=$(ps -ef | grep "mqcbridge" | grep -v grep | tail -n 1 | awk '{print $2}')
    if [ ! -z $PID ]
    then
        echo "${COUNT}. pmap -p mqcbridge"
        pmap -p $PID 2>&1
        COUNT=$(($COUNT+1))
        echo "-------------------------------------"

        echo "${COUNT}. gstack mqcbridge"
        gstack $PID 2>&1
        COUNT=$(($COUNT+1))
        echo "-------------------------------------"
    fi
fi
if [ $GET_WEBUI_INFO -eq 1 ]
then
	NOW=$(date +%y.%m.%d_%H.%M.%S)
	JAVAPATH=${JAVABINARY%/*}
	PATH=$PATH:$JAVAPATH ${WEBUI_SOURCE}/bin/server dump ISMWebUI --archive="${WORKING_DIR}/${PREFIX}_webui.dump-${NOW}.zip"
	FILES_TO_COMPRESS="${FILES_TO_COMPRESS} ./${PREFIX}_webui.dump-${NOW}.zip ${WEBUI_SOURCE}/usr/servers/ISMWebUI/logs ${WEBUI_SOURCE}/usr/servers/ISMWebUI/*.xml ${WEBUI_SOURCE}/usr/servers/ISMWebUI/server.* ${WEBUI_SOURCE}/usr/servers/ISMWebUI/properties.*"
	FILES_TO_CLEANUP="${FILES_TO_CLEANUP} ${WORKING_DIR}/${PREFIX}_webui.dump-${NOW}.zip"
fi

cd "${WORKING_DIR}" || { echo "Failed to change directory to ${WORKING_DIR}: $?"; exit $?; }
tar -czf ${WORKING_DIR}/${PREFIX}_mustgather.tar.gz ${FILES_TO_COMPRESS} --ignore-failed-read > /dev/null 2>&1
rm -rf -rf ${FILES_TO_CLEANUP} 2> /dev/null
