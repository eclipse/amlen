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
export LDAP_INSTANCE="slapd-imawebui"
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
export NSSLAPD_INST_ETC_DIR="/etc/dirsrv/$LDAP_INSTANCE"
export NSSLAPD_INST_SCHEMA_DIR="${NSSLAPD_INST_ETC_DIR}/schema"
export NSSLAPD_PID_FILE="/run/dirsrv/imawebui.pid"
export NSSLAPD_VAR_RUN_DIR="/var/run/dirsrv"
export NSSLAPD_VAR_RUN_SOCKET="/var/run/${LDAP_INSTANCE}.socket"
export NSSLAPD_VAR_LOCK_DIR="/var/lock/dirsrv/$LDAP_INSTANCE"
export DSEFILE="${NSSLAPD_INST_ETC_DIR}/dse.ldif"
export LDAP_SERVER_UP=0
export LDAP_SUFFIX="dc=ism.ibm,dc=com"

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
        if [ -f "/usr/lib/openldap/slapd" ]; then
            # set proper path to slapd command on SLES
            export slapdcmd="/usr/lib/openldap/slapd"
        elif [ -f "/usr/libexec/slapd" ]; then
            # location of slapd when openldap compiled in /usr prefix
            export slapdcmd="/usr/libexec/slapd"
        elif [ -f "/usr/local/libexec/slapd" ]; then
            # location of slapd when openldap compiled in /usr/local prefix
            export slapdcmd="/usr/local/libexec/slapd"
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
    if ! command -v dscreate > /dev/null 2>&1; then
        echo "$(date): Could not find the dscreate command in the PATH." >> $INSTALL_LOG
        exit 1
    fi

    # dsctl remove <instance> does not cleanup dbs in custom locations
    if [ -f "$LDAPINSTANCEDIR/db/DBVERSION" ]; then
        echo "$(date): Removing existing 389 LDAP database." >> $INSTALL_LOG
        if [ -d "$LDAPINSTANCEDIR/db/userroot" ]; then
            rm -rf "$LDAPINSTANCEDIR/db/userroot"
        fi
        rm -f "$LDAPINSTANCEDIR/db/*"
    fi

    cat << INFFILE > /tmp/imawebui_389.inf
[general]
defaults = 999999999
full_machine_name = localhost.localdomain
start = True
strict_host_checking = False

[slapd]
instance_name = imawebui
port = 9389
root_password = $PWDCLR
secure_port = 9636

self_sign_cert = True
self_sign_cert_valid_months = 120

user = $IMAWEBUI_USER
group = $IMAWEBUI_GROUP

