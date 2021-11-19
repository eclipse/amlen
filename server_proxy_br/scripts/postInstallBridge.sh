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
LOGDIR=${IMA_BRIDGE_DATA_PATH}/diag/logs
mkdir -p -m 770 ${LOGDIR} > /dev/null 2>&1

INITLOG=${LOGDIR}/BridgeInstall.log
touch ${INITLOG}


#Work out what user and group we should be running as
source ${IMA_BRIDGE_INSTALL_PATH}/bin/getUserGroup.sh >> ${INITLOG}

#Set up server writable directories (repeated on startup esp. in containers in case they change)
source ${IMA_BRIDGE_INSTALL_PATH}/bin/initImabridgeInstance.sh >> ${INITLOG}

# system tuning
mkdir -p -m 770 /etc/imabridge
if [ ! -f /etc/imabridge/MessageSightInstance.inited ]
then
    LMSTR=`cat /etc/security/limits.conf | grep "core unlimited"`
    if [ $? -eq 1 ]
    then
        echo "* soft core unlimited" >> /etc/security/limits.conf
        echo "* hard core unlimited" >> /etc/security/limits.conf
        echo "* soft nproc unlimited" >> /etc/security/limits.conf
        echo "* hard nproc unlimited" >> /etc/security/limits.conf
        echo "* soft nofile 200000" >> /etc/security/limits.conf
        echo "* hard nofile 200000" >> /etc/security/limits.conf
        echo "* - rtprio 100" >> /etc/security/limits.conf
    fi
    touch /etc/imabridge/MessageSightInstance.inited
fi

# Copy imaserver logrorate configuration file if logrorate is enabled
if [ -d /etc/logrotate.d ]
then
    if [ ! -f /etc/logrotate.d/imabridge ]
    then
        cp ${IMA_BRIDGE_INSTALL_PATH}/config/imabridge.logrotate /etc/logrotate.d/imabridge
    fi 
fi

# Now that common libs (e.g. libcurl and libldap) are shipped with MessageSight we can no longer expose the 
# ${IMA_BRIDGE_INSTALL_PATH}/lib64 directory as a common library path.  It breaks other system commands (e.g. yum)
#    # reload LD_LIBRARYPATH
#    if [ ! -f /etc/ld.so.conf.d/imabridge.conf ]
#    then
#        echo "${IMA_BRIDGE_INSTALL_PATH}/lib64" > /etc/ld.so.conf.d/imabridge.conf
#    fi
#    ldconfig -v > /dev/null 2>&1 3>&1
if [ -f /etc/ld.so.conf.d/imabridge.conf ] ; then
    rm -f /etc/ld.so.conf.d/imabridge.conf
    ldconfig -v > /dev/null 2>&1 3>&1
fi

#Create user+group if necessary and configure us to run as them:
${IMA_BRIDGE_INSTALL_PATH}/bin/createUserGroup.sh "${IMABRIDGE_USER}" "${IMABRIDGE_GROUP}" >> ${INITLOG}
${IMA_BRIDGE_INSTALL_PATH}/bin/setUserGroup.sh    "${IMABRIDGE_USER}" "${IMABRIDGE_GROUP}" >> ${INITLOG}

# Install imabridge systemd unit file
#
if [ -d "/etc/systemd/system" ]
then   
    # Update systemd (including removing old service name)
    rm -f /etc/systemd/system/IBMIoTMessageSightBridge.service
    cp ${IMA_BRIDGE_INSTALL_PATH}/config/imabridge.service /etc/systemd/system/.
    ln -s /etc/systemd/system/imabridge.service /etc/systemd/system/IBMIoTMessageSightBridge.service

    INIT_SYSTEM=$(ps --no-headers -o comm 1)
    if [ "$INIT_SYSTEM" == "systemd" ]
    then
        systemctl daemon-reload
    fi
fi

