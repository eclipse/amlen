#!/bin/bash 

if [ "$#" -ne "7" ]; then
    echo "Usage: $0 <GROUP_DN> <GROUP_DESC> <USER_BASE_DN> <UID_PREFIX> <FIRST_UID> <LAST_UID> <DIGITS>"
    echo "For examples,"
    echo "    To create an LDIF file to add a new group \"svtAppsIntranet\" and add 100 UIDs (a00..a99) as members:"
    echo "       $0 \"cn=svtAppsIntranet,ou=groups,ou=SVT,o=IBM,c=US\" \"Group of Intranet Applications\" \"ou=users,ou=SVT,o=IBM,c=US\" \"a\" 0 99 2"
    echo "    To create an LDIF file to add a new group \"svtCarsInternet\" and add 1000001 UIDs (c0000000..c1000000) as members:"
    echo "       $0 \"cn=svtCarsInternet,ou=groups,ou=SVT,o=IBM,c=US\" \"Group of Internet cars\" \"ou=users,ou=SVT,o=IBM,c=US\" \"c\" 0 1000000 7"
    echo ""
    exit 99
fi

GROUP_DN=$1
GROUP_DESC=$2
USER_BASE_DN=$3
UID_PREFIX=$4
FIRST_UID=$5
LAST_UID=$6
DIGITS=$7

if [ "$FIRST_UID" -gt "$LAST_UID" ]; then
   echo "Error: FIRST_UID is greater than LAST_UID"
   exit 99
fi

# Strip out longest match between ',' and '*', from back of string
GROUP_CN=${GROUP_DN%%,*}     # cn=group_name

# Strip out longest match between '*' and '=' from front of string
GROUP_NAME=${GROUP_CN##*=}

LDIF_FILE="addGroupAndMember.${GROUP_NAME}.${UID_PREFIX}.${FIRST_UID}.${LAST_UID}.ldif"
echo "version: 1"      > ${LDIF_FILE}
echo "" >> ${LDIF_FILE}
echo "dn: ${GROUP_DN}" >> ${LDIF_FILE}
echo "changetype: add" >> ${LDIF_FILE}
echo "objectClass: groupofnames" >> ${LDIF_FILE}
echo "objectClass: top" >> ${LDIF_FILE}
echo "cn: ${GROUP_NAME}" >> ${LDIF_FILE}
echo "description: ${GROUP_DESC}" >> ${LDIF_FILE}
echo "member: uid=dummy" >> ${LDIF_FILE}

let entries=(${LAST_UID}-${FIRST_UID})+1
#let splitEntries=(${entries}/100)

## If there are 10000 entries or more, create 100 temp files, concatenate them to ${LDIF_FILE}
if [ "${entries}" -gt 9999 ]; then

    files=""
    first=$FIRST_UID;
    let last=($first-1)
    let subCount=(${entries}/100)
    for i in `seq 1 100`
    do
        let first=($last+1)
        if [ "$i" -ne 100 ]; then
	    let last=(${first}+${subCount})
        else
            let last=${LAST_UID}
        fi
    
        filename="add.${UID_PREFIX}.${first}.${last}.tmp"
        rm -f ${filename}
	
	echo "" > ${filename}
	echo "dn: ${GROUP_DN}"    >> ${filename}
	echo "changetype: modify" >> ${filename}
	echo "add: member"        >> ${filename}
	
        for user in $(eval echo {${first}..${last}})
        do
            USER=`printf "%0${DIGITS}d" $user`
	    echo "member: uid=${UID_PREFIX}${USER},${USER_BASE_DN}" >> ${filename}
        done
        files="$files $filename"
    done

    cat $files >> ${LDIF_FILE}
    rm -f $files

else

    filename="${LDIF_FILE}"
    echo "" >> ${filename}
    echo "dn: ${GROUP_DN}"    >> ${filename}
    echo "changetype: modify" >> ${filename}
    echo "add: member"        >> ${filename}
    
    for user in $(eval echo {${FIRST_UID}..${LAST_UID}})
    do
        USER=`printf "%0${DIGITS}d" $user`
	echo "member: uid=${UID_PREFIX}${USER},${USER_BASE_DN}" >> ${filename}
    done
fi

echo "" >> ${LDIF_FILE}
echo "${LDIF_FILE} was successfully created with ${entries} members."
echo "Next step is to run \"ldapmodify -f ${LDIF_FILE}\" to add new group \"${GROUP_NAME}\" to the target LDAP server with these $entries UIDs as members."
