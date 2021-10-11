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

PXY_INSTALL_DIR=${IMA_PROXY_INSTALL_PATH}
PXY_DATA_DIR=${IMA_PROXY_DATA_PATH}

CURDIR=`pwd`
export CURDIR
DATADIR=${PXY_DATA_DIR}
export DATADIR
DIAGDIR=${PXY_DATA_DIR}/diag
export DIAGDIR
LOGDIR=${PXY_DATA_DIR}/diag/logs
export LOGDIR
COREDIR=${PXY_DATA_DIR}/diag/cores
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
INITLOG=${LOGDIR}/MessageSightProxyInstall.log
export INITLOG
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure Amlen Proxy " >> ${INITLOG}
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
IMASERVERCFG=${PXY_INSTALL_DIR}/bin/imaproxy.cfg
IMADYNSERVERCFG=${PXY_DATA_DIR}/proxy.cfg

# Check if container is already inited
if [ -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    echo "Amlen Proxy Instance is already initialized. " >> ${INITLOG}
    echo "---------------------------------------------"  >> ${INITLOG}
    echo  >> ${INITLOG}

else

    echo "Initialize Amlen Proxy Instance -- " >> ${INITLOG}

    # Initialize the instance
    # No action currently required that is not idempotent

    touch ${IMACFGDIR}/MessageSightInstance.inited
fi
