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

IMASERVER_USERNAME=$1
IMASERVER_GROUPNAME=$2

IMASERVER_HOMEDIR=${IMA_SVR_DATA_PATH}

if ! getent group ${IMASERVER_GROUPNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating group ${IMASERVER_GROUPNAME}"
    groupadd -f -r ${IMASERVER_GROUPNAME}
fi

if ! getent passwd ${IMASERVER_USERNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating user ${IMASERVER_USERNAME}"
    useradd -r -g ${IMASERVER_GROUPNAME} -d ${IMASERVER_HOMEDIR} -s /sbin/nologin -c "User for MessageSight" ${IMASERVER_USERNAME}
fi
