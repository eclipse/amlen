#!/bin/bash
#------------------------------------------------------------------------------
#  findInterface.sh
#
#  This script is used to find the interface name of the specified address on
#  the appliance.  The automation systems are all plugged in differently
#  so it is not guaranteed what interface will be used for A1_IPv4_1, etc.
#
#  If the interface name is found, IFNAME is replaced in the specified file.
#
#  Arguments:
#    -a: (required) IP address to search for.
#    -f: (required) Log file
#    -t: (optional) Target file to perform replacement on
#
#  If target file not specified, the result will just
#  end up in the log file.
#  In this case, you could get result with something like:
#    "grep '^eth[0-9]+$|^mgt[0-9]+$' $LOG"
#
#  Returns: 0 on success, 1 on failure.
#
#  TODO: Maybe this should just be another function in commonFunctions.sh
#        instead of a tool? And then just have it return the interface name.
#
#------------------------------------------------------------------------------

ADDRESS=""
LOG=""
TARGET=""
INTERFACE=""

while getopts "a:f:t:" opt; do
    case ${opt} in
    a )
        ADDRESS=${OPTARG}
        ;;
    f )
        LOG=${OPTARG}
        ;;
    t )
        TARGET=${OPTARG}
        ;;
    * )
        exit 1
        ;;
    esac
done

if [[ "${ADDRESS}" == "" ]] ; then
    exit 1
fi
if [[ "${LOG}" == "" ]] ; then
    exit 1
fi

echo -e "Entering $0 with $# arguments: $@\n" > ${LOG}

iflist=`echo "list ethernet-interface" | ssh ${A1_USER}@${A1_HOST} 2>/dev/null`
ifnames=`echo "${iflist}" | grep "eth[0-9]\+\|mgt[0-9]\+" | sed s/[[:space:]]//g`

saveIFS=$IFS
IFS=$'\n'

ifnames=( `echo "${ifnames}"` )
for line in "${ifnames[@]}" ; do
    showif=`echo "show ethernet-interface ${line}" | ssh ${A1_USER}@${A1_HOST} 2>/dev/null | grep "${ADDRESS}" | awk '{ print $2 }'`
    if [[ "${showif}" != "" ]] ; then
        echo -e "SUCCESS: Found matching interface ${line}: ${showif}\n" | tee -a ${LOG}
        INTERFACE=${line}
        break
    fi
done

IFS=$saveIFS

if [[ "${INTERFACE}" != "" ]] ; then
    if [[ "${TARGET}" != "" ]] ; then
        echo "Replacing IFNAME with ${INTERFACE} in $TARGET" | tee -a ${LOG}
        sed -i -e s/IFNAME/${INTERFACE}/g ${TARGET} 1>/dev/null 2>&1
    else
        echo "${INTERFACE}" >> ${LOG} | tee -a ${LOG}
    fi
else
    echo "FAILURE! Could not find matching interface" | tee -a ${LOG}
    exit 1
fi
echo "findInterface.sh Test result is Success!" | tee -a ${LOG}
exit 0
