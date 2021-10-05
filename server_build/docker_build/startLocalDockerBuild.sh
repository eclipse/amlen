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
    echo "Usage: startLocalDockerBuild.sh <ReleaseName> <BuileType> <BuildLevel> <ContName> <version> <build_root>"
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

SRCDIR=/home/dockerbuild/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}
export SRCDIR
echo "SRCDIR: ${SRCDIR}"

FNAME=${CONTNAME}-${VERSION}.${BLDLEVEL}.tz
if [ ! -f "${BLDROOT}/${BLDLEVEL}/appliance/${FNAME}" ]
then
    echo "${FNAME} doesn't exist in ${BLDROOT}/${BLDLEVEL}/appliance directory"
    exit 1
fi
export FNAME
echo "FNAME: ${FNAME}"

TZFILEPATH=${SRCDIR}/${FNAME}
export TZFILEPATH
echo "TZFILEPATH: ${TZFILEPATH}"


# Change working dir to BLDROOT dir
cd ${BLDROOT}/${BLDLEVEL}/appliance

# echo "Create remote dir: ${SRCDIR}"
# ssh ${LOCALDOCKBLD} mkdir -p ${SRCDIR}
echo "Create source dir: ${SRCDIR}"
mkdir -p ${SRCDIR}

# echo "Remote copy: ${TZFILEPATH}"
# scp ${FNAME} ${LOCALDOCKBLD}:${SRCDIR}/.
# scp ${CONTNAME}_bldtools.tz ${LOCALDOCKBLD}:${SRCDIR}/.
echo "Remote copy: ${TZFILEPATH}"
cp ${FNAME} ${SRCDIR}/.
cp ${CONTNAME}_bldtools.tz ${SRCDIR}/.


# echo "Untar ${FNAME}"
# ssh ${LOCALDOCKBLD} "cd ${SRCDIR}; tar xzf ${FNAME}"
# ssh ${LOCALDOCKBLD} "cd ${SRCDIR}; tar xzf ${CONTNAME}_bldtools.tz"
echo "Untar ${FNAME}"
cd ${SRCDIR}; tar xzf ${FNAME}
cd ${SRCDIR}; tar xzf ${CONTNAME}_bldtools.tz

# echo "Start docker build"
# ssh ${LOCALDOCKBLD} "${SRCDIR}/buildDockerImage.sh ${RELEASE} ${BLDTYPE} ${BLDLEVEL} ${CONTNAME} ${VERSION}"
echo "Start docker build"
${SRCDIR}/buildLocalDockerImage.sh ${RELEASE} ${BLDTYPE} ${BLDLEVEL} ${CONTNAME} ${VERSION}

# echo "Get docker image"
# scp ${LOCALDOCKBLD}:/home/dockerbuild/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}/${CONTNAME}-${VERSION}-docker.zip .
echo "Get docker image"
sudo cp /home/dockerbuild/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}/${CONTNAME}-${VERSION}-docker.zip ${BLDROOT}/${BLDLEVEL}/appliance
if [ $? -eq 1 ]
then
    exit 1
fi

cd ${CURDIR}

exit 0