backup_dir = ${LDAPINSTANCEDIR}/backups
db_dir = ${LDAPINSTANCEDIR}/db
db_home_dir = ${LDAPINSTANCEDIR}/db
ldif_dir = ${LDAPINSTANCEDIR}/ldifs
log_dir = ${LDAPINSTANCEDIR}/logs
[backend-userroot]
create_suffix_entry = True
suffix = $LDAP_SUFFIX
INFFILE

    if dscreate -v from-file /tmp/imawebui_389.inf >> ${LOGDIR}/dscreate.log 2>&1; then
        echo "$(date): Successfully created new 389 LDAP instance." >> $INSTALL_LOG
    else
        ldapinstance=$(dsctl -l)
        if [ "$ldapinstance" = "$LDAP_INSTANCE" ]; then
            echo "$(date): There was an issue while creating the 389 LDAP instance, but it was created" >> $INSTALL_LOG
        else
            echo "$(date): We failed to create the 389 LDAP instance. See the ${LOGDIR}/dscreate.log file." >> $INSTALL_LOG
            exit 1
        fi
    fi

    if sed -i 's#^nsslapd-rootpw: supersecret#nsslapd-rootpw: '"$DSEPWD"'#' "${DSEFILE}"; then
        echo "Successfully modified $DSEFILE." >> ${INSTALL_LOG}
    else
        echo "Failed to modify  $DSEFILE." >> ${INSTALL_LOG}
    fi

    # extract the 389 server config; might need to change this first "if condition" if
    #  other distros keep the base 389 config files in another location
    #if [ -f ${INSTALLCFGDIR}/dse.ldif ] && [ -d "$NSSLAPD_INSTALL_ETC" ]; then
    #    mkdir "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to create directory ${NSSLAPD_INST_ETC_DIR}" >> "${INSTALL_LOG}"; exit 1; }
    #    mkdir "$NSSLAPD_INST_SCHEMA_DIR" || { echo "Failed to create directory $NSSLAPD_INST_SCHEMA_DIR" >> "${INSTALL_LOG}"; exit 1; }
    #
    #    if cp "${INSTALLCFGDIR}/dse.ldif" "$NSSLAPD_INST_ETC_DIR"; then
    #        echo "Successfully copied 389 server dse config file." >> ${INSTALL_LOG}
    #    else
    #        echo "Failed to copy 389 server dse config file." >> "${INSTALL_LOG}"
    #        exit 1
    #    fi
    #
    #    # This is the default installed location on RHEL-based distros. Look there first.
    #    if [ -f "${NSSLAPD_INSTALL_ETC_SCHEMA}/99user.ldif" ]; then
    #        cp "${NSSLAPD_INSTALL_ETC_SCHEMA}/99user.ldif" "${NSSLAPD_INST_SCHEMA_DIR}" || { echo "Failed to copy 99user.ldif to ${NSSLAPD_INST_SCHEMA_DIR}" >> "${INSTALL_LOG}"; exit 1; }
    #    else
    #        USER_SCHEMA_FILE=$(find /etc -name 99user.ldif | tail -n 1)
    #        if [ -f "$USER_SCHEMA_FILE" ]; then
    #            cp "$USER_SCHEMA_FILE" "${NSSLAPD_INST_SCHEMA_DIR}" || { echo "Failed to copy 99user.ldif to ${NSSLAPD_INST_SCHEMA_DIR}" >> "${INSTALL_LOG}"; exit 1; }
    #        else
    #            echo "Could not find the schema file for user entries: 99user.ldif." >> ${INSTALL_LOG}
    #            exit 1
    #        fi
    #    fi
    #
    #    # This is the default installed location on RHEL-based distros. Look there first.
    #    if [ -f "${NSSLAPD_INSTALL_ETC}/slapd-collations.conf" ]; then
    #        cp "${NSSLAPD_INSTALL_ETC}/slapd-collations.conf" "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to copy slapd-collations.conf to $NSSLAPD_INST_ETC_DIR" >> "${INSTALL_LOG}"; exit 1; }
    #    else
    #        NSSLAPD_COLLATIONS_FILE=$(find /etc -name slapd-collations.conf | tail -n 1)
    #        cp "${NSSLAPD_COLLATIONS_FILE}" "$NSSLAPD_INST_ETC_DIR" || { echo "Failed to copy slapd-collations.conf to $NSSLAPD_INST_ETC_DIR" >> "${INSTALL_LOG}"; exit 1; }
    #    fi
    #else
    #    echo "There was a problem with either the imawebui package install or the 389 server install." >> "${INSTALL_LOG}"
    #    exit 1
    #fi
}

