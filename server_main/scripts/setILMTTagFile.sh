#!/bin/sh
# Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

LOGFILE=${IMA_SVR_DATA_PATH}/diag/logs/licenseTag.out
export LOGFILE

touch ${LOGFILE}

echo "-----------------------------------" >> ${LOGFILE}
echo "Set ILMT Tag file" >> ${LOGFILE}
echo "Date: `date`" >> ${LOGFILE}
echo "" >> ${LOGFILE}


LTYPE=$1
export LTYPE

VERSION=$2
export VERSION

VERSION=`echo ${VERSION} | sed 's/ /./g'`

if [ "${VERSION}" == "" ] || [ "${LTYPE}" == "" ]
then
    echo "Invalid License ${LTYPE} or Version ${VERSION} is specified." >> ${LOGFILE}
    exit 1
else
    echo "License: ${LTYPE}" >> ${LOGFILE}
    echo "Version: ${VERSION}" >> ${LOGFILE}
fi


# Create ILMT ISO based tag directory
mkdir -p ${IMA_SVR_INSTALL_PATH}/swidtag >> ${LOGFILE}
# rm -f ${IMA_SVR_INSTALL_PATH}/swidtag/* >> ${LOGFILE}

if [ "${LTYPE}" == "PROD" ]
then
    echo "Copy *Server-*.swidtag to swidtag dir" >> ${LOGFILE}
    cp ${IMA_SVR_INSTALL_PATH}/config/*Server-*.swidtag ${IMA_SVR_INSTALL_PATH}/swidtag/.
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server_Non_Production-*.swidtag >> ${LOGFILE}
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server_Idle_Standby-*.swidtag >> ${LOGFILE}
elif [ "${LTYPE}" == "NONPROD" ]
then
    echo "Copy *Server_Non-Production-*.swidtag to swidtag dir" >> ${LOGFILE}
    cp ${IMA_SVR_INSTALL_PATH}/config/*Server_Non_Production-*.swidtag ${IMA_SVR_INSTALL_PATH}/swidtag/.
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server-*.swidtag >> ${LOGFILE}
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server_Idle_Standby-*.swidtag >> ${LOGFILE}
elif [ "${LTYPE}" == "STANDBY" ]
then
    echo "Copy *Server_Idle_Standby-*.swidtag to swidtag dir" >> ${LOGFILE}
    cp ${IMA_SVR_INSTALL_PATH}/config/*Server_Idle_Standby-*.swidtag ${IMA_SVR_INSTALL_PATH}/swidtag/.
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server-*.swidtag >> ${LOGFILE}
    rm ${IMA_SVR_INSTALL_PATH}/swidtag/*Server_Non-Production-*.swidtag >> ${LOGFILE}
elif [ "${LTYPE}" == "DEVELOPERS" ]
then
    echo "No license files copied, existing files removed" >> ${LOGFILE}
    rm -f ${IMA_SVR_INSTALL_PATH}/swidtag/* >> ${LOGFILE}
fi


exit 0


