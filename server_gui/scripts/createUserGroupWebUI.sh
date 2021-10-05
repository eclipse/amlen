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

IMAWEBUI_USERNAME=$1
IMAWEBUI_GROUPNAME=$2

IMAWEBUI_HOMEDIR=${IMA_WEBUI_DATA_PATH}

if ! getent group ${IMAWEBUI_GROUPNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating group ${IMAWEBUI_GROUPNAME}"
    groupadd -f -r ${IMAWEBUI_GROUPNAME}
fi

if ! getent passwd ${IMAWEBUI_USERNAME} >/dev/null ; then
    echo "$SCRIPTNAME Creating user ${IMAWEBUI_USERNAME}"
    useradd -r -g ${IMAWEBUI_GROUPNAME} -d ${IMAWEBUI_HOMEDIR} -s /sbin/nologin -c "User for MessageSight" ${IMAWEBUI_USERNAME}
fi
