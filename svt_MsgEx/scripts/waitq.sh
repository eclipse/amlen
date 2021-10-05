#!/bin/bash 

############################################################################################################
#
#  This testcase launches 6 publishers on each of M_COUNT clients.  If the client is M1
#  Then it is the LEADER and executes runsync method below.  Other clients execute the run method.
#
#  if [ "${THIS}" == "M1" ]; then
#    runsync
#  else 
#    run
#  fi
#
#  The runsync method sets up the durable subscription /APP/M1 and starts publishing messages into it.
#  The other clients also publish to /APP/M1 and then wait for a signal on /APP/PUB from the LEADER.
#
#  After the appliance can no longer accept any more messages, the LEADER removes all stored messages.
#  The LEADER then signals the other clients to start publishing again.
#
#  When finished "${THIS}:  SUCCESS" is printed to the log
#  
#############################################################################################################

if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else 
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

GetPrimaryServer() {
  i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done

  if [[ ! "${list[*]}" =~ "HARole" ]]; then
    printf "A1\n"
  else
    if [[ "${list[*]}" =~ "PRIMARY" ]]; then
      printf "A1\n"
    else
      if [ "${A2_HOST}" == "" ]; then
        printf "A1\n"
      else
        i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done

        if [[ "${list2[*]}" =~ "PRIMARY" ]]; then
          printf "A2\n"
        else
          printf "A1\n"
        fi
      fi
    fi
  fi
  unset list
}

# "Name","Producers","Consumers","BufferedMsgs","BufferedMsgsHWM","BufferedPercent","MaxMessages","ProducedMsgs","ConsumedMsgs","RejectedMsgs","BufferedHWMPercent","ExpiredMsgs"


stat()
{
CMD="imaserver stat Queue 'Name=$1'"
out=`ssh admin@${AP_HOST} "$CMD"`

FIELD=$2
index=0
for line in ${out[@]}; do
  if [ ${index} -eq 0 ]; then
    VAR=${line/${FIELD}*/}
    VAR_TMP="${VAR//[^,]}" ;
    CNT=$((${#VAR_TMP}+1))
  else
    f=`echo ${line}|cut -d',' -f${CNT}`
    break;
  fi
  ((index++))
done
printf $f
}

PRIMARY=`GetPrimaryServer`
AP_HOST=$(eval echo \$\{${PRIMARY}_HOST\})
unset PRIMARY
f=0

f=`stat $1 $2`
while [ ! ${f} $3 $4 ]; do
   sleep 10s
   f=`stat $1 $2`
done
unset f

