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
    echo "SIGTERM in startBridge"
    LOOP=0
    kill -TERM "$bridge" 2>/dev/null
    wait "$bridge"
}

# check if running in a docker container
isDocker=0
if [ -f /.dockerenv ]
then
    isDocker=1
fi

mkdir -p -m 770 ${IMA_BRIDGE_DATA_PATH}/diag/logs
INITLOG=${IMA_BRIDGE_DATA_PATH}/diag/logs/imabridge_init.log
export INITLOG

exec 200> /tmp/imabridge.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "IBM IoT MessageSight bridge process is already running." >&2
    exit 255
fi

echo "" >> ${INITLOG}
echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo "START IBM IoT MessageSight Bridge" >> ${INITLOG}
echo "Date: $(date) " >> ${INITLOG}

# Initialize imabridge service for docker container if running in a container
if [ $isDocker -eq 1 ]
then
    ${IMA_BRIDGE_INSTALL_PATH}/bin/initBridge.sh
fi

# Start imabridge
echo "Start imabridge" >> ${INITLOG}

LOOP=1

while [ $LOOP -gt 0 ];
do
    # Start service
    echo "Running bridge"
    cd ${IMA_BRIDGE_DATA_PATH}
    ${IMA_BRIDGE_INSTALL_PATH}/bin/imabridge -d >>${IMA_BRIDGE_DATA_PATH}/diag/logs/console.log 2>&1 &
    bridge=$!
    trap _term SIGTERM
    wait "$bridge"

    if [ "$?" = "2" ]
    then
        LOOP=0
        ${IMA_BRIDGE_INSTALL_PATH}/bin/extractstackfromcore.sh
        exit 0
    fi

    if [ $LOOP -eq 0 ]
    then
        # Extract stack trace if previous run failed for any reason
        ${IMA_BRIDGE_INSTALL_PATH}/bin/extractstackfromcore.sh
        echo "startBridge terminated" $?
        exit 15
    fi
    
    # Extract stack trace if previous run failed for any reason
    ${IMA_BRIDGE_INSTALL_PATH}/bin/extractstackfromcore.sh
done

# Extract stack trace if previous run failed for any reason
${IMA_BRIDGE_INSTALL_PATH}/bin/extractstackfromcore.sh

