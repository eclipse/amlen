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
if [ $# -lt 5 ]
then
    echo "Usage: startDockerBuild.sh <ReleaseName> <BuileType> <BuildLevel> <ContName> <version>"
    exit 1
fi


RELEASE=$1
BLDTYPE=$2
BLDLEVEL=$3
CONTNAME=$4
VERSION=$5

# Working dir
WORKDIR=/home/dockerbuild/$RELEASE/$BLDTYPE/$BLDLEVEL/$CONTNAME

if [ ! -d ${WORKDIR} ]
then
    echo "Could not find ${WORKDIR}"
    exit 1
fi

cd $WORKDIR

LOGFILE=docker-${CONTNAME}-${BLDLEVEL}.log
echo "Start Docker build of ${CONTNAME}" > $LOGFILE
echo "Date: `date`" >> $LOGFILE
echo "LOGFILE: ${LOGFILE}"

# Check if RPM exits
echo "buildDockerImage.sh Check if RPM exsits" >> $LOGFILE
echo "buildDockerImage.sh Check if RPM exsits"
CONTRPM=`ls ${CONTNAME}*.rpm`
export CONTRPM
if [ "${CONTNAME}" != "" ]
then
    echo "RPM to build ${CONTNAME} is found" >> $LOGFILE
else
    echo "RPM to build ${CONTNAME} is not found" >> $LOGFILE
    exit 1
fi

# Check container Docker file and rename container Dockerfile to Dockerfile
echo "buildDockerImage: Check container Docker file and rename container Dockerfile to Dockerfile" >> $LOGFILE
echo "buildDockerImage: Check container Docker file and rename container Dockerfile to Dockerfile"
if [ -f Dockerfile.${CONTNAME} ]
then
    cp -f Dockerfile.${CONTNAME} Dockerfile
    # Update RPM name in docker file
    if [ "${CONTNAME}" == "IBMWIoTPMessageGatewayServer" ]
    then
        sed -i 's/imaserver.rpm/'$CONTRPM'/g' Dockerfile
    fi
    if [ "${CONTNAME}" == "IBMWIoTPMessageGatewayWebUI" ]
    then
        sed -i 's/imawebui.rpm/'$CONTRPM'/g' Dockerfile
    fi
else
    echo "Dockerfile to build ${CONTNAME} is not found" >> $LOGFILE
    exit 1
fi


# Build docker image
echo "buildDockerImage: Build docker image"
echo "LOGFILE: ${LOGFILE}"
echo "PWD:"
pwd
ls
echo "buildDockerImage: Build docker image" >> $LOGFILE
echo "buildDockerImage: Build docker image"

DOCKPKGNAME=`echo ${CONTNAME} | tr '[:upper:]' '[:lower:]'`
export DOCKPKGNAME

echo "sudo docker build --force-rm=true -t ${DOCKPKGNAME}:$VERSION ."
sudo docker build --force-rm=true -t ${DOCKPKGNAME}:$VERSION . >> $LOGFILE 2>&1 3>&1
if [ $? -ne 0 ]
then
    echo "Failed to build ${DOCKPKGNAME} docker image" >> $LOGFILE
    echo "EXITING:  Failed to build ${DOCKPKGNAME} docker image"
    exit 1
fi

# Save docker image
echo "buildDockerImage: Save docker image"
sudo docker save -o ./${CONTNAME}-$VERSION-dockerImage.tar ${DOCKPKGNAME}:$VERSION >> $LOGFILE 2>&1 3>&1
if [ $? -ne 0 ]
then
    echo "Failed to save ${DOCKPKGNAME} docker image" >> $LOGFILE
    exit 1
else
    echo "Docker image in tar format is created: ${CONTNAME}-$VERSION-dockerImage.tar" >> $LOGFILE
fi

# Package docker image
echo "buildDockerImage: Package docker image"
echo "${CONTNAME}-docker.env" > ziplist.list
echo "${CONTNAME}-$VERSION-dockerImage.tar" >> ziplist.list

cat ziplist.list | zip -jm -@ "${CONTNAME}-${VERSION}-docker.zip"


# Cleanup docker images
(
PID=$$
sudo docker images > /tmp/docker.img.$PID.list
while read -r line
do
    name=$line
    echo "$line" | grep "^<none>" > /dev/null 2>&1
    if [ $? -eq 0 ]
    then
        imgname=`echo "$line" | awk '{ print $3 }'`
        echo "Remove image: $imgname"
        if [ "$imgname" != "" ]
        then
            sudo docker rmi -f $imgname
        fi
    fi
done < "/tmp/docker.img.$PID.list"

# Remove docker image
sudo docker rmi -f ${DOCKPKGNAME}:${VERSION}

) | tee -a $LOGFILE

exit 0

