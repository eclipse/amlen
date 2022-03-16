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

# Create install log file
LOGDIR=${IMA_SVR_DATA_PATH}/diag/logs
mkdir -p -m 770 ${LOGDIR}
chmod -R 770 ${LOGDIR}
INITLOG=${LOGDIR}/MessageSightInstall.log
touch ${INITLOG}

SVR_INSTALL_DIR=${IMA_SVR_INSTALL_PATH}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure MessageSight Server " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}

#Work out what user and group we should be running as
source ${SVR_INSTALL_DIR}/bin/getUserGroup.sh >> ${INITLOG}

#Set up server writable directories (repeated on startup in containers in case they change)
source ${SVR_INSTALL_DIR}/bin/initImaserverInstance.sh >> ${INITLOG}

# Additional required dir and links that need root-level access to create
mkdir -p -m 770 /tmp/userfiles >> ${INITLOG} 2>&1

cd /tmp >> ${INITLOG} 2>&1
if [ ! -L /tmp/cores ]
then
    ln -s ${COREDIR} cores >> ${INITLOG} 2>&1
fi

cd / >> ${INITLOG} 2>&1
if [ ! -L store ]
then
    ln -s ${STOREDIR} /store >> ${INITLOG} 2>&1
fi

# Copy imaserver logrotate configuration file if logrotate is enabled
if [ -d /etc/logrotate.d ]
then
    if [ ! -f /etc/logrotate.d/imaserver ]
    then
        cp ${SVR_INSTALL_DIR}/config/imaserver.logrotate /etc/logrotate.d/imaserver >> ${INITLOG}
        perl -pi -e 's/\r\n$/\n/g' /etc/logrotate.d/imaserver >> ${INITLOG}
    fi 
fi

#Do MQBridge related setup (in case we're bundling MQ Bridge)
${SVR_INSTALL_DIR}/bin/postInstallMQCBridge.sh serverinstall

# Unpack the newest IBM Java JRE TGZ file
unset -v latestjava
for f in ${SVR_INSTALL_DIR}/ibm-java-jre-8.0-*.tgz
do 
    [[ $f -nt $latestjava ]] && latestjava=$f
done
if [ -e "$latestjava" ]
then
    echo "Unpack IBM Java JRE: $latestjava" >>${INITLOG} 2>&1 
    tar -xzf "$latestjava" -C ${SVR_INSTALL_DIR} >>${INITLOG} 2>&1
    # if we remove this file, we'll get warnings from rpm when we rpm -e
    #/usr/bin/rm "$latestjava" >>${INITLOG} 2>&1
fi

# check and add snmp libs
if [ -f ${SVR_INSTALL_DIR}/lib64/netsnmp.tar ]
then
    cd ${SVR_INSTALL_DIR}/lib64
    tar xvf netsnmp.tar > /dev/null 2>&1
    rm -f netsnmp.tar > /dev/null 2>&1
fi

# system tuning
mkdir -p -m 750 /etc/messagesight
if [ ! -f /etc/messagesight/MessageSightInstance.inited ]
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
    touch /etc/messagesight/MessageSightInstance.inited
fi

# Now that common libs (e.g. libcurl and libldap) are shipped with MessageSight we can no longer expose the 
# /opt/ibm/imaserver/lib64 directory as a common library path.  It breaks other system commands (e.g. yum)
#            # Add IMA lib path to ld config
#            if [ ! -f /etc/ld.so.conf.d/ima.conf ]
#            then
#                echo "/opt/ibm/imaserver/lib64" > /etc/ld.so.conf.d/ima.conf
#            fi
#            ldconfig -v > /dev/null 2>&1
if [ -f /etc/ld.so.conf.d/ima.conf ] ; then
    rm -f /etc/ld.so.conf.d/ima.conf
    ldconfig -v > /dev/null 2>&1
fi

#Create user+group if necessary and configure us to run as them:
${SVR_INSTALL_DIR}/bin/createUserGroup.sh "${IMASERVER_USER}" "${IMASERVER_GROUP}" >> ${INITLOG}
${SVR_INSTALL_DIR}/bin/setUserGroup.sh    "${IMASERVER_USER}" "${IMASERVER_GROUP}" >> ${INITLOG}

#
# In systemd environments, install imaserver systemd unit file
#
if [ -d "/etc/systemd/system" ]
then    
    # Update systemd (including removing old service name)
    rm -f /etc/systemd/system/IBMIoTMessageSightServer.service
    cp ${SVR_INSTALL_DIR}/config/imaserver.service /etc/systemd/system/.
    perl -pi -e 's/\r\n$/\n/g' /etc/systemd/system/imaserver.service

    INIT_SYSTEM=$(ps --no-headers -o comm 1)
    if [ "$INIT_SYSTEM" == "systemd" ]
    then
        systemctl daemon-reload
    fi
fi

#
# SLES Specific Install code
#
if [ -f "/etc/os-release" ]; then
    . /etc/os-release
    OS="$ID"

    if [ "$OS" = "sles" ]; then
       sed -i '/^AssertPathExists/d' /etc/systemd/system/imaserver.service
       if [ ! -e "/etc/pki/tls" ]
       then
            # Added so that our custom version of openssl finds system trust dir
            # our custom openssl build sets openssldir to /etc/pki/tls
            ln -s /etc/ssl /etc/pki/tls
       else
            if [ -L /etc/pki/tls ]; then
                echo "System trust directory already linked."
            else
                echo "Could not link to system trust directory."
            fi
       fi
    fi
fi


