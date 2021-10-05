#! /bin/bash
#
# Assumes:  SVT_TOP_OBJECTS:  ou=SVT..; ou=users,ou=SVT.. and ou=groups,ou=SVT..  have been created
# MAY NEED TO RUN SVT_TOP_OBJECTS.ldif
#
#Create the LDAP LDIF file for the Basic ATELM UIDs and GROUPs for SVT
#
# TO Create the LDAP LDIF file for SVT UIDs for Cars, Users, and Apps  use the appropriate add-svt[Apps|Cars|Users]Internet-[UIDs|Members].sh
# ONLY 10 Apps and 50 Cars|Users are created initially.
#
# Assumes 7 DIGITS for USERS and CARS, and 2 DIGITS for APPS
# For ALL UIDs, Password=imasvtest
NO_APPS=10
NO_CARS=50
NO_USERS=50


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

LDIF_FN="SVT_ATELM_BASIC.ldif"


## Automotive Telematic Groups(UIDs)
## svtInternet(svtUserInternet, svtCarInternet), svtUserInternet(u#######), svtCarInternet(c#######), svtAppIntranet(a##)

echo "version: 1" > ${LDIF_FN}
echo "
DN: cn=svtInternet,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtInternet
description: Master Group of USERS and CARS in the Internet
member: cn=svtCarsInternet,ou=groups,ou=SVT,o=IBM,c=US
member: cn=svtUsersInternet,ou=groups,ou=SVT,o=IBM,c=US
" >> ${LDIF_FN}

# Build CARS Group
echo "DN: cn=svtCarsInternet,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtCarsInternet
description: Group of CARS in the Internet
member: uid=dummy" >> ${LDIF_FN}
i=0
while [ ${i} -le ${NO_CARS} ]
do 
   str_i=$(printf "%07d" ${i})
   echo "member: uid=c${str_i},ou=users,ou=SVT,o=IBM,c=US" >> ${LDIF_FN}
   i=$(($i + 1))
done
echo "" >>${LDIF_FN}
 
# Build USERS Group
echo "DN: cn=svtUsersInternet,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtUsersInternet
description: Group of USERS in Internet
member: uid=dummy" >> ${LDIF_FN}
i=0
while [ ${i} -le ${NO_USERS} ]
do 
   str_i=$(printf "%07d" ${i})
   echo "member: uid=u${str_i},ou=users,ou=SVT,o=IBM,c=US" >> ${LDIF_FN}
   i=$(($i + 1))
done
echo "" >>${LDIF_FN}

# Build APPS Group
echo "DN: cn=svtAppsIntranet,ou=groups,${BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: svtAppsIntranet
description: Master Group of Application inside Company Intranet
member: uid=dummy" >> ${LDIF_FN}
i=0
while [ ${i} -le ${NO_APPS} ]
do 
   str_i=$(printf "%02d" ${i})
   echo "member: uid=a${str_i},ou=users,ou=SVT,o=IBM,c=US" >> ${LDIF_FN}
   i=$(($i + 1))
done
echo "" >>${LDIF_FN}


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
