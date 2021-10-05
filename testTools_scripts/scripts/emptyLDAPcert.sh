#!/bin/bash +x
RUNCMD="echo `echo \"/bin/rm /opt/ibm/imaserver/certificates/LDAP/ldap.pem;/bin/touch /opt/ibm/imaserver/certificates/LDAP/ldap.pem;/bin/ls -l /opt/ibm/imaserver/certificates/LDAP/ldap.pem;\" | ssh ${A1_USER}@${A1_HOST} busybox` "
echo Running command: "${RUNCMD}"
$RUNCMD
run01Result=$?
echo run01Result=${run01Result}
