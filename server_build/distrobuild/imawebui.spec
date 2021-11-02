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

#We expect the source to be in a directory called this, zipped to a file with the same name (+.zip)
#This is the default format when I download the source from a branch in GHE
%define sourcename messagegateway-buildoss


#Disable debuginfo generation and stripping
%define __os_install_post %{nil}
%define debug_package %{nil}


Summary: Amlen Web UI for Linux x86_64
Name: AmlenWebUI
Version: 0.1.0.0
Release: 1%{?dist}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications

Source0: %{sourcename}.zip
Source1: imawebui-deps.zip

BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
BuildRequires: rpm-build,junit,ant-contrib,ant,java-11-openjdk-devel,icu,javapackages-tools,dos2unix
Requires: net-tools, openldap, openldap-servers, openldap-clients, tar, openssl, procps
Obsoletes: IBMIoTMessageSightWebUI < 6.0

%description
Amlen Web UI for Linux x86_64

%prep
%setup -n %{sourcename}


%build
export BUILD_LABEL="$(date +%Y%m%d-%H%M)_git_private"
#Where the source is
export SROOT=$RPM_BUILD_DIR/%{sourcename}
#Where binaries are built and assembled
export BROOT=$RPM_BUILD_DIR/broot
#Where we should arrange files the hierarchy for the RPM:
export IMAGUI_BASE_DIR=$BROOT/rpmtree
export USE_REAL_TRANSLATIONS=true
export SLESNORPMS=yes
mkdir $BROOT

mkdir $RPM_BUILD_DIR/deps
export DEPS_HOME=$RPM_BUILD_DIR/deps
cd $DEPS_HOME
unzip %{_sourcedir}/imawebui-deps.zip

cd $SROOT/server_build
#Override versions/names/paths from the defaults in server_build/paths.properties 
#export IMA_PATH_PROPERTIES=${SROOT}/server_build/ossbuild/samplerebrand/amlen-paths.properties
ant -f $SROOT/server_build/build.xml buildwebui-oss

%install
export DONT_STRIP=1
export IMAGUI_BASE_DIR=$RPM_BUILD_DIR/broot/rpmtree
#cp -dpR * "$RPM_BUILD_ROOT"
#mv $IMAGUI_BASE_DIR/* "$RPM_BUILD_ROOT/"
mv $IMAGUI_BASE_DIR/* "%{buildroot}/"

%files
%defattr (-, root, bin)
/usr/share/amlen-webui

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p /var/lib/amlen-webui/markers
touch /var/lib/amlen-webui/markers/install-imawebui

if [ -L /usr/share/amlen-webui/wlp/usr ]
then
    rm -f /usr/share/amlen-webui/wlp/usr
fi

%post
/usr/share/amlen-webui/bin/postInstallWebUI.sh "$1"

%preun
/usr/share/amlen-webui/bin/preUninstallWebUI.sh "$1"


%postun
if [ -n "$IMSTMPDIR" ]; then
	"${IMSTMPDIR}"/postUninstallIBMIoTWebUI.sh "$1"
else
	/tmp/postUninstallIBMIoTWebUI.sh "$1"
fi

%posttrans
rm -f /var/lib/amlen-webui/markers/install-imawebui

