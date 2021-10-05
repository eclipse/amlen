#! /bin/bash
UIDPREFIX="a"
FIRST_UID=0
LAST_UID=10
HOST=`hostname`
LDIF_FILE=mod-uid.${UIDPREFIX}.ldif

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

   echo "version: 1"                             >   ${LDIF_FILE}
   echo ${MEMBER}                                >>  ${LDIF_FILE}
   echo "changetype: modify"                     >>  ${LDIF_FILE}
#   echo "objectClass: inetorgperson"             >>  ${LDIF_FILE}
#   echo "objectClass: organizationalPerson"      >>  ${LDIF_FILE}
#   echo "objectClass: person"                    >>  ${LDIF_FILE}
#   echo "objectClass: top"                       >>  ${LDIF_FILE}
#   echo "description: Internal Intranet Application UIDs"  >>  ${LDIF_FILE}
   echo "add: employeeNumber"                    >>  ${LDIF_FILE}
   echo "employeeNumber: "${USER}                >>  ${LDIF_FILE}
   echo "-"                                      >>  ${LDIF_FILE}
#   echo "uid: "${UIDPREFIX}${USER}               >>  ${LDIF_FILE}
   echo "replace: sn"                            >>  ${LDIF_FILE}
   echo "sn: "${UIDPREFIX}${USER}                >>  ${LDIF_FILE}
   echo "-"                                      >>  ${LDIF_FILE}
   echo "replace: cn"                            >>  ${LDIF_FILE}
   echo "cn: "${UIDPREFIX}${USER}                >>  ${LDIF_FILE}
#   echo "userPassword: imasvtest"                >>  ${LDIF_FILE}

   if [[ ${firstloop} == 0 ]]; then
      echo "%===> Going to Add UIDs using this Template:"
      echo " "
      cat   ${LDIF_FILE}
      echo " "
      echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
      read y
      firstloop=1
   fi

   echo "Modifying UID: " ${MEMBER}

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f  ${LDIF_FILE}
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=root,o=IBM,c=US" -w ima4test  -h 10.10.10.10 -p 389  -f  ${LDIF_FILE}
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapmodify -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f  ${LDIF_FILE}
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

done

echo "%===> Completed modifying ${FIRST_UID} .. ${LAST_UID} Application UIDs"
cat   ${LDIF_FILE}
