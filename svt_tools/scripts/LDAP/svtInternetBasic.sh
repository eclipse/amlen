#! /bin/bash
#
# MAY NEED TO RUN SVT_TOP_OBJECTS.ldif  
#
#Create the LDAP LDIF file for the Basic SVT UIDs and GROUPs
#

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
LDIF_FN="SVT_INTERNET_BASIC.ldif"

## Build the Generic Lines of the LDAP SubTree and original SVT groups(users)
## Original:  svtGroup(svtUser,svtPubGroup, svtSubGroup), svtPubGroup(svtPubUser), svtSubGroup(svtSubUser), svtBadGroup(svtBadUser), (svtOrphan)

echo "version: 1" > ${LDIF_FN}
echo "
DN: cn=svtGroup,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtGroup
description: Master Group of UIDs with Publisher and Subscriber Privileges 
member: uid=svtUser,ou=users,ou=SVT,o=IBM,c=US
member: cn=svtPubGroup,ou=groups,ou=SVT,o=IBM,c=US
member: cn=svtSubGroup,ou=groups,ou=SVT,o=IBM,c=US
member: uid=dummy

DN: uid=svtUser,ou=users,${BASE_DN}
changetype: add
objectClass: inetorgperson
objectClass: organizationalPerson
objectClass: person
objectClass: top
cn: svt
mail: 
sn: User
uid: svtUser
userPassword:: ima4test

DN: cn=svtPubGroup,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtPubGroup
description: Master Group of Publishers ONLY
member: uid=dummy
member: uid=svtPubUser,ou=users,ou=SVT,o=IBM,c=US

DN: uid=svtPubUser,ou=users,${BASE_DN}
changetype: add
objectClass: inetorgperson
objectClass: organizationalPerson
objectClass: person
objectClass: top
cn: svtPub
sn: User
uid: svtPubUser
userPassword:: ima4test

DN: cn=svtSubGroup,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtSubGroup
description: Master Group of Subscribers ONLY
member: uid=dummy
member: uid=svtSubUser,ou=users,ou=SVT,o=IBM,c=US

DN: uid=svtSubUser,ou=users,${BASE_DN}
changetype: add
objectClass: inetorgperson
objectClass: organizationalPerson
objectClass: person
objectClass: top
cn: svtSub
sn: User
uid: svtSubUser
userPassword:: ima4test

DN: cn=svtBadGroup,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtBadGroup
description: Group that has known users, yet have NO IMA Privileges in ANY IMA Policy
member: uid=svtBadUser,ou=users,ou=SVT,o=IBM,c=US
member: uid=dummy

DN: uid=svtBadUser,ou=users,${BASE_DN}
changetype: add
objectClass: inetorgperson
objectClass: organizationalPerson
objectClass: person
objectClass: top
cn: svtBad
sn: User
uid: svtBadUser
userPassword:: ima4test

DN: uid=svtOrphan,ou=users,${BASE_DN}
changetype: add
objectClass: inetorgperson
objectClass: organizationalPerson
objectClass: person
objectClass: top
cn: svtOrphan
sn: User
uid: svtOrphan
userPassword:: ima4test
" >> ${LDIF_FN}
 
cat  ${LDIF_FN}
echo "=====> DOES THAT LOOK GOOD?  Press ENTER to continue"
read y

echo "Adding Basic SVT Groups and Members from file : " ${LDIF_FN}

if [[ "${HOST}" == "mar099"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=Manager,o=jndiTest" -w secret  -h 10.10.10.10 -p 389  -f ${LDIF_FN}
elif [[ "${HOST}" == "mar032"* ]]; then
   /usr/bin/ldapmodify -x -D "cn=root,o=IBM,c=US" -w ima4test  -h 10.10.10.10 -p 389  -f ${LDIF_FN}
elif [[ "${HOST}" == "mar080"* ]]; then
   /opt/ibm/ldap/V6.3/bin/ldapmodify -x -D "cn=root" -w ima4test  -h 10.10.10.10 -p 5389  -f ${LDIF_FN}
else 
     echo "HELP!! LDAP HOSTNAME NOT EXPECTED:  ${HOST}"
fi

exit
