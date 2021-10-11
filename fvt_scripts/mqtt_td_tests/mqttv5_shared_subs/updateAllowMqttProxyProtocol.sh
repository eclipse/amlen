#!/bin/bash +x

# usage: updateAllowMqttProxyProtocol [on|off]

LOGFILE=updateAllowMqttProxyProtocol.log

echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Starting updateAllowMqttProxyProtocol.sh:  $# : $@" >> ${LOGFILE}

if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

statusProduction="Running (production)"
statusStandby="Standby"

appliance=1

if [ "$1" = "on" ]; then
  while [ ${appliance} -le ${A_COUNT} ]
  do
      aIP="A${appliance}_HOST"
      aIPVal=$(eval echo \$${aIP})

      aPORT="A${appliance}_PORT"
      aPORTVal=$(eval echo \$${aPORT})

      aTYPE="A${appliance}_TYPE"
      aTYPEVal=$(eval echo \$${aTYPE})
      if [ ${aTYPEVal} == "DOCKER" ]; then
          DIRPATH="/mnt/A${appliance}"
      else
          DIRPATH="/var"
      fi

      isPresent=`echo "grep 'Protocol.AllowMqttProxyProtocol' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null`
      if [[ "${isPresent}" =~ "Protocol.AllowMqttProxyProtocol" ]] ; then
        echo "AllowMqttProxyProtocol present, setting to true" | tee -a ${LOGFILE}
        echo "sed -i -e 's/Protocol.AllowMqttProxyProtocol.*/Protocol.AllowMqttProxyProtocol=true/g' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null
      else
      	echo "echo 'Protocol.AllowMqttProxyProtocol=true' >> ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null
      fi

  	echo "A${appliance} server.cfg:" | tee -a ${LOGFILE}
  	echo "grep 'Protocol.AllowMqttProxyProtocol' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null

    statusResponse=`curl -sSf -XGET http://${aIPVal}:${aPORTVal}/ima/service/status 2>/dev/null`
    StateDescription=`echo $statusResponse | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`
    haStatus=`echo $statusResponse | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"HighAvailability\"][\"Status\"])"`

    echo "Restarting A${appliance}.."
    if [[ "${haStatus}" =~ "Active" ]] ; then
      #if in HA then after restart should expect standby
      if [[ "${StateDescription}" =~ "${statusProduction}" ]] ; then
        ../scripts/serverControl.sh -a restartServer -i ${appliance} -s standby >> ${LOGFILE}
      elif [[ "${StateDescription}" =~ "${statusStandby}" ]] ; then
        ../scripts/serverControl.sh -a restartServer -i ${appliance} -s standby >> ${LOGFILE}
      fi
    else
      #if not in HA should expect production
      ../scripts/serverControl.sh -a restartServer -i ${appliance} -s production >> ${LOGFILE}
    fi

  	((appliance+=1))
  done
else
  if [ "$1" = "off" ]; then
    while [ ${appliance} -le ${A_COUNT} ]
    do
        aIP="A${appliance}_HOST"
        aIPVal=$(eval echo \$${aIP})

        aPORT="A${appliance}_PORT"
        aPORTVal=$(eval echo \$${aPORT})

        aTYPE="A${appliance}_TYPE"
        aTYPEVal=$(eval echo \$${aTYPE})
        if [ ${aTYPEVal} == "DOCKER" ]; then
            DIRPATH="/mnt/A${appliance}"
        else
            DIRPATH="/var"
        fi

        isPresent=`echo "grep 'Protocol.AllowMqttProxyProtocol' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null`
        if [[ "${isPresent}" =~ "Protocol.AllowMqttProxyProtocol" ]] ; then
          echo "sed -i -e '/Protocol.AllowMqttProxyProtocol/d' ${DIRPATH}/messagesight/data/config/server.cfg" | ssh ${aIPVal} 2>/dev/null
        fi

      echo "AllowMqttProxyProtocol removed from A${appliance} server.cfg" | tee -a ${LOGFILE}


      statusResponse=`curl -sSf -XGET http://${aIPVal}:${aPORTVal}/ima/service/status 2>/dev/null`
      StateDescription=`echo $statusResponse | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`
      haStatus=`echo $statusResponse | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"HighAvailability\"][\"Status\"])"`

      echo "Restarting A${appliance}.."
      if [[ "${haStatus}" =~ "Active" ]] ; then
        if [[ "${StateDescription}" =~ "${statusProduction}" ]] ; then
          ../scripts/serverControl.sh -a restartServer -i ${appliance} -s standby >> ${LOGFILE}
        elif [[ "${StateDescription}" =~ "${statusStandby}" ]] ; then
          ../scripts/serverControl.sh -a restartServer -i ${appliance} -s standby >> ${LOGFILE}
        fi
      else
        ../scripts/serverControl.sh -a restartServer -i ${appliance} -s production >> ${LOGFILE}
      fi

      ((appliance+=1))
    done
  else
    echo "Usage is: $0 [on|off]"
  fi
fi
