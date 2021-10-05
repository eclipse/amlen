#! /bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

A1_USER=admin
export A1_USER
A1_IPv4_1=10.10.10.10
export A1_IPv4_1
M1_USER=root
export M1_USER
M1_IPv4_1=10.10.10.10
export M1_IPv4_1
M1_TESTROOT=/home/testuser/fvtTests/common
export M1_TESTROOT

WHERE=${A1_USER}@${A1_IPv4_1}
export WHERE

function doCLICommand {
  result=`ssh  ${WHERE} imaserver "$*" 2>&1`
  rc=$?
  if [[ $rc -ne 0 ]] ; then
    echo Return code $rc from command imaserver "$*" 
    echo Output was $result
    FAILURES=`expr $FAILURES + 1`
  fi
}

# Begin the main process

echo "START TEST ------ `date` " > cliPerfTest.log

count=0
tdiff=0

(

# memory status - start
memStatStart=`ssh  ${WHERE} status memory`
echo "START --- $memStatStart"

for ((  i = $1 ;  i <= $2;  i++  ))
do
   stime=`date +%s`
   #echo "`date` - Adding queue $i..."
   doCLICommand create Queue Name=myQueue$i Description=myDescriptionOfTheQueue$i AllowSend=True MaxMessages=5000 ConcurrentConsumers=true

   (( count = count + 1 ))
   if [[ $count -eq 5 ]]
   then
        #echo "`date` - list queue $i"
        doCLICommand list Queue
        count=0
   fi

   #echo "`date` - Deleting queue $i"
   doCLICommand delete Queue Name=myQueue$i
   etime=`date +%s`
   (( tdiff = etime - stime ))
       echo "Time taken: $tdiff"
   if [[ $tdiff -gt 2 ]]
   then
       echo "Time taken: $tdiff  ------ "
   fi 
done

# memory status - end
memStatEnd=`ssh  ${WHERE} status memory`
echo "END --- $memStatEnd"

) | tee -a cliPerfTest.log

echo "END TEST ------ `date` " >> cliPerfTest.log
