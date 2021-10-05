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

# RPM erase and upgrade post install script

POSTUNFLAG=0

if [ $# -eq 0 ]
then
    POSTUNFLAG=0
else
    POSTUNFLAG=$1
fi

if [ -e ${IMA_SVR_DATA_PATH}/markers/install-imaserver ]
then
    #We can see a marker that some version of us (maybe with a different
    #name?) is being installed.... treat this as an upgrade
    POSTUNFLAG=1
fi

if [ -z "${IMSTMPDIR}" ]; then
    IMSTMPDIR="/tmp"
fi

echo "" > "${IMSTMPDIR}"/postUninstallIBMIoTMSServer.log

(
echo "Stopping legacy MessageSight service..."
systemctl stop IBMIoTMessageSightServer

echo "Stopping imaserver process ..."
systemctl stop imaserver

if [ "$POSTUNFLAG" -eq 0 ]
then
    
    #Not deleting user data - if we did then:
    #echo "Deleting ${IMA_SVR_DATA_PATH} server directories if they exist ..."
    #if [ -d ${IMA_SVR_DATA_PATH} ]
    #then
    #    rm -rf ${IMA_SVR_DATA_PATH}/data
    #    rm -rf ${IMA_SVR_DATA_PATH}/diag
    #    rm -rf ${IMA_SVR_DATA_PATH}/store
    #fi   
    
    echo "Deleting systemd unit file(s) if any exist ..."
    # remove old for Systemd unit name
    if [ -f /etc/systemd/system/IBMIoTMessageSightServer.service ]
    then
        rm -f /etc/systemd/system/IBMIoTMessageSightServer.service
    fi
    
    if [ -f /etc/systemd/system/imaserver.service ]
    then
        rm -f /etc/systemd/system/imaserver.service
    fi
    
    echo "Deleting /etc/messagesight directory if it exists ..."
    if [ -d /etc/messagesight ]
    then
        rm -rf /etc/messagesight
    fi

    echo "Deleting /ima dir if it exists ..."
    if [ -d /ima ]
    then
        rm -rf /ima
    fi

    echo "Deleting /store link if it exists ..."
    if [ -L /store ]
    then
        rm -f /store
    fi

    echo "Deleting ${IMA_SVR_INSTALL_PATH} dir if it exists ..."
    if [ -d ${IMA_SVR_INSTALL_PATH} ]
    then
        rm -rf ${IMA_SVR_INSTALL_PATH}
    fi
fi

) | tee -a "${IMSTMPDIR}"/postUninstallIBMIoTMSServer.log
