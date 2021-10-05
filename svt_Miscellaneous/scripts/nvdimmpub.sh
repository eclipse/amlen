#!/bin/bash

############################################################################################################
#
#  This script publishes a 1 byte JMS message to port 16111 of the appliance.
#
#  This script takes 2 parameters:
#    clientid
#    topic
#
#  SSL is used to connect to the appliance and a userid/password is specified for authentication/authorization
#
#############################################################################################################

#set -x
if [ "$1" == "" ]; then
  echo usage error:  specify client id as param 1
  exit 1
fi

if [ "$2" == "" ] ; then
  echo usage error:  specify topic as param 2
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



imaserver="${AP_IPv4_1}"
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

curDir=$(dirname ${0})

port=16111
clientid=${1}p
topic=$2
#SSL="-Djavax.net.ssl.keyStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.keyStorePassword=k1ngf1sh -Djavax.net.ssl.trustStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh"
SSL="-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks -Djavax.net.ssl.trustStorePassword=password"

sample=svt.jms.ism.JMSSample
#debug="-DIMATraceLevel=9 -DIMATraceFile=jms.log"

printf "java ${debug} ${stack} ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcps://${imaserver}:${port} -n $2 -u c0000000 -p imasvtest -r -z 1\n"
java ${debug} ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcps://${imaserver}:${port} -n 1 -u c0000000 -p imasvtest -r -z 1

exit

