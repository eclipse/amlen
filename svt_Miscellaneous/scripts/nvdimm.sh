#!/bin/bash

############################################################################################################
#
#  This testcase creates as many durable subscriptions on the appliance as possible (up to 10,000,000)
#  in the time alloted.  The subscriptions are created from each of the M_COUNT clients.
#
#  The topic intentionally contains many characters in an order to use more nvdimm memory.
#  topic=/APP/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/${THIS}
#
#  The testcase passes if the appliance does not abort and therefore create a core file.
#  When finished "${THIS}:  SUCCESS" is printed to the log
#
#############################################################################################################

curDir=$(dirname ${0})
sample=svt.jms.ism.JMSSample
#SSL="-Djavax.net.ssl.keyStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.keyStorePassword=k1ngf1sh -Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh"

if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else 
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

run()
{
  declare -li rt

  nvtopic=/APP/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/${THIS}
  nvcount=$((10000000/${M_COUNT}))

  printf "${THIS}:  Start fill\n"

  rt=0
  while [ ${rt} -lt ${nvcount} ]; do
     ${curDir}/nvdimmsub.sh ${THIS}_${rt} ${nvtopic}_${rt} &
     
#    sleep .3s
     
#    ${curDir}/nvdimmpub.sh ${THIS}p_${r} ${nvtopic}_${r} &

     while [[ `ps -ef|grep java|wc -l` -gt 30 ]]; do
        sleep 1s
     done
     ((rt++))
  done
  
  printf "${THIS}:  Completed fill\n"
}


delete()
{
   if [ -z ${SERVER} ]; then
     SERVER=`../scripts/getPrimaryServerAddr.sh`
   fi 

   ssh admin@${SERVER} imaserver stat Subscription StatType=PublishedMsgsHighest ResultCount=100 | tail -n +2 > ./console.out
   i=0; while read line; do out[$i]=${line}; ((i++)); done < ./console.out

    while [ ${#out[@]} -gt 0 ]; do
      for line in ${out[@]}; do
        if [ ${#line} -gt 3 ]; then
          sn=`echo ${line}| cut -d"," -f1`
          cn=`echo ${line}| cut -d"," -f3`
          if [[ "${sn}" == \"*\" ]]; then
             sn=${sn:1:$((${#sn}-2))}
          fi
          if [[ "${cn}" == \"*\" ]]; then
             cn=${cn:1:$((${#cn}-2))}
          fi

        if [[ ${sn} =~ ${THIS} ]]; then
          echo ssh admin@${SERVER} "imaserver delete Subscription \"SubscriptionName=${sn}\" \"ClientID=${cn}\""
          ssh admin@${SERVER} "imaserver delete Subscription \"SubscriptionName=${sn}\" \"ClientID=${cn}\""
        fi
        fi
      done

      if [ ${#out[@]} -lt 100 ]; then
         break
      fi
    
      > ./console.out
      ssh admin@${SERVER} imaserver stat Subscription StatType=PublishedMsgsHighest ResultCount=100 | tail -n +2 > ./console.out
      i=0; while read line; do out[$i]=${line}; ((i++)); done < ./console.out
    done
}

run
sleep 10s
delete


printf "${THIS}:  SUCCESS"


