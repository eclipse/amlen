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

Summary: Amlen Bridge for Linux x86_64
Name: AmlenBridge
Version: 1.0.0.0
Release: 1%{?dist}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications

Source0: %{sourcename}.zip

BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
BuildRequires: openssl-devel,curl-devel,openldap-devel,net-snmp-devel,libicu-devel,rpm-build,vim-common,gcc,gcc-c++,make,CUnit-devel,junit,ant-contrib,boost-devel,dos2unix,ant,java-11-openjdk-devel,icu,javapackages-local
Requires: gdb, net-tools, openssl, tar, perl, procps >= 3.3.9, libjansson.so.4()(64bit), logrotate, zip, bzip2, unzip
Obsoletes: IBMIoTMessageSightServer < 6.0

%description
Amlen Bridge for Linux x86_64

%prep
%setup -n %{sourcename}

%build
export BUILD_LABEL="$(date +%Y%m%d-%H%M)_git_private"
#Where the source is
export SROOT=$RPM_BUILD_DIR/%{sourcename}
#Where binaries are built and assembled
export BROOT=$RPM_BUILD_DIR/broot 
#Where we should arrange files the hierarchy for the RPM:
export BRIDGE_BASE_DIR=$BROOT/rpmtree
export USE_REAL_TRANSLATIONS=true
export SLESNORPMS=yes
mkdir $BROOT
cd $SROOT/server_build
export JAVA_HOME=/etc/alternatives/java_sdk
#export IMA_PATH_PROPERTIES=${SROOT}/server_build/ossbuild/samplerebrand/amlen-paths.properties
ant -f $SROOT/server_build/build.xml buildimabridge-oss

%install
export DONT_STRIP=1
export BRIDGE_BASE_DIR=$RPM_BUILD_DIR/broot/rpmtree
mv $BRIDGE_BASE_DIR/* "$RPM_BUILD_ROOT/"

%files
%defattr (-, root, bin)
/usr/share/amlen-bridge
/var/lib/amlen-bridge

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p /var/lib/amlen-bridge
touch /var/lib/amlen-bridge/markers/install-imabridge

%post
#echo "IBM WIoTP - Message Gateway Bridge Standalone RPM install."
/usr/share/amlen-bridge/bin/postInstallBridge.sh

%preun
#echo "IBM WIoTP - Message Gateway Bridge RPM Pre-uninstall."
/usr/share/amlen-bridge/bin/preUninstallBridge.sh "$1"

%postun
#echo "IBM WIoTP - Message Gateway Bridge RPM Post-uninstall."
if [ -n "$IMSTMPDIR" ]; then
    "${IMSTMPDIR}"/postUninstallBridge.sh "$1"
else
    /tmp/postUninstallBridge.sh "$1"
fi

%posttrans
rm -f /var/lib/amlen-bridge/markers/install-imabridge