function find_389_instance() {
    local EXISTING_INST=$(/usr/sbin/dsctl -l)

    if [ -n "$EXISTING_INST" ] && [ "$EXISTING_INST" = "$LDAP_INSTANCE" ]; then
        echo "Found an existing instance: ${EXISTING_INST}. Will attempt to verify it." >> ${INSTALL_LOG}
        if [ -f "$DSEFILE" ] && [ -d "$NSSLAPD_INST_ETC_DIR" ] && [ -d "${LDAPINSTANCEDIR}/db" ]; then
            echo "We will reuse the existing instance: $EXISTING_INST." >> ${INSTALL_LOG}
            export IMA_EXISTING_389="$LDAP_INSTANCE"
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
    #mkdir -p "$NSSLAPD_VAR_RUN_DIR"
    #mkdir -p "$NSSLAPD_VAR_LOCK_DIR"
    #mkdir -p ${LDAPINSTANCEDIR}/db
    #mkdir -p ${LDAPINSTANCEDIR}/backups
    #mkdir -p ${LDAPINSTANCEDIR}/logs
    #touch ${LDAPINSTANCEDIR}/logs/errors

    # starting the server so we can configure it

    #start_389_server
    #search_389_server

    # Creating the suffix base entry in the 389 server
    # We already create this if using dscreate
    #echo "Creating the suffix entry on the 389 server." >> "${INSTALL_LOG}"
    #if dsconf -v $LDAP_INSTANCE backend create --suffix dc=ism.ibm,dc=com --be-name userRoot >> "${INSTALL_LOG}" 2>&1; then
    #    echo "Successfully created the LDAP backend database for suffix dc=ism.ibm,dc=com" >> "${INSTALL_LOG}"
    #else
    #    echo "Failed to create the LDAP backend database for suffix dc=ism.ibm,dc=com" >> "${INSTALL_LOG}"
    #    exit 1
    #fi

    echo "Adding the base LDAP objects required by the WebUI service to the suffix." >> "${INSTALL_LOG}"
    #ldapadd -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' -f ${LDAPDIR}/imawebui_users_389.ldif >> "$INSTALL_LOG" 2>&1
    cp "${LDIFFILE}" /tmp
    chown $IMAWEBUI_USER /tmp/imawebui_users_389.ldif

    stop_389_server

    if dsctl -v $LDAP_INSTANCE ldif2db userRoot /tmp/imawebui_users_389.ldif >> "${LOGDIR}/dsctl-ldif2db.log" 2>&1; then
        echo "Successfully imported LDAP entries for the WebUI suffix." >> "${INSTALL_LOG}"
    else
        echo "Failed to import LDAP entries for the WebUI suffix." >> "${INSTALL_LOG}"
    fi

    start_389_server

    # set cn=Directory Manager,dc=ism.ibm,dc=com as a password admin
    echo "Configuring cn=Directory Manager,dc=ism.ibm,dc=com as a password admin." >> "${INSTALL_LOG}"
    ldapmodify -v -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' >> "$INSTALL_LOG" 2>&1 <<LDAPHERE3
dn: cn=config
changetype: modify
replace: passwordAdminDN
passwordAdminDN: cn=Directory Manager,dc=ism.ibm,dc=com
LDAPHERE3

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully modified the bind dn password." >> ${INSTALL_LOG}
    else
        echo "Failed to modify the bind dn password." >> ${INSTALL_LOG}
        exit $rc
    fi

    # allow hashed passwords
    echo "Configuring the 389 server to allow hashed passwords in LDAP adds." >> "${INSTALL_LOG}"
    ldapmodify -v -D 'cn=Directory Manager' -y ${LDAPDIR}/.key -x -H 'ldap://127.0.0.1:9389' >> "$INSTALL_LOG" 2>&1 <<LDAPHERE4
dn: cn=config
changetype: modify
replace: nsslapd-allow-hashed-passwords
nsslapd-allow-hashed-passwords: on
LDAPHERE4

    # Check the outcome
    rc=$?
    if [ "$rc" = "0" ]; then
        echo "Successfully enabled hashed passwords on add." >> ${INSTALL_LOG}
    else
        echo "Failed to enabled hashed passwords on add." >> ${INSTALL_LOG}
        exit $rc
    fi
}

