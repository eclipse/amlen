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
# Post install script to initialize ${IMA_PRODUCTNAME_FULL} WebUI service on a linux system

# Directories
export WLPDIR=${IMA_WEBUI_APPSRV_DATA_PATH}
export WLPINSTALLDIR=${IMA_WEBUI_APPSRV_INSTALL_PATH}
export LDAPDIR=${IMA_WEBUI_DATA_PATH}/openldap-data
export LOGDIR=${IMA_WEBUI_DATA_PATH}/diag/logs
export CFGDIR=${IMA_WEBUI_DATA_PATH}/config
export WEBUIBINDIR=${IMA_WEBUI_INSTALL_PATH}/bin
export INSTALLCFGDIR=${IMA_WEBUI_INSTALL_PATH}/config
export LDAPINSTANCEDIR=${IMA_WEBUI_DATA_PATH}/ldap-instance
export NSSLAPD_INSTALL_ETC="/etc/dirsrv"
export NSSLAPD_INSTALL_ETC_SCHEMA="${NSSLAPD_INSTALL_ETC}/schema"
export NSSLAPD_INST_ETC_DIR="/etc/dirsrv/slapd-imawebui"
export NSSLAPD_INST_SCHEMA_DIR="${NSSLAPD_INST_ETC_DIR}/schema"
export NSSLAPD_PID_FILE="/run/dirsrv/imawebui.pid"
export NSSLAPD_VAR_RUN_DIR="/var/run/dirsrv"
export NSSLAPD_VAR_LOCK_DIR="/var/lock/dirsrv/slapd-imawebui"
export DSEFILE="${NSSLAPD_INST_ETC_DIR}/dse.ldif"
export LDAP_SERVER_UP=0

if [ -z "$LDAP_DEBUG" ]; then
    LDAP_DEBUG=0
fi

# Create required directories
mkdir -p -m 770 ${WLPDIR} > /dev/null 2>&1
mkdir -p -m 770 ${LDAPDIR} > /dev/null 2>&1
mkdir -p -m 770 ${LOGDIR} > /dev/null 2>&1
mkdir -p -m 770 ${CFGDIR} > /dev/null 2>&1

# Create install log file
chmod -R 770 ${LDAPDIR}
INSTALL_LOG=${LOGDIR}/postInstallWebUI.log
export INSTALL_LOG

touch ${INSTALL_LOG}

echo "--------------------------------------------------"  >> ${INSTALL_LOG}
echo "Configuring WebUI Server " >> ${INSTALL_LOG}
echo "Date: `date` " >> ${INSTALL_LOG}

#Work out what user and group we should be running as
source ${WEBUIBINDIR}/getUserGroupWebUI.sh >> ${INSTALL_LOG}

#Create user+group if necessary and configure us to run as them:
${WEBUIBINDIR}/createUserGroupWebUI.sh "${IMAWEBUI_USER}" "${IMAWEBUI_GROUP}" >> ${INSTALL_LOG}

# Unpack the IBM Java JRE TGZ file
for f in ${IMA_WEBUI_INSTALL_PATH}/ibm-java-jre-8.0-*.tgz
do
    if [ -e "$f" ]
    then
        echo "Unpack IBM Java JRE" >> ${INSTALL_LOG}
        tar -xzf "$f" -C ${IMA_WEBUI_INSTALL_PATH}
        # Removing the java tarball causes a warning from rpm since it can't find
        # this file later when we go to remove it.  So let's keep it.
        # See defect 19197. So we're not going to remove this anymore.
        # Will this one be replaced by the new one when we get updated?
        # /usr/bin/rm "$f"
    fi
    break
done

