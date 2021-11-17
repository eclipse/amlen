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
INITLOG=${LOGDIR}/MQCBridgeInstall.log
touch ${INITLOG}

SVR_INSTALL_DIR=${IMA_SVR_INSTALL_PATH}

echo "--------------------------------------------------"  >> ${INITLOG}
echo "Configure MQCBridge " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}


MODE=$1
STANDALONE=1

if [ "${MODE}" == "serverinstall" ]; 
then
    STANDALONE=0 #running as part of server install => not standalone
fi

    #Work out what user and group we should be running as
    source ${SVR_INSTALL_DIR}/bin/getUserGroup.sh >> ${INITLOG}

    #Set up server writable directories (repeated on startup in containers in case they change)
    source ${SVR_INSTALL_DIR}/bin/initImaserverInstance.sh >> ${INITLOG}
else
    echo "Running as part of standalone bridge install" >> ${INITLOG}
fi

# Unpack the newest MQ client redistributable if one exists
unset -v latestmq
for f in ${SVR_INSTALL_DIR}/mqclient/*MQC-Redist*.tar.gz
do 
    [[ $f -nt $latestmq ]] && latestmq=$f
done
if [ -e "$latestmq" ]
then
    echo "Unpack the MQ client: $latestmq" >>${INITLOG} 2>&1
    tar -xzf "$latestmq" -C ${SVR_INSTALL_DIR}/mqclient >>${INITLOG} 2>&1
    # if we remove this file, we'll get warnings from rpm when we rpm -e
    #/usr/bin/rm "$latestmq" >>${INITLOG} 2>&1
fi

if [ "$STANDALONE" == "1" ];
then
    #Create user+group if necessary and configure us to run as them:
    ${SVR_INSTALL_DIR}/bin/createUserGroup.sh "${IMASERVER_USER}" "${IMASERVER_GROUP}" >> ${INITLOG}
    ${SVR_INSTALL_DIR}/bin/setUserGroup.sh    "${IMASERVER_USER}" "${IMASERVER_GROUP}" >> ${INITLOG}
fi

