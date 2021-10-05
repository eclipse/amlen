#/bin/bash
#  LDAPSEARCH for entry counts of UIDs for CARs, USERs and APPs
#
if [ $# -ne 1 ];then
   echo "Do you want to Query the Cars/Users/Apps? (c/u/a)"
   read y 
else 
   y=$1
fi 

if [ "${y}" == "c" ];then
   UID_PREFIX="c"
   FIRST_DIGIT=1
   LAST_DIGIT=5
elif [ "${y}" == "u" ];then
   UID_PREFIX="u"
   FIRST_DIGIT=1
   LAST_DIGIT=5
elif [ "${y}" == "a" ];then
   UID_PREFIX="a"
   FIRST_DIGIT=1
   LAST_DIGIT=1
else
   echo "YOU DID NOT RESPOND WITH APPROPRIATE LETTER:  c, u, or a  !"
   exit 99
fi

LDAP_SERVER="10.10.10.10"
LDAP_PORT="389"
DN_ROOT="cn=root,O=IBM,C=US"
DN_ROOT_PSWD="ima4test"
DN_SUFFIX="ou=SVT,o=IBM,C=US"

for digit in $(eval echo {${FIRST_DIGIT}..${LAST_DIGIT}})
do
   leadingzero=`printf "%0${digit}d" 0000000000`

   for index in {0..9}
   do
#     Build the SUFFIX e.g. 9999999 an "X" digit number based off $index with $leadingzero as necessary to make a string of "X" $digits
      SUFFIX=`printf "%${leadingzero}${digit}d" ${index}`
      COUNT=`ldapsearch  -x -h ${LDAP_SERVER}  -p ${LDAP_PORT}  -b "${DN_SUFFIX}"  -D "${DN_ROOT}" -w ${DN_ROOT_PSWD} "uid=${UID_PREFIX}${SUFFIX}*"  ldapentrycount | grep -i numEntries`
      echo " Query: 'uid=${UID_PREFIX}${SUFFIX}' shows: ${COUNT}"
   done
done
