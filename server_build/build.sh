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

target="$1"
export BUILD_ID=$(whoami)

# export QUIET_BUILD=1
# BUILD_LOG is now being passed into the docker container as an env
# export BUILD_LOG="../messagegateway-build.log"

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
    
    if [ ${JOBS} -gt ${AMLEN_MAX_BUILD_JOBS} ] ; then
        JOBS=${AMLEN_MAX_BUILD_JOBS}
    fi
fi

START_DIR=`pwd`
LOG_DIR=${START_DIR}/../build_log
PRJ_NAME=${PWD##*/}
LOG_FILE_NAME=${PRJ_NAME}

echo "Starting build in $START_DIR with target: ( $target ) - GCC_HOME=$GCC_HOME ASAN_BUILD=$ASAN_BUILD"
# Echo which version of GCC we are building with for each target
if [ -z $GCC_HOME ] ; then
    echo "GCC version: $(gcc --version | head -n 1)"
else
    echo "GCC version: $(${GCC_HOME}/bin/gcc --version | head -n 1)"
fi
# CUnit Test Update for IMADev
# Do to new build paradigm we are switching cunit to fulltest for development builds
#
# Set CUNIT_TYPE to test or fulltest depending on BLD_TYPE
# Development build will only run partial CUNIT Tests
# Production build will run full CUNIT Tests

if [ "${target}" == "cunit" ]
then
   # several problems were found with concurrent CUNIT tests (for now CUNIT will still be serial)
   JOBS=1
   export CUNIT_TARGET=fulltest
   target=${CUNIT_TARGET}
   LOG_FILE_NAME=${PRJ_NAME}_cunit
   export LD_LIBRARY_PATH=$(pwd)/../server_ship/lib:$LD_LIBRARY_PATH
elif [ "${target}" == "coverage" ]
then
   JOBS=1
   export CUNIT_TARGET=coveragefulltest
   target=${CUNIT_TARGET}
   LOG_FILE_NAME=${PRJ_NAME}_coverage
   export LD_LIBRARY_PATH=$(pwd)/../server_ship/lib:$LD_LIBRARY_PATH
fi

if [ "${SHIP_OPENSSL}" == "yes" ] ; then
  # OSS lib dependencies
  export SSL_HOME="/usr/local/openssl"
  export CURL_HOME="/usr/local/curl"
  export OPENLDAP_HOME="/usr/local/openldap"
  export NETSNMP_HOME="/usr/local/netsnmp"
  export MONGOC_HOME="/usr/local/mongoc"

  export LD_LIBRARY_PATH=${SSL_HOME}/lib:${CURL_HOME}/lib:${OPENLDAP_HOME}/lib:${NETSNMP_HOME}/lib:${MONGOC_HOME}/lib64:$LD_LIBRARY_PATH

  echo Building with SSL_HOME=$SSL_HOME
fi

if [ -n "$BLD_ENGINE" ]; then
  QUALIFY_SHARED="$BLD_ENGINE"
else
  QUALIFY_SHARED="$USER.${$}"
fi

# Do we want a quiet build
if [ -z ${QUIET_BUILD} ]; then
    MAKCMD="make -j${JOBS}"
else
    MAKCMD="make -s -j${JOBS}"
fi

if [ "${target}" == "coveragefulltest" ]; then
    if [ -d "../static_analysis_ext" ]; then
        # Wrap the make command in the SonarQube wrapper that collects details needed by SonarQube.
        MAKCMD="build-wrapper-linux-x86-64 --out-dir bw_output ${MAKCMD}"
        export PATH=../static_analysis_ext/sonar-build-wrapper:$PATH
        mkdir -p bw_output
    fi
fi

echo 1 > ismrc.txt
{
# Prevent this group command from terminating if make ends with non-zero rc
set +e
cd ${START_DIR}
echo "Changing to directory $START_DIR to build component."

##### Invoke the make command here #####
if [ -n "$BUILD_LOG" ]; then
    #exec 3>&1 
    #exec > >(tee -a ${BUILD_LOG} >/dev/null) 2> >(tee -a ${BUILD_LOG} >&3)
    # ${MAKCMD} ${target} >> "$BUILD_LOG"
    if [ "$ASAN_BUILD" == "yes" ] ; then
        # until we can clean up compiler warnings (with gcc9) we cannot tee stderr without breaking the travis build (exceed 4MB log size)
        ${MAKCMD} ${target} >> $BUILD_LOG 2>&1
    else
        ${MAKCMD} ${target} >> $BUILD_LOG 2> >(tee -a $BUILD_LOG >&2 | tail -n 200)
    fi
else
    ${MAKCMD} ${target}
fi

RC=$?
echo $RC > ismrc.txt

if [ $RC -eq 0 ]
then
    echo Build successful in directory $START_DIR
else
    echo Build failed in directory $START_DIR
fi

if [ "${target}" == "coveragefulltest" ]; then
    if [ -d "../static_analysis_ext" ]; then
        ../static_analysis_ext/run-sonar-scanner.sh c ${PRJ_NAME} ${ISM_VERSION}.${BUILD_LABEL}
        RC=$?
        if [ $RC -ne 0 ]; then
            echo SonarQube scanner failed
            echo $RC > ismrc.txt
        fi
    fi
fi

} 2>&1 | tee ${LOG_DIR}/${LOG_FILE_NAME}.buildlog 2>&1

RC=`cat ismrc.txt`
exit $RC

