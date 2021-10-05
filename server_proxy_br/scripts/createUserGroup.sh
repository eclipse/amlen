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
SCRIPTNAME=$(basename $0)

if [ "$#" -ne "2" ]
then
    echo "Usage: $SCRIPTNAME <username> <groupname>"
    exit 10
fi

IMABRIDGE_USERNAME=$1
IMABRIDGE_GROUPNAME=$2

IMABRIDGE_HOMEDIR=${IMA_BRIDGE_DATA_PATH}

if ! getent group ${IMABRIDGE_GROUPNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating group ${IMABRIDGE_GROUPNAME}"
    groupadd -f -r ${IMABRIDGE_GROUPNAME}
fi

if ! getent passwd ${IMABRIDGE_USERNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating user ${IMABRIDGE_USERNAME}"
    useradd -r -g ${IMABRIDGE_GROUPNAME} -d ${IMABRIDGE_HOMEDIR} -s /sbin/nologin -c "User for MessageSight Bridge" ${IMABRIDGE_USERNAME}
fi
