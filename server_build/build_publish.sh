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
START_DIR=$(pwd)

echo Starting Publish Image in $START_DIR


# Set variables from args
WORKSPACE=$1
export WORKSPACE
RSYNCCMD=$2

HOST=$(hostname)

if [ -n "$DEPS_HOME" ]; then
    echo "DEPS_HOME: $DEPS_HOME"
else
    export DEPS_HOME="/root/deps"
fi

#SLPUBSERVER=184.173.54.133
#HPUBSERVER=9.20.135.205

BRVERSION_ID=${ISM_VERSION_ID/ /.}.${BUILD_LABEL}

echo "Build Label: ${BUILD_LABEL}"
echo "Build Type: ${BLD_TYPE}"
echo "Workspace: ${WORKSPACE}"
echo "ISM_VERSION_ID: ${ISM_VERSION_ID}"
echo "RELEASE: ${IMARELEASE}"
echo "HOST: ${HOST}"
echo "IMARELEASE: ${IMARELEASE}"

if [ "${SHIP_OPENSSL}" = "yes" ] ; then
    # OSS lib dependencies
    # OSS lib dependencies
    export SSL_HOME=/usr/local/openssl
    export CURL_HOME=/usr/local/curl
    export OPENLDAP_HOME=/usr/local/openldap
    export NETSNMP_HOME=/usr/local/netsnmp
    export MONGOC_HOME=/usr/local/mongoc
fi

# Create proxy ship dirs
mkdir -p ${WORKSPACE}/proxy/server_ship/bin
mkdir -p ${WORKSPACE}/proxy/server_ship/debug/bin
mkdir -p ${WORKSPACE}/proxy/server_ship/lib
mkdir -p ${WORKSPACE}/proxy/server_ship/debug/lib
mkdir -p ${WORKSPACE}/proxy/client_ship/lib
mkdir -p ${WORKSPACE}/proxy/server_ship/statsd-C-client

# Copy proxy files to proxy server ship dirs
cp ${WORKSPACE}/server_ship/bin/imaproxy ${WORKSPACE}/proxy/server_ship/bin
cp ${WORKSPACE}/server_ship/debug/bin/imaproxy ${WORKSPACE}/proxy/server_ship/debug/bin
cp ${WORKSPACE}/server_ship/lib/libimaproxy.so ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/debug/lib/libimaproxy.so ${WORKSPACE}/proxy/server_ship/debug/lib
cp ${WORKSPACE}/server_ship/bin/imaproxyapm ${WORKSPACE}/proxy/server_ship/bin
cp ${WORKSPACE}/server_ship/debug/bin/imaproxyapm ${WORKSPACE}/proxy/server_ship/debug/bin
cp ${WORKSPACE}/server_ship/lib/libimaproxyapm.so ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/debug/lib/libimaproxyapm.so ${WORKSPACE}/proxy/server_ship/debug/lib
cp ${WORKSPACE}/server_ship/lib/libismutil.so ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/debug/lib/libismutil.so ${WORKSPACE}/proxy/server_ship/debug/lib
cp ${WORKSPACE}/server_ship/lib/libicudata.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/lib/libicui18n.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/lib/libicuuc.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/client_ship/lib/imaproxy_config.jar ${WORKSPACE}/proxy/client_ship/lib
cp "${DEPS_DIR}/statsd-c-client-*.rpm" "${WORKSPACE}/proxy/server_ship/statsd-C-client"

# statsdclient
if [ -f /usr/lib64/libstatsdclient.so ]; then
  cp --no-preserve=ownership /usr/lib64/libstatsdclient.so ${WORKSPACE}/proxy/server_ship/lib/.
  cp --no-preserve=ownership /usr/lib64/libstatsdclient.so ${WORKSPACE}/server_ship/oss-deps/.
fi

if [ "$SHIP_OPENSSL" = "yes" ] ; then
    # OpenSSL
    if [ -n "$SSL_HOME" -a -e $SSL_HOME/lib ] ; then
        cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 ${WORKSPACE}/proxy/server_ship/debug/lib/.
        cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 ${WORKSPACE}/proxy/server_ship/debug/lib/.
        cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 ${WORKSPACE}/client_ship/lib64/.
        cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 ${WORKSPACE}/client_ship/lib64/.
        cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 ${WORKSPACE}/server_ship/oss-deps/.
        cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 ${WORKSPACE}/server_ship/oss-deps/.
    fi

    # Curl
    if [ -n "$CURL_HOME" -a -e $CURL_HOME/lib ] ; then
        cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 ${WORKSPACE}/proxy/server_ship/debug/lib/.
        cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 ${WORKSPACE}/server_ship/oss-deps/.
    fi

    # Mongo-c-driver
    if [ -n "$MONGOC_HOME" -a -e $MONGOC_HOME/lib ] ; then
        cp --no-preserve=ownership $MONGOC_HOME/lib/libbson-1.0.so.0 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $MONGOC_HOME/lib/libmongoc-1.0.so.0 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $MONGOC_HOME/lib/libbson-1.0.so.0 ${WORKSPACE}/proxy/client_ship/lib/.
        cp --no-preserve=ownership $MONGOC_HOME/lib/libmongoc-1.0.so.0 ${WORKSPACE}/proxy/client_ship/lib/.
        cp --no-preserve=ownership $MONGOC_HOME/lib/libbson-1.0.so.0 ${WORKSPACE}/server_ship/oss-deps/.
        cp --no-preserve=ownership $MONGOC_HOME/lib/libmongoc-1.0.so.0 ${WORKSPACE}/server_ship/oss-deps/.
    fi

    # Openldap libraries
    if [ -n "$OPENLDAP_HOME" -a -e $OPENLDAP_HOME/lib ] ; then
        cp --no-preserve=ownership $OPENLDAP_HOME/lib/liblber-2.4.so.2 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $OPENLDAP_HOME/lib/libldap_r-2.4.so.2 ${WORKSPACE}/proxy/server_ship/lib/.
        cp --no-preserve=ownership $OPENLDAP_HOME/lib/liblber-2.4.so.2 ${WORKSPACE}/server_ship/oss-deps/.
        cp --no-preserve=ownership $OPENLDAP_HOME/lib/libldap_r-2.4.so.2 ${WORKSPACE}/server_ship/oss-deps/.
    fi
fi

# Saving these so we have a copy
cp ${WORKSPACE}/server_ship/lib/libicudata.so.60 ${WORKSPACE}/server_ship/oss-deps
cp ${WORKSPACE}/server_ship/lib/libicui18n.so.60 ${WORKSPACE}/server_ship/oss-deps
cp ${WORKSPACE}/server_ship/lib/libicuuc.so.60 ${WORKSPACE}/server_ship/oss-deps

# Need these for proxy server deployment
cp ${WORKSPACE}/server_ship/lib/libicudata.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/lib/libicui18n.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/lib/libicuuc.so.60 ${WORKSPACE}/proxy/server_ship/lib
cp ${WORKSPACE}/server_ship/lib/libicudata.so.60 ${WORKSPACE}/proxy/server_ship/debug/lib
cp ${WORKSPACE}/server_ship/lib/libicui18n.so.60 ${WORKSPACE}/proxy/server_ship/debug/lib
cp ${WORKSPACE}/server_ship/lib/libicuuc.so.60 ${WORKSPACE}/proxy/server_ship/debug/lib
