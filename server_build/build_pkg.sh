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

# This script creates RPM for e.g. imaserver and WebUI RPMs that can be used to
# install imaserver (and mqconnectivity) and WebUI on supported
# Linux X86_64 systems.
# The RPMs can also be used as a source to build docker images.
#
# SYNTAX: build_pkg.sh <mode> <bldroot> <build_id> <version_id> <build_label>
#
# Where mode is one of:
# default_rpms
# all_rpms
# serverplus_rpm
# server_rpm
# prep_all
# prep_default
# prep_server
# prep_mqcbridge
# prep_protocolplugin
# prep_webui
# prep_proxy
#
# The prep modes - designed to be called from an rpm/dpkg build. They just put the files in 
#                  the places needed for packaging
#
# Serverplus rpm includes the MQ bridge and protocol plugin
# server rpm does not (so you need separate rpms for those) 
#
# Script uses a number of env_vars, in particular
# SHIP_OPENSSL - whether we are going to package openssl and the things that rely on it
# SHIP_BOOST - whether we are going to package the boost libraries we were built against
# SHIP_ICU - whether we are going to package the icu libraries we were built against
# SHIP_IBM_JRE - whether are are going to package an IBM JRE

set -e     #terminate on error
set -x
START_DIR=`pwd`

#Describe how we were run
echo "$(printf %q "$BASH_SOURCE")$((($#)) && printf ' %q' "$@")"

# Set variables from args
export BUILD_MODE=$1
export BUILD_ROOT=$2
export SOURCE_ROOT=$3
export ISM_VERSION_ID=$4
export BUILD_LABEL=$5

# Get paths from properties
if [ -z "$IMA_PATH_PROPERTIES" ]; then
    export IMA_PATH_PROPERTIES="${START_DIR}/paths.properties"
fi
source $IMA_PATH_PROPERTIES


export RPM_VERSION=$(echo ${ISM_VERSION_ID} | sed 's/ /./')
export RPM_RELEASE=$(echo $BUILD_LABEL | sed 's/-/./' | sed 's/-/_/g')
export VERSION_ID=$(echo ${ISM_VERSION_ID}.${BUILD_LABEL} | sed 's/ /./')
if [ ! -z "${BUILD_OUTPUT}" ]; then
    export RPMDIR=${BUILD_OUTPUT}/rpms
else
    export RPMDIR=${BUILD_ROOT}/rpms
fi 
export RPM_BUILD_DIR=${RPMDIR}/build
export RPM_BUILD_LABEL=$(echo $BUILD_LABEL | sed 's/-/./')

export RPM_BUILD_NCPUS=1
if [ $(nproc) -gt 1 ] ; then
    export RPM_BUILD_NCPUS=$((`nproc` - 1))

    if [ ${RPM_BUILD_NCPUS} -gt ${AMLEN_MAX_BUILD_JOBS} ] ; then
        RPM_BUILD_NCPUS=${AMLEN_MAX_BUILD_JOBS}
fi
fi

export LINUXDISTRO_ID=$(grep -oP '(?<=^ID=).+' /etc/os-release | tr -d '"')
export LINUXDISTRO_VERSION=$(grep -oP '(?<=^VERSION_ID=).+' /etc/os-release | tr -d '"')

#Remove anything after a . do 8.4->8 but 34 is unchanged
export LINUXDISTRO_VERSION_MAJOR=${LINUXDISTRO_VERSION%.*}
export LINUXDISTRO_FULL=${LINUXDISTRO_ID}${LINUXDISTRO_VERSION_MAJOR}

echo "Packaging on ${LINUXDISTRO_FULL}"

if [ "${SHIP_OPENSSL}" == "yes" ]; then
    # OSS lib dependencies
    export SSL_HOME=/usr/local/openssl
    export CURL_HOME=/usr/local/curl
    export OPENLDAP_HOME=/usr/local/openldap
    export NETSNMP_HOME=/usr/local/netsnmp
    export MONGOC_HOME=/usr/local/mongoc

    echo Building with SSL_HOME=$SSL_HOME
    
    if [ -z "${SHIP_ICU}" ]; then
        echo "SHIP_ICU not set - defaulting to  SHIP_OPENSSL i.e. yes"
        SHIP_ICU=yes
    fi

    if [ -z "${SHIP_BOOST}" ]; then
        echo "SHIP_BOOST not set - defaulting to  SHIP_OPENSSL i.e. yes"
        SHIP_BOOST=yes
    fi

    if [ -z "${SHIP_IBM_JRE}"]; then
        echo "SHIP_IBM_JRE not set - defaulting to  SHIP_OPENSSL i.e. yes"
        SHIP_IBM_JRE=yes
    fi
fi


if [ -n "${IMA_ICU_HOME}" ]; then
    export IMA_ICU_LIBDIR=${IMA_ICU_HOME}/lib
else
    export IMA_ICU_LIBDIR=/usr/lib64
fi

if [ -n "${IMA_BOOST_HOME}" ]; then
    export IMA_BOOST_LIBDIR=${IMA_BOOST_HOME}/lib
else
    export IMA_BOOST_LIBDIR=/usr/lib64
fi

IBM_JRE_TARBALL_GLOB=ibm-java-jre-*.tgz
IBM_MQC_TARBALL_GLOB=*MQC-Redist*.tar.gz

#VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV

