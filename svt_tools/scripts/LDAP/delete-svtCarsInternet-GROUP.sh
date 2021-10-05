#! /bin/bash 
UIDPREFIX="c"
GROUPNAME="svtCarsInternet"
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
   # create LDIF
   echo "cn=${GROUPNAME},ou=groups,${BASE_DN}"        > group.${UIDPREFIX}.ldif

   echo "%===> Going to Delete Group using this Template:"
   echo " "
   cat  group.${UIDPREFIX}.ldif
   echo " "
   echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
   read y

   echo "Deleting 'cn=' for: " ${GROUPNAME}

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapdelete -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f group.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapdelete -x -D "cn=root,o=IBM,c=US" -w ima4test  -h 10.10.10.10 -p 389  -f group.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapdelete -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f group.${UIDPREFIX}.ldif
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

echo "%===> Completed deleting Group " ${GROUPNAME}
cat  group.${UIDPREFIX}.ldif
