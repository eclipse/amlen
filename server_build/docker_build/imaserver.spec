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

%define _topdir %(echo $PWD)/..
%define distrel %(day="`date +%j`"; echo "0.$day")
%define _tmppath %{_topdir}/tmp

Summary: $[IMA_SVR_COMPONENT_NAME_FULL} for Linux x86_64
Name: ${IMA_PKG_SERVER}
Version: ${ISM_VERSION_ID}
Release: 1%{?dist}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications
Source: %{name}.tar.gz
BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
Requires: gdb, net-tools, openssl, tar, perl, procps >= 3.3.9, libjansson.so.4()(64bit), logrotate, zip, bzip2, unzip, boost, libicu, net-snmp, python3, python3-requests, python3-pyyaml
Obsoletes: IBMIoTMessageSightServer

%description
${IMA_SVR_COMPONENT_NAME_FULL} for Linux x86_64

%prep
%setup -n %{name}

%install
export DONT_STRIP=1
cp -dpR * "$RPM_BUILD_ROOT"

%files
%defattr (-, root, bin)
${IMA_SVR_INSTALL_PATH}

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p ${IMA_SVR_DATA_PATH}/markers
touch ${IMA_SVR_DATA_PATH}/markers/install-imaserver

%post
#echo "$[IMA_SVR_COMPONENT_NAME_FULL} Standalone RPM install."
${IMA_SVR_INSTALL_PATH}/bin/postInstallServer.sh

%preun
#echo "$[IMA_SVR_COMPONENT_NAME_FULL} RPM Pre-uninstall."
${IMA_SVR_INSTALL_PATH}/bin/preUninstallServer.sh "$1"

%postun
#echo "$[IMA_SVR_COMPONENT_NAME_FULL} RPM Post-uninstall."
if [ -n "$IMSTMPDIR" ]; then
    "${IMSTMPDIR}"/postUninstallIBMIoTServer.sh "$1"
else
    /tmp/postUninstallIBMIoTServer.sh "$1"
fi

%posttrans
rm -f ${IMA_SVR_DATA_PATH}/markers/install-imaserver
