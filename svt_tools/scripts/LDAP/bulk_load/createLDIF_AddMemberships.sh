#!/bin/bash 

if [ "$#" -ne "6" ]; then
    echo "Usage: $0 <USER_DN> <GROUP_PREFIX> <GROUP_BASE_DN> <FIRST_GID> <LAST_GID> <DIGITS>"
    echo "For examples,"
    echo "    To create an LDIF file to add DN uid=u0000001,ou=users,ou=SVT,o=IBM,c=US as member of 1000 groups(G0000001 .. G0001000):"
    echo "       $0 \"uid=u0000001,ou=users,ou=SVT,o=IBM,c=US\" \"G\" \"ou=groups,ou=SVT,o=IBM,c=US\" 1 1000 7"
    echo ""
    exit 99
fi

USER_DN=$1
GROUP_PREFIX=$2
GROUP_BASE_DN=$3
FIRST_GID=$4
LAST_GID=$5
DIGITS=$6

if [ "$FIRST_GID" -gt "$LAST_GID" ]; then
   echo "Error: FIRST_GID is greater than LAST_GID"
   exit 99
fi

# Strip out longest match between ',' and '*', from back of string
USER_UID=${USER_DN%%,*}     # uid=user_name

# Strip out longest match between '*' and '=' from front of string
USER_NAME=${USER_UID##*=}

LDIF_FILE="addMembership.${USER_NAME}.${GROUP_PREFIX}.${FIRST_GID}.${LAST_GID}.ldif"
echo "version: 1"      > ${LDIF_FILE}

let entries=(${LAST_GID}-${FIRST_GID})+1
#let splitEntries=(${entries}/100)

## If there are 10000 entries or more, create 100 temp files, concatenate them to ${LDIF_FILE}
if [ "${entries}" -gt 9999 ]; then

    files=""
    first=$FIRST_GID;
    let last=($first-1)
    let subCount=(${entries}/100)
    for i in `seq 1 100`
    do
        let first=($last+1)
        if [ "$i" -ne 100 ]; then
            let last=(${first}+${subCount})
        else
            let last=${LAST_GID}
        fi
    
        filename="add.${GROUP_PREFIX}.${first}.${last}.tmp"
        rm -f ${filename}
        for group in $(eval echo {${first}..${last}})
        do
	    GROUP=`printf "%0${DIGITS}d" $group`
	    echo ""     > ${filename}
	    echo "dn: cn=${GROUP_PREFIX}${GROUP},${GROUP_BASE_DN}" >> ${filename}
	    echo "changetype: modify" >> ${filename}
	    echo "add: member"        >> ${filename}
	    echo "member: ${USER_DN}" >> ${filename}
        done
        files="$files $filename"
    done

    cat $files >> ${LDIF_FILE}
    rm -f $files

else
    filename="${LDIF_FILE}"
    for group in $(eval echo {${FIRST_GID}..${LAST_GID}})
    do
	GROUP=`printf "%0${DIGITS}d" $group`
	echo ""     >> ${filename}
	echo "dn: cn=${GROUP_PREFIX}${GROUP},${GROUP_BASE_DN}" >> ${filename}
	echo "changetype: modify" >> ${filename}
	echo "add: member"        >> ${filename}
	echo "member: ${USER_DN}" >> ${filename}
    done
fi

echo "" >> ${LDIF_FILE}
echo "${LDIF_FILE} was successfully created."
echo "Next step is to run \"ldapmodify -f ${LDIF_FILE}\" to update the target LDAP server."