# Remove old version of application
if [ -d ${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr.org ]
then
    rm -rf ${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr.org
fi

#
# Install imawebui systemd unit file
#
if [ -d "/etc/systemd/system" ]
then
    # Update systemd (including removing old service name)
    rm -f /etc/systemd/system/IBMIoTMessageSightWebUI.service
    cp ${IMA_WEBUI_INSTALL_PATH}/config/imawebui.service /etc/systemd/system/.
    ln -s /etc/systemd/system/imawebui.service /etc/systemd/system/IBMIoTMessageSightWebUI.service

    INIT_SYSTEM=$(ps --no-headers -o comm 1)
    if [ "$INIT_SYSTEM" == "systemd" ]
    then
        systemctl daemon-reload
    fi
fi

if [ -n "$IMAWEBUI_USER" ]; then
    if [ "$IMAWEBUI_USER" != "root" ]; then
        echo "Configuring a non-root configuration with user $IMAWEBUI_USER" >> ${INSTALL_LOG}
        export NONROOT=1
    fi
fi

function configure_openldap_server() {
    # Start the OpenLDAP server so we can configure it
    # slapd allocates a large chunk of memory based on the value of ulimit -n, i.e. RLIMIT_NOFILE, the internal slapd process
    # does not require more than 1024 open file descriptors

    if ps -ef | grep slapd | grep "${CFGDIR}/slapd.conf" | grep -v grep > /dev/null; then
        echo "Embedded LDAP server already running." >> ${INSTALL_LOG}
    else
        if [ "$IMAOS" = "sles" ]; then
            # set proper path to slapd command on SLES
            export slapdcmd="/usr/lib/openldap/slapd"
        else
            # otherwise use default location on RHEL/CENTOS
            export slapdcmd="/usr/sbin/slapd"
        fi
        if bash -c "ulimit -n 1024 && ${slapdcmd} -h \"ldap://127.0.0.1:9389\" -f ${CFGDIR}/slapd.conf" >> "${LOGDIR}/openldap.log" 2>&1; then
            echo "OpenLDAP server started successfully." >> ${INSTALL_LOG}
        else
            echo "There was a problem starting the OpenLDAP server. See the ${LOGDIR}/openldap.log file." >> ${INSTALL_LOG}
            exit 1
        fi
    fi
}

function create_389_instance() {
    # extract the 389 server config; might need to change this first "if condition" if
    #  other distros keep the base 389 config files in another location
    if [ -f ${INSTALLCFGDIR}/dse.ldif ] && [ -d "$NSSLAPD_INSTALL_ETC" ]; then
        mkdir "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to create directory ${NSSLAPD_INST_ETC_DIR}" >> "${INSTALL_LOG}"; exit 1; }
        mkdir "$NSSLAPD_INST_SCHEMA_DIR" || { echo "Failed to create directory $NSSLAPD_INST_SCHEMA_DIR" >> "${INSTALL_LOG}"; exit 1; }

        if cp "${INSTALLCFGDIR}/dse.ldif" "$NSSLAPD_INST_ETC_DIR"; then
            echo "Successfully copied 389 server dse config file." >> ${INSTALL_LOG}
        else
            echo "Failed to copy 389 server dse config file." >> "${INSTALL_LOG}"
            exit 1
        fi

        # This is the default installed location on RHEL-based distros. Look there first.
        if [ -f "${NSSLAPD_INSTALL_ETC_SCHEMA}/99user.ldif" ]; then
            cp "${NSSLAPD_INSTALL_ETC_SCHEMA}/99user.ldif" "${NSSLAPD_INST_SCHEMA_DIR}" || { echo "Failed to copy 99user.ldif to ${NSSLAPD_INST_SCHEMA_DIR}" >> "${INSTALL_LOG}"; exit 1; }
        else
            USER_SCHEMA_FILE=$(find /etc -name 99user.ldif | tail -n 1)
            if [ -f "$USER_SCHEMA_FILE" ]; then
                cp "$USER_SCHEMA_FILE" "${NSSLAPD_INST_SCHEMA_DIR}" || { echo "Failed to copy 99user.ldif to ${NSSLAPD_INST_SCHEMA_DIR}" >> "${INSTALL_LOG}"; exit 1; }
            else
                echo "Could not find the schema file for user entries: 99user.ldif." >> ${INSTALL_LOG}
                exit 1
            fi
        fi

        # This is the default installed location on RHEL-based distros. Look there first.
        if [ -f "${NSSLAPD_INSTALL_ETC}/slapd-collations.conf" ]; then
            cp "${NSSLAPD_INSTALL_ETC}/slapd-collations.conf" "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to copy slapd-collations.conf to $NSSLAPD_INST_ETC_DIR" >> "${INSTALL_LOG}"; exit 1; }
        else
            NSSLAPD_COLLATIONS_FILE=$(find /etc -name slapd-collations.conf | tail -n 1)
            cp "${NSSLAPD_COLLATIONS_FILE}" "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to copy slapd-collations.conf to $NSSLAPD_INST_ETC_DIR" >> "${INSTALL_LOG}"; exit 1; }
        fi
    else
        echo "There was a problem with either the imawebui package install or the 389 server install." >> "${INSTALL_LOG}"
        exit 1
    fi
}

