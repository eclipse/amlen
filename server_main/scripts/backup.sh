#!/bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

# set -x

FILENAME=$1

LOGFILE=/var/messagesight/diag/logs/backup.log
export LOGFILE


echo "--------------------" >> ${LOGFILE}
echo "Backup Date: `date`" >> ${LOGFILE}
echo "Backup File: $FILENAME" >> ${LOGFILE}
echo "" >> ${LOGFILE}

TMPDIR=/tmp/userfiles
NOW=$(date +"%H%M%S-%m%d%Y")

#echo $NOW
mkdir -p $TMPDIR/$FILENAME.tmpdir >> ${LOGFILE} 2>&1 3>&1
cd $TMPDIR/$FILENAME.tmpdir >/dev/null 2>&1 3>&1

echo "/var/messagesight/data/config/*" > /tmp/imaBackup.lst
echo "/var/messagesight/data/certificates/*" >> /tmp/imaBackup.lst

zip -r -q -y backup /var -i@/tmp/imaBackup.lst >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    cd $TMPDIR
    rm -rf $TMPDIR/$FILENAME.tmpdir >/dev/null 2>&1 3>&1
    echo "$(($rc+100))"
    exit 1
fi

md5sum backup.zip > backup.zip.md5
rc=$?
if [[ $rc != 0 ]] ; then
    cd $TMPDIR
    rm -rf $TMPDIR/$FILENAME.tmpdir >/dev/null 2>&1 3>&1
    echo "$(($rc+200))"
    exit 1
fi

tar -cj backup.* | openssl enc -aes128 -salt -out $TMPDIR/$FILENAME -e -a -k "$2" >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    cd $TMPDIR
    rm -rf $TMPDIR/$FILENAME.tmpdir >/dev/null 2>&1 3>&1
    echo 114
    exit 1
fi


cd $TMPDIR
rm -rf $TMPDIR/$FILENAME.tmpdir >/dev/null 2>&1 3>&1

echo "0"
exit 0