function generate_389_passwords() {

    LDIFFILE="${LDAPDIR}/imawebui_users_389.ldif"
    
    cp "${INSTALLCFGDIR}/imawebui_users_389.ldif" "$LDIFFILE" || { echo "Failed to copy imawebui_users_389.ldif" >> "${INSTALL_LOG}"; exit 1; }

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

    #Can't do this - haven't created the instance et
    ## modify the password in the dse.ldif file

    if sed -i 's#userPassword: supersecret#userPassword: '"$PWDHASH"'#' "${LDIFFILE}"; then
        echo "Successfully modified ldif file: $LDIFFILE." >> ${INSTALL_LOG}
    else
        echo "Failed to modify ldif file: $LDIFFILE." >> ${INSTALL_LOG}
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

#function start_389_server() {
#    local pid389=$(check_389_server)
#    if [ -z "$pid389" ]; then
#        if /usr/sbin/ns-slapd -D "$NSSLAPD_INST_ETC_DIR" -i "$NSSLAPD_PID_FILE" >> "${LOGDIR}/ns-slapd.log" 2>&1; then
#            # server takes roughly 10 seconds to be responsive
#            search_389_server
#            while (( LDAP_SERVER_UP == 0 )); do
#                sleep 2
#                search_389_server
#            done
#        else
#            echo "Failed to start the 389 server." >> ${INSTALL_LOG}
#            exit 1
#        fi
#    else
#        echo "389 server is already running."
#    fi
#}

#function stop_389_server() {
#    local pid389=""
#    if [ -z "$1" ]; then
#        pid389=$(ps -ef | grep "ns-slapd -D $NSSLAPD_INST_ETC_DIR" | grep -v grep | awk '{print $2}')
#    else
#        pid389="$1"
#    fi
#
#    if [ -z "$pid389" ]; then
#        echo "Could not find pid for the 389 directory server." >> "${INSTALL_LOG}"
#        return 0
#    fi
#
#    count=1
#    while (( count <= 10 )); do
#        echo "Attempt $count to stop the 389 server." >> "${INSTALL_LOG}"
#        kill "$pid389"
#        sleep 3
#        pid389=$(ps -ef | grep "ns-slapd -D $NSSLAPD_INST_ETC_DIR" | grep -v grep | awk '{print $2}')
#        if [ -z "$pid389" ]; then
#            echo "Could not find pid for the 389 directory server. Server must be stopped." >> "${INSTALL_LOG}"
#            return 0
#        fi
#        count=$(( count+1 ))
#    done
#    echo "Could not stop the 389 directory server." >> "${INSTALL_LOG}"
#    return 1
#}

function start_389_server() {
    dsctl $LDAP_INSTANCE start
}

function stop_389_server() {
    dsctl $LDAP_INSTANCE stop
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
    install -m 600 /dev/null "${LDAPDIR}/.key"
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

    cp "${IMA_WEBUI_INSTALL_PATH}/openldap/config/slapd.conf" "${CFGDIR}"/. || { echo "Failed to copy slapd.conf" >> "${INSTALL_LOG}"; exit 1; }
    sed -i 's#rootpw .*#rootpw '"${PWDHASH}"'#' "${CFGDIR}"/slapd.conf

    echo "Updated LDAP password in ${CFGDIR}/slapd.conf." >> ${INSTALL_LOG}
}

function update_liberty_ldap_password() {

    sed -i 's#keystore_imakey" value=.*#keystore_imakey" value="'"${PWDBASE64}"'" />#' "${WLPDIR}"/usr/servers/ISMWebUI/properties.xml
    echo "Updated LDAP password in ${WLPDIR}/usr/servers/ISMWebUI/properties.xml." >> ${INSTALL_LOG}

    PASSWD=$(cat ${LDAPDIR}/.key)

    if [ -z "$PASSWD" ]; then
        echo "Updating liberty: don't have key file" >> ${INSTALL_LOG}
        exit 1
    fi

    genpasswd=true
    numtries=0

    #We sometimes seem to fail to generate the encoded password - so we just retry
    while $genpasswd; do
        lval=$(${WLPINSTALLDIR}/bin/securityUtility encode --encoding=aes ${PASSWD})

        if [ ! -z "$lval" ]; then
            #Successfully encoded the password
            echo "Updating liberty: successfully encoded password" >> ${INSTALL_LOG}
            genpasswd=false
            sleep 1
        else
            #Encoding password weirdly failed
            if [ "$numtries" -lt 10 ]; then
                numtries=$((numtries + 1))
                echo "Updating liberty: failed encoding of password - will retry" >> ${INSTALL_LOG}
            else
                echo "Updating liberty: don't have encoded password - after many retries" >> ${INSTALL_LOG}
                echo "$(ls -l ${WLPINSTALLDIR}/bin/securityUtility)" >> ${INSTALL_LOG}
                exit 1
            fi
        fi
    done


    sed -i 's#bindPassword=.*#bindPassword="'"${lval}"'"#' "${WLPDIR}"/usr/servers/ISMWebUI/ldap.xml 2>&1 >> ${INSTALL_LOG}
    echo "Updated LDAP password in ${WLPDIR}/usr/servers/ISMWebUI/ldap.xml." >> ${INSTALL_LOG}
}

# Handle various cases of webui app install
if [ ! -d "${WLPDIR}"/usr ]
then
    echo "${WLPDIR}/usr does not exist at start."  >> ${INSTALL_LOG}
    if [ -d ${WLPINSTALLDIR}/usr ]
    then
        cp -r ${WLPINSTALLDIR}/usr "${WLPDIR}"/.
        rm -rf ${WLPINSTALLDIR}/usr.org
        echo "Copied ${WLPINSTALLDIR}/usr to ${WLPDIR}/usr"  >> ${INSTALL_LOG}
    else
        if [ -d ${WLPINSTALLDIR}/usr.org ]
        then
            cp -r ${WLPINSTALLDIR}/usr.org "${WLPDIR}"/usr
            echo "Contents of ${WLPINSTALLDIR}/usr.org  copied into ${WLPDIR}/usr"  >> ${INSTALL_LOG}
        else
            echo "Can not continue. WebUI Application directory is missing." >> ${INSTALL_LOG}
            exit 255
        fi
    fi
else
    echo "${WLPDIR}/usr exists at start."  >> ${INSTALL_LOG}

    if [ ! -L ${WLPINSTALLDIR}/usr ]
    then
        if [ -d ${WLPINSTALLDIR}/usr ]
        then
            cp ${WLPINSTALLDIR}/usr/servers/ISMWebUI/apps/ISMWebUI.war "${WLPDIR}"/usr/servers/ISMWebUI/apps/.
            echo "Updating ${WLPDIR}/usr/servers/ISMWebUI/apps/ISMWebUI.war" >> ${INSTALL_LOG}
        fi
    fi
fi

# Use Java if we shipped it
if [ -d "${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80" ]
then
    export JAVA_HOME="${IMA_WEBUI_INSTALL_PATH}/ibm-java-x86_64-80"
fi

# Configure LDAP
if [ ! -f "${LDAPDIR}"/.accountsCreated ]; then
    if ps -ef | grep slapd | grep "webui/config/slapd.conf" | grep -v grep > /dev/null; then
        echo "Found an old WebUI ldap process running.  Killing this process before we continue." >> ${INSTALL_LOG}
        kill -9 $(ps -ef | grep slapd | grep "webui/config/slapd.conf" | grep -v grep | awk '{print $2}')
    fi

    # slapd is in /usr/sbin/slapd for CentOS/RHEL/Debian
    # slapd is in /usr/lib/openldap/slapd on SLES
    # if we need to support other distros we may need to update this test below
    # openldap server is the default so we check for it first
    if [ -f "/usr/sbin/slapd" ] || [ -f "/usr/lib/openldap/slapd" ] || [ -f "/usr/libexec/slapd" ]; then
        # we found openldap server
        echo "Starting OpenLDAP config" >> ${INSTALL_LOG}
        generate_openldap_password
        configure_openldap_server
        touch "${LDAPDIR}"/.configuredOpenLDAP
    elif [ -f "/usr/sbin/ns-slapd" ]; then
        # we found 389 ds server
        find_389_instance
        if [ "$IMA_EXISTING_389" = "$LDAP_INSTANCE" ]; then
            echo "Existing 389 server instance $IMA_EXISTING_INSTANCE will be used." >> ${INSTALL_LOG}
        else
            generate_389_passwords
            create_389_instance
            configure_389_server
            touch "${LDAPDIR}"/.configured389
        fi
    else
        echo "Could not find a supported LDAP server." >> ${INSTALL_LOG}
        exit 1
    fi
    update_liberty_ldap_password

    ${WEBUIBINDIR}/createAcct.sh "${LDAPDIR}/.key" 2>&1 >> "${LOGDIR}/createAcct.log"

    if [ "$?" -eq "0" ]; then
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
    echo "WebUI LDAP account config previously completed - leaving unchanged" >> ${INSTALL_LOG}
fi

if (( LDAP_DEBUG == 0 )); then
    rm -f "${LDAPDIR}/.key"
fi

${WEBUIBINDIR}/setUserGroupWebUI.sh    "${IMAWEBUI_USER}" "${IMAWEBUI_GROUP}" >> ${INSTALL_LOG}

if (( NONROOT == 1 )); then
    if [ -f "${LDAPDIR}/.configured389" ]; then
        sed -i 's/^nsslapd-localuser:.*/nsslapd-localuser: '"${IMAWEBUI_USER}"'/' "$DSEFILE"

        # disable the ldapi socket since it requires root and we don't need it
        sed -i '/^nsslapd-ldapifilepath/d' "$DSEFILE"
        sed -i '/^nsslapd-ldapilisten/d' "$DSEFILE"
        sed -i '/^nsslapd-ldapiautobind/d' "$DSEFILE"
        sed -i '/^nsslapd-ldapimaprootdn/d' "$DSEFILE"

        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_VAR_RUN_DIR"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_VAR_LOCK_DIR"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_INST_ETC_DIR"
        if [ -f "$NSSLAPD_VAR_RUN_SOCKET" ]; then
            chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "$NSSLAPD_VAR_RUN_SOCKET"
        fi
    else
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "${IMA_WEBUI_INSTALL_PATH}"
        chown -R ${IMAWEBUI_USER}:${IMAWEBUI_GROUP} "${IMA_WEBUI_DATA_PATH}"
    fi
fi
