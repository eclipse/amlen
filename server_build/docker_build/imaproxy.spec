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

Summary: ${IMA_PRODUCTNAME_FULL} Proxy for Linux x86_64
Name: ${IMA_PKG_PROXY}
Version: ${IMA_PROXY_VERSION_ID}
Release: %{distrel}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications
Source: %{name}.tar.gz
BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
Requires: gdb, openssl, tar, zip, bzip2, unzip, libicu
Obsoletes: IBMIoTMessageSightProxy

%description
${IMA_PRODUCTNAME_FULL} Proxy for Linux x86_64

%prep
%setup -n %{name}

%install
export DONT_STRIP=1
cp -dpR * "$RPM_BUILD_ROOT"

%files
%defattr (-, root, bin)
${IMA_PROXY_INSTALL_PATH}
${IMA_PROXY_DATA_PATH}

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p ${IMA_PROXY_DATA_PATH}/markers
touch ${IMA_PROXY_DATA_PATH}/markers/install-imaproxy

%post
#echo "${IMA_PRODUCTNAME_FULL} Proxy Standalone RPM install."
${IMA_PROXY_INSTALL_PATH}/bin/postInstallProxy.sh

%preun
#echo "${IMA_PRODUCTNAME_FULL} Proxy RPM Pre-uninstall."
${IMA_PROXY_INSTALL_PATH}/bin/preUninstallProxy.sh "$1"

%postun
#echo "${IMA_PRODUCTNAME_FULL} Proxy RPM Post-uninstall."
if [ -n "$IMSTMPDIR" ]; then
    "${IMSTMPDIR}"/postUninstallProxy.sh "$1"
else
    /tmp/postUninstallProxy.sh "$1"
fi

%posttrans
rm -f ${IMA_PROXY_DATA_PATH}/markers/install-imaproxy
