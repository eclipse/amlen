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

function clean_tmpfiles
{
    cd $TMPDIR
    rm -rf "$FILENAME.tmpdir"
    rm -rf "$FILENAME.tmpdir.bz2"
}

FILENAME=$1
export FILENAME

LOGFILE=${IMA_SVR_DATA_PATH}/diag/logs/restore.log
export LOGFILE


echo "" >> ${LOGFILE}
echo "------------------------------------------------------------" >> ${LOGFILE}
echo "" >> ${LOGFILE}
echo "Start restore process: `date` " >> ${LOGFILE}
echo "" >> ${LOGFILE}
echo "Backup file name: ${FILENAME}" >> ${LOGFILE}
echo "" >> ${LOGFILE}

TMPDIR=${IMA_SVR_DATA_PATH}/userfiles
NOW=$(date +"%H%M%S-%m%d%Y")
RSTMPDIR="$TMPDIR/$FILENAME.tmpdir"
cd $TMPDIR

if [[ ! -f "$FILENAME" ]] ; then
    echo "Backup File $FILENAME is missing." >> ${LOGFILE}
    echo "1"
    exit 1
fi

if [[ ! -d "$RSTMPDIR" ]] ; then
    mkdir -p "$RSTMPDIR" >/dev/null 2>&1 3>&1
fi

echo "Decrypt backup file" >> ${LOGFILE}

openssl enc -aes128 -in "$FILENAME" -out "$FILENAME.tmpdir.bz2" -d -a -k "$2" >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    echo "Failed to uncompress $FILENAME." >> ${LOGFILE}
    clean_tmpfiles
    echo "182"
    exit 1
fi

if [[ ! -f "$FILENAME.tmpdir.bz2" ]] ; then
    clean_tmpfiles
    echo "110"
    exit 1
fi

cd "$RSTMPDIR"
tar -xjf "$TMPDIR/$FILENAME.tmpdir.bz2" >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    echo "Failed to untar $FILENAME." >> ${LOGFILE}
    clean_tmpfiles
    echo "108"
    exit 1
fi

if [[ ! -f backup.zip ]] ; then
    sleep 5
    if [[ ! -f backup.zip ]] ; then
        echo "Backup zip file is missing." >> ${LOGFILE}
        echo "2"
        exit 1
    fi
fi

#check the data integrity
md5sum -c backup.zip.md5 >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    echo "$FILENAME data integrity check failed." >> ${LOGFILE}
    clean_tmpfiles
    echo "$(($rc+200))"
    exit 1
fi

echo "$FILENAME data integrity check .... OK." >> ${LOGFILE}

unzip -qq -o backup.zip >> ${LOGFILE} 2>&1 3>&1
rc=$?
if [[ $rc != 0 ]] ; then
    echo "Failed to unzip restore files." >> ${LOGFILE}
    clean_tmpfiles
    echo "$(($rc+100))"
    exit 1
fi


v2config=0

if [[ ! -f $RSTMPDIR/${IMA_SVR_INSTALL_PATH}/config/server_dynamic.cfg ]] ; then
    if [[ ! -f $RSTMPDIR/${IMA_SVR_DATA_PATH}/data/config/server_dynamic.json ]] ; then
        echo "Cannot find the version information of the restore file." >> ${LOGFILE}
        clean_tmpfiles
        echo "13"
        exit 1
    else
        echo "Restoring version 2 configuratopn" >> ${LOGFILE}
        v2config=1
    fi
