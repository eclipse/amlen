#!/bin/bash 

if [ "$#" -ne "6" ]; then
    echo "Usage: $0 <LEVELS> <GROUP_PREFIX> <GROUP_BASE_DN> <UID_PREFIX> <USER_BASE_DN> <DIGITS> "
    echo "For examples,"
    echo "    To create an LDIF file to add 10000 new nested groups"
    echo "        G0000000 with u0000000 as member,"
    echo "        G0000001 with u0000000 and u0000001 as members,"
    echo "        ..."
    echo "        G0009999 with u0000000 and u0009999 as members"
    echo "       $0  10000 G \"ou=groups,ou=SVT,o=IBM,c=US\" \"u\" \"ou=users,ou=SVT,o=IBM,c=US\" 7"
    echo ""
    exit 99
fi

LEVELS=$1
GROUP_PREFIX=$2
GROUP_BASE_DN=$3
UID_PREFIX=$4
USER_BASE_DN=$5
DIGITS=$6

LDIF_FILE="addNestedGroup.${LEVELS}.${GROUP_PREFIX}.${UID_PREFIX}.ldif"
echo "version: 1"      > ${LDIF_FILE}

let "LAST_INDEX=${LEVELS}-1"

# Start from the last group to first group 0
for i in `seq ${LAST_INDEX} -1 0`
do
    ID=`printf "%0${DIGITS}d" $i`
    let "j=$i+1"
    CHILD_ID=`printf "%0${DIGITS}d" $j`
    GROUP_NAME="${GROUP_PREFIX}${ID}"
    
    CHILD_GROUP_NAME="${GROUP_PREFIX}${CHILD_ID}"
    MEMBER="${UID_PREFIX}${ID}"
    echo "
dn: cn=${GROUP_NAME},${GROUP_BASE_DN}
changetype: add
objectClass: groupofnames
objectClass: top
cn: ${GROUP_NAME}
member: cn=${CHILD_GROUP_NAME},${GROUP_BASE_DN}
member: uid=${MEMBER},${USER_BASE_DN}" >> ${LDIF_FILE}
    
    if [ "$i" != "0" ]; then
        ID0=`printf "%0${DIGITS}d" 0`
	echo "member: uid=${UID_PREFIX}${ID0},${USER_BASE_DN}" >> ${LDIF_FILE}	
    fi
done

echo "" >> ${LDIF_FILE}
echo "${LDIF_FILE} was successfully created with ${entries} members."
echo "Next step is to run \"ldapmodify -f ${LDIF_FILE}\" to add new groups \"${GROUP_PREFIX}0\" .. \"${GROUP_PREFIX}${LAST_INDEX}\" the target LDAP server with these $entries UIDs as members."