function find_389_instance() {
    local EXISTING_INST=$(/usr/sbin/dsctl -l)

    if [ -n "$EXISTING_INST" ] && [ "$EXISTING_INST" = "slapd-imawebui" ]; then
        echo "Found an existing instance: ${EXISTING_INST}. Will attempt to verify it." >> ${INSTALL_LOG}
        if [ -f "$DSEFILE" ] && [ -d "$NSSLAPD_INST_ETC_DIR" ] && [ -d "${LDAPINSTANCEDIR}/db" ]; then
            echo "We will reuse the existing instance: $EXISTING_INST." >> ${INSTALL_LOG}
            export IMA_EXISTING_389="slapd-imawebui"
        else
            echo "Did not find a usable instance. We will create a new 389 instance." >> ${INSTALL_LOG}
        fi
    else
        echo "Did not find a usable instance. We will create a new 389 instance." >> ${INSTALL_LOG}
    fi
}

function search_389_server() {
    echo "Testing the ability to search against the LDAP server." >> "${INSTALL_LOG}"
    if ldapsearch -x -H ldap://localhost:9389 -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -s base -b "" 'objectclass=*' > /dev/null 2>&1; then
        echo "Was able to search the server successfully." >> ${INSTALL_LOG}
        LDAP_SERVER_UP=1
    else
        echo "Was not able to search the server successfully." >> ${INSTALL_LOG}
        LDAP_SERVER_UP=0
    fi
}

function configure_389_server() {
    mkdir -p "$NSSLAPD_VAR_RUN_DIR"
    mkdir -p "$NSSLAPD_VAR_LOCK_DIR"
    mkdir -p ${LDAPINSTANCEDIR}/db
    mkdir -p ${LDAPINSTANCEDIR}/backups
    mkdir -p ${LDAPINSTANCEDIR}/logs
    touch ${LDAPINSTANCEDIR}/logs/errors

    # starting the server so we can configure it

    start_389_server
    search_389_server

    # 389 has a different way of creating a suffix entry in the config on the server
    echo "Creating the suffix config entry on the 389 server." >> "${INSTALL_LOG}"
    ldapmodify -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' >> "$INSTALL_LOG" 2>&1 <<LDAPHERE1
dn: cn="dc=ism.ibm,dc=com",cn=mapping tree,cn=config
changetype: add
objectClass: top
objectclass: extensibleObject
objectclass: nsMappingTree
nsslapd-state: backend
nsslapd-backend: UserData
cn: dc=ism.ibm,dc=com
LDAPHERE1

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully added the suffix confing entry to the LDAP server config." >> ${INSTALL_LOG}
    else
        echo "Did not add the suffix config entry to the LDAP server config successfully." >> ${INSTALL_LOG}
        exit $rc
    fi

    # Creating the suffix base entry in the 389 server
    echo "Creating the suffix entry on the 389 server." >> "${INSTALL_LOG}"
    ldapmodify -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' >> "$INSTALL_LOG" 2>&1 <<LDAPHERE2
dn: cn=UserData,cn=ldbm database,cn=plugins,cn=config
changetype: add
objectclass: extensibleObject
objectclass: nsBackendInstance
nsslapd-suffix: dc=ism.ibm,dc=com
LDAPHERE2

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully created the suffix entry in the LDAP server." >> ${INSTALL_LOG}
    else
        echo "Did not add the suffix entry to the LDAP server config successfully." >> ${INSTALL_LOG}
        exit $rc
    fi

    echo "Adding the base LDAP objects required by the WebUI service to the suffix." >> "${INSTALL_LOG}"
    ldapadd -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' -f ${LDAPDIR}/imawebui_users_389.ldif >> "$INSTALL_LOG" 2>&1

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully added the base entries to the LDAP server." >> ${INSTALL_LOG}
    else
        echo "Did not add the base entries to the LDAP server successfully." >> ${INSTALL_LOG}
        exit $rc
    fi

    # for some reason this user's password never works unless I modify it
    echo "Adding an LDAP bind user required by the WebUI service to the suffix." >> "${INSTALL_LOG}"
    ldapmodify -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' >> "$INSTALL_LOG" 2>&1 <<LDAPHERE3
dn: cn=Directory Manager,dc=ism.ibm,dc=com
changetype: modify
replace: userPassword
userPassword: $PWDCLR
LDAPHERE3

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully modified the bind dn password." >> ${INSTALL_LOG}
    else
        echo "Failed to modify the bind dn password." >> ${INSTALL_LOG}
        exit $rc
    fi
}