else
    echo "Restoring version 1 configuratopn" >> ${LOGFILE}
    # adding sed commands to remove options we cannot process
    # this is a stop-gap until I can figure out why the code isn't doing this right
    #sed -i '/EnableAdminServer/d' $RSTMPDIR/${IMA_SVR_INSTALL_PATH}/config/server_dynamic.cfg
    #sed -i '/AuditLogControl/d' $RSTMPDIR/${IMA_SVR_INSTALL_PATH}/config/server_dynamic.cfg
    
    cat $RSTMPDIR/${IMA_SVR_INSTALL_PATH}/config/server_dynamic.cfg | while read LINE
    do
       if [[ $LINE =~ "Server.Version" ]] ; then
          ver=$(echo $LINE | /usr/bin/awk -F'Server.Version = ' '{print $NF}')
          # some versions contain beta, such as 1.2 Beta
          if [[ $ver =~ "Beta" ]] ; then
             VERSION=$(echo $ver | /usr/bin/awk -F' ' '{print $1}')
          else
             VERSION=$ver
          fi

          echo "Backup file version: $VERSION" >> /tmp/restore.log
          echo $VERSION | grep "^1.2" > /dev/null 2>&1 3>&1
          if [ $? -ne 0 ]
          then
              echo "Maybe incompatible MessageSight version?: $VERSION" >> ${LOGFILE}
              echo "Continuing anyway since sometimes these versions are wrong." >> ${LOGFILE}
              #clean_tmpfiles
              #echo "13"
              #exit 1
          fi
       fi
    done
fi


# replace exiting files with backup files
if [[ $v2config == 0 ]] ; then
    echo "Update configuration from V1 data." >> ${LOGFILE}
    cp -r "$RSTMPDIR"/${IMA_SVR_INSTALL_PATH}/certificates ${IMA_SVR_DATA_PATH}/data/. >> ${LOGFILE} 2>&1 3>&1
    rc=$?
else
    echo "Update configuration from V2 data." >> ${LOGFILE}
    cp -r "$RSTMPDIR"/${IMA_SVR_DATA_PATH}/data/certificates ${IMA_SVR_DATA_PATH}/data/. >> ${LOGFILE} 2>&1 3>&1
    rc=$?
    if [[ $rc == 0 ]]
    then
        touch ${IMA_SVR_DATA_PATH}/userfiles/.v2Config
    fi
fi

if [[ $rc != 0 ]] ; then
    echo "Update configuration with restore files.... Failed. rc=$rc" >> ${LOGFILE}
    clean_tmpfiles
    exit 1
fi

# fixing broken trusted cert links after migration from 1.2 per defect 169366
if [[ $v2config == 0 ]] ; then
    server_cfg_file=$(find ${IMA_SVR_DATA_PATH} -name server.cfg)
    perl -pi -e 's/\r\n$/\n/g' $server_cfg_file >> ${LOGFILE} 2>&1 3>&1
    trusted_cert_dir=$(grep TrustedCertificateDir $server_cfg_file | awk '{print $3}')

    echo "Trusted certs for client certs may have broken links when migrated."  >> ${LOGFILE}
    echo "Finding and fixing those links now ..."  >> ${LOGFILE}

    for slink in $(find $trusted_cert_dir -type l -exec test ! -e {} \; -print)
    do
        echo "Found broken link file $slink" >> ${LOGFILE}
        blink=$(readlink -m $slink)
        echo "Broken link was: $blink"  >> ${LOGFILE}
        flink=$(echo $blink | sed "s#${IMA_SVR_INSTALL_PATH}/certificates/truststore#${trusted_cert_dir}#")
        echo "Fixed link is: $flink" >> ${LOGFILE}
        echo "Checking to make sure new link exists" >> ${LOGFILE}

        if [ -f "$flink" ]; then
            echo "New link exists. Continuing ..."  >> ${LOGFILE}
        else
            echo "New link file does not exist.  Skipping this link ..."
            continue
        fi

        echo "Running: rm -f $slink" >> ${LOGFILE}
        mv "$slink" "${slink}.old" >> ${LOGFILE} 2>&1 3>&1
        echo "Adding new link with: ln -s $slink $flink" >> ${LOGFILE}
        ln -s $flink $slink && rm -f "${slink}.old" >> ${LOGFILE} 2>&1 3>&1
    done
fi

#clean the tmp files
rm -rf "$FILENAME"

echo "Restored configuration using file $FILENAME." >> ${LOGFILE}
exit 0