function prep_imabridge {
    BRIDGE_NAME=${IMA_PKG_BRIDGE}

    if [ -z "$BRIDGE_BASE_DIR" ] ; then
        BRIDGE_BASE_DIR=$RPM_BUILD_DIR/$BRIDGE_NAME
    fi

    # Create dirs in rpm build area
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/config
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/sample-config
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/bin
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/lib
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/certificates/keystore
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/resource
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_DATA_PATH}
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_DATA_PATH}/truststore
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_DATA_PATH}/keystore
    mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_DATA_PATH}/diag/logs

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    # Copy required binaries from server_ship
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/imabridge $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/.

    # Copy default certificate for AdminEndpoint
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/keystore/imabridge_default_cert.pem $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/certificates/keystore/imabridge_default_cert.pem
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/keystore/imabridge_default_key.pem $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/certificates/keystore/imabridge_default_key.pem

    # Copy required scripts
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/scripts/*.sh $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/scripts/gdb.batch $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/.

    dos2unix $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/*.sh
    chmod +x $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/*.sh

    ## Package Dependencies
    if [ "$SHIP_OPENSSL" == "yes" ] ; then
      # OpenSSL
      if [ ! -z $SSL_HOME -a -e $SSL_HOME/lib ] ; then
        cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/.
        cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/.
      fi
    fi
    
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        cp --no-preserve=ownership ${BUILD_OUTPUT}/asanlibs/* $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/.
        cp --no-preserve=ownership ${BUILD_OUTPUT}/asanlibs/* $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/lib/.
    fi

    # Copy required libraries from server_ship
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libimabridge.so $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libismutil.so $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/.

    # Copy debug bin/lib
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/debug/bin/imabridge $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/bin.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/debug/lib/libimabridge.so $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/lib/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/debug/lib/libismutil.so $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/debug/lib/.
    
    if [ "$SHIP_ICU" == "yes" ] ; then
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicudata.so.60.3 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/libicudata.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicui18n.so.60.3 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/libicui18n.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicuuc.so.60.3 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/libicuuc.so.60
    fi
    
    # Copy resource bundle
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/msgcat/res/*.res $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/resource

    # Copy html files
    cp -r --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/http $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/.
    cp -r --no-preserve=ownership ${BUILD_ROOT}/server_main/http/license $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/http/.

    # Copy default config files
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/config/imabridge.cfg $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/config/imabridge.service $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/config/imabridge.logrotate $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/config/.
    if [ -d "${BUILD_ROOT}/server_build_ext/config" ]; then
        mkdir -p $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/swidtag
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build_ext/config/*Bridge*.swidtag $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/config/.
    fi
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy_br/config/*.cfg $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/sample-config

    chmod 755 $BRIDGE_BASE_DIR/${IMA_BRIDGE_INSTALL_PATH}/lib64/*
}

function set_docker_from {
    DIR=$1
    if [ "${LINUXDISTRO_FULL}" == "centos7"  ] ; then
        sed -i 's+IMA_SVR_DISTRO+quay.io/centos/centos:7+' $DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    elif [ "${LINUXDISTRO_FULL}" == "almalinux8"  ] ; then
        sed -i 's+IMA_SVR_DISTRO+quay.io/almalinuxorg/almalinux:8+' $DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    elif [ "${LINUXDISTRO_FULL}" == "almalinux9"  ] ; then
        sed -i 's+IMA_SVR_DISTRO+quay.io/almalinuxorg/almalinux:9+' $DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    elif [ "${LINUXDISTRO_FULL}" == "fedora"  ] ; then
        sed -i 's+IMA_SVR_DISTRO+quay.io/fedora/fedora:35-x86_64+' $DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    else
        # default to almalinux 8
        sed -i 's+IMA_SVR_DISTRO+quay.io/almalinuxorg/almalinux:8+' $DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    fi
}

function bld_imabridge_rpm {
    echo "Building MessageGateway Bridge RPM package"

    #-----------------------------------------------------------------------------------
    #
    #   Build RPM of Standalone MessageSight Bridge
    #
    #-----------------------------------------------------------------------------------

    prep_imabridge
    
    BRIDGE_RPMBUILD_DIR=$RPM_BUILD_DIR/bridge

    # BUILD RPMS --------------------------------------------
    mkdir -p $BRIDGE_RPMBUILD_DIR
    
    export RPMBUILD_ROOT_BRIDGE_EL=$BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild

    # Create rpmbuild dirs
    mkdir -p $RPMBUILD_ROOT_BRIDGE_EL/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}

    # Create tar file of RPM contents
    cd $RPM_BUILD_DIR
    tar czf $RPMBUILD_ROOT_BRIDGE_EL/SOURCES/${BRIDGE_NAME}.tar.gz $BRIDGE_NAME

    # Copy spec file to rpm build root
    cd $BRIDGE_RPMBUILD_DIR
    dos2unix ${BUILD_ROOT}/server_build/docker_build/imamqttbridge.spec
    cp ${BUILD_ROOT}/server_build/docker_build/imamqttbridge.spec $RPMBUILD_ROOT_BRIDGE_EL/SPECS/imamqttbridge.spec

    # Change Version and Release in SPEC file
    sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_BRIDGE_EL/SPECS/imamqttbridge.spec

    #Delete any rpms from older builds that we don't want included
    rm -f $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${BRIDGE_NAME}*.rpm
    
    # start rpm build
    cd $RPMBUILD_ROOT_BRIDGE_EL/SPECS
    QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_BRIDGE_EL/BUILDROOT imamqttbridge.spec

    # create Bridge tar file
    mkdir -p $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    cp --no-preserve=ownership $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${BRIDGE_NAME}*.rpm $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imamqttbridge $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    set_docker_from $BRIDGE_RPMBUILD_DIR
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imabridge-docker.env $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    dos2unix $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    dos2unix $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/imabridge-docker.env

    # Copy tarballs and rpms
    cd $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    tar czf $RPMDIR/${BRIDGE_NAME}-${LINUXDISTRO_FULL}-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
    cp $BRIDGE_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR
    
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        mv $RPMDIR/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.${LINUXDISTRO_FULL}.x86_64.rpm
    fi

    if [ "$SLESNORPMS" = "yes" ]; then
        echo "SLESNORPMS was set to yes.  Not building SLES rpms."
    else
        # SLES build
        export RPMBUILD_ROOT_BRIDGE_SLES=$BRIDGE_RPMBUILD_DIR/sles/rpmbuild
        mkdir -p $RPMBUILD_ROOT_BRIDGE_SLES/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}
        cd $RPM_BUILD_DIR
        tar czf $RPMBUILD_ROOT_BRIDGE_SLES/SOURCES/${BRIDGE_NAME}-SLES.tar.gz $BRIDGE_NAME
        cd $BRIDGE_RPMBUILD_DIR
        dos2unix ${BUILD_ROOT}/server_build/docker_build/imamqttbridge-sles.spec
        cp ${BUILD_ROOT}/server_build/docker_build/imamqttbridge-sles.spec $RPMBUILD_ROOT_BRIDGE_SLES/SPECS/imamqttbridge-sles.spec
        sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_BRIDGE_SLES/SPECS/imamqttbridge-sles.spec
        rm -f $BRIDGE_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${BRIDGE_NAME}*.rpm
        cd $RPMBUILD_ROOT_BRIDGE_SLES/SPECS
        QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_BRIDGE_SLES/BUILDROOT imamqttbridge-sles.spec
        mkdir -p $BRIDGE_RPMBUILD_DIR/sles/temp
        cp --no-preserve=ownership $BRIDGE_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${BRIDGE_NAME}*.rpm $BRIDGE_RPMBUILD_DIR/sles/temp/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imamqttbridge $BRIDGE_RPMBUILD_DIR/sles/temp/Dockerfile
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imabridge-docker.env $BRIDGE_RPMBUILD_DIR/sles/temp
        dos2unix $BRIDGE_RPMBUILD_DIR/sles/temp/Dockerfile
        dos2unix $BRIDGE_RPMBUILD_DIR/sles/temp/imabridge-docker.env

        cd $BRIDGE_RPMBUILD_DIR/sles/temp
        tar czf $RPMDIR/${BRIDGE_NAME}-SLES-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
        cp $BRIDGE_RPMBUILD_DIR/sles/temp/${BRIDGE_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm $RPMDIR
    fi

}

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

function prep_server {
    # Create dirs in rpm build area
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/certificates/keystore
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/resource
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/jars

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    # Copy required binaries from server_ship
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/imaserver $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/icu_gettext $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/imahasher $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.

    # Copy default certificate for AdminEndpoint
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/keystore/MessageSightCert.pem $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/certificates/keystore/AdminDefaultCert.pem
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/keystore/MessageSightKey.pem $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/certificates/keystore/AdminDefaultKey.pem

    # Copy required scripts
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/*.py $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/*.sh $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/gdb.batch $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.

    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/*.py $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/*.sh $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/gdb.batch $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/.
    dos2unix $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/*.sh
    chmod +x $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/*.sh
    dos2unix $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/*.py
    chmod +x $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/*.py
    dos2unix $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/*.sh
    chmod +x $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/*.sh
    dos2unix $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/*.py
    chmod +x $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/*.py

    ## Package Dependencies
    if [ "$SHIP_OPENSSL" == "yes" ] ; then
        # OpenSSL
        if [ ! -z $SSL_HOME -a -e $SSL_HOME/lib ] ; then
            cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
            cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
            cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
            cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
        fi

        # OpenLDAP
        if [ ! -z $OPENLDAP_HOME -a -e $OPENLDAP_HOME/lib ] ; then
          cp --no-preserve=ownership $OPENLDAP_HOME/lib/liblber-2.4.so.2 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $OPENLDAP_HOME/lib/libldap_r-2.4.so.2 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $OPENLDAP_HOME/lib/liblber-2.4.so.2 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
          cp --no-preserve=ownership $OPENLDAP_HOME/lib/libldap_r-2.4.so.2 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
        fi

        # Curl
        if [ ! -z $CURL_HOME -a -e $CURL_HOME/lib -a -e $CURL_HOME/lib/libcurl.so.4 ] ; then
          cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
        elif [ ! -z $CURL_HOME -a -e $CURL_HOME/lib -a -e $CURL_HOME/lib64/libcurl.so.4 ] ; then
          cp --no-preserve=ownership $CURL_HOME/lib64/libcurl.so.4 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $CURL_HOME/lib64/libcurl.so.4 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
        elif [ ! -z $CURL_HOME -a -e $CURL_HOME/lib ] ; then
            # oh dead
            echo "Cant find libcurl either $CURL_HOME/lib/libcurl.so.4 or $CURL_HOME/lib64/libcurl.so.4"
            exit 12
        fi

        # NetSNMP
        if [ ! -z $NETSNMP_HOME -a -e $NETSNMP_HOME/lib ] ; then
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmp.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmpagent.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmphelpers.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmp.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmpagent.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
          cp --no-preserve=ownership $NETSNMP_HOME/lib/libnetsnmphelpers.so.35 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
        fi
    fi
    
    # If ASAN_BUILD then copy libasan and libstdc++.so from gcc9 into rpm
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        cp --no-preserve=ownership ${BUILD_OUTPUT}/asanlibs/* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
        cp --no-preserve=ownership ${BUILD_OUTPUT}/asanlibs/* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
    fi

    # Copy required libraries from server_ship
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libism*.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libMCP*.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libSpiderCast.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/librum.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/.

    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libism*.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libMCP*.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libSpiderCast.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/librum.so $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/.

    # Copy debug bin/lib
    cp -r --no-preserve=ownership ${BUILD_ROOT}/server_ship/debug/bin $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/.
    cp -r --no-preserve=ownership ${BUILD_ROOT}/server_ship/debug/lib $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/.
    
    if [ "$SHIP_ICU" == "yes" ] ; then
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicudata.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/libicudata.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicui18n.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/libicui18n.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicuuc.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/libicuuc.so.60

        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicudata.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/libicudata.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicui18n.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/libicui18n.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicuuc.so.60.3 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/libicuuc.so.60
    fi

    if [ "$SHIP_BOOST" == "yes" ] ; then
        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_thread.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/
        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_date_time.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/
        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_system.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/

        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_thread.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/
        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_date_time.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/
        cp --no-preserve=ownership ${IMA_BOOST_LIBDIR}/libboost_system.so.1.* $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/
    fi

    # Copy resource bundle
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/msgcat/res/*.res $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/resource
    
    # Copy html files
    cp -r --no-preserve=ownership ${BUILD_ROOT}/server_main/http $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/.

    # Copy default config files
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/server.cfg $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/server_dynamic.json $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/testLicense.json $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.
    if [ -d "${BUILD_ROOT}/server_build_ext/config" ]; then
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build_ext/config/*Server*.swidtag $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.
    fi
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/imaserver.service $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/imaserver.logrotate $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.

    chmod 755 $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/lib64/*

    # Remove files not required for RPM
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/mqbench >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/imaproxy >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/imaStanaloneAPD >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/imadumpformatter >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/imacli >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/imasnmp >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/mqttbench >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/mqttbench2 >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/offloader >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/bin/*.cfg >/dev/null 2>&1 3>&1
    rm -f $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/debug/lib/libcli*.so >/dev/null 2>&1 3>&1
}

function prep_mqcbridge {
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/mqclient
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build


    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/mqcbridge $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/bin/.

    # Copy default config file
    cp --no-preserve=ownership ${BUILD_ROOT}/server_mqcbridge/config/mqclient.ini $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/config/.

    # Copy MQ Redistributable client and IBM JRE tarballs - these gets untarred at run time
    
    if ls ${BUILD_ROOT}/server_mqcbridge/mqc_extract/${IBM_MQC_TARBALL_GLOB} 1> /dev/null 2>&1; then
      #We extract it as part of the build
      cp --no-preserve=ownership ${BUILD_ROOT}/server_mqcbridge/mqc_extract/${IBM_MQC_TARBALL_GLOB} $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/mqclient/.
    else
      #in messagegatway build containers, the tarball we need is pre-extracted:
      cp --no-preserve=ownership ${DEPS_HOME}/mqc/${IBM_MQC_TARBALL_GLOB} $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/mqclient/.
    fi

    if [ "${SHIP_IBM_JRE}" == "yes" ]; then
        cp -r --no-preserve=ownership ${DEPS_HOME}/${IBM_JRE_TARBALL_GLOB} $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/.
    fi
}

function prep_protocolplugin {
    mkdir -p $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/jars/

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    cp --no-preserve=ownership ${BUILD_ROOT}/server_plugin/bin/lib/imaPlugin.jar $IMASERVER_BASE_DIR/${IMA_SVR_INSTALL_PATH}/jars/.
}

function prep_sdk {
    if [ -z "$IMASDK_BASE_DIR" ] ; then
        IMASDK_BASE_DIR=$RPM_BUILD_DIR/imasdk
    fi
    mkdir -p $IMASDK_BASE_DIR/${IMA_SDK_INSTALL_PATH}

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    #Copies the JMS client (and the RA if it was built)
    cp -r --no-preserve=ownership ${BUILD_ROOT}/client_ship/ImaClient $IMASDK_BASE_DIR/${IMA_SDK_INSTALL_PATH}/
    
    #Copies the Protocol Plugin SDK
    cp -r --no-preserve=ownership ${BUILD_ROOT}/client_ship/ImaTools $IMASDK_BASE_DIR/${IMA_SDK_INSTALL_PATH}/
}

#Arranges WebUI file to be put into a package (either by rpmbuild called by this script) or by the rpmbuild calling this script
function prep_webui {
    IMAGUI_NAME=${IMA_PKG_WEBUI}
    
    if [ -z "$IMAGUI_BASE_DIR" ] ; then
        IMAGUI_BASE_DIR=$RPM_BUILD_DIR/$IMAGUI_NAME
    fi
    
    IMAGUI_TMP_MINIFY_DIR=${IMA_WEBUI_INSTALL_PATH}

    #Set product name & paths
    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    # Add files required for wlp executable
    mkdir -p $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin
    mkdir -p $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/config
    mkdir -p $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/scripts
    mkdir -p $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/openldap/config
    mkdir -p $IMAGUI_BASE_DIR/opt/openldap/openldap-data
    mkdir -p $IMAGUI_BASE_DIR/opt/openldap/bin

    # Copy scripts and required lib from server_main including the ones that are shared with the server
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/scripts/*.sh $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/matchkeycert.sh $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/internal_mustgather.sh $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/scripts/messagesight-must-gather.sh $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_main/config/imawebui.service $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/config/.

    dos2unix $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/*.sh
    chmod +x $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/*.sh

    # Add wlp - assumes the end of IMA_WEBUI_APPSRV_INSTALL_PATH is /wlp
    mkdir -p $IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}
    cd $IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}/..
    cp -r  ${BUILD_ROOT}/server_gui/tools/wlp .
    
    mkdir -p wlp/usr/servers/ISMWebUI/workarea
    mkdir -p wlp/usr/servers/ISMWebUI/dropins
    mkdir -p wlp/usr/servers/ISMWebUI/apps
    mkdir -p wlp/usr/servers/ISMWebUI/resources
    mkdir -p wlp/usr/servers/ISMWebUI/resources/security
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/server.xml wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/properties.xml wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/properties.xml wlp/usr/servers/ISMWebUI/properties.xml.default
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/staticProperties.xml wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/ldap.xml wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/server.env wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/server.env wlp/usr/servers/ISMWebUI/server.env.default
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/jvm.options wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/bootstrap.properties wlp/usr/servers/ISMWebUI/.
    cp ${BUILD_ROOT}/server_gui/ISMWebUI/resources/security/key.jks wlp/usr/servers/ISMWebUI/resources/security/key.jks
    cp ${BUILD_ROOT}/server_ship/lib/ISMWebUI.war wlp/usr/servers/ISMWebUI/apps/.
    chmod +x wlp/bin/*
    chmod -R 775 wlp/usr/

    if [ "${SHIP_IBM_JRE}" == "yes" ]; then
        cp -r --no-preserve=ownership ${DEPS_HOME}/${IBM_JRE_TARBALL_GLOB} $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/.
    fi
    webui_minify

    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/config/imawebui_slapd.conf $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/openldap/config/slapd.conf
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/config/imawebui_users.ldif $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/openldap/config/users.ldif
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/config/imawebui_users_openldap.ldif $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/config/imawebui_users_389.ldif $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/config/dse.ldif $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/config/.
    dos2unix $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/createAcct.sh
    chmod +x $IMAGUI_BASE_DIR/${IMA_WEBUI_INSTALL_PATH}/bin/createAcct.sh
    echo "IMA_SERVER_HOST=127.0.0.1" >> $IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr/servers/ISMWebUI/server.env
    echo "IMA_SERVER_PORT=9089" >> $IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr/servers/ISMWebUI/server.env
}

function webui_minify {
    wlp/bin/server package ISMWebUI --archive="$IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}/../ISMWebUI.zip" --include=minify --os=Linux
    chmod a+rwx ./ISMWebUI.zip
    rm -rf wlp
    unzip -o -q ISMWebUI.zip
    rm -f ./ISMWebUI.zip
    chmod -R a+rw wlp
    chmod a+x wlp/bin/*

    #Now the minify has occurred we can add files that refer to product log directories etc that
    #we don't have write access to during build/pkg (i.e. minify)
    cp --no-preserve=ownership ${BUILD_ROOT}/server_gui/ISMWebUI_templates/*  $IMAGUI_BASE_DIR/${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr/servers/ISMWebUI/
}

#Packages the files (laid out by prep_server) into an RPM
#This works for serverplain or server plus - just depends what files have been 
#added to the tree
function rpmbuild_server {
    # BUILD RPMS --------------------------------------------
    mkdir -p $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}
    export RPMBUILD_ROOT_EL=$IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild

    # Create rpmbuild dirs
    mkdir -p $RPMBUILD_ROOT_EL/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}

    # Create tar file of RPM contents
    cd $RPM_BUILD_DIR
    tar czf $RPMBUILD_ROOT_EL/SOURCES/${IMASERVER_NAME}.tar.gz $IMASERVER_NAME

    # Copy spec file to rpm build root
    #cd $RPMDIR/build/server/${LINUXDISTRO_FULL}
    dos2unix ${BUILD_ROOT}/server_build/docker_build/imaserver.spec
    cp ${BUILD_ROOT}/server_build/docker_build/imaserver.spec $RPMBUILD_ROOT_EL/SPECS/imaserver.spec

    # Change Version and Release in SPEC file
    sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_EL/SPECS/imaserver.spec

    # If we are shipping boost, stop the server rpm depending on it:
    if [ "$SHIP_BOOST" == "yes" ] ; then
        sed -i -E 's/^Requires: (.*)boost,(.*)/Requires: \1\2/' $RPMBUILD_ROOT_EL/SPECS/imaserver.spec
    fi
    
    # Delete any old server rpms from earlier builds we don't want included
    rm -f $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${IMASERVER_NAME}*.rpm

    # start rpm build
    cd $RPMBUILD_ROOT_EL/SPECS
    QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_EL/BUILDROOT imaserver.spec

    # create imaserver tar file
    mkdir -p $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    cp --no-preserve=ownership $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${IMASERVER_NAME}*.rpm $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imaserver $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    set_docker_from $IMASERVER_RPMBUILD_DIR
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imaserver-docker.env $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    dos2unix $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    dos2unix $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/imaserver-docker.env

    # Copy tarballs and rpms
    cd $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    tar czf $RPMDIR/${IMASERVER_NAME}-${LINUXDISTRO_FULL}-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
    cp $IMASERVER_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR
    
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        mv $RPMDIR/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.${LINUXDISTRO_FULL}.x86_64.rpm
    fi

    if [ "$SLESNORPMS" == "yes" ]; then
        echo "SLESNORPMS was set to yes. Not building SLES RPMS."
    else
        # SLES RPM Build
        echo "SLESNORPMS was not set. Building SLES RPMS."
        mkdir -p $IMASERVER_RPMBUILD_DIR/sles
        export RPMBUILD_ROOT_SLES=$IMASERVER_RPMBUILD_DIR/sles/rpmbuild
        mkdir -p $RPMBUILD_ROOT_SLES/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}
        cd $RPM_BUILD_DIR
        tar czf $RPMBUILD_ROOT_SLES/SOURCES/${IMASERVER_NAME}-SLES.tar.gz $IMASERVER_NAME
        dos2unix ${BUILD_ROOT}/server_build/docker_build/imaserver-sles.spec
        cp ${BUILD_ROOT}/server_build/docker_build/imaserver-sles.spec $RPMBUILD_ROOT_SLES/SPECS/imaserver-sles.spec
        sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_SLES/SPECS/imaserver-sles.spec
        cd $RPMBUILD_ROOT_SLES/SPECS
        # Delete any old server rpms from earlier builds we don't want included
        rm -f $IMASERVER_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${IMASERVER_NAME}*.rpm

        QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_SLES/BUILDROOT imaserver-sles.spec
        mkdir -p $IMASERVER_RPMBUILD_DIR/sles/temp
        cp --no-preserve=ownership $IMASERVER_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${IMASERVER_NAME}*.rpm $IMASERVER_RPMBUILD_DIR/sles/temp/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imaserver $IMASERVER_RPMBUILD_DIR/sles/temp/Dockerfile
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imaserver-docker.env $IMASERVER_RPMBUILD_DIR/sles/temp
        dos2unix $IMASERVER_RPMBUILD_DIR/sles/temp/Dockerfile
        dos2unix $IMASERVER_RPMBUILD_DIR/sles/temp/imaserver-docker.env

        cd $IMASERVER_RPMBUILD_DIR/sles/temp
        tar czf $RPMDIR/${IMASERVER_NAME}-SLES-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
        cp $IMASERVER_RPMBUILD_DIR/sles/temp/${IMASERVER_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm $RPMDIR
    fi
}

function bld_imaserverplus_rpm {
    echo "Building MessageGateway Server RPM package incl MQ & Protocol Plugin"

    #-----------------------------------------------------------------------------------
    #
    #   Build RPM of Standalone MessageSight Server incl. MQ and Protocol Plugin
    #
    #-----------------------------------------------------------------------------------

    IMASERVER_NAME=${IMA_PKG_SERVER}
    IMASERVER_BASE_DIR=$RPM_BUILD_DIR/$IMASERVER_NAME
    IMASERVER_RPMBUILD_DIR=$RPM_BUILD_DIR/imaserver
    
    prep_server
    prep_mqcbridge
    prep_protocolplugin

    rpmbuild_server
}
function bld_imaserverplain_rpm {
    echo "Building MessageGateway Server RPM package (plain - no mq etc.)"

    #-----------------------------------------------------------------------------------
    #
    #   Build RPM of Standalone MessageSight Server (plain - no mq etc.)
    #
    #-----------------------------------------------------------------------------------

    IMASERVER_NAME=${IMA_PKG_SERVER}
    IMASERVER_BASE_DIR=$RPM_BUILD_DIR/$IMASERVER_NAME
    IMASERVER_RPMBUILD_DIR=$RPM_BUILD_DIR/imaserver
    
    prep_server
    rpmbuild_server
}

function bld_imagui_rpm {
    echo "Building MessageGateway GUI RPM package"

    prep_webui

    #-----------------------------------------------------------------------------------
    #
    #   Build RPM of Standalone MessageSight Web UI
    #
    #-----------------------------------------------------------------------------------

    IMAGUI_RPMBUILD_DIR=$RPM_BUILD_DIR/imawebui

    # Temporary dir locations to build imawebui RPM
    mkdir -p $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}
    export RPMBUILD_ROOT_WEBUI_EL=$IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild

    # Create rpmbuild dirs
    mkdir -p $RPMBUILD_ROOT_WEBUI_EL/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}

    # Create tar file of RPM contents
    cd $RPM_BUILD_DIR
    tar czf $RPMBUILD_ROOT_WEBUI_EL/SOURCES/${IMAGUI_NAME}.tar.gz ${IMAGUI_NAME}

    dos2unix ${BUILD_ROOT}/server_build/docker_build/imawebui.spec
    cp ${BUILD_ROOT}/server_build/docker_build/imawebui.spec $RPMBUILD_ROOT_WEBUI_EL/SPECS

    # Change Version and Release in SPEC file
    sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_WEBUI_EL/SPECS/imawebui.spec

    # Delete any old imawebui rpms from earlier builds we don't want included
    rm -f $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${IMAGUI_NAME}*.rpm

    # start rpm build
    cd $RPMBUILD_ROOT_WEBUI_EL/SPECS
    QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_WEBUI_EL/BUILDROOT imawebui.spec

    # create imawebui tar file
    mkdir -p $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    cp --no-preserve=ownership $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${IMAGUI_NAME}*.rpm $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${IMAGUI_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imawebui $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    set_docker_from $IMAGUI_RPMBUILD_DIR
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imawebui-docker.env $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    dos2unix $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    dos2unix $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/imawebui-docker.env

    # Copy tarballs and rpms
    cd $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    tar czf $RPMDIR/${IMAGUI_NAME}-${LINUXDISTRO_FULL}-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
    cp $IMAGUI_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${IMAGUI_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR

    # SLES RPM Build
    if [ "$SLESNORPMS" = "yes" ]; then
        echo "SLESNORPMS was set to yes. Not building SLES RPMS."
    else
        echo "SLESNORPMS was not set.  Building SLES RPMS."
        mkdir -p $IMAGUI_RPMBUILD_DIR/sles
        export RPMBUILD_ROOT_WEBUI_SLES=$IMAGUI_RPMBUILD_DIR/sles/rpmbuild
        mkdir -p $RPMBUILD_ROOT_WEBUI_SLES/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}
        cd $RPM_BUILD_DIR
        tar czf $RPMBUILD_ROOT_WEBUI_SLES/SOURCES/${IMAGUI_NAME}-SLES.tar.gz ${IMAGUI_NAME}
        dos2unix ${BUILD_ROOT}/server_build/docker_build/imawebui-sles.spec
        cp ${BUILD_ROOT}/server_build/docker_build/imawebui-sles.spec $RPMBUILD_ROOT_WEBUI_SLES/SPECS
        sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_WEBUI_SLES/SPECS/imawebui-sles.spec
        # Delete any old webui rpms from earlier builds we don't want included
        rm -f $IMAGUI_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${IMAGUI_NAME}*.rpm

        cd $RPMBUILD_ROOT_WEBUI_SLES/SPECS
        QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_WEBUI_SLES/BUILDROOT imawebui-sles.spec
        mkdir -p $IMAGUI_RPMBUILD_DIR/sles/temp
        cp --no-preserve=ownership $IMAGUI_RPMBUILD_DIR/sles/rpmbuild/RPMS/x86_64/${IMAGUI_NAME}*.rpm $IMAGUI_RPMBUILD_DIR/sles/temp/${IMAGUI_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imawebui $IMAGUI_RPMBUILD_DIR/sles/temp/Dockerfile
        cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imawebui-docker.env $IMAGUI_RPMBUILD_DIR/sles/temp
        dos2unix $IMAGUI_RPMBUILD_DIR/sles/temp/Dockerfile
        dos2unix $IMAGUI_RPMBUILD_DIR/sles/temp/imawebui-docker.env

        cd $IMAGUI_RPMBUILD_DIR/sles/temp
        tar czf $RPMDIR/${IMAGUI_NAME}-SLES-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
        cp $IMAGUI_RPMBUILD_DIR/sles/temp/${IMAGUI_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.sle.x86_64.rpm $RPMDIR
    fi
}

function bld_imaproxy_rpm {
    echo "Building MessageGateway Proxy RPM package"
    PROXY_NAME=${IMA_PKG_PROXY}
    PROXY_BASE_DIR=$RPM_BUILD_DIR/$PROXY_NAME
    PROXY_RPMBUILD_DIR=$RPM_BUILD_DIR/proxy

    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/config"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/sample-config"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/debug"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/certificates/keystore"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/resource"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/config"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_DATA_PATH}/diag/logs"
    mkdir -p "${PROXY_BASE_DIR}/${IMA_PROXY_DATA_PATH}/config"

    python3 ${SOURCE_ROOT}/server_build/path_parser.py -mdirreplace -i ${BUILD_ROOT}/server_build/docker_build -o ${BUILD_ROOT}/server_build/docker_build

    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/bin/imaproxy ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin/.

    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/scripts/*.sh ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/scripts/gdb.batch ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin/.

    dos2unix ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin/*.sh
    chmod +x ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/bin/*.sh

    ## Package Dependencies
    if [ "$SHIP_OPENSSL" == "yes" ] ; then
        # OpenSSL
        if [ ! -z $SSL_HOME -a -e $SSL_HOME/lib ] ; then
            cp --no-preserve=ownership $SSL_HOME/lib/libssl.so.1.1 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
            cp --no-preserve=ownership $SSL_HOME/lib/libcrypto.so.1.1 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        fi

        if [ ! -z $CURL_HOME -a -e $CURL_HOME/lib -a -e $CURL_HOME/lib/libcurl.so.4 ] ; then
            cp --no-preserve=ownership $CURL_HOME/lib/libcurl.so.4 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        elif [ ! -z $CURL_HOME -a -e $CURL_HOME/lib -a -e $CURL_HOME/lib64/libcurl.so.4 ] ; then
            cp --no-preserve=ownership $CURL_HOME/lib64/libcurl.so.4 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        elif [ ! -z $CURL_HOME -a -e $CURL_HOME/lib ] ; then
            # oh dead
            echo "Cant find libcurl either $CURL_HOME/lib/libcurl.so.4 or $CURL_HOME/lib64/libcurl.so.4"
            exit 11
        fi
    fi

    if [ "$SHIP_MONGO" == "yes" ] ; then
        if [ -n "$MONGOC_HOME" -a -e $MONGOC_HOME/lib64 ] ; then
          cp --no-preserve=ownership $MONGOC_HOME/lib64/libbson-1.0.so.0 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
          cp --no-preserve=ownership $MONGOC_HOME/lib64/libmongoc-1.0.so.0 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        fi

        if [ ! -z $MONGOC_HOME -a -e $MONGOC_HOME/lib -a -e $MONGOC_HOME/lib/libcurl.so.4 ] ; then
            cp --no-preserve=ownership $MONGOC_HOME/lib/libcurl.so.4 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        elif [ ! -z $MONGOC_HOME -a -e $MONGOC_HOME/lib -a -e $MONGOC_HOME/lib64/libcurl.so.4 ] ; then
            cp --no-preserve=ownership $MONGOC_HOME/lib64/libcurl.so.4 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
        elif [ ! -z $MONGOC_HOME -a -e $MONGOC_HOME/lib ] ; then
            # oh dead
            echo "Cant find libcurl either $MONGOC_HOME/lib/libcurl.so.4 or $MONGOC_HOME/lib64/libcurl.so.4"
            exit 11
        fi
    fi

    # If ASAN_BUILD then copy libasan and libstdc++.so from gcc9 into rpm
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        cp --no-preserve=ownership ${BUILD_OUTPUT}/asanlibs/* ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
    fi

    # Copy required libraries from server_ship
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libimaproxy.so ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/lib/libismutil.so ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.

    if [ "$SHIP_ICU" == "yes" ] ; then
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicudata.so.60.3 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/libicudata.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicui18n.so.60.3 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/libicui18n.so.60
        cp --no-preserve=ownership ${IMA_ICU_LIBDIR}/libicuuc.so.60.3 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/libicuuc.so.60
    fi

    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/proxy.cfg ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/config/imaproxy.cfg
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/proxy.cfg ${PROXY_BASE_DIR}/${IMA_PROXY_DATA_PATH}/config/imaproxy.cfg

    cp --no-preserve=ownership /usr/lib64/libstatsdclient.so ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/.

    # Copy imaproxy_config.jar file to lib dir (not in lib64, per Bao) 
    cp --no-preserve=ownership ${BUILD_ROOT}/client_ship/lib/imaproxy_config.jar ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib/.

    chmod 755 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib64/*
    chmod 755 ${PROXY_BASE_DIR}/${IMA_PROXY_INSTALL_PATH}/lib/*

    cp --no-preserve=ownership ${BUILD_ROOT}/server_ship/msgcat/res/*.res $PROXY_BASE_DIR/${IMA_PROXY_INSTALL_PATH}/resource

    # Copy default config files
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/config/imaproxy.cfg $PROXY_BASE_DIR/${IMA_PROXY_INSTALL_PATH}/bin/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/config/imaproxy.service $PROXY_BASE_DIR/${IMA_PROXY_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/config/imaproxy.logrotate $PROXY_BASE_DIR/${IMA_PROXY_INSTALL_PATH}/config/.
    cp --no-preserve=ownership ${BUILD_ROOT}/server_proxy/config/*.cfg $PROXY_BASE_DIR/${IMA_PROXY_INSTALL_PATH}/sample-config

    # BUILD RPMS --------------------------------------------
    mkdir -p $PROXY_RPMBUILD_DIR
    export RPMBUILD_ROOT_PROXY=$PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild

    # Create rpmbuild dirs
    mkdir -p $RPMBUILD_ROOT_PROXY/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS,tmp}

    # Create tar file of RPM contents
    cd $RPM_BUILD_DIR
    tar czf $RPMBUILD_ROOT_PROXY/SOURCES/${PROXY_NAME}.tar.gz $PROXY_NAME

    # Copy spec file to rpm build root
    cd $PROXY_RPMBUILD_DIR
    dos2unix ${BUILD_ROOT}/server_build/docker_build/imaproxy.spec

    cp ${BUILD_ROOT}/server_build/docker_build/imaproxy.spec $RPMBUILD_ROOT_PROXY/SPECS/imaproxy.spec

    # Change Version and Release in SPEC file
    sed -i 's/Release:.*/Release: '$RPM_RELEASE/ $RPMBUILD_ROOT_PROXY/SPECS/imaproxy.spec

    # Delete any old proxy rpms from earlier builds we don't want included
    rm -f $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${PROXY_NAME}*.rpm

    # start rpm build
    cd $RPMBUILD_ROOT_PROXY/SPECS
    QA_RPATHS=$(( 0x0002|0x0004|0x0008)) rpmbuild --quiet -bb --buildroot $RPMBUILD_ROOT_PROXY/BUILDROOT imaproxy.spec

    # create Proxy tar file
    mkdir -p $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    cp --no-preserve=ownership $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/rpmbuild/RPMS/x86_64/${PROXY_NAME}*.rpm $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${PROXY_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/Dockerfile.imaproxy $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    set_docker_from $PROXY_RPMBUILD_DIR
    cp --no-preserve=ownership ${BUILD_ROOT}/server_build/docker_build/imaproxy-docker.env $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    dos2unix $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/Dockerfile
    dos2unix $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/imaproxy-docker.env

    # Copy tarballs and rpms
    cd $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp
    tar czf $RPMDIR/${PROXY_NAME}-${LINUXDISTRO_FULL}-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz *
    cp $PROXY_RPMBUILD_DIR/${LINUXDISTRO_FULL}/temp/${PROXY_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR
    if [ "${ASAN_BUILD}" == "yes" ] ; then
        mv $RPMDIR/${PROXY_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.${LINUXDISTRO_FULL}.x86_64.rpm $RPMDIR/${PROXY_NAME}-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.${LINUXDISTRO_FULL}.x86_64.rpm
    fi
}
function bld_all_rpms {
    bld_imabridge_rpm
    bld_imaserverplain_rpm
    bld_imagui_rpm
    bld_imaproxy_rpm
}

function bld_default_rpms {
    bld_imabridge_rpm
    bld_imaserverplain_rpm
    bld_imagui_rpm
}

function bld_prep_default {
    prep_server
    prep_webui
    prep_imabridge
}

function bld_prep_all {
    bld_prep_default
    prep_sdk
    prep_protocolplugin
    prep_mqcbridge
}

function badmode {
    echo "$BUILD_MODE is not a valid build mode (e.g. all_rpms)"
    exit 10
}
###############################################################
# MAIN ROUTINE
###############################################################
echo "BUILD_MODE is $BUILD_MODE"

case $BUILD_MODE in
    all_rpms)             bld_all_rpms;;
    default_rpms)         bld_default_rpms;;
    serverplus_rpm)       bld_serverplus_rpm;;
    server_rpm)           bld_imaserverplain_rpm;;
    webui_rpm)            bld_imagui_rpm;;
    prep_server)          prep_server;;
    prep_mqcbridge)       prep_mqcbridge;;
    prep_protocolplugin)  prep_protocolplugin;;
    prep_webui)           prep_webui;;
    prep_imabridge)       prep_imabridge;;
    prep_sdk)             prep_sdk;;
    prep_default)         bld_prep_default;;
    prep_all)             bld_prep_all;;
    none)                 echo "Doing nothing as asked";;
    *) badmode;;
esac


# Cleanup
# rm -rf "$RPM_BUILD_DIR"