function generate_389_passwords() {

    LDIFFILE="${LDAPDIR}/imawebui_users_389.ldif"
    if [ -f "${INSTALLCFGDIR}/imawebui_users_389.ldif" ]; then
        cp "${INSTALLCFGDIR}/imawebui_users_389.ldif" "$LDIFFILE" || { echo "Failed to copy imawebui_users_389.ldif" >> "${INSTALL_LOG}"; exit 1; }
    fi

    cd "${LDAPDIR}" || { echo "Failed to change to directory $LDAPDIR" >> "${INSTALL_LOG}"; exit 1; }

    if command -v pwgen > /dev/null 2>&1; then
        if /usr/bin/pwgen -s -c -n 20 1 | tr -d "\n" > ${LDAPDIR}/.key 2>> "${INSTALL_LOG}"; then
            PWDCLR=$(cat ${LDAPDIR}/.key)
        else
            echo "There was a problem creating a new password with pwgen." >> "${INSTALL_LOG}"
            exit 1
        fi
    else
        echo "You must install the pwgen package to continue." >> ${INSTALL_LOG}
        exit 1
    fi

    DSEPWD=$(/usr/bin/pwdhash -s SHA512 $PWDCLR)
    if [ -z "$DSEPWD" ]; then
        echo "Failed to create password hash for dse.ldif file." >> ${INSTALL_LOG}
        exit 1
    fi

    PWDHASH=$(/usr/bin/pwdhash -s SHA512 $(cat ${LDAPDIR}/.key) | tr -d "\n")
    if [ -z "$PWDHASH" ]; then
        echo "Failed to create password hash." >> ${INSTALL_LOG}
        exit 1
    fi

    PWDBASE64=$(base64 ${LDAPDIR}/.key | tr -d "\n")
    if [ -z "$PWDBASE64" ]; then
        echo "Failed to create encoded password." >> ${INSTALL_LOG}
        exit 1
    fi

    chmod 600 ${LDAPDIR}/.key

    # modify the password in the dse.ldif file
    if sed -i 's#^nsslapd-rootpw: supersecret#nsslapd-rootpw: '"$DSEPWD"'#' "${DSEFILE}"; then
        echo "Successfully modified $DSEFILE." >> ${INSTALL_LOG}
    fi

    if sed -i 's#userPassword: secret#userPassword: '"$PWHASH"'#' "${LDIFFILE}"; then
        echo "Successfully modified ldif file: $LDIFFILE." >> ${INSTALL_LOG}
    fi
}

function check_openldap_server() {
    local pidslapd=$(ps -ef | grep slapd | grep "webui/config/slapd.conf" | grep -v grep | awk '{print $2}')
    if [ -n "$pidslapd" ]; then
        echo "$pidslapd"
    else
        echo ""
    fi
}

function check_389_server() {
    local pid389=$(ps -ef | grep "ns-slapd -D $NSSLAPD_INST_ETC_DIR" | grep -v grep | awk '{print $2}')
    if [ -n "$pid389" ]; then
        echo "$pid389"
    else
        echo ""
    fi
}

