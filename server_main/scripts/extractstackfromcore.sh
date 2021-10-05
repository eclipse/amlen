#!/bin/sh
# Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

# set -x

LOCK="/tmp/imaextractstackfromcore.lock"

exec 200>$LOCK
flock -n 200 || exit 0

INSTALLDIR=$1
if [ -z $INSTALLDIR ]
then
        INSTALLDIR=${IMA_SVR_INSTALL_PATH}
fi
export INSTALLDIR

DIAGDIR=${IMA_SVR_DATA_PATH}/diag
export DIAGDIR

coretype=0

CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern)
if [ "$CORE_PATTERN" == "/tmp/cores/bt.%e" ]
then
        COREDIR=/tmp/cores
        coretype=1
elif [ "$CORE_PATTERN" == "core" ] || [ "$CORE_PATTERN" == "" ]
then
        COREDIR=$INSTALLDIR/bin
        coretype=2
else
        exit 0
fi

FILEEXT=`date +%y%m%d_%H%M%S`
OUTF=$DIAGDIR/cores/messagesight_stack.$FILEEXT
export OUTF

LOGF=$DIAGDIR/logs/coreProcess.log
GDBBATCH=${INSTALLDIR}/bin/gdb.batch
export GDBBATCH

echo "" > $OUTF
echo "IBM IoT MessageSight Server - stack trace of last failed process" >> $OUTF
echo "----------------------------------------------------------------" >> $OUTF
echo "" >> $OUTF
echo "`strings ${INSTALLDIR}/bin/imaserver | grep version | grep IBM`" >> $OUTF
echo "Date and time: `date`" >> $OUTF
echo >> $OUTF

# check current core files and try to identify imaserver or other MessageSight executable code
cd $COREDIR
found=0

if [ $coretype == "1" ]
then
    CFILES=`ls bt.* 2>/dev/null 3>&1`
    export CFILES
else
    CFILES=`ls core.* 2>/dev/null 3>&1`
    export CFILES
fi

for i in `echo ${CFILES}`
do
    TGZFILE=`echo $i | grep -o ".tar.gz"`
    if [ $? -eq 0 ]
    then
        continue
    fi

    TMPPROC=`gdb $i -q --batch 2>/dev/null 3>&2 | grep "Core was " | grep -o "imaserver\|mqcbridge"`

    PROCBIN="imaserver"
    TMPSTR=`echo $TMPPROC | grep -o imaserver`
    if [ $? -eq 0 ]
    then
        PROCBIN="${INSTALLDIR}/bin/imaserver"
    fi

    TMPSTR=`echo $TMPPROC | grep -o mqcbridge`
    if [ $? -eq 0 ]
    then
        PROCBIN="${INSTALLDIR}/bin/mqcbridge"
    fi

    if [ "$PROCBIN" != "" ]
    then
        # process this core file
        echo >> $OUTF
        echo >> $OUTF
        echo "Core file name: $i" >> $OUTF
        echo "Process binary: ${PROCBIN}" >> $OUTF
        echo >> $OUTF

        CORF=$i
        export CORF

        FILEEXT=`date +%y%m%d_%H%M%S`

        gdb $PROCBIN $CORF -x $GDBBATCH --batch >> $OUTF 2>&1
        TZFILENAME=${CORF}.${FILEEXT}.tar.gz
        if [ $coretype == "2" ]
        then
            TZFILENAME=${DIAGDIR}/cores/${CORF}.${FILEEXT}.tar.gz
        fi
        tar --remove-files -czf - $CORF 2> /dev/null > ${TZFILENAME}
        found=1
        rm -f ${CORF} 2> /dev/null
    fi
done

if [ "$found" == "0" ]
then
    cat $OUTF >> $LOGF
    echo "No stack trace is found." >> $LOGF
    rm -f ${OUTF}
else
    cat $OUTF >> $LOGF
    rm -f ${OUTF}
fi

