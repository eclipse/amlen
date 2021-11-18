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
# This script does some initial setup for the bridge. It can be run as root
# by systemd before the bridge runs or (esp in a container) called from startBridge.

INITLOG=${IMA_BRIDGE_DATA_PATH}/diag/logs/imabridge_init.log
export INITLOG
mkdir -p -m 770 $(dirname $INITLOG)
CURDIR=`pwd`
export CURDIR

touch ${INITLOG}

echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo "Initialize imabridge " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}
echo "User: `whoami` " >> ${INITLOG}

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
    echo "Start imabridge container: ${SHORT_UUID}" >> ${INITLOG}
else
    SHORT_UUID="imabridge"
    export SHORT_UUID
    echo "Start imabridge instance: ${SHORT_UUID}" >> ${INITLOG}
fi

# Set default values

# MESSAGESIGHT_ADMIN_HOST - Admin server host
# Default is All
##if [ "${MESSAGESIGHT_ADMIN_HOST}" == "" ]
##then
##    MESSAGESIGHT_ADMIN_HOST="All"
##    export MESSAGESIGHT_ADMIN_HOST
##fi

# MESSAGESIGHT_ADMIN_PORT - Admin server port
# Default is 9089
##if [ "${MESSAGESIGHT_ADMIN_PORT}" == "" ]
##then
##    MESSAGESIGHT_ADMIN_PORT="9089"
##    export MESSAGESIGHT_ADMIN_PORT
##fi

# Predefined directory locations in the container
IMADATADIR=${IMA_BRIDGE_DATA_PATH}/data
IMALOGDIR=${IMA_BRIDGE_DATA_PATH}/diag/logs
IMACOREDIR=${IMA_BRIDGE_DATA_PATH}/diag/cores
IMACFGDIR=${IMA_BRIDGE_DATA_PATH}
IMASERVERCFG=${IMA_BRIDGE_INSTALL_PATH}/bin/imabridge.cfg
IMADYNBRIDGECFG=${IMA_BRIDGE_DATA_PATH}/bridge.cfg

# Initialize instance if required
if [ ! -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    ${IMA_BRIDGE_INSTALL_PATH}/bin/initImabridgeInstance.sh >> ${INITLOG}
fi

# Initialize container specific data
if [ ! -f ${IMACFGDIR}/.serverCFGUpdated ]
then

    if [ ! -f ${IMADYNBRIDGECFG} ]
    then

        if [ ! -z ${IMABRIDGE_ADMINPORT+x} ]  # Check if IMABRIDGE_ADMINPORT has been set in environment
        then
            # IMABRIDGE_ADMINPORT=${IMABRIDGE_ADMINPORT:-9082}
            IMABRIDGE_ADMINIFACE=${IMABRIDGE_ADMINIFACE:-localhost}
            IMABRIDGE_ADMINSECURE=${IMABRIDGE_ADMINSECURE:-true}
            IMABRIDGE_ADMINUSER=${IMABRIDGE_ADMINUSER:-adminUser}
            IMABRIDGE_ADMINPW=${IMABRIDGE_ADMINPW:-adminPassword}

            re='^[0-9]+$'
            if [[ ! $IMABRIDGE_ADMINPORT =~ $re ]] ||  [[ $IMABRIDGE_ADMINPORT -lt 1 ]] ||  [[ $IMABRIDGE_ADMINPORT -gt 65535 ]] 
            then
                echo "IMABRIDGE_ADMINPORT is set as $IMABRIDGE_ADMINPORT .  A number between 1 and 65535 is expected."
                exit 1
            fi

            if [[ $IMABRIDGE_ADMINSECURE != "true" ]] && [[ $IMABRIDGE_ADMINSECURE != "false" ]]
            then
                echo "IMABRIDGE_ADMINSECURE is set as $IMABRIDGE_ADMINSECURE . true is set now."
                IMABRIDGE_ADMINSECURE="true"
            fi

            cat > $IMADYNBRIDGECFG <<EOF
{
    "Endpoint": {
        "admin": {
            "Port": $IMABRIDGE_ADMINPORT,
            "Interface": "$IMABRIDGE_ADMINIFACE",
            "Secure": $IMABRIDGE_ADMINSECURE,
            "Protocol": "Admin",
            "Method": "TLSv1.2",
            "Certificate": "imabridge_default_cert.pem",
            "Key": "imabridge_default_key.pem",
            "EnableAbout": true,
            "Authentication": "basic"
        }
    },
    "User": {
        "$IMABRIDGE_ADMINUSER": { "Password": "$IMABRIDGE_ADMINPW" }
    }
}
EOF

        fi #if [ ! -z ${IMABRIDGE_ADMINPORT+x} ]
        touch ${IMADYNBRIDGECFG}
        touch ${IMACFGDIR}/.serverCFGUpdated
    fi
fi

if [ ! -f ${IMACFGDIR}/.defaultAdminCerts ]
then
    mkdir -p -m 770 ${IMA_BRIDGE_DATA_PATH}/keystore

    if [[ ! -f ${IMA_BRIDGE_DATA_PATH}/keystore/imabridge_default_key.pem ]]  ||  [[ ! -f ${IMA_BRIDGE_DATA_PATH}/keystore/imabridge_default_cert.pem ]]
    then
        cp ${IMA_BRIDGE_INSTALL_PATH}/certificates/keystore/imabridge_default_key.pem ${IMA_BRIDGE_DATA_PATH}/keystore/imabridge_default_key.pem
        cp ${IMA_BRIDGE_INSTALL_PATH}/certificates/keystore/imabridge_default_cert.pem ${IMA_BRIDGE_DATA_PATH}/keystore/imabridge_default_cert.pem
    fi
    touch ${IMACFGDIR}/.defaultAdminCerts
fi


echo "imabridge instance is initialized. " >> ${INITLOG}
echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo  >> ${INITLOG}


