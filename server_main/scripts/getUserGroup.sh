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

#Set default user if not sepecified
IMASERVER_USER=amlen
IMASERVER_GROUP=$IMASERVER_USER

#File to read user and group from:
IMASERVER_USERINFO_FILE='/etc/messagesight-user.cfg'

SCRIPTNAME="getUserGroup.sh"

#Validation Regular expression for User and Group
VALIDATION_REGEXP="^[a-z][-a-z0-9]{0,31}\$"

if [ -f $IMASERVER_USERINFO_FILE ]
then
    USERFROMFILE=$(sed -n 's/^user=//pI' $IMASERVER_USERINFO_FILE| head -n 1)
    
    if [ ! -z "$USERFROMFILE" ]
    then
        if [[ $USERFROMFILE =~ $VALIDATION_REGEXP ]]
        then
            IMASERVER_USER=$USERFROMFILE
        
            #Only get the group from the file if there was a valid user
            #if no group info default to the name of the user
            GROUPFROMFILE=$(sed -n 's/^group=//pI' $IMASERVER_USERINFO_FILE| head -n 1)
    
            if [ ! -z "$GROUPFROMFILE" ]
            then
                if [[ $GROUPFROMFILE =~ $VALIDATION_REGEXP ]]
                then
                    IMASERVER_GROUP=$GROUPFROMFILE
                else
                    echo "$SCRIPTNAME: Group in  $IMASERVER_USERINFO_FILE was invalid. Defaulting group to be same as user."
                    IMASERVER_GROUP=$IMASERVER_USER
                fi
            else
                echo "$SCRIPTNAME: No valid group in  $IMASERVER_USERINFO_FILE. Defaulting group to be same as user."
                IMASERVER_GROUP=$IMASERVER_USER
            fi
        else
            echo "$SCRIPTNAME:User in $IMASERVER_USERINFO_FILE was invalid. Using default user and group."
        fi
    else
        echo "$SCRIPTNAME:No valid user in  $IMASERVER_USERINFO_FILE. Using default user and group."
    fi
else
    echo "$SCRIPTNAME:No userdata file: $IMASERVER_USERINFO_FILE. Using default user and group."
fi

echo "$SCRIPTNAME:  User is $IMASERVER_USER"
echo "$SCRIPTNAME: Group is $IMASERVER_GROUP"
