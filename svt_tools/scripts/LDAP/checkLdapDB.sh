# /bin/bash
#set -x
# 
if [ $# -ne 1 ];then
   echo "Do you want to Query the Cars/Users/Apps? (c/u/a)"
   read y 
else 
   y=$1
fi 

if [ "${y}" == "c" ];then
   UID_PREFIX="c"
   UID_TYPE="car"
   WHOLE_LIST="cars.out"
   TRIMMED_LIST="cars.list"
   QUERY="uid=c*"
   FIRST_DIGIT=1
   LAST_DIGIT=5
elif [ "${y}" == "u" ];then
   UID_PREFIX="u"
   UID_TYPE="user"
   WHOLE_LIST="users.out"
   TRIMMED_LIST="users.list"
   QUERY="uid=u*"
   FIRST_DIGIT=1
   LAST_DIGIT=5
elif [ "${y}" == "a" ];then
   UID_PREFIX="a"
   UID_TYPE="app"
   WHOLE_LIST="apps.out"
   TRIMMED_LIST="apps.list"
   QUERY="uid=a*"
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

echo "Searching LDAP DB for all UIDs that match: '${QUERY}' and writing to file:  '${WHOLE_LIST}'"
#ldapsearch -h 10.10.10.10      -p 389           -b "ou=SVT,O=IBM,c=US" -D "cn=root,O=IBM,C=US" -x -w ima4test "uid=c*" > cars.out
echo " "
`ldapsearch -h ${LDAP_SERVER}  -p ${LDAP_PORT}  -b ${DN_SUFFIX}  -D ${DN_ROOT}  -x -w ${DN_ROOT_PSWD} "${QUERY}" > ${WHOLE_LIST}`

echo "Trimming the results to one line per UID and writing to file:  '${TRIMMED_LIST}'"
`grep -i "DN: uid=${UID_PREFIX}" ${WHOLE_LIST} > ${TRIMMED_LIST}`

for digit in $(eval echo {${FIRST_DIGIT}..${LAST_DIGIT}})
do
   leadingzero=`printf "%0${digit}d" 0000000000`

   for index in {0..9}
   do
      SUFFIX=`printf "%${leadingzero}${digit}d" ${index}`
      `grep "${UID_PREFIX}${SUFFIX}" ${TRIMMED_LIST} > ${UID_TYPE}.${UID_PREFIX}${SUFFIX}`
      LINES=` cat ${UID_TYPE}.${UID_PREFIX}${SUFFIX} | wc -l`
      echo "Number of entries in file:  ${UID_TYPE}.${UID_PREFIX}${SUFFIX}  was ${LINES}"
   done
done
