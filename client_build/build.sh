#!/bin/sh
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

target=$1
export BUILD_ID=$(whoami)
set -e     #terminate on error
if [ -z ${QUIET_BUILD} ]; then
set -x
   else
    set +x
fi

# Configuring the maximum allowed level of parallelism used by MAKE
# e.g. JOBS=4 means up to 4 concurrent compilations per make target
JOBS=1
if [ $(nproc) -gt 1 ] ; then
    JOBS=$((`nproc` - 1))
    fi

START_DIR=`pwd`
LOG_DIR=${START_DIR}/../build_log
PRJ_NAME=${PWD##*/}
LOG_FILE_NAME=${PRJ_NAME}

echo "Starting build in $START_DIR with target: ( $target )"

if [ "${SHIP_OPENSSL}" == "yes" ] ; then
  # OSS lib dependencies
  export SSL_HOME=/usr/local/openssl
  export CURL_HOME=/usr/local/curl
  export OPENLDAP_HOME=/usr/local/openldap
  export NETSNMP_HOME=/usr/local/netsnmp
  export MONGOC_HOME=/usr/local/mongoc

  echo Building with SSL_HOME=$SSL_HOME
fi

echo 1 > ismrc.txt
{
cd ${START_DIR}

MAKCMD="make -j${JOBS}"
${MAKCMD} ${target}

RC=$?
echo $RC > ismrc.txt

if [ $RC -eq 0 ]
then
    echo Build successful in directory $START_DIR
else
    echo Build failed in directory $START_DIR
fi
} | tee ${LOG_DIR}/${LOG_FILE_NAME}.buildlog 2>@1

RC=`cat ismrc.txt`
exit $RC

