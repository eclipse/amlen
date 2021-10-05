#! /bin/bash
### Adding 1 million members at one time fails with Memory Overflow.
### Adding 1 milion members one menber at a time takes about a week.
### Changes to suppport adding 1000 members at a time 
###  PLEASE BESURE YOU GIVE Full UID number without leading zero and realize 000-999 will replace the last THREE digits of that RANGE
###
UIDPREFIX="c"
#FIRST_UID=51
FIRST_UID=1000
LAST_UID=999999
HOST=`hostname`
LDIF_FILE=member.${UIDPREFIX}.ldif

if [[ "${HOST}" == "mar099"* ]]; then
   BASE_DN="ou=SVT,o=jndiTest"
   DB_HOME=/var/lib/ldap
   ROOT_DN="cn=Manager,o=jndiTest"
   ROOT_PW="secret"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif  [[ "${HOST}" == "mar032"* ]]; then
   BASE_DN="ou=SVT,o=IBM,c=US"
   DB_HOME=/home/var/lib/ldap
   ROOT_DN="cn=root,o=IBM,c=US"
   ROOT_PW="ima4test"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif  [[ "${HOST}" == "mar345"* ]]; then
   BASE_DN="ou=SVT,o=IBM,c=US"
   DB_HOME="/etc/openldap/openldap-data"
   ROOT_DN="cn=root,o=IBM,c=US"
   ROOT_PW="ima4test"
   LDAP_IP="10.10.10.10"
   LDAP_PORT="389"
   LDAP_HOME="/usr/bin"

elif [[ "${HOST}" == "mar080"* ]]; then
   BASE_DN="ou=SVT,O=IBM,C=US"
   DB_HOME=/var/lib/ldap
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

# Create the leading FOUR digits of a FULL USERNAME's UID
set -x
   FIRST_UID_UNTRIMMED=`printf "%07d" ${FIRST_UID}`
   FIRST_UID_ROOT=${FIRST_UID_UNTRIMMED:0:4}
   LAST_UID_UNTRIMMED=`printf "%07d" ${LAST_UID}`
   LAST_UID_ROOT=${LAST_UID_UNTRIMMED:0:4}
set +x

for UID_ROOT in $(eval echo {${FIRST_UID_ROOT}..${LAST_UID_ROOT}})

do
   # create LDIF
   echo ${GROUPDN}            > ${LDIF_FILE}
   echo "changetype: modify" >> ${LDIF_FILE}
   echo "add: member"        >> ${LDIF_FILE}

   # Create the trailing THREE digits of a FULL USERNAME's UID
   for userSuffix in {0..999}
   do
      UID_SUFFIX=`printf "%03d" $userSuffix`
      MEMBER="member: uid=${UIDPREFIX}${UID_ROOT}${UID_SUFFIX},ou=users,${BASE_DN}"

      echo ${MEMBER}            >> ${LDIF_FILE}
   done

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

   # After every 100th modify, do db_checkpoint to keep logfile size down...
   NUM_ADD_ITERAITONS=$((NUM_ADD_ITERAITONS+1))
   if [ ${NUM_ADD_ITERAITONS} == 100 ]; then
      db_checkpoint -1 -h ${DB_HOME}
      #!#!#!# need clean_logs function like createLdapDb.sh has to remove the purged logs..., but have to stop LDAP.
      NUM_ADD_ITERAITONS=0
   fi

done

echo "%===> Completed adding ${FIRST_UID} .. ${LAST_UID} MEMBER UIDs to group:  ${GROUPDN}"
cat  ${LDIF_FILE}