function start_389_server() {
    local pid389=$(check_389_server)
    if [ -z "$pid389" ]; then
        if /usr/sbin/ns-slapd -D "$NSSLAPD_INST_ETC_DIR" -i "$NSSLAPD_PID_FILE" >> "${LOGDIR}/ns-slapd.log" 2>&1; then
            # server takes roughly 10 seconds to be responsive
            search_389_server
            while (( LDAP_SERVER_UP == 0 )); do
                sleep 2
                search_389_server
            done
        else
            echo "Failed to start the 389 server." >> ${INSTALL_LOG}
            exit 1
        fi
    else
        echo "389 server is already running."
    fi
}

function stop_389_server() {
    local pid389="$1"
    if [ -n "$pid389" ]; then
        kill $pid389
        sleep 2
    else
        echo "No pid for 389 server found."
    fi
}

function stop_openldap_server() {
    local pidslapd="$1"
    if [ -n "$pidslapd" ]; then
        kill $pidslapd
        sleep 2
    else
        echo "No pid for openldap server found." >> ${INSTALL_LOG}
    fi
}

function generate_openldap_password() {

    LDIFFILE="${LDAPDIR}/imawebui_users_openldap.ldif"
    if [ -f "${INSTALLCFGDIR}/imawebui_users_openldap.ldif" ]; then
        cp "${INSTALLCFGDIR}/imawebui_users_openldap.ldif" "$LDIFFILE" || { echo "Failed to copy imawebui_users_openldap.ldif" >> "${INSTALL_LOG}"; exit 1; }
    fi

    cd "${LDAPDIR}" || exit 2
    slappasswd -g -n > "${LDAPDIR}/.key"

    PWDHASH=$(/usr/sbin/slappasswd -h {SSHA} -T ${LDAPDIR}/.key)
    if [ -z "$PWDHASH" ]; then
        echo "Failed to create password hash." >> ${INSTALL_LOG}
        exit 1
    fi

    PWDBASE64=$(base64 ${LDAPDIR}/.key)
    if [ -z "$PWDBASE64" ]; then
        echo "Failed to create encoded password." >> ${INSTALL_LOG}
        exit 1
    fi

    chmod 600 ${LDAPDIR}/.key
    cp "${IMA_WEBUI_INSTALL_PATH}/openldap/config/slapd.conf" "${CFGDIR}"/. || { echo "Failed to copy slapd.conf" >> "${INSTALL_LOG}"; exit 1; }
    sed -i 's#rootpw .*#rootpw '"${PWDHASH}"'#' "${CFGDIR}"/slapd.conf
}

function update_liberty_ldap_password() {
    sed -i 's#keystore_imakey" value=.*#keystore_imakey" value="'"${PWDBASE64}"'" />#' "${WLPDIR}"/usr/servers/ISMWebUI/properties.xml
    lval=$(${WLPINSTALLDIR}/bin/securityUtility encode --encoding=aes $(cat ${LDAPDIR}/.key))

    sed -i 's#bindPassword=.*#bindPassword="'"${lval}"'"#' "${WLPDIR}"/usr/servers/ISMWebUI/ldap.xml
}

# Handle various cases of webui app install
if [ ! -d "${WLPDIR}"/usr ]
then
    if [ -L "${WLPINSTALLDIR}"/usr ]
    then
        if [ -d "${WLPINSTALLDIR}"/usr.org ]
        then
            cp -r ${WLPINSTALLDIR}/usr.org "${WLPDIR}"/usr
        else
            echo "Can not continue. WebUI Application directory is missing." >> ${INSTALL_LOG}
            exit 255
        fi
    else
        if [ -d ${WLPINSTALLDIR}/usr ]
        then
            cp -r ${WLPINSTALLDIR}/usr "${WLPDIR}"/.
            rm -rf ${WLPINSTALLDIR}/usr.org
            mv ${WLPINSTALLDIR}/usr ${WLPINSTALLDIR}/usr.org
            ln -s "${WLPDIR}"/usr ${WLPINSTALLDIR}/usr
        else
            if [ -d ${WLPINSTALLDIR}/usr.org ]
            then
                cp -r ${WLPINSTALLDIR}/usr.org "${WLPDIR}"/usr
                ln -s "${WLPDIR}"/usr ${WLPINSTALLDIR}/usr
            else
                echo "Can not continue. WebUI Application directory is missing." >> ${INSTALL_LOG}
                exit 255
            fi
        fi
    fi
