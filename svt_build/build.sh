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

set -e     #terminate on error
set -x
START_DIR=`pwd`

echo Starting build in $START_DIR

if [ -z ${ASAN_BUILD} ]
then
    ASAN_BUILD=`cat ../server_build/RTCBuild.properties | grep "^ASAN_BUILD=" | cut -d"=" -f2`
    export ASAN_BUILD
fi

if [ -z ${SHIP_OPENSSL} ]
then
    SHIP_OPENSSL=`cat ../server_build/RTCBuild.properties | grep "^SHIP_OPENSSL=" | cut -d"=" -f2`
    export SHIP_OPENSSL
fi

if [ "${SHIP_OPENSSL}" == "yes" ] ; then
  # OSS lib dependencies
  export SSL_HOME=/usr/local/openssl
  export CURL_HOME=/usr/local/curl
  export OPENLDAP_HOME=/usr/local/openldap
  export NETSNMP_HOME=/usr/local/netsnmp
  export MONGOC_HOME=/usr/local/mongoc

  echo Building with SSL_HOME=$SSL_HOME
fi

MAKCMD="make"
cd ${START_DIR}

if [ "$USE_RHDEVTOOLS" == "yes" ] ; then
  scl enable devtoolset-2 "${MAKCMD} ${target}"
else
  ${MAKCMD} ${target}
fi

RC=$?

if [ $RC -eq 0 ]
then
    echo Build successful in directory $START_DIR
fi

exit $RC

