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
# This script does some initial setup for the server. It can be run as root
# by systemd before the server runs or (esp in a container) called from startServer.
# (unlike initImaserverInstance.sh it is not run during install so this happens 
#  for the first time at first run and then on subsequent starts)

INITLOG=${IMA_SVR_DATA_PATH}/diag/logs/imaserver_init.log
touch ${INITLOG}

echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo "Initialize imaserver" >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}
echo "User: `whoami` " >> ${INITLOG}

#Try and find some way to identify this instance so if there are multiple clashing servers
#e.g. in "duelling" containers writing to same logs, they can be identified


# Find container UUID
UUID=`cat /proc/self/cgroup | grep -o  -e "docker-.*.scope" | head -n 1 | sed "s/docker-\(.*\).scope/\\1/"`
# Alternative form of information containing container ID
if [ "${UUID}" == "" ]
then
    UUID=`cat /proc/self/cgroup | grep -o -e ".*:/docker/.*" | head -n 1 | sed "s/.*:\/docker\/\(.*\)/\\1/"`
fi

#If we haven't found a UUID yet, try a k8s style pod....
if [ "${UUID}" == "" ]
then
    UUID=`cat /proc/self/cgroup | grep -o -E -e "pod[^s/]+" | head -n 1`
fi

if [ "${UUID}" != "" ]
then
    SHORT_UUID=`echo ${UUID} | cut -c1-12`
    export SHORT_UUID
    echo "Start imaserver container: ${SHORT_UUID}" >> ${INITLOG}
else
    SHORT_UUID="imaserver"
    export SHORT_UUID
    echo "Start imaserver instance: ${SHORT_UUID}" >> ${INITLOG}
fi

# Set default values

# MESSAGESIGHT_ADMIN_HOST - Admin server host
# Default is All
if [ "${MESSAGESIGHT_ADMIN_HOST}" == "" ]
then
    MESSAGESIGHT_ADMIN_HOST="All"
    export MESSAGESIGHT_ADMIN_HOST
fi

# MESSAGESIGHT_ADMIN_PORT - Admin server port
# Default is 9089
if [ "${MESSAGESIGHT_ADMIN_PORT}" == "" ]
then
    MESSAGESIGHT_ADMIN_PORT="9089"
    export MESSAGESIGHT_ADMIN_PORT
fi

# Predefined directory locations in the container
IMADATADIR=${IMA_SVR_DATA_PATH}/data
IMALOGDIR=${IMA_SVR_DATA_PATH}/diag/logs
IMACOREDIR=${IMA_SVR_DATA_PATH}/diag/cores
IMACFGDIR=${IMADATADIR}/config
IMASERVERCFG=${IMACFGDIR}/server.cfg
IMADYNSERVERCFG=${IMACFGDIR}/server_dynamic.json

# Setup server-writable directories (if required
if [ ! -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    ${IMA_SVR_INSTALL_PATH}/bin/initImaserverInstance.sh >> ${INITLOG}
fi

# Initialize container specific data
if [ ! -f ${IMACFGDIR}/.serverCFGUpdated ]
then
    # Update AdminServerHost in server.cfg file
    echo "Set imaserver admin host to $MESSAGESIGHT_ADMIN_HOST" >> ${INITLOG}
    sed -i 's/ADMIN_INTERFACE/'$MESSAGESIGHT_ADMIN_HOST'/' $IMADYNSERVERCFG

    # Update AdminServerPort in server.cfg file
    echo "Set imaserver admin port to $MESSAGESIGHT_ADMIN_PORT" >> ${INITLOG}
    sed -i 's/ADMIN_PORT/'$MESSAGESIGHT_ADMIN_PORT'/' $IMADYNSERVERCFG

    touch ${IMACFGDIR}/server_dynamic.json

    touch ${IMACFGDIR}/.serverCFGUpdated
fi

echo "imaserver instance is initialized. " >> ${INITLOG}
echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo  >> ${INITLOG}


