#!/bin/bash 

if [ "$#" -ne "6" ]; then
    echo "Usage: $0 <UID_PREFIX> <USER_BASE_DN> <DESCRIPTION> <FIRST_UID> <LAST_UID> <DIGITS>"
    echo "For examples,"
    echo "    To create an LDIF file for 100 entries (a00..a99):"
    echo "       $0 \"a\" \"ou=users,ou=SVT,o=IBM,c=US\" \"Internal Intranet Application UIDs\" 0 99 2"
    echo "    To create an LDIF file for 1,000,001 entries (c0000000..c1000000):"
    echo "       $0 \"c\" \"ou=users,ou=SVT,o=IBM,c=US\" \"Cars in the Internet\" 0 1000000 7"
    echo ""
    exit 99
fi

UID_PREFIX=$1
USER_BASE_DN=$2
DESC=$3
FIRST_UID=$4
LAST_UID=$5
DIGITS=$6

if [ "$FIRST_UID" -gt "$LAST_UID" ]; then
   echo "Error: FIRST_UID is greater than LAST_UID"
   exit 99
fi


LDIF_FILE="addUser.${UID_PREFIX}.${FIRST_UID}.${LAST_UID}.ldif"
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
        for user in $(eval echo {${first}..${last}})
        do
            USER=`printf "%0${DIGITS}d" $user`
	    MEMBER="DN: uid=${UID_PREFIX}${USER},${USER_BASE_DN}"
	    echo "" >> ${filename}
            echo ${MEMBER}             >> ${filename}
            echo "changetype: add"     >> ${filename}
            echo "objectClass: inetorgperson"         >> ${filename}
            echo "objectClass: organizationalPerson"  >> ${filename}
            echo "objectClass: person"                >> ${filename}
            echo "objectClass: top"                   >> ${filename}
            echo "employeeNumber: ${USER}"            >> ${filename}
            echo "cn: ${UID_PREFIX}${USER}"           >> ${filename}
            echo "description: ${DESC}"               >> ${filename}
            echo "sn: ${UID_PREFIX}${USER}"           >> ${filename}
            echo "uid: ${UID_PREFIX}${USER}"          >> ${filename}
            echo "userPassword: imasvtest"            >> ${filename}
        done
        files="$files $filename"
    done

    cat $files >> ${LDIF_FILE}
    rm -f $files

else
    filename="${LDIF_FILE}"
    for user in $(eval echo {${FIRST_UID}..${LAST_UID}})
    do
        USER=`printf "%0${DIGITS}d" $user`
	MEMBER="DN: uid=${UID_PREFIX}${USER},${USER_BASE_DN}"
	echo "" >> ${filename}
        echo ${MEMBER}             >> ${filename}
        echo "changetype: add"     >> ${filename}
        echo "objectClass: inetorgperson"         >> ${filename}
        echo "objectClass: organizationalPerson"  >> ${filename}
        echo "objectClass: person"                >> ${filename}
        echo "objectClass: top"                   >> ${filename}
        echo "employeeNumber: "${USER}            >> ${filename}
        echo "cn: "${UID_PREFIX}${USER}            >> ${filename}
        echo "description: ${DESC}"               >> ${filename}
        echo "sn: "${UID_PREFIX}${USER}            >> ${filename}
        echo "uid: "${UID_PREFIX}${USER}           >> ${filename}
        echo "userPassword: imasvtest"            >> ${filename}
    done
fi

echo "" >> ${LDIF_FILE}
echo "${LDIF_FILE} was successfully created with $entries entries."
echo "Next step is to run \"ldapmodify -f ${LDIF_FILE}\" to populate the target LDAP server with these $entries entries."
