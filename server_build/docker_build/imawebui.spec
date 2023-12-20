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

%define _topdir %(echo $PWD)/..
%define distrel %(day="`date +%j`"; echo "0.$day")
%define _tmppath %{_topdir}/tmp

Summary: ${IMA_PRODUCTNAME_FULL} Web UI for Linux x86_64
Name: ${IMA_PKG_WEBUI}
Version: ${ISM_VERSION_ID}
Release: %{distrel}
License: EPL-2.0
Packager: IMA Build
AutoReqProv: no
Group: Applications/Communications
Source: %{name}.tar.gz
BuildRoot: %{_topdir}/tmp/%{name}-%{Version}.${Release}
#NB: If the java version is changed from 1.8.0 below, the postInstallWebUI (which in edge cases guesses a java path) needs to be updated
Requires: net-tools, openldap, openldap-servers, openldap-clients, tar, openssl, procps, java-1.8.0-openjdk-headless, which
Obsoletes: IBMIoTMessageSightWebUI

%description
${IMA_PRODUCTNAME_FULL} Web UI for Linux x86_64

%prep
%setup -n %{name}

%install
export DONT_STRIP=1
cp -dpR * "$RPM_BUILD_ROOT"

%files
%defattr (-, root, bin)
${IMA_WEBUI_INSTALL_PATH}
${IMA_WEBUI_INSTALL_PATH}/bin/createAcct.sh
${IMA_WEBUI_INSTALL_PATH}/openldap/config/slapd.conf
${IMA_WEBUI_INSTALL_PATH}/openldap/config/users.ldif


%clean
rm -rf "$RPM_BUILD_ROOT"

%pre
#Create a "marker" file during install/upgrade so we can detect the upgrade even if rpm name changes (e.g. MessageSight -> Message Gateway)
mkdir -p ${IMA_WEBUI_DATA_PATH}/markers
touch ${IMA_WEBUI_DATA_PATH}/markers/install-imawebui

if [ -L ${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr ]
then
    rm -f ${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr
fi

%post
#echo "${IMA_PRODUCTNAME_FULL} Web UI Standalone RPM is built"
${IMA_WEBUI_INSTALL_PATH}/bin/postInstallWebUI.sh "$1"

%preun
#echo "${IMA_PRODUCTNAME_FULL} WebUI RPM Pre-uninstall."
${IMA_WEBUI_INSTALL_PATH}/bin/preUninstallWebUI.sh "$1"


%postun
#echo "${IMA_PRODUCTNAME_FULL} WebUI RPM Post-uninstall."
if [ -n "$IMSTMPDIR" ]; then
	"${IMSTMPDIR}"/postUninstallIBMIoTWebUI.sh "$1"
else
	/tmp/postUninstallIBMIoTWebUI.sh "$1"
fi

%posttrans
rm -f ${IMA_WEBUI_DATA_PATH}/markers/install-imawebui

