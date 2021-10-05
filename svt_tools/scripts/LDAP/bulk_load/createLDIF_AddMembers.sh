#!/bin/bash 

if [ "$#" -ne "6" ]; then
    echo "Usage: $0 <UID_PREFIX> <USER_BASE_DN> <GROUP_DN> <FIRST_UID> <LAST_UID> <DIGITS>"
    echo "For examples,"
    echo "    To create an LDIF file to add 100 UIDs (a00..a99) as members of \"svtAppsIntranet\" group:"
    echo "       $0 \"a\" \"ou=users,ou=SVT,o=IBM,c=US\" \"cn=svtAppsIntranet,ou=groups,ou=SVT,o=IBM,c=US\" 0 99 2"
    echo "    To create an LDIF file for 1,000,001 UIDs (c0000000..c1000000) as members of \"svtCarsInternet\" group"
    echo "       $0 \"c\" \"ou=users,ou=SVT,o=IBM,c=US\" \"cn=svtCarsInternet,ou=groups,ou=SVT,o=IBM,c=US\" 0 1000000 7"
    echo ""
    exit 99
fi

UID_PREFIX=$1
USER_BASE_DN=$2
GROUP_DN=$3
FIRST_UID=$4
LAST_UID=$5
DIGITS=$6

if [ "$FIRST_UID" -gt "$LAST_UID" ]; then
   echo "Error: FIRST_UID is greater than LAST_UID"
   exit 99
fi

# Strip out longest match between ',' and '*', from back of string
GROUP_CN=${GROUP_DN%%,*}     # cn=group_name

# Strip out longest match between '*' and '=' from front of string
GROUP_NAME=${GROUP_CN##*=}

LDIF_FILE="addMember.${GROUP_NAME}.${UID_PREFIX}.${FIRST_UID}.${LAST_UID}.ldif"
echo "version: 1"      > ${LDIF_FILE}

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
	echo ""     > ${filename}
	echo "dn: ${GROUP_DN}"     >> ${filename}
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
    echo ""     >> ${filename}
    echo "dn: ${GROUP_DN}"     >> ${filename}
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
echo "Next step is to run \"ldapmodify -f ${LDIF_FILE}\" to update \"${GROUP_NAME}\" of the target LDAP server with these $entries members."
