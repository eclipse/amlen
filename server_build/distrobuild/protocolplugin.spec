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

#We expect the source to be in a directory called this, zipped to a file with the same name (+.zip)
#This is the default format when I download the source from a branch in GHE
%define sourcename messagegateway-buildoss

#Disable debuginfo generation and stripping
%define __os_install_post %{nil}
%define debug_package %{nil}

Summary: Amlen Protocol Plugin Support for Linux x86_64
Name: AmlenProtocolPlugin
Version: 1.0.0.0
Release: 1%{?dist}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications

Source0: %{sourcename}.zip

BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
BuildRequires: junit,ant-contrib,python3,java-devel,icu,libxslt,ant-junit
Requires:  EclipseAmlenServer

%description
Amlen Server Protocol Plugin Support for Linux x86_64

%prep
%setup -n %{sourcename}

%build
export BUILD_LABEL="$(date +%Y%m%d-%H%M)_git_private"
#Where the source is
export SROOT=$RPM_BUILD_DIR/%{sourcename}
#Where binaries are built and assembled
export BROOT=$SROOT
#Where we should find any dependencies
export DEPS_HOME=$RPM_BUILD_DIR/broot/deps
#Where we should arrange files the hierarchy for the RPM:
export IMASERVER_BASE_DIR=$BROOT/rpmtree
export SLESNORPMS=yes
export JAVA_HOME=/etc/alternatives/java_sdk
mkdir -p $BROOT
cd $SROOT/server_build
#export IMA_PATH_PROPERTIES=${SROOT}/server_build/ossbuild/samplerebrand/amlen-paths.properties
ant -f $SROOT/server_build/build.xml buildprotocolplugin-oss

%install
export DONT_STRIP=1
export IMASERVER_BASE_DIR=$RPM_BUILD_DIR/%{sourcename}/rpmtree
#cp -dpR $IMASERVER_BASE_DIR/* "$RPM_BUILD_ROOT/"
mv $IMASERVER_BASE_DIR/* "$RPM_BUILD_ROOT/"

%files
%defattr (-, root, bin)
/usr/share/amlen-server/jars/imaPlugin.jar

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p /var/lib/amlen-server/markers
touch /var/lib/amlen-server/markers/install-protocolplugin

%post

%preun

%postun

%posttrans
rm -f /var/lib/amlen-server/markers/install-protocolplugin
