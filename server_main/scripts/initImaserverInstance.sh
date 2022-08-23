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

# This script is called during install and is called by initServer 
# if the marker it creates is not found (e.g. first time or data 
# directories gone) 
#
# This tries to create all the writeable directories that the server
# uses. It may be run as root before the service runs (called by
# systemd->initServer.sh->this script or (usually in a container,
# run as the same user as the server runs as startServer->initServer->
# this script.))

# Predefined directory locations for the instance
DATADIR=${IMA_SVR_DATA_PATH}/data
DIAGDIR=${IMA_SVR_DATA_PATH}/diag
LOGDIR=${IMA_SVR_DATA_PATH}/diag/logs
COREDIR=${IMA_SVR_DATA_PATH}/diag/cores
STOREDIR=${IMA_SVR_DATA_PATH}/store

#Sourcing this script sets ${IMA_DEFAULT_PERMS} and ${IMA_DIAGCFG_PERMS}
source ${IMA_SVR_INSTALL_PATH}/bin/getFilePermissions.sh

# Create install log file
mkdir -p -m ${IMA_DIAGCFG_PERMS} ${LOGDIR}
chmod -R ${IMA_DIAGCFG_PERMS} ${LOGDIR}
INITLOG=${LOGDIR}/imaserver_initinstance.log
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure imaserver " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}

echo "Using ${IMA_DIAGCFG_PERMS} for diag/cfg/userfiles"
echo "Using ${IMA_DEFAULT_PERMS} for other things"

echo "Create required directories" >> ${INITLOG}
mkdir -p -m ${IMA_DIAGCFG_PERMS} ${COREDIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DIAGCFG_PERMS} ${COREDIR} >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DIAGCFG_PERMS} ${DATADIR}/config >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/hasync >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/keystore >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/LDAP >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/MQC >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/truststore >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/LTPAKeyStore >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/OAuth >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/CRL >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates/PSK >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${STOREDIR} >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DIAGCFG_PERMS} ${DATADIR}/userfiles >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS}${SVR_DATA_DIR}/data/import >> ${INITLOG} 2>&1
mkdir -p -m ${IMA_DEFAULT_PERMS} ${SVR_DATA_DIR}/data/export >> ${INITLOG} 2>&1

chmod -R ${IMA_DEFAULT_PERMS} ${DATADIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DEFAULT_PERMS} ${STOREDIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DIAGCFG_PERMS} ${COREDIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DEFAULT_PERMS} ${STOREDIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DIAGCFG_PERMS} ${DIAGDIR} >> ${INITLOG} 2>&1
chmod -R ${IMA_DEFAULT_PERMS} ${DATADIR}/certificates >> ${INITLOG} 2>&1
chmod -R ${IMA_DIAGCFG_PERMS} ${DATADIR}/userfiles >> ${INITLOG} 2>&1
chmod -R ${IMA_DEFAULT_PERMS} ${DATADIR}/data/import >> ${INITLOG} 2>&1
chmod -R ${IMA_DEFAULT_PERMS} ${DATADIR}/data/export >> ${INITLOG} 2>&1

# Set default values
IMACFGDIR=${DATADIR}/config
IMASERVERCFG=${IMACFGDIR}/server.cfg
IMADYNSERVERCFG=${IMACFGDIR}/server_dynamic.json

# Check if we are already inited
if [ -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    echo "imaserver instance is already initialized. " >> ${INITLOG}
    echo "---------------------------------------------"  >> ${INITLOG}
    echo  >> ${INITLOG}
else
    echo "Initialize imaserver Instance -- " >> ${INITLOG}
    # Initialize the instance (i.e. do one-time nonidempotent things)

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
    chmod ${IMA_DEFAULT_PERMS} ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1

    # Set store.init file
    touch ${IMACFGDIR}/store.init

    # Make use of a unix domain socket for MQConnectivity connection into server
    sed -i 's%Interface\.!MQConnectivityEndpoint\( *\)=\( *\).*%Interface.!MQConnectivityEndpoint\1=\2'${DATADIR}'/MQConnectivityEndpoint_'${SHORT_UUID}'%g' ${IMASERVERCFG}

    touch ${IMACFGDIR}/MessageSightInstance.inited

    #Ensure all data files owned by the right user (if we are running as
    #root with the power to change ownership)... this actually can be rerun
    #but we avoid doing it unnecessarily
    if [ "`whoami`" == "root" ]
    then
        source ${IMA_SVR_INSTALL_PATH}/bin/getUserGroup.sh
        chown -RH $IMASERVER_USER:$IMASERVER_GROUP ${IMA_SVR_DATA_PATH}
    fi
fi

