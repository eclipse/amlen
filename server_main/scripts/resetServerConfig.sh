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

STARTMODE=$1
export STARTMODE
SLEEPTIME=$2
export SLEEPTIME

# Predefined directory locations for the instance
CURDIR=`pwd`
export CURDIR
DATADIR=${IMA_SVR_DATA_PATH}/data
export DATADIR
LOGDIR=${IMA_SVR_DATA_PATH}/diag/logs
export LOGDIR
COREDIR=${IMA_SVR_DATA_PATH}/diag/cores
export COREDIR
STOREDIR=${IMA_SVR_DATA_PATH}/store
export STOREDIR

# Create reset log file
INITLOG=${LOGDIR}/MessageSightReset.log
export INITLOG
touch ${INITLOG}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Reset Configuration of MessageSight Server " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}

touch /tmp/.reset_inited

# Check for docker container
isDocker=0
if [ -f /tmp/.dockerinit ]
then
    echo "Server is configured in a docker container" >> ${INITLOG}
    isDocker=1
    export isDocker
fi

if [ $STARTMODE -eq 0 ]
then
    echo "Restart IBM IoT MessageSight Server " >> ${INITLOG}
    # Just restart the server in container
    if [ "${SLEEPTIME}" != "" ]
   then
        sleep $SLEEPTIME
    fi
    exit 0
fi


# Reset config
# Make sure that you are in config directory
sanityCheck=0
if [ -d ${DATADIR} ]
then
    cd ${DATADIR}
    # check if config and certificates directory exist
    if [ -d config ] && [ -d certificates ] && [ -f config/server_dynamic.json ]
    then
        # check if default configuration exists
        if [ -d ${IMA_SVR_INSTALL_PATH}/config ] && [ -d ${IMA_SVR_INSTALL_PATH}/certificates ]
        then
            sanityCheck=1
        fi
    fi
fi

if [ $sanityCheck -eq 0 ]
then
    # Exist out - something is wrong
    echo "Installation sanity check failed"
    exit 1
fi

echo "Remove old configuration files" >> ${INITLOG}
cd ${DATADIR} >> ${INITLOG} 2>&1 3>&1
rm -r config >> ${INITLOG} 2>&1 3>&1
rm -r certificates >> ${INITLOG} 2>&1 3>&1
cd ${LOGDIR} >> ${INITLOG} 2>&1 3>&1
rm -f *.log
cd ${STOREDIR} >> ${INITLOG} 2>&1 3>&1
rm -rf com.ibm.ism >> ${INITLOG} 2>&1 3>&1
rm -rf persist >> ${INITLOG} 2>&1 3>&1
if [ -d ${COREDIR} ]
then
    rm -rf ${COREDIR} >> ${INITLOG} 2>&1 3>&1
fi
if [ -d /tmp/userfiles ]
then
    rm -rf /tmp/userfiles/* >> ${INITLOG} 2>&1 3>&1
fi
    

echo "Create required directories" >> ${INITLOG}
mkdir -p -m 770 ${COREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${COREDIR} >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 770 ${DATADIR}/config >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/keystore >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/LDAP >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/MQC >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/truststore >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/LTPAKeyStore >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/OAuth >> ${INITLOG} 2>&1 3>&1
mkdir -p -m 700 ${DATADIR}/certificates/PSK >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${DATADIR} >> ${INITLOG} 2>&1 3>&1
mkdir -p ${STOREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${STOREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${COREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${STOREDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 770 ${DIAGDIR} >> ${INITLOG} 2>&1 3>&1
chmod -R 700 ${DATADIR}/certificates >> ${INITLOG} 2>&1 3>&1


# Set default values
IMACFGDIR=${DATADIR}/config
IMASERVERCFG=${CFGDIR}/server.cfg
IMADYNSERVERCFG=${IMACFGDIR}/server_dynamic.json

echo "Re-Initialize MessageSight Instance -- " >> ${INITLOG}

# copy required files/directories from install directory

echo "Copy files & Directories to ${DATADIR}" >> ${INITLOG} 2>&1 3>&1
cp -rf ${IMA_SVR_INSTALL_PATH}/config ${DATADIR}/. >> ${INITLOG} 2>&1 3>&1
cp -rf ${IMA_SVR_INSTALL_PATH}/certificates ${DATADIR}/. >> ${INITLOG} 2>&1 3>&1

# Set static config file
cd ${DATADIR}/config

# Set AdminMode to 0
sed -i 's/"AdminMode":.*/"AdminMode": 0,/' ${IMADYNSERVERCFG}
# Enable DiskPersistence
sed -i 's/"EnableDiskPersistence":.*/"EnableDiskPersistence": true,/' ${IMADYNSERVERCFG}
# Set MemoryType to 2
sed -i 's/"MemoryType":.*/"MemoryType": 2,/' ${IMADYNSERVERCFG}

# some setup for non-docker instance
if [ $isDocker -eq 0 ]
then
    MESSAGESIGHT_ADMIN_HOST=All
    MESSAGESIGHT_ADMIN_PORT=9089
    export MESSAGESIGHT_ADMIN_HOST
    export MESSAGESIGHT_ADMIN_PORT
fi

# Update AdminServerHost in server_dynamic.json file
echo "Set imaserver admin host to $MESSAGESIGHT_ADMIN_HOST" >> ${INITLOG}
sed -i 's/ADMIN_INTERFACE/'$MESSAGESIGHT_ADMIN_HOST'/' ${IMADYNSERVERCFG}
# Update AdminServerPort in server.cfg file
echo "Set imaserver admin port to $MESSAGESIGHT_ADMIN_PORT" >> ${INITLOG}
sed -i 's/ADMIN_PORT/'$MESSAGESIGHT_ADMIN_PORT'/' ${IMADYNSERVERCFG}

perl -pi -e 's/\r\n$/\n/g' ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1 3>&1
chmod 770 ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1 3>&1

# Set store.init file
touch ${IMA_SVR_INSTALL_PATH}/config/store.init

touch ${IMACFGDIR}/MessageSightInstance.inited

if [ "${SLEEPTIME}" != "" ]
then
    sleep $SLEEPTIME
fi

echo "Restart imaserver." >> ${INITLOG}

exit 0


