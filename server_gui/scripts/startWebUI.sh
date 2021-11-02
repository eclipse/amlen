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
# Script to start Standalone MessageSight Web UI on an Standalone system
# or Docker container.
#if [ -f /tmp/startWebUI.log ];then
#   mv /tmp/startWebUI.log /tmp/startWebUI.$(date +%Y%m%d-%H%M%S).log
#fi

export WLPDIR=${IMA_WEBUI_APPSRV_DATA_PATH}
export WLPINSTALLDIR=${IMA_WEBUI_APPSRV_INSTALL_PATH}
export LDAPDIR=${IMA_WEBUI_DATA_PATH}/openldap-data
export LOGDIR=${IMA_WEBUI_DATA_PATH}/diag/logs
export CFGDIR=${IMA_WEBUI_DATA_PATH}/config
export PID_FILE="${CFGDIR}/imawebui.pid"
export NSSLAPD_INST_ETC_DIR="/etc/dirsrv/slapd-imawebui"
export NSSLAPD_PID_FILE="/run/dirsrv/imawebui.pid"
export START_LOG=${LOGDIR}/startWebUI.log

# Use Java if we shipped it
if [ -d "${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80" ]
then
    export JAVA_HOME="${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80/jre"
    export PATH=${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80/jre/bin:$PATH
fi


if [ ! -f "${LDAPDIR}"/.accountsCreated ]; then
    echo "There was a problem with the rpm post-install script. Please reinstall the rpm." >> "$START_LOG"
    exit 2
fi

chmod 770 "${WLPDIR}"
chmod 770 "${LDAPDIR}"
chmod 770 "${LOGDIR}"
chmod 770 "${CFGDIR}"

if [ -n "${WEBUI_PORT}" ]; then
    sed -i 's/9087/'"${WEBUI_PORT}"'/' "${WLPDIR}"/usr/servers/ISMWebUI/properties.xml
fi

if [ -n "${WEBUI_HOST}" ]; then
    sed -i 's/\"default_http_host\" value=\"\*\"/\"default_http_host\" value=\"'"${WEBUI_HOST}"'\"/' "${WLPDIR}"/usr/servers/ISMWebUI/properties.xml
fi

# fix liberty 18.0.0.4+ problen with ${keystore_id} variable
if [ -f ${WLPDIR}/usr/servers/ISMWebUI/server.xml ]; then
    sed -i 's/${keystore_id}/defaultKeyStore/' ${WLPDIR}/usr/servers/ISMWebUI/server.xml
fi

exec 200> /tmp/imawebui.lock
flock -e -n 200 2> /dev/null
if [ "$?" != "0" ]; then
    echo "IBM IoT MessageSight Web UI process is already running." >> "$START_LOG"
    exit 255
fi

if [ -f "${LDAPDIR}"/.configured389 ]; then
    pid=$(ps -ef | grep "ns-slapd -D /etc/dirsrv/slapd-imawebui" | grep -v grep | awk '{print $2}')
    if [ -z "$pid" ]; then
        if command -v ns-slapd; then
            if ns-slapd -D "$NSSLAPD_INST_ETC_DIR" -i "$NSSLAPD_PID_FILE" >> "${LOGDIR}/ns-slapd.log" 2>&1; then
                echo "389-ds server started successfully." >> "$START_LOG"
                # Wait 5 seconds for 389 server to be responsive
                sleep 5
            else
                echo "389-ds server failed to start." >> "$START_LOG"
                exit 1
            fi
        else
            echo "ns-slapd binary could not be found. Please make sure the 389 server package is installed." >> "$START_LOG"
            exit 1
        fi
    else
        echo "389-ds server was already started." >> "$START_LOG"
    fi
elif [ -f "${LDAPDIR}"/.configuredOpenLDAP ]; then
    if ps -ef | grep slapd | grep "${IMA_WEBUI_DATA_PATH}/config/slapd.conf" | grep -v grep > /dev/null; then
        echo "Embedded LDAP server already running." >> "$START_LOG"
    else
        # I think slapd was not in $PATH on SLES, so had to find full path to run it
        if [ -f "/usr/lib/openldap/slapd" ]; then
            # set proper path to slapd command on SLES
            export slapdcmd="/usr/lib/openldap/slapd"
        elif [ -f "/usr/sbin/slapd" ]; then
            # otherwise use default location on RHEL/CENTOS
            export slapdcmd="/usr/sbin/slapd"
	elif [ -f "/usr/libexec/slapd" ]; then
             # location of slapd when openldap compiled in /usr prefix
            export slapdcmd="/usr/libexec/slapd"
        elif [ -f "/usr/local/libexec/slapd" ]; then
            # location of slapd when openldap compiled in /usr/local prefix
            export slapdcmd="/usr/local/libexec/slapd"
        else
            export slapdcmd=$(which slapd)
        fi

        if [ ! -f "$slapdcmd" ]; then
            echo "Could not find the openldap server binary." >> "$START_LOG"
            exit 1
        fi

        if bash -c "ulimit -n 1024 && ${slapdcmd} -h \"ldap://127.0.0.1:9389\" -f ${CFGDIR}/slapd.conf" >> "${LOGDIR}/openldap.log" 2>&1; then
            echo "OpenLDAP server started successfully." >> "$START_LOG"
        else
            echo "There was a problem starting the OpenLDAP server. See the ${LOGDIR}/openldap.log file." >> "$START_LOG"
            exit 1
        fi
    fi
fi

if [ "$SYSTEMD_STARTED_IMAWEBUI" == "1" ]
then
    #We told systemd (in our .service file) we would return after starting...
    ${WLPINSTALLDIR}/bin/server start ISMWebUI > /dev/null 2>&1
    WLP_RC=$?

    #In start mode, WLP returns 143 in some circumstances but the start is still successful
    if [ "$WLP_RC" -eq "143" ] ; then
        WLP_RC=0
    fi
else
    ${WLPINSTALLDIR}/bin/server run ISMWebUI 2>&1
    WLP_RC=$?
fi

exit $WLP_RC
