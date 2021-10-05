#! /bin/bash
UIDPREFIX="a"
FIRST_UID=0
LAST_UID=10
HOST=`hostname`

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
   USER=`printf "%02d" $user`
   MEMBER="DN: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"

   echo "version: 1"                             >  uid.${UIDPREFIX}.ldif
   echo ${MEMBER}                                >> uid.${UIDPREFIX}.ldif
   echo "changetype: add"                        >> uid.${UIDPREFIX}.ldif
   echo "objectClass: inetorgperson"             >> uid.${UIDPREFIX}.ldif
   echo "objectClass: organizationalPerson"      >> uid.${UIDPREFIX}.ldif
   echo "objectClass: person"                    >> uid.${UIDPREFIX}.ldif
   echo "objectClass: top"                       >> uid.${UIDPREFIX}.ldif
   echo "employeeNumber: "${USER}                >> uid.${UIDPREFIX}.ldif
   echo "cn: "${UIDPREFIX}${USER}                >> uid.${UIDPREFIX}.ldif
   echo "description: Internal Intranet Application UIDs"  >> uid.${UIDPREFIX}.ldif
   echo "sn: "${UIDPREFIX}${USER}                >> uid.${UIDPREFIX}.ldif
   echo "uid: "${UIDPREFIX}${USER}               >> uid.${UIDPREFIX}.ldif
   echo "userPassword: imasvtest"                >> uid.${UIDPREFIX}.ldif

   if [[ ${firstloop} == 0 ]]; then
      echo "%===> Going to Add UIDs using this Template:"
      echo " "
      cat  uid.${UIDPREFIX}.ldif
      echo " "
      echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
      read y
      firstloop=1
   fi

   echo "Creating UID for: " ${MEMBER}

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f uid.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=root,O=IBM,C=US" -w ima4test  -h 10.10.10.10 -p 389  -f uid.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapmodify -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f uid.${UIDPREFIX}.ldif
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

done

echo "%===> Completed creating ${FIRST_UID} .. ${LAST_UID} Application UIDs"
cat  uid.${UIDPREFIX}.ldif
