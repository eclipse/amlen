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
UNITFILE="${IMA_BRIDGE_INSTALL_PATH}/config/imabridge.service"

SCRIPTNAME=$(basename $0)

if [ "$#" -eq "1" ] && [ "$1" == "preset" ]
then
    source ${IMA_BRIDGE_INSTALL_PATH}/bin/getUserGroup.sh
    USERNAME=$IMABRIDGE_USER
    GROUPNAME=$IMABRIDGE_GROUP
elif [ "$#" -eq "2" ]
then
    USERNAME=$1
    GROUPNAME=$2
else
    echo "Usage: $SCRIPTNAME <username> <groupname>"
    echo "   or: $SCRIPTNAME preset"
    exit 10
fi


echo "$SCRIPTNAME: Updating file/dir ownerships so that MessageSight Bridge can be run as user $USERNAME and group $GROUPNAME"

chown -R $USERNAME:$GROUPNAME ${IMA_BRIDGE_DATA_PATH}
chown -R $USERNAME:$GROUPNAME ${IMA_BRIDGE_INSTALL_PATH}
chown -fR $USERNAME:$GROUPNAME /dev/shm/com.ibm.messagesight.imabridge*

LOCKFILES="/tmp/imaextractstackfromcore_bridge.lock /tmp/imabridge.lock"

#None of the lock files should exist whilst this script is run. We could chown them. Instead we issue an
#error, remove the file and continue
for lf in ${LOCKFILES}
do
    if [ -e "${lf}" ]
    then
        echo "$SCRIPTNAME: Lockfile ${lf} exists. Removing"
        rm ${lf}
    fi
done


#Now update the user and group in the systemd unit file
echo "$SCRIPTNAME: Updating $UNITFILE for user $USERNAME and group $GROUPNAME"

FOUNDGROUP=$(grep -ciE "^group=" $UNITFILE)
if [ "$FOUNDGROUP" -eq "0" ]
then
     sed -i -e '/^\[Service\]/a\'  -e 'Group='"$GROUPNAME" $UNITFILE
else
     sed -i -r 's/group=.*/Group='"$GROUPNAME"'/I' $UNITFILE
fi

FOUNDUSER=$(grep -ciE "^user=" $UNITFILE)
if [ "$FOUNDUSER" -eq "0" ]
then
     sed -i -e '/^\[Service\]/a\'  -e 'User='"$USERNAME" $UNITFILE
else
     sed -i -r 's/user=.*/User='"$USERNAME"'/I' $UNITFILE
fi

echo "$SCRIPTNAME: Complete"
