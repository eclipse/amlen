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

mcppath=/opt/mcp/bin
if [[ $# -eq 4 ]] ; then
  mcppath=$3
fi

if [ -z ${FAKEROOT} ]
then
    FAKEROOT=`cat ../server_build/RTCBuild.properties | grep "^FAKEROOT=" | cut -d"=" -f2`
    # If FAKEROOT is not defined in the build definition then set the default for this release
    if [ -z ${FAKEROOT} ] ; then
      FAKEROOT=mcp74_64
    fi
    export FAKEROOT
fi

if [ -z ${USE_RHDEVTOOLS} ]
then
    USE_RHDEVTOOLS=`cat ../server_build/RTCBuild.properties | grep "^USE_RHDEVTOOLS=" | cut -d"=" -f2`
    export USE_RHDEVTOOLS
fi

if [ -z ${ASAN_BUILD} ]
then
    ASAN_BUILD=`cat ../server_build/RTCBuild.properties | grep "^ASAN_BUILD=" | cut -d"=" -f2`
    export ASAN_BUILD
fi

MAKCMD="make"
cd ${START_DIR}

if [ -z ${FAKEROOT} ] ; then
  CHROOT_CMD=
else
  CHROOT_CMD=${mcppath}/mdchroot
fi

if [ "$USE_RHDEVTOOLS" == "yes" ] ; then
  $CHROOT_CMD $FAKEROOT scl enable devtoolset-2 "${MAKCMD} ${target}"
else
  $CHROOT_CMD $FAKEROOT ${MAKCMD} ${target}
fi

RC=$?

if [ $RC -eq 0 ]
then
    echo Build successful in directory $START_DIR
fi

exit $RC

