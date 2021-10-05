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


PRIMARY=`GetPrimaryServer`
AP_HOST=$(eval echo \$\{${PRIMARY}_HOST\})
unset PRIMARY
declare -a f

CMD="imaserver stat Subscription 'TopicString=$1'"
f=`ssh admin@${AP_HOST} "$CMD"`

for i in $f; do
  ((c++))
done
((c--))

while [ ! $c $2 $3 ]; do
   sleep 10s
   f=`ssh admin@${AP_HOST} "$CMD"`
   c=0
   for i in $f; do
     ((c++))
   done
   ((c--))
done
unset f
unset i
unset c

