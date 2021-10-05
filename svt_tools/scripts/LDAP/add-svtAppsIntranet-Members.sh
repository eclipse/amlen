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
GROUPDN="dn: cn=svtAppsIntranet,ou=groups,${BASE_DN}"
firstloop=0

for user in $(eval echo {${FIRST_UID}..${LAST_UID}})
do
   # create LDIF
   echo ${GROUPDN}            > member.${UIDPREFIX}.ldif
   echo "changetype: modify" >> member.${UIDPREFIX}.ldif
   echo "add: member"        >> member.${UIDPREFIX}.ldif

   USER=`printf "%02d" $user`
   MEMBER="member: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"
   echo ${MEMBER}            >> member.${UIDPREFIX}.ldif

   if [[ ${firstloop} == 0 ]]; then
      echo "%===> Going to Add MEMBER UIDs using this Template:"
      echo " "
      cat member.${UIDPREFIX}.ldif
      echo " "
      echo "%===> DOES THAT LOOK GOOD?  Press ENTER to continue"
      read y
      firstloop=1
   fi

   echo "Adding Member:  ${MEMBER} to group: ${GROUPDN}"

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f member.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=root,o=IBM,c=US" -w ima4test  -h 10.10.10.10 -p 389  -f member.${UIDPREFIX}.ldif
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapmodify -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f member.${UIDPREFIX}.ldif
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

done

echo "%===> Completed adding ${LAST_UID} MEMBER UIDs to group:  ${GROUPDN}"
cat  member.${UIDPREFIX}.ldif
