#!/bin/bash +x
echo arg0=${0} arg1=${1} arg2=${2} arg3=${3} arg4=${4} arg5=${5}
#RUNCMD="echo `echo \"/bin/sed -i 's/Security.LDAP.URL.ldapconfig = ldap:\/\/${LDAP_SERVER_1}:${LDAP_SERVER_1_PORT}/Security.LDAP.URL.ldapconfig = ldaps:\/\/${LDAP_SERVER_1}:${LDAP_SERVER_1_SSLPORT}/g' /opt/ibm/imaserver/config/server_dynamic.cfg;/bin/grep Security.LDAP.URL.ldapconfig /opt/ibm/imaserver/config/server_dynamic.cfg\" | ssh ${A1_USER}@${A1_HOST} busybox` "
RUNCMD="echo `echo \"/bin/grep Security.LDAP /opt/ibm/imaserver/config/server_dynamic.cfg;/bin/sed -i 's/Security.LDAP.URL.ldapconfig = ${2}:\/\/${1}:${3}/Security.LDAP.URL.ldapconfig = ${4}:\/\/${1}:${5}/g' /opt/ibm/imaserver/config/server_dynamic.cfg;/bin/grep Security.LDAP /opt/ibm/imaserver/config/server_dynamic.cfg\" | ssh ${A1_USER}@${A1_HOST} busybox` "
echo Running command: "${RUNCMD}"
$RUNCMD
run01Result=$?
echo run01Result=${run01Result}

