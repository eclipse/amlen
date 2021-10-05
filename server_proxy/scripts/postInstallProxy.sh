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
LOGDIR=/var/imaproxy/diag/logs
mkdir -p -m 770 ${LOGDIR} > /dev/null 2>&1

INITLOG=${LOGDIR}/ProxyInstall.log
touch ${INITLOG}


#Work out what user and group we should be running as
source /opt/ibm/imaproxy/bin/getUserGroup.sh >> ${INITLOG}


perl -pi -e 's/\r\n$/\n/g' ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1 3>&1
chmod 770 ${IMADYNSERVERCFG} >> ${INITLOG} 2>&1 3>&1


# system tuning
mkdir -p -m 770 /etc/imaproxy
if [ ! -f /etc/imaproxy/MessageSightInstance.inited ]
then
    LMSTR=`cat /etc/security/limits.conf | grep "core unlimited"`
    if [ $? -eq 1 ]
    then
        echo "* soft core unlimited" >> /etc/security/limits.conf
        echo "* hard core unlimited" >> /etc/security/limits.conf
        echo "* soft nofile 200000" >> /etc/security/limits.conf
        echo "* hard nofile 200000" >> /etc/security/limits.conf
    fi
    touch /etc/imaproxy/MessageSightInstance.inited
fi

# Copy imaserver logrorate configuration file if logrorate is enabled
if [ -d /etc/logrotate.d ]
then
    if [ ! -f /etc/logrotate.d/imaproxy ]
    then
        cp /opt/ibm/imaproxy/config/imaproxy.logrotate /etc/logrotate.d/imaproxy
    fi
fi
#
if [ -d "/etc/systemd/system" ]
then
    # Update systemd (including removing old service name)
    rm -f /etc/systemd/system/IBMIoTMessageSightProxy.service
    cp /opt/ibm/imaproxy/config/imaproxy.service /etc/systemd/system/.
    ln -s /etc/systemd/system/imaproxy.service /etc/systemd/system/IBMIoTMessageSightProxy.service

    INIT_SYSTEM=$(ps --no-headers -o comm 1)
    if [ "$INIT_SYSTEM" == "systemd" ]
    then
        systemctl daemon-reload
    fi
fi

