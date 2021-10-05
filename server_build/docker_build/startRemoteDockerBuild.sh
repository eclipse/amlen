#!/bin/bash
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

# Validate number of args
if [ $# -lt 6 ]
then
    echo "Usage: startRemoteDockerBuild.sh <ReleaseName> <BuileType> <BuildLevel> <ContName> <version> <build_root>"
    exit 1
fi


RELEASE=$1
export RELEASE
BLDTYPE=$2
export BLDTYPE
BLDLEVEL=$3
export BLDLEVEL
CONTNAME=$4
export CONTNAME
VERSION=$5
export VERSION
BLDROOT=$6
export BLDROOT

CURDIR=`pwd`
export CURDIR

echo "Release: ${RELEASE}"
echo "Build Type: ${BLDTYPE}"
echo "Build Level: ${BLDLEVEL}"
echo "Version: ${VERSION}"
echo "Build Root Directory: ${BLDROOT}"
echo "Docker Container Name: ${CONTNAME}"

SRCDIR=/root/dockerbuild/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}
export SRCDIR

FNAME=${CONTNAME}-${VERSION}.${BLDLEVEL}.tz
if [ ! -f "${BLDROOT}/${BLDLEVEL}/appliance/${FNAME}" ]
then
    echo "${FNAME} doesn't exist in ${BLDROOT}/${BLDLEVEL}/appliance directory"
    exit 1
fi
export FNAME

TZFILEPATH=${SRCDIR}/${FNAME}
export TZFILEPATH

# Change working dir to BLDROOT dir
cd ${BLDROOT}/${BLDLEVEL}/appliance

echo "Create remote dir: ${SRCDIR}"
ssh DOCKBLD mkdir -p ${SRCDIR}


echo "Remote copy: ${TZFILEPATH}"
scp ${FNAME} DOCKBLD:${SRCDIR}/.
scp ${CONTNAME}_bldtools.tz DOCKBLD:${SRCDIR}/.


echo "Untar ${FNAME}"
ssh DOCKBLD "cd ${SRCDIR}; tar xzf ${FNAME}"
ssh DOCKBLD "cd ${SRCDIR}; tar xzf ${CONTNAME}_bldtools.tz"

echo "Start docker build"
ssh DOCKBLD "${SRCDIR}/buildDockerImage.sh ${RELEASE} ${BLDTYPE} ${BLDLEVEL} ${CONTNAME} ${VERSION}"

echo "Get docker image"
scp DOCKBLD:/root/dockerbuild/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}/${CONTNAME}-${VERSION}-docker.zip .
if [ $? -eq 1 ]
then
    exit 1
fi

cd ${CURDIR}

exit 0
