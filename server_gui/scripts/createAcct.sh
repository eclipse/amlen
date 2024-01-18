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
# Invokes ldapmodify to create default ISM login account

export LDAPDIR="${IMA_WEBUI_DATA_PATH}/openldap-data"
export LOGDIR="${IMA_WEBUI_DATA_PATH}/diag/logs"

secret="$1"

LOG="${LOGDIR}/ldapmodify.log"
echo "-----------------" >> "${LOG}"
echo "Updating LDAP users" >> "${LOG}"
echo "Date: `date`" >> "${LOG}"
echo >> "${LOG}"

if [[ $# -lt 3 ]]; then

    # we just started the ldap server
    sleep 3

    if [ -f "${LDAPDIR}/.configuredOpenLDAP" ]; then
        /usr/bin/ldapmodify -v -H "ldap://127.0.0.1:9389" -D "cn=Directory Manager,dc=ism.ibm,dc=com" -y "${secret}" -x -c -a -f ${LDAPDIR}/imawebui_users_openldap.ldif >> "${LOG}" 2>&1
    error=$?
        echo "Error: $error" >> "${LOG}"
    echo "Error 68 just means the entry already existed." >> "$LOG"

        db_checkpoint -1 -h "$LDAPDIR"
    else
        echo "LDAP users were already configured for the 389 server."
    fi
fi

echo >> "${LOG}"
exit 0
