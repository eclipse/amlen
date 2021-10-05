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

# Determine if this is a docker container (by finding /docker in /proc/self/cgroup)
grep /docker /proc/self/cgroup >/dev/null

if [ $? == 0 ]
then
   INDOCKER=1
else
   INDOCKER=0
fi

# Create install log file
mkdir -p -m 770 ${LOGDIR}
chmod -R 770 ${LOGDIR}
INITLOG=${LOGDIR}/MessageSightBridgeInstall.log
export INITLOG
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure MessageSight Bridge " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}
echo "In Docker: ${INDOCKER} " >> ${INITLOG}

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
    echo "MessageSight Bridge Instance is already initialized. " >> ${INITLOG}
    echo "---------------------------------------------"  >> ${INITLOG}
    echo  >> ${INITLOG}

else

    echo "Initialize MessageSight Bridge Instance -- " >> ${INITLOG}

    # Initialize the instance
    # No action currently required that is not idempotent

    touch ${IMACFGDIR}/MessageSightInstance.inited
fi
