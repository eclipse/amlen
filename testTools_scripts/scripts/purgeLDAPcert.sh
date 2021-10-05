#!/bin/bash +x
#RUNCMD="echo `echo \"/bin/rm /opt/ibm/imaserver/certificates/LDAP/ldap.pem\" | ssh ${A1_USER}@${A1_HOST} busybox` "
CMD="/bin/rm /var/messagesight/data/certificates/LDAP/ldap.pem"
if [[ "${A1_TYPE}" == "DOCKER" ]] ; then
   RUNCMD="ssh ${A1_USER}@${A1_HOST} docker exec imaserver ${CMD}"
else
   RUNCMD="ssh ${A1_USER}@${A1_HOST} ${CMD}"
fi
echo Running command: "${CMD}"
$RUNCMD
run01Result=$?
echo run01Result=${run01Result}
