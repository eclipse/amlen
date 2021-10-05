#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
ACTION=$1
FILE=$2
NUMFILES=$3
DESTINATION=$4
JUSTMOVE=$5

FILE_DOES_NOT_EXIST=1
BAD_PROTOCOL=4
DESTINATION_DOES_NOT_EXIST=5
BAD_OPTION=6
TEMP_FILE_ERROR=7
UNKNOWN_HOST=8

TRACE_BACKUP_LOGS="${IMA_SVR_DATA_PATH}/diag/logs/imatrace.backup.logtimes"

function parse_url {
  # extract the protocol
  proto="$(echo $1 | grep :// | sed -e's,^\(.*://\).*,\1,g')"

  # remove the protocol
  url="$(echo ${1/$proto/})"
  url="$(echo $url | sed 's/\/*$//g')"

  # extract the user (if any)
  user="$(echo $url | grep @ | cut -d@ -f1)"

  # extract the host
  host="$(echo ${url/$user@/} | cut -d/ -f1)"

  # extract the path (if any)
  if [[ $host == *:* ]]
  then
    host="$(echo $host | awk -F':' '{ print $1 }')"
    if [ "$user" == "" ]
    then
      path="$(echo ${url/$host:/})"
    else
      path="$(echo ${url/$user@$host:/})"
    fi
  else
    if [ "$user" == "" ]
    then
      path="$(echo ${url/$host/})"
    else
      path="$(echo ${url/$user@$host/})"
    fi

    if [[ $path == /* ]]
    then
      path="$(echo $path | sed 's/^\/*//g')"
    fi
  fi
  
  if [ "$path" == "" ]
  then
    path="."
  fi

  eval "$2=$proto"
  eval "$3=$host"
  eval "$4=$user"
  eval "$5=\"$path\""
}

function build_ssh_string {
    DEST=$1
    USER=$2
    
    if [ -z $USER ]
    then
        OUTPUT="ssh -o PasswordAuthentication=no $DEST"
    else
        OUTPUT="ssh -o PasswordAuthentication=no $USER@$DEST"
    fi
    
    echo $OUTPUT
}

function build_curl_ftp_string {
    FILE=$1
    DESTINATION=$2
    USER=$3

    OUTPUT="curl -s -T $FILE $DESTINATION"
    if [ ! -z $USER ]
    then
        if [[ $USER == *:* ]]
        then
            OUTPUT=" $OUTPUT -u $USER"
        else
            OUTPUT=" $OUTPUT -u $USER:"
        fi
    fi        
    if [ -f ~/.netrc ]
    then
        OUTPUT=" $OUTPUT --netrc-optional"
    fi

    echo "$OUTPUT"
}

if [ "$NUMFILES" == "" ]; then
  NUMFILES=0
fi

RC=0
if [ -f "$FILE" ]; then
    if [ $ACTION -eq 0 ]; then
        # Don't do anything
        exit 0
    elif [ $ACTION -eq 1 ]; then
        cat "$FILE" | gzip -5 > "${FILE}.gz"
        RC=$?
    elif [ "$DESTINATION" != "" ]; then
        # Sending file $FILE to $DESTINATION
        parse_url $DESTINATION PROTO DEST U DEST_DIR

        if [ $ACTION -eq 2 ]; then
            if [ "$PROTO" != "scp://" ]
            then
                exit $BAD_PROTOCOL
            fi
            if [ "$DEST" == "" ]
            then
                exit $DESTINATION_DOES_NOT_EXIST
            fi 

            ssh-keygen -H  -F $DEST > /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                exit $UNKNOWN_HOST
            fi

            if [ "$JUSTMOVE" == "move" ]; then
                TARGET_FILE="$DEST_DIR/${FILE##*/}"
                cat "$FILE" | $(build_ssh_string $DEST $U) "cat > $TARGET_FILE"
                RC=$?
                if [ $RC -eq 0 ]; then
                    rm -rf "${FILE}.gz"
                fi
            else
                TARGET_FILE="$DEST_DIR/${FILE##*/}.gz"
                cat "$FILE" | gzip -5 | $(build_ssh_string $DEST $U) "cat > $TARGET_FILE"
                RC=$?
            fi
        elif [ $ACTION -eq 3 ]; then
            if [ "$PROTO" != "ftp://" ]
            then
                exit $BAD_PROTOCOL
            fi
            if [ "$DEST" == "" ]
            then
                exit $DESTINATION_DOES_NOT_EXIST
            fi

            TARGET_FILE="$PROTO$DEST/$DEST_DIR/${FILE##*/}.gz"
                        
            if [ "$JUSTMOVE" == "move" ]; then
                RUNSTRING=$(build_curl_ftp_string $FILE "$TARGET_FILE" $U)
                $RUNSTRING > /dev/null 2>&1
                RC=$?
            else
                CUR_DIR="${FILE%/*}"
                if [ -z $CUR_DIR ]
                then
                    CUR_DIR="."
                fi
                TEMPFILE="$(mktemp -q --tmpdir=$CUR_DIR)"
                if [ $? -ne 0 ]
                then
                    exit $TEMP_FILE_ERROR
                fi

                cat "$FILE" | gzip -5 > $TEMPFILE
                if [ -f $TEMPFILE ]
                then
                    RUNSTRING=$(build_curl_ftp_string $TEMPFILE "$TARGET_FILE" $U)
                    $RUNSTRING > /dev/null 2>&1
                    RC=$?
                    rm -rf $TEMPFILE > /dev/null 2>&1
                fi
            fi
        else
            exit $BAD_OPTION
        fi
    else
        exit $BAD_OPTION
    fi  
    if [ $RC -eq 0 ]; then
        rm -rf "$FILE"
    fi
else
    exit $FILE_DOES_NOT_EXIST
fi

if [ $RC -ne 0 ]; then
    exit $RC
fi

#--------- Defect 151772 -----------
if [ ! -e $TRACE_BACKUP_LOGS ]; then
    touch $TRACE_BACKUP_LOGS
fi
echo $FILE >> $TRACE_BACKUP_LOGS
#--------- End Defect 151772 -------

# For trace compression only, remove older backups
if [ $ACTION -eq 1 -a $NUMFILES -gt 0 ]; then
    # To get the file name, remove last 2 fields separated by _ (underscore)
    FILENAME=$(echo "$FILE" | awk -F'_' '{ NF=NF-2; print $0 }' OFS='_')

    if [ "$NUMFILES" != "" ]; then
        FILEMASK="${FILENAME}_[0-9]*_[0-9]*.log.gz"
        (ls -t $FILEMASK | head -n $NUMFILES; ls $FILEMASK) | sort | uniq -u | xargs rm -rf
    fi
fi

exit 0
