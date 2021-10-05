#! /bin/bash
UIDPREFIX="c"
FIRST_UID=51
LAST_UID=99
HOST=`hostname`
LDIF_FILE=member.${UIDPREFIX}.ldif

if [[ "${HOST}" == "mar099"* ]]; then
   BASE_DN="ou=SVT,o=jndiTest"
   ROOT_DN="cn=Manager,o=jndiTest"
   ROOT_PW="secret"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif  [[ "${HOST}" == "mar032"* ]]; then
   BASE_DN="ou=SVT,o=IBM,c=US"
   ROOT_DN="cn=root,o=IBM,c=US"
   ROOT_PW="ima4test"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif  [[ "${HOST}" == "mar345"* ]]; then
   BASE_DN="ou=SVT,o=IBM,c=US"
   ROOT_DN="cn=root,o=IBM,c=US"
   ROOT_PW="ima4test"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif [[ "${HOST}" == "mar080"* ]]; then
   BASE_DN="ou=SVT,O=IBM,C=US"
   ROOT_DN="cn=root"
   ROOT_PW="ima4test"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="5389"
   LDAP_HOME="/opt/ibm/ldap/V6.3/bin"

else
   echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
   exit 99
fi

#set -x
GROUPDN="dn: cn=svtCarsInternet,ou=groups,${BASE_DN}"
firstloop=0

for user in $(eval echo {${FIRST_UID}..${LAST_UID}})

do
   # create LDIF
   echo ${GROUPDN}            > ${LDIF_FILE}
   echo "changetype: modify" >> ${LDIF_FILE}
   echo "add: member"        >> ${LDIF_FILE}

   USER=`printf "%07d" $user`
   MEMBER="member: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"
   echo ${MEMBER}            >> ${LDIF_FILE}

   if [[ ${firstloop} == 0 ]]; then
      echo "%===> Going to Add MEMBER UIDs using this Template:"
      echo " "
      cat ${LDIF_FILE}
      echo " "
      echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
      read y
      firstloop=1
   fi

   echo "Adding Member:  ${MEMBER} to group: ${GROUPDN}"

   ${LDAP_HOME}/ldapmodify -x -D ${ROOT_DN} -w ${ROOT_PW}  -h ${LDAP_IP} -p ${LDAP_PORT}  -f ${LDIF_FILE}

done

echo "%===> Completed adding ${FIRST_UID} .. ${LAST_UID} MEMBER UIDs to group:  ${GROUPDN}"
cat  ${LDIF_FILE}
