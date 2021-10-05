#!/bin/bash 

############################################################################################################
#
#  This script publishes JMS messages to the appliance.
#
#  This script takes 3 parameters:
#    clientid
#    number of messages
#    message size
#
#  If the number of messages is >= 6, then 6 publishers are started, each publishing 1/6
#  of the message count. 
#  
#  SSL is used to connect to the appliance and a userid/password is specified for authentication/authorization
#
#############################################################################################################

#set -x
if [ "$1" == "" ]; then
  echo usage error:  specify client id as param 1
  exit 1
fi

if [ "$2" == "" ]; then
  echo usage error:  specify number of messsages as param 2
  exit 1
fi

if [ "$3" == "" ] ; then
  echo usage error:  specify message size as param 3
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

#PRIMARY=`GetPrimaryServer`
PRIMARY=A1
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

size=$3
#port=16111
port=16102
topic=/APP/M1
clientid=${1}p
stack=-Xss256M
#SSL="-Djavax.net.ssl.keyStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.keyStorePassword=k1ngf1sh -Djavax.net.ssl.trustStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh"
#SSL="-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks -Djavax.net.ssl.trustStorePassword=password"
#SSL="-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh"


sample=svt.jms.ism.JMSSample
#debug="-DIMATraceLevel=9 -DIMATraceFile=jms.log"

if [ $2 -gt 0 ]; then
  if [ $2 -ge 6 ]; then
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}B -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6)) -u c0000000 -p imasvtest -r -z ${size} &
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}C -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6)) -u c0000000 -p imasvtest -r -z ${size} &
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}D -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6)) -u c0000000 -p imasvtest -r -z ${size} &
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}E -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6)) -u c0000000 -p imasvtest -r -z ${size} &
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}K -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6)) -u c0000000 -p imasvtest -r -z ${size} &
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid}A -a publish -t ${topic} -s tcp://${A1_IPv4_1}:${port},tcp://${A2_IPv4_1}:${port} -n $(($2/6+$2%6)) -u c0000000 -p imasvtest -r -z ${size}

    wait
    wait
    wait
    wait
    wait
  else
    printf "java ${debug} ${stack} ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcp://${imaserver}:${port},tcp://${A2_IPv4_1}:${port} -n $2 -u c0000000 -p imasvtest -r -z ${size}\n"
    java ${debug} ${stack} ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcp://${imaserver}:${port},tcp://${A2_IPv4_1}:${port} -n $2 -u c0000000 -p imasvtest -r -z ${size}
  fi
fi

exit

