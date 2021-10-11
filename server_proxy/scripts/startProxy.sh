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


_term() {
    trap - SIGTERM
    echo "SIGTERM in startProxy"
    LOOP=0
    kill -TERM "$proxy" 2>/dev/null
    wait "$proxy"
}

PXY_INSTALL_DIR=${IMA_PROXY_INSTALL_PATH}
PXY_DATA_DIR=${IMA_PROXY_DATA_PATH}

# check if running in a docker container
isDocker=0
if [ -f /.dockerenv ]
then
    isDocker=1
fi

mkdir -p -m 770 ${PXY_DATA_DIR}/diag/logs
INITLOG=${PXY_DATA_DIR}/diag/logs/imaproxy_init.log
export INITLOG

exec 200> /tmp/imaproxy.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "IBM IoT MessageSight proxy process is already running." >&2
    exit 255
fi

echo "" >> ${INITLOG}
echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo "Starting Proxy" >> ${INITLOG}
echo "Date: $(date) " >> ${INITLOG}

# Initialize imaproxy service for docker container if running in a container
if [ $isDocker -eq 1 ]
then
    ${PXY_INSTALL_DIR}/bin/initProxy.sh
fi

# Start imaproxy
echo "Start imaproxy" >> ${INITLOG}

LOOP=1

while [ $LOOP -gt 0 ];
do
    # Start service
    echo "Running proxy"
    cd ${PXY_DATA_DIR}
    ${PXY_INSTALL_DIR}/bin/imaproxy -d >>${PXY_DATA_DIR}/diag/logs/console.log 2>&1 &
    proxy=$!
    trap _term SIGTERM
    wait "$proxy"

    if [ "$?" = "2" ]
    then
        LOOP=0
        ${PXY_INSTALL_DIR}/bin/extractstackfromcore.sh
        exit 0
    fi

    if [ $LOOP -eq 0 ]
    then
        # Extract stack trace if previous run failed for any reason
        ${PXY_INSTALL_DIR}/bin/extractstackfromcore.sh
        echo "startProxy terminated" $?
        exit 15
    fi

    # Extract stack trace if previous run failed for any reason
    ${PXY_INSTALL_DIR}/bin/extractstackfromcore.sh
done

# Extract stack trace if previous run failed for any reason
${PXY_INSTALL_DIR}/bin/extractstackfromcore.sh

