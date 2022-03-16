#!/bin/sh
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

SERVERTYPE=$1

LOGFILE="${IMA_SVR_DATA_PATH}/diag/logs/syncCerts.log"
CERTTAR="${IMA_SVR_DATA_PATH}/data/hasync/certdir.tar"


if [[ $SERVERTYPE = "primary" ]]
then

    date > $LOGFILE
    echo "Backup files on Primary" >> $LOGFILE
    cd ${IMA_SVR_DATA_PATH}/data
    /bin/tar -czv -f ${CERTTAR} certificates >> $LOGFILE 2>&1    
   
else

    echo >> $LOGFILE
    date >> $LOGFILE
    echo "Restore file on Standby" >> $LOGFILE
    
    cd ${IMA_SVR_DATA_PATH}/data
    mv certificates certificates.org
    tar xhvf ${CERTTAR} >> $LOGFILE 2>&1
    if [ -d ${IMA_SVR_DATA_PATH}/data/certificates ]
    then
        rm -rf certificates.org >> $LOGFILE 2>&1
    else
        mv certificates.org certificates >> $LOGFILE 2>&1
    fi
    
    # copy config file transferred from v1 system to config dir
    if [ -d ${IMA_SVR_DATA_PATH}/data/ima ]
    then
        echo "Process v1 configuration" >> $LOGFILE 2>&1
        mkdir -p ${IMA_SVR_DATA_PATH}/data/config >> $LOGFILE 2>&1
        cp ${IMA_SVR_DATA_PATH}/data/ima/config/server_dynamic.cfg ${IMA_SVR_DATA_PATH}/data/config/. >> $LOGFILE 2>&1
        cp ${IMA_SVR_DATA_PATH}/data/ima/config/mqcbridge_dynamic.cfg ${IMA_SVR_DATA_PATH}/data/config/. >> $LOGFILE 2>&1
    fi

    # check if certs are transferred from v1 system
    if [ -d ${IMA_SVR_DATA_PATH}/data/${IMA_SVR_INSTALL_PATH}/certificates ]
    then
        cp -r ${IMA_SVR_DATA_PATH}/data/${IMA_SVR_INSTALL_PATH}/certificates ${IMA_SVR_DATA_PATH}/data/. >> $LOGFILE 2>&1
    fi

    # delete unwanted dirs transferred from v1 system
    if [ -d ${IMA_SVR_DATA_PATH}/data/home ]
    then
        rm -r ${IMA_SVR_DATA_PATH}/data/home >> $LOGFILE 2>&1
        rm -r ${IMA_SVR_DATA_PATH}/data/etc >> $LOGFILE 2>&1
        rm -r ${IMA_SVR_DATA_PATH}/data/opt >> $LOGFILE 2>&1
        rm -r ${IMA_SVR_DATA_PATH}/data/ima >> $LOGFILE 2>&1
    fi

fi

