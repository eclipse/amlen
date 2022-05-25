#!/bin/bash
# Copyright (c) 2018-2022 Contributors to the Eclipse Foundation
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

#Works out whether other users in the group should have read/write/no access to
#our data files
# The setting is groupaccess=(None|LimitedRead|FullRead|FullWrite) in
# /etc/messagesight-user.cfg
# After sourcing this script the follows vars will be exported:
# ${IMA_DEFAULT_PERMS} = 700 (None or LimitedRead - default) 740 (FullRead) 770 (FullWrite)
# ${IMA_DIAGCFG_PERMS} = 700 (None) 740 (LimitedRead - default, FullRead) 770 (FullWrite)


#File to read user and group from:
IMASERVER_USERINFO_FILE='/etc/messagesight-user.cfg'

SCRIPTNAME="getFilePermission.sh"

#By default we use LimitedRead (which gives group read access to diag/config)
export IMA_DEFAULT_PERMS=700
export IMA_DIAGCFG_PERMS=740

if [ -f $IMASERVER_USERINFO_FILE ]
then
    GROUPACCESSFROMFILE=$(sed -n 's/^groupaccess=//pI' $IMASERVER_USERINFO_FILE| head -n 1)
    
    if [ ! -z "$GROUPACCESSFROMFILE" ]
    then
        if [[ "${GROUPACCESSFROMFILE,,}" == "fullwrite" ]]
        then
            #Group has full write access - useful for OpenShift
            export IMA_DEFAULT_PERMS=770
            export IMA_DIAGCFG_PERMS=770
            IMASERVER_USER=$USERFROMFILE
        elif [[ "${GROUPACCESSFROMFILE,,}" == "fullread" ]]
        then
            export IMA_DEFAULT_PERMS=740
            export IMA_DIAGCFG_PERMS=740
        elif [[ "${GROUPACCESSFROMFILE,,}" == "limitedread" ]]
        then
            #This is the default - no action required
            echo "$SCRIPTNAME:GroupAccess is set to LimitedRead - the default"
        elif [[ "${GROUPACCESSFROMFILE,,}" == "none" ]]
        then
            export IMA_DEFAULT_PERMS=700
            export IMA_DIAGCFG_PERMS=700
        else
            echo "$SCRIPTNAME:GroupAccess in $IMASERVER_USERINFO_FILE was invalid. Using default file permissions."
        fi
    else
        echo "$SCRIPTNAME:No valid GroupAccess in  $IMASERVER_USERINFO_FILE. Using default file permissions."
    fi
else
    echo "$SCRIPTNAME:No userdata file: $IMASERVER_USERINFO_FILE. Using default file permissions."
fi

echo "$SCRIPTNAME:  Default permissions are ${IMA_DEFAULT_PERMS}"
echo "$SCRIPTNAME:  Diagnostics and Config permissions are ${IMA_DIAGCFG_PERMS}"
