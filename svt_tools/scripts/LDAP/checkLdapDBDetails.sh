# /bin/bash
#set -x
# 
echo ""
if [ $# -ne 3 ]; then
   echo "Typcially, '${0}' is run after running 'checklist.sh' on cars, users or apps."
   echo "Now you are wanting to drill down to find the missing entries."
   echo ""
   echo "Syntax:  ${0}  INPUT_FILE  FIRST_ENTRY  LAST_ENTRY"
   echo ""
   echo "You will be prompted for those values now."
   echo "What file do you want to scan for a range of UID LDAP entries?"
   read y 
   INPUT_FILE=${y}

   echo "What is the first UID in the range of LDAP entries you want to scan?"
   read y 
   FIRST_ENTRY=${y}

   echo "What is the first UID in the range of LDAP entries you want to scan?"
   read y 
   LAST_ENTRY=${y}

else
   INPUT_FILE=$1
   FIRST_ENTRY=$2
   LAST_ENTRY=$3
fi 

LDAP_SERVER="10.10.10.10"
LDAP_PORT="389"
DN_ROOT="cn=root,o=IBM,c=US"
DN_ROOT_PSWD="ima4test"
DN_SUFFIX="ou=SVT,o=IBM,c=US"

if [ -e ${INPUT_FILE} ];then
   echo "SCAN file: '${INPUT_FILE}' for entries with range from '${FIRST_ENTRY}' to '${LAST_ENTRY}'.  "
#   echo "Is that correct? (y/n)"
#   read y
#   if [ "${y}" == "y" ];then
      echo "Scan starting..."
#   else
#      echo "Scan aborted by user."
#      exit 1
#   fi
else
   echo "DANGER, DANGER --------"
   echo "The file, '${INPUT_FILE}' does not exist!"
   echo " --- ABORT!"   
   exit 99
fi

# Finally -- here's where the work is done....

for ENTRY in $(eval echo {${FIRST_ENTRY}..${LAST_ENTRY}})
do
   UID_NUMBER=`printf "%07d" ${ENTRY}`

      GREP_RC=`grep "${UID_NUMBER}" ${INPUT_FILE} `
      if [ $? -ne 0 ];then
         echo "This entry, '${UID_NUMBER}' is missing in file ${INPUT_FILE}"
      fi

done