else
    if [ ! -L ${WLPINSTALLDIR}/usr ]
    then
        if [ -d ${WLPINSTALLDIR}/usr ]
        then
            cp ${WLPINSTALLDIR}/usr/servers/ISMWebUI/apps/ISMWebUI.war "${WLPDIR}"/usr/servers/ISMWebUI/apps/.
            rm -rf ${WLPINSTALLDIR}/usr.org
            mv ${WLPINSTALLDIR}/usr ${WLPINSTALLDIR}/usr.org
            ln -s "${WLPDIR}"/usr ${WLPINSTALLDIR}/usr
        fi
    fi
fi

# Configure LDAP
export JAVA_HOME="${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80"
if [ ! -f "${LDAPDIR}"/.accountsCreated ]; then
    if ps -ef | grep slapd | grep "webui/config/slapd.conf" | grep -v grep > /dev/null; then
        echo "Found an old WebUI ldap process running.  Killing this process before we continue." >> ${INSTALL_LOG}
        kill -9 $(ps -ef | grep slapd | grep "webui/config/slapd.conf" | grep -v grep | awk '{print $2}')
    fi

    # slapd is in /usr/sbin/slapd for CentOS/RHEL/Debian
    # slapd is in /usr/lib/openldap/slapd on SLES
    # if we need to support other distros we may need to update this test below
    # openldap server is the default so we check for it first
    if [ -f "/usr/sbin/slapd" ] || [ -f "/usr/lib/openldap/slapd" ]; then
        # we found openldap server
        generate_openldap_password
        configure_openldap_server
        touch "${LDAPDIR}"/.configuredOpenLDAP
    elif [ -f "/usr/sbin/ns-slapd" ]; then
        # we found 389 ds server
        find_389_instance
        if [ "$IMA_EXISTING_389" = "slapd-imawebui" ]; then
            echo "Existing 389 server instance $IMA_EXISTING_INSTANCE will be used." >> ${INSTALL_LOG}
        else
            create_389_instance
            generate_389_passwords
            configure_389_server
            touch "${LDAPDIR}"/.configured389
        fi
    else
        echo "Could not find a supported LDAP server." >> ${INSTALL_LOG}
        exit 1
    fi
    update_liberty_ldap_password

    if ${WEBUIBINDIR}/createAcct.sh "${LDAPDIR}/.key" >> "${LOGDIR}/createAcct.log" 2>&1 ; then
        touch "${LDAPDIR}"/.accountsCreated

        if [ -f "${LDAPDIR}"/.configuredOpenLDAP ]; then
            slapdpid=$(check_openldap_server)
            if [ -n "$slapdpid" ]; then
                stop_openldap_server "$slapdpid"
            fi
        elif [ -f "${LDAPDIR}"/.configured389 ]; then
            serverpid=$(check_389_server)
            if [ -n "$serverpid" ]; then
                stop_389_server "$serverpid"
            fi
        fi
    else
        echo "There was an issue creating accounts for imawebui. See the ${LOGDIR}/createAcct.log" >> ${INSTALL_LOG}
        exit 1
    fi
else
    echo "WebUI install config already completed." >> ${INSTALL_LOG}
fi

if (( LDAP_DEBUG == 0 )); then
    rm -f "${LDAPDIR}/.key"
fi

${WEBUIBINDIR}/setUserGroupWebUI.sh    "${IMAWEBUI_USER}" "${IMAWEBUI_GROUP}" >> ${INSTALL_LOG}

if (( NONROOT == 1 )); then
    if [ -f "/usr/sbin/ns-slapd" ]; then
        sed -i 's/^nsslapd-localuser:.*/nsslapd-localuser: '"${IMAWEBUI_USER}"'/' "$DSEFILE"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_VAR_RUN_DIR"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_VAR_LOCK_DIR"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_INST_ETC_DIR"
    fi
fi
