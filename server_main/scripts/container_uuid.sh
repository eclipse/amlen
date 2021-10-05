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

# HOSTNAME environment variable is set by docker engine. It is set to
# short UUID of the container.
CONTAINER_UUID=$HOSTNAME

# If HOSTNAME is not set (could be possible depending on docker version)
# then get container UUID from cgroup
if [ "${CONTAINER_UUID}" == "" ]
then
    # get container UUID from cgroup
    cat /proc/self/cgroup | grep "memory:/docker/" > /dev/null 2>&1 3>&1
    if [ $? -eq 0 ]
    then
        CONTAINER_UUID=`cat /proc/self/cgroup | grep "memory:/docker/" | cut -d "/" -f3 | cut -c1-12`
    fi
fi

# If it is not possible to get container UUID from cgroup,
# then generate a random UUID. Prifix UUID with IMA_ to
# identify that UUID is a generated one.
if [ "${CONTAINER_UUID}" == "" ]
then
    # generate a random UUID
    ID=`cat /proc/sys/kernel/random/uuid | cut -d"-" -f1`
    export ID
    CONTAINER_UUID="IMA_"${ID}
fi

export CONTAINER_UUID

echo ${CONTAINER_UUID}

