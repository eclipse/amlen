#!/bin/bash

#if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
#    echo "Build is a Pull Request.  No build notification is required for PRs"
#    exit 0
#fi

if [ -n "$1" ]; then
    export BUILD_LABEL="$1"
else
    echo "Create fake build label for AFharness ..."
    export BUILD_LABEL=$(date +%Y%m%d-%H%M)
fi

RPM_BUILD_LABEL=$(echo $BUILD_LABEL | sed 's/-/./')

# Check the git commit message to see if developer requested an "asanbuild"
if [[ $TRAVIS_COMMIT_MESSAGE == *"ASANBUILD"* || $TRAVIS_COMMIT_MESSAGE == *"asanbuild"* ]]; then
    echo "A build of MessageGateway servers with ASAN was requested"
    docker exec -e BUILD_LOG=$BUILD_LOG -e BUILD_LABEL=$BUILD_LABEL -e ISM_VERSION_ID=$ISM_VERSION_ID -e SLESNORPMS=yes -it mgbld bld_servers_asan -b $BUILD_LABEL || exit $?
    set -x
    mkdir ${TRAVIS_BUILD_DIR}/ship_asan
    docker cp mgbld:/ship/asanlibs-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz "${TRAVIS_BUILD_DIR}/ship_asan" || exit $?
    docker cp mgbld:/ship/rpms/IBMWIoTPMessageGatewayServer-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm "${TRAVIS_BUILD_DIR}/ship_asan"
    docker cp mgbld:/ship/rpms/IBMWIoTPMessageGatewayBridge-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm "${TRAVIS_BUILD_DIR}/ship_asan"
    docker cp mgbld:/ship/rpms/IBMWIoTPMessageGatewayProxy-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm "${TRAVIS_BUILD_DIR}/ship_asan"
    docker cp mgbld:/ship/build-logs-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz "${TRAVIS_BUILD_DIR}/ship_asan/build-logs-${ISM_VERSION_ID}-${BUILD_LABEL}.asan.tar.gz"
fi
