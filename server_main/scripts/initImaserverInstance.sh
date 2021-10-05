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

# Predefined directory locations for the instance
DATADIR=${IMA_SVR_DATA_PATH}/data
DIAGDIR=${IMA_SVR_DATA_PATH}/diag
LOGDIR=${IMA_SVR_DATA_PATH}/diag/logs
COREDIR=${IMA_SVR_DATA_PATH}/diag/cores
STOREDIR=${IMA_SVR_DATA_PATH}/store

# Create install log file
mkdir -p -m 770 ${LOGDIR}
chmod -R 770 ${LOGDIR}
INITLOG=${LOGDIR}/MessageSightInstall.log
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure MessageSight Server " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}

echo "Create required directories" >> ${INITLOG}
mkdir -p -m 770 ${COREDIR} >> ${INITLOG} 2>&1
chmod -R 770 ${COREDIR} >> ${INITLOG} 2>&1
mkdir -p -m 770 ${DATADIR}/config >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/keystore >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/LDAP >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/MQC >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/truststore >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/LTPAKeyStore >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/OAuth >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/CRL >> ${INITLOG} 2>&1
mkdir -p -m 700 ${DATADIR}/certificates/PSK >> ${INITLOG} 2>&1
chmod -R 770 ${DATADIR} >> ${INITLOG} 2>&1
mkdir -p -m 770 ${STOREDIR} >> ${INITLOG} 2>&1
chmod -R 770 ${STOREDIR} >> ${INITLOG} 2>&1
chmod -R 770 ${COREDIR} >> ${INITLOG} 2>&1
chmod -R 770 ${STOREDIR} >> ${INITLOG} 2>&1
chmod -R 770 ${DIAGDIR} >> ${INITLOG} 2>&1
chmod -R 700 ${DATADIR}/certificates >> ${INITLOG} 2>&1


# Set default values
IMACFGDIR=${DATADIR}/config
IMASERVERCFG=${IMACFGDIR}/server.cfg
IMADYNSERVERCFG=${IMACFGDIR}/server_dynamic.json

# Check if we are already inited
if [ -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    echo "MessageSight Instance is already initialized. " >> ${INITLOG}
    echo "---------------------------------------------"  >> ${INITLOG}
    echo  >> ${INITLOG}
else
    echo "Initialize MessageSight Instance -- " >> ${INITLOG}
        # Initialize the instance

        echo "Copy files & Directories to ${DATADIR}" >> ${INITLOG} 2>&1
        cp -rf ${IMA_SVR_INSTALL_PATH}/config ${DATADIR}/. >> ${INITLOG} 2>&1
        cp -rf ${IMA_SVR_INSTALL_PATH}/certificates ${DATADIR}/. >> ${INITLOG} 2>&1

        # Set static config file
        cd ${DATADIR}/config

        # Set AdminMode to 0
        sed -i 's/"AdminMode":.*/"AdminMode": 0,/' ${IMADYNSERVERCFG}
        # Enable DiskPersistence
        sed -i 's/"EnableDiskPersistence":.*/"EnableDiskPersistence": true,/' ${IMADYNSERVERCFG}
        # Set MemoryType to 2
        sed -i 's/"MemoryType":.*/"MemoryType": 2,/' ${IMADYNSERVERCFG}

        perl -pi -e 's/\r\n$/\n/g' ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1
        chmod 770 ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1

        # Set store.init file
        touch ${IMA_SVR_INSTALL_PATH}/config/store.init

        # Make use of a unix domain socket for MQConnectivity connection into server
        sed -i 's%Interface\.!MQConnectivityEndpoint\( *\)=\( *\).*%Interface.!MQConnectivityEndpoint\1=\2'${DATADIR}'/MQConnectivityEndpoint_'${SHORT_UUID}'%g' ${IMASERVERCFG}

        touch ${IMACFGDIR}/MessageSightInstance.inited
fi

