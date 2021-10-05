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

THISUID=$(id -u)
test $THISUID -eq 0 && echo "Do not run this script as root" && exit 1

# Set variables from args
WORKSPACE=$1
export WORKSPACE
RSYNCCMD=$2

HOST=$(hostname)

#BLDSERVER=9.3.179.145
#PUBSERVER=9.3.179.28
#SLPUBSERVER=${SLPUBSERVER}
#HPUBSERVER=9.20.135.205
SLPUBSERVER="169.61.23.34"

echo "Publish tests to ${PUBLISH_DIR}"
echo PUBLISH_ROOT = ${PUBLISH_ROOT}
echo BUILD_LABEL = ${BUILD_LABEL}
echo WORKSPACE = ${WORKSPACE}
echo PERSONAL_BUILD = ${PERSONAL_BUILD}
echo VIRTUALBUILDSERVER = ${VIRTUALBUILDSERVER}
echo BLD_TYPE = ${BLD_TYPE}
echo DOCKERONLYBUILD = ${DOCKERONLYBUILD}
echo IMARELEASE = ${IMARELEASE}
PUBLISH_ROOT_SAVE=${PUBLISH_ROOT}

if [ ${PERSONAL_BUILD} != "true" ] && [ ${BLD_TYPE} == "development" ]
then
    ${RSYNCCMD} publish/${BUILD_LABEL} ${PUBLISH_DIR}/
    ${RSYNCCMD} --links ${WORKSPACE}/fvt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/fvt
    ${RSYNCCMD} --links ${WORKSPACE}/svt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/svt
    ${RSYNCCMD} --links ${WORKSPACE}/pvt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/pvt
    ${RSYNCCMD} --links ${WORKSPACE}/testTools_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/tools

    # Change some permissions on the local workspace
    sudo -u root chmod 775 ${WORKSPACE}
    sudo -u root find ${WORKSPACE} -type d -print -exec chmod 775 {} \;
    sudo -u root chmod -R 775 ${WORKSPACE}/fvt_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/svt_ship
    # sudo -u root chmod -R 775 ${WORKSPACE}/pvt_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/testTools_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/build_log
fi

if [ ${PERSONAL_BUILD} != "true" ] && [ ${BLD_TYPE} == "production" ]
then
    ${RSYNCCMD} publish/${BUILD_LABEL} ${PUBLISH_DIR}/
    ${RSYNCCMD} --links ${WORKSPACE}/fvt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/fvt
    ${RSYNCCMD} --links ${WORKSPACE}/svt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/svt
    ${RSYNCCMD} --links ${WORKSPACE}/pvt_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/pvt
    ${RSYNCCMD} --links ${WORKSPACE}/testTools_ship/* ${PUBLISH_DIR}/${BUILD_LABEL}/tools

    # Change some permissions on the local workspace
    sudo -u root chmod 775 ${WORKSPACE}
    sudo -u root find ${WORKSPACE} -type d -print -exec chmod 775 {} \;
    sudo -u root chmod -R 775 ${WORKSPACE}/fvt_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/svt_ship
    # sudo -u root chmod -R 775 ${WORKSPACE}/pvt_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/testTools_ship
    sudo -u root chmod -R 775 ${WORKSPACE}/build_log
fi

if [ ${PERSONAL_BUILD} != "true" ]
then
    # Hursley Production Build
    # Note: starting with V2.0, DOCKERONLYBUILD is always true
    # CentOS Austin Development Build
    if [ ${BLD_TYPE} == "development" ]&& [ ${DOCKERONLYBUILD} == "true" ]
    then
        echo PUBLISH_DIR BEFORE = ${PUBLISH_DIR}
        #PUBLISH_DIR=$(echo ${PUBLISH_DIR} | sed s/imadev@9.20.135.205://)
        PUBLISH_DIR=$(echo ${PUBLISH_DIR} | sed s/root@${SLPUBSERVER}://)
        echo PUBLISH_DIR = ${PUBLISH_DIR}
        echo PUBLISH_ROOT BEFORE = ${PUBLISH_ROOT}
        #PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/imadev@9.20.135.205://)
        PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/root@${SLPUBSERVER}://)
        echo PUBLISH_ROOT = ${PUBLISH_ROOT}
        set +e
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_development;rm -f latest;ln -s ${BUILD_LABEL} latest"
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_test;rm -f latest;ln -s ${BUILD_LABEL} latest"
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/development;rm -f latest;ln -s ${BUILD_LABEL} latest"
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/test;rm -f latest;ln -s ${BUILD_LABEL} latest"
        set -e

        # Create build reports and results directories
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_development/${BUILD_LABEL};mkdir -p reports results"
    fi
    # CentOS Hursley Production Build
    if [ ${BLD_TYPE} == "production" ]&& [ ${DOCKERONLYBUILD} == "true" ]
    then
        echo "CentOS Production Build Test Publish"
        #PUBLISH_DIR=$(echo ${PUBLISH_DIR} | sed s/imadev@9.20.135.205://)
        PUBLISH_DIR=$(echo ${PUBLISH_DIR} | sed s/root@${SLPUBSERVER}://)
        echo PUBLISH_DIR = ${PUBLISH_DIR}
        #PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/imadev@9.20.135.205://)
        PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/root@${SLPUBSERVER}://)
        echo PUBLISH_ROOT = ${PUBLISH_ROOT}
        # Create symbolic link "latest" to most recent build
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_production;rm latest;ln -s ${BUILD_LABEL} latest"
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_prod_test;rm latest;ln -s ${BUILD_LABEL} latest"

        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/production;rm latest;ln -s ${BUILD_LABEL} latest"
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/prod_test;rm latest;ln -s ${BUILD_LABEL} latest"
        set -e

        # Create build reports and results directories
        ssh root@${SLPUBSERVER} "cd /gsacache/release/${IMARELEASE}/centos_production/${BUILD_LABEL};mkdir -p reports results"
    fi
fi

# Publish personal build from Personal Virtual Build Server to Softlayer Publishing Server
if [ ${PERSONAL_BUILD} == "true" ] && [ ${VIRTUALBUILDSERVER} == "true" ] && [ ${DOCKERONLYBUILD} == "true" ]
then
	set +e
	PUBLISH_ROOT=${PUBLISH_ROOT_SAVE}
    PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/imadev@9.20.135.205:/root@${SLPUBSERVER}:/)
	${RSYNCCMD} --links ${WORKSPACE}/client_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/fvt_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/svt_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/testTools_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/build_log ${PUBLISH_ROOT}/workspace
	set -e
fi

# Publish personal build from Personal Build Server to Softlayer Publishing Server
if [ ${PERSONAL_BUILD} == "true" ] && [ ${VIRTUALBUILDSERVER} != "true" ] && [ ${DOCKERONLYBUILD} == "true" ]
then
	set +e
	PUBLISH_ROOT=${PUBLISH_ROOT_SAVE}
    PUBLISH_ROOT=$(echo ${PUBLISH_ROOT} | sed s/imadev@9.20.135.205:/root@${SLPUBSERVER}:/)
	${RSYNCCMD} --links ${WORKSPACE}/client_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/fvt_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/svt_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/testTools_ship ${PUBLISH_ROOT}/workspace
	${RSYNCCMD} --links ${WORKSPACE}/build_log ${PUBLISH_ROOT}/workspace
	set -e
fi
