#!/bin/bash
# Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

# This script is called by install and by initBridge if the marker it 
# creates is not found (e.g. first time or data directories gone)
#
# This tries to create all the writeable directories that the Bridge
# uses. It may be run as root before the service runs (called by
# systemd->initBridge.sh->this script or (usually in a container,
# run as the same user as the server runs as startBridge->initBridge->
# this script.))



# Predefined directory locations for the instance
CURDIR=`pwd`
export CURDIR
DATADIR=${IMA_BRIDGE_DATA_PATH}
export DATADIR
DIAGDIR=${IMA_BRIDGE_DATA_PATH}/diag
export DIAGDIR
LOGDIR=${IMA_BRIDGE_DATA_PATH}/diag/logs
export LOGDIR
COREDIR=${IMA_BRIDGE_DATA_PATH}/diag/cores
export COREDIR

# Create install log file
mkdir -p -m 770 ${LOGDIR}
chmod -R 770 ${LOGDIR}
INITLOG=${LOGDIR}/imabridge_initinstance.log
export INITLOG
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure imabridge " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}

echo "Create required directories" >> ${INITLOG}
mkdir -p -m 770 ${COREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${COREDIR} >> ${INITLOG} 2>&1 3>&1

# Data dir and Diag dir exist as they were created when Log dir was created
chmod -R 770 ${DATADIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${DIAGDIR} >> ${INITLOG} 2>&1 3>&1

mkdir -p -m 770 ${DATADIR}/truststore >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${DATADIR}/truststore >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 770 ${DATADIR}/keystore >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${DATADIR}/keystore >> ${INITLOG} 2>&1 3>&1

# Set default values
IMACFGDIR=${DATADIR}
IMASERVERCFG=${IMA_BRIDGE_INSTALL_PATH}/bin/imabridge.cfg
IMADYNSERVERCFG=${IMA_BRIDGE_DATA_PATH}/bridge.cfg

# Check if container is already inited
if [ -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    echo "imabridge is already initialized. " >> ${INITLOG}
    echo "---------------------------------------------"  >> ${INITLOG}
    echo  >> ${INITLOG}

else

    echo "Initialize imabridge instance -- " >> ${INITLOG}

    #Do things that are not-idempotent...(nothing strictly 
    #non-idempotent for the bridge at the mo)

    touch ${IMACFGDIR}/MessageSightInstance.inited

    #Ensure all data files owned by the right user (if we are running as
    #root with the power to change ownership)... this actually can be rerun
    #but we avoid doing it unnecessarily
    if [ "`whoami`" == "root" ]
    then
        source ${IMA_BRIDGE_INSTALL_PATH}/bin/getUserGroup.sh
        chown -RH $IMABRIDGE_USER:$IMABRIDGE_GROUP ${IMA_BRIDGE_DATA_PATH}
    fi
fi
