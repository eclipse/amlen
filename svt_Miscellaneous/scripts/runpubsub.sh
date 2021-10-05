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

repeat=10000
curDir=$(dirname ${0})

sample=svt.jms.ism.JMSSample
#SSL="-Djavax.net.ssl.keyStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.keyStorePassword=k1ngf1sh -Djavax.net.ssl.trustStore=${curDir}/../svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh"
SSL="-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks -Djavax.net.ssl.trustStorePassword=password"

if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else 
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

# GetPrimaryServer() {
# i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done

# if [[ ! "${list[*]}" =~ "HARole" ]]; then
#   printf "A1\n"
# else
#   if [[ "${list[*]}" =~ "PRIMARY" ]]; then
#     printf "A1\n"
#   else
#     if [ "${A2_HOST}" == "" ]; then
#       printf "A1\n"
#     else
#       i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done

#       if [[ "${list2[*]}" =~ "PRIMARY" ]]; then
#         printf "A2\n"
#       else
#         printf "A1\n"
#       fi
#       unset list2
#     fi
#   fi
# fi
# unset list
# }

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

monmem()
{
  data=`curl -s -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Memory`

  printf "${THIS}:  ${data}\n"
} 

memfree()
{
# mem=`ssh admin@${imaserver} imaserver stat memory | grep MemoryFreePercent | sed -e "s/^MemoryFreePercent = //"`
  echo curl -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Store
  data=`curl -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Store`
  mem=`data=${data//*Pool1UsedPercent\":/}; echo ${data//,*/}`

  printf "${THIS}:  mem=${mem}\n"
} 

monsub()
{
  data=`curl -s -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Subscription`

  printf "${THIS}:  ${data}\n"
}

stored_messages()
{
# msgs=`ssh admin@${imaserver} imaserver stat Subscription StatType=PublishedMsgsHighest | grep "$1" |cut -d',' -f5`
  echo curl -s -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Subscription
  data=`curl -s -X GET http://${imaserver}:${A1_PORT}/ima/v1/monitor/Subscription`
  msgs=`data=${data//*BufferedMsgs\":/}; echo ${data//,*/}`

  printf "${THIS}:  msgs=${msgs}\n"
}

subscribe()
{
    ${curDir}/jmssub.sh ${THIS} 1 &
    sleep 1
    ${curDir}/ssljmspub.sh ${THIS} 1 1
    wait
}

fill()
{
  printf "${THIS}:  ${curDir}/ssljmspub.sh ${THIS} $1 $((150*1024*1024))\n"
  ${curDir}/ssljmspub.sh ${THIS} $1 $((150*1024*1024))
}

fillmore()
{
  memfree
  for f in $((50*1024*1024)) $((1024*1024)) 1024 512 64 32; do
    while [ ${mem} -gt 15 ]; do
      printf "${THIS}:  ${curDir}/ssljmspub.sh ${THIS} 8 ${f}\n"
      ${curDir}/ssljmspub.sh ${THIS} 8 ${f}
      memfree
    done
    printf "${THIS}:  ${curDir}/ssljmspub.sh ${THIS} 1 ${f}\n"
    ${curDir}/ssljmspub.sh ${THIS} 1 ${f}
  done
  
  printf "${THIS}:  fill complete\n"
}

drain_messages()
{
# msgs=256
# printf "${THIS}$1:  ${curDir}/jmssub.sh ${THIS}$1 ${msgs}\n"
# echo ${curDir}/jmssub.sh ${THIS}$1 ${msgs}
# ${curDir}/jmssub.sh ${THIS}$1 ${msgs}

  stored_messages ${THIS}$1
  printf "${THIS}$1:  ${curDir}/jmssub.sh ${THIS}$1 ${msgs}\n"
  ${curDir}/jmssub.sh ${THIS}$1 ${msgs}
  printf "${THIS}$1:  drain complete\n"
}

waiting()
{
  port=16999
  clientid=${THIS}waitsync
  topic=/APP/PUB
  
  printf "java ${sample} -a subscribe -t ${topic} -i ${clientid} -s tcp://${AP_IPv4_2}:${port} -n 1\n"
  java ${sample} -a subscribe -t ${topic} -i ${clientid} -s tcp://${AP_IPv4_2}:${port} -n 1 &
  printf "wait for sync...\n"
  wait
}

signal_others()
{
  port=16111
  clientid=sigpub
  topic=/APP/PUB
  
  printf "sending sync...\n"
  printf "java ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcps://${AP_IPv4_1}:${port} -n 1 -z 1 -u c0000000 -p imasvtest \n"
  java ${SSL} ${sample} -i ${clientid} -a publish -t ${topic} -s tcps://${AP_IPv4_1}:${port} -n 1 -z 1 -u c0000000 -p imasvtest 
}


run()
{
  sleep 2s

# for r in {1..${repeat}}; do
    printf "${THIS}:  Start fill loop ${r} of ${repeat}\n"
    fill $((680/${M_COUNT}))
#   waiting
#   printf "${THIS}:  Completed fill loop ${r} of ${repeat}\n"
# done
}

runsync()
{
  subscribe

  for r in $(eval echo {1..${repeat}}); do
    printf "${THIS}:  Start fill loop ${r} of ${repeat}\n"
echo --------------------------------------------------------------------
    fill 497
    sleep 30s
    monsub
    monmem
echo --------------------------------------------------------------------
#   fill 10
  #   fill 10
  #   fill 10
  #   fill $((630/${M_COUNT}))
#   fillmore

 #   for i in {1..6}; do
 #     ${curDir}/jmssub.sh ${THIS} 1
 #     fill 2
 #   done

#   sleep 5s
#   ../scripts/haFunctions.py -a stopBoth -t 300
#   sleep 5s
#   ../scripts/haFunctions.py -a verifyStatus -m1 1 -status STATUS_STOPPED -t 300
#   ../scripts/haFunctions.py -a verifyStatus -m1 2 -status STATUS_STOPPED -t 300

#   ../scripts/haFunctions.py -a startBoth -t 300
#   sleep 10s
#   ../scripts/haFunctions.py -a verifyStatus -m1 2 -status STATUS_STANDBY -t 600
#   ../scripts/haFunctions.py -a verifyStatus -m1 1 -status STATUS_RUNNING -t 600

#   ../scripts/haFunctions.py -a restartPrimary -t 300
#   sleep 5s
#   ../scripts/haFunctions.py -a verifyStatus -m1 1 -status STATUS_STANDBY -t 600
#   ../scripts/haFunctions.py -a verifyStatus -m1 2 -status STATUS_RUNNING -t 600

#   sleep 5s
echo --------------------------------------------------------------------
    drain_messages 
    sleep 30s
    monsub
    monmem
echo --------------------------------------------------------------------
    sleep 5s

#   signal_others
    printf "${THIS}:  Completed fill loop ${r} of ${repeat}\n"
  done
}

#if [ "${THIS}" == "M1" ]; then
  runsync
#else 
#  run
#fi

printf "${THIS}:  SUCCESS"


