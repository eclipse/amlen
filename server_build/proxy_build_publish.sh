#!/bin/sh
# Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

echo Starting PROXY Publish Image in $START_DIR

THISUID=$(id -u)
test $THISUID -eq 0 && echo "Do not run this script as root" && exit 1

# Set variables from args
WORKSPACE=$1
export WORKSPACE
RSYNCCMD=$2

echo "Publish build to ${PUBLISH_DIR}"
echo "Build Label: ${BUILD_LABEL}"
echo "Build Type: ${BLD_TYPE}"
echo "Workspace: ${WORKSPACE}"
echo "IMA_PROXY_VERSION_ID: ${IMA_PROXY_VERSION_ID}"

${RSYNCCMD} publish/${BUILD_LABEL} ${PUBLISH_DIR}/
${RSYNCCMD} --links ${WORKSPACE}/server_ship ${PUBLISH_DIR}/${BUILD_LABEL}/application
${RSYNCCMD} --links ${WORKSPACE}/client_ship ${PUBLISH_DIR}/${BUILD_LABEL}/application
${RSYNCCMD} --links ${WORKSPACE}/build_log ${PUBLISH_DIR}/${BUILD_LABEL}/application



