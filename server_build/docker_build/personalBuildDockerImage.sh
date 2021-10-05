#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
if [ $# -lt 1 ]
then
    echo "Usage: personalBuildDockerImage <compressed-tar-fileName>"
    exit 1
fi

SRCFILE=$1
export SRCFILE

NFIELDS=`echo ${SRCFILE} | grep -o "\." | wc -l`
export NFIELDS
if [ $NFIELDS -le 1 ]
then
    echo "Invalid file name specified: $SRCFILE"
    echo "Valid format: <containerName>-<version>.<buildLevel>.tz"
    exit 1
fi

PID=$$
export PID

CONTNAME=`echo ${SRCFILE} | cut -d"-" -f 1`
export CONTNAME

BLDLEVEL=`echo ${SRCFILE} | cut -d"." -f $NFIELDS`
export BLDLEVEL

if [ $NFIELDS -eq 4 ]
then
    VERSION=`echo ${SRCFILE} | cut -d"-" -f2 | cut -d"." -f 1-3`
else
    VERSION=`echo ${SRCFILE} | cut -d"-" -f2 | cut -d"." -f 1-2`
fi
export VERSION

RELEASE=IMADev
export RELEASE
BLDTYPE=development
export BLDTYPE

BLDROOT=/root/dockerbuild
export BLDROOT

CURDIR=`pwd`
export CURDIR

echo "Release: ${RELEASE}"
echo "Build Type: ${BLDTYPE}"
echo "Build Level: ${BLDLEVEL}"
echo "Version: ${VERSION}"
echo "Build Root Directory: ${BLDROOT}"
echo "Docker Container Name: ${CONTNAME}"

SRCDIR=${BLDROOT}/${RELEASE}/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}
export SRCDIR

FNAME=${CONTNAME}-${VERSION}.${BLDLEVEL}.tz
export FNAME

# Check if any build is currently running on build system
sudo sh -c "ssh msdockerbuild ls /root/dockerbuild/IMA15a/.lock.${CONTNAME}"
if [ $? -eq 0 ]
then
    echo "A personal build is in progress. Try after 10 minutes"
    exit 1
fi

echo "Create remote dir: ${SRCDIR}"
sudo sh -c "ssh msdockerbuild mkdir -p ${SRCDIR}"

echo "Lock build"
sudo sh -c "ssh msdockerbuild touch /root/dockerbuild/IMA15a/.lock.${CONTNAME}"

echo "Remote copy: ${SRCDIR}/${SRCFILE}"
sudo sh -c "scp ${SRCFILE} msdockerbuild:${SRCDIR}/."
sudo sh -c "scp ${CONTNAME}_bldtools.tz msdockerbuild:${SRCDIR}/."

echo "Untar ${SRCFILE}"
sudo sh -c "ssh msdockerbuild 'cd ${SRCDIR}; tar xzf ${SRCFILE}'"
sudo sh -c "ssh msdockerbuild 'cd ${SRCDIR}; tar xzf ${CONTNAME}_bldtools.tz'"

echo "Start docker build"
sudo sh -c "ssh msdockerbuild '/root/dockerbuild/buildImage.sh ${RELEASE} ${BLDTYPE} ${BLDLEVEL} ${CONTNAME} ${VERSION}'"

echo "Get docker image"
sudo sh -c "scp msdockerbuild:${SRCDIR}/${CONTNAME}-${VERSION}-docker.zip ."

echo "Get docker image build log"
sudo sh -c "scp msdockerbuild:${SRCDIR}/*.log ."

echo "Remove build tree"
sudo sh -c "ssh msdockerbuild 'cd /root/dockerbuild; rm -rf IMA15a/${BLDTYPE}/${BLDLEVEL}/${CONTNAME}'"

echo "Remove build lock"
sudo sh -c "ssh msdockerbuild rm -f /root/dockerbuild/IMA15a/.lock.${CONTNAME}"

exit 0
