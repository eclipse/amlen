#!/bin/bash

############################################################################################################
#
#  This script subscribes to a JMS message on port 16999 of the appliance.
#
#  This script takes 2 parameters:
#    clientid
#    topic
#
#  SSL is used to connect to the appliance and a userid/password is specified for authentication/authorization
#
#############################################################################################################

if [ "$1" == "" ]; then
  echo usage error:  specify clientid as param 1
  exit 1
fi

if [ "$2" == "" ]; then
  echo usage error:  specify topic param 2
  exit 1
fi

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
        unset list2
      fi
    fi
  fi
  unset list
}

PRIMARY=`GetPrimaryServer`
AP_HOST=$(eval echo \$\{${PRIMARY}_HOST\})
AP_IPv4_1=$(eval echo \$\{${PRIMARY}_IPv4_1\})
AP_IPv4_2=$(eval echo \$\{${PRIMARY}_IPv4_2\})
unset PRIMARY

imaserver="${AP_IPv4_2}"
if [[ "${imaserver}" == "" ]]; then
  printf "WARNING A?_IPv4_1 not set, using A?_HOST instead\n"
  printf "imaserver=${AP_HOST}\n"
  imaserver="${AP_HOST}"
fi

if [[ "${imaserver}" != "10."* ]]; then
  printf "\n"
  printf "!!!!!!!!!!!   WARNING imaserver ${imaserver} not set to a 10dot address!!!!!!!!!!!!!!!!!!!\n"
  printf "\n"
fi

port=16999
clientid=$1
topic=$2
 
#sample=com.ibm.ism.samples.jms.JMSSample
sample=svt.jms.ism.JMSSample

java ${sample} -a subscribe -t ${topic} -b -i ${clientid} -s tcp://${imaserver}:${port} -n 0

exit

