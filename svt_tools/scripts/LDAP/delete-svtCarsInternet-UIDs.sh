#! /bin/bash
UIDPREFIX="c"
#FIRST_UID=0
FIRST_UID=5782
LAST_UID=14301
HOST=`hostname`
LDIF_FILE=del-uid.${UIDPREFIX}.ldif

if [[ "${HOST}" == "mar099"* ]]; then
   BASE_DN="ou=SVT,o=jndiTest"

elif  [[ "${HOST}" == "mar032"* ]]; then
   BASE_DN="ou=SVT,o=IBM,c=US"

elif [[ "${HOST}" == "mar080"* ]]; then
   BASE_DN="ou=SVT,O=IBM,C=US"

else
   echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
   exit 99
fi

#set -x
firstloop=0

for user in $(eval echo {${FIRST_UID}..${LAST_UID}})
do
   # create LDIF
   USER=`printf "%07d" $user`
   MEMBER="DN: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"

   echo "uid="${UIDPREFIX}${USER},ou=users,${BASE_DN}        > ${LDIF_FILE}

   if [[ ${firstloop} == 0 ]]; then
      echo "%===> Going to Delete UIDs using this Template:"
      echo " "
      cat  ${LDIF_FILE}
      echo " "
      echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
      read y
      firstloop=1
   fi

   echo "Deleting UID for: " ${MEMBER}

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapdelete -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f ${LDIF_FILE}
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapdelete -x -D "cn=root,o=IBM,c=US" -w ima4test  -h 10.10.10.10 -p 389  -f ${LDIF_FILE}
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapdelete -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f ${LDIF_FILE}
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

done

echo "%===> Completed deleting ${FIRST_UID}..${LAST_UID} Car UIDs"
cat  ${LDIF_FILE}
