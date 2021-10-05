#!/bin/bash +x
RUNCMD="echo `echo \"/bin/sed -i 's/Security.LDAP.Enabled.ldapconfig = False/Security.LDAP.Enabled.ldapconfig = True/g' /opt/ibm/imaserver/config/server_dynamic.cfg;/bin/grep Security.LDAP.Enabled.ldapconfig /opt/ibm/imaserver/config/server_dynamic.cfg\" | ssh ${A1_USER}@${A1_HOST} busybox` "
echo Running command: "${RUNCMD}"
$RUNCMD
run01Result=$?
echo run01Result=${run01Result}
