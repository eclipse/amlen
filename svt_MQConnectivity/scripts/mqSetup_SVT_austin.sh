#!/bin/bash

export MQKEY=AFsvt01

su -l mqm -c 'echo ". /opt/mqm/bin/setmqenv -p /opt/mqm/" >> .bash_profile'

su -l mqm -c '/opt/mqm/bin/endmqm -i MQC_SVT_MSGCONVERSION'
sleep 5
su -l mqm -c '/opt/mqm/bin/dltmqm MQC_SVT_MSGCONVERSION'
su -l mqm -c '/opt/mqm/bin/crtmqm MQC_SVT_MSGCONVERSION'
su -l mqm -c '/opt/mqm/bin/strmqm MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "ALTER AUTHINFO('\'SYSTEM.DEFAULT.AUTHINFO.IDPWOS\'') AUTHTYPE(IDPWOS) CHCKCLNT(NONE) CHCKLOCL(NONE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "REFRESH SECURITY" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE CHANNEL(IMA.SVT.TEST.SVRCONN)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE CHANNEL(IMA.SVT.TEST.SVRCONN) CHLTYPE(SVRCONN) TRPTYPE(TCP) SHARECNV(1)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DELETE CHANNEL(SVT_MQCLIENTS)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE CHANNEL(SVT_MQCLIENTS) CHLTYPE(SVRCONN) TRPTYPE(TCP) SHARECNV(1)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DELETE LISTENER(SVT_MQ_LISTENER)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE LISTENER(SVT_MQ_LISTENER) TRPTYPE (TCP) CONTROL (QMGR) PORT (1424)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "START LISTENER(SVT_MQ_LISTENER)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DELETE CHANNEL(CHNLJMS)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE CHANNEL(CHNLJMS) CHLTYPE(SVRCONN) TRPTYPE(TCP) SHARECNV(1)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'


su -l mqm -c 'echo "DEFINE QLOCAL(SYSTEM.MQTT.TRANSMIT.QUEUE) USAGE(XMITQ) MAXDEPTH(10000000)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "ALTER QMGR DEFXMITQ(SYSTEM.MQTT.TRANSMIT.QUEUE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c '/opt/mqm/bin/setmqaut -m MQC_SVT_MSGCONVERSION -t q -n SYSTEM.MQTT.TRANSMIT.QUEUE -p nobody -all +put'
su -l mqm -c '/opt/mqm/bin/setmqaut -m MQC_SVT_MSGCONVERSION -t topic -n SYSTEM.BASE.TOPIC -p nobody -all +pub +sub'
su -l mqm -c 'cat /opt/mqm/mqxr/samples/installMQXRService_unix.mqsc | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "START SERVICE('\'SYSTEM.MQXR.SERVICE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DELETE CHANNEL(IMA.SVT.TEST.MQTT)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE CHANNEL(IMA.SVT.TEST.MQTT) CHLTYPE(MQTT) PORT(1893) MCAUSER('\'mqm\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "SET CHLAUTH('\'IMA.SVT.TEST.MQTT\'') TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.SVT.TEST.MQTT\'') TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "SET CHLAUTH('\'CHNLJMS\'') TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'CHNLJMS\'') TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "set authrec objtype(QMGR) principal('\'mqm\'') authadd(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.SVT.TEST.SVRCONN\'') TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.SVT.TEST.SVRCONN\'') TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'mqm\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.DEFAULT.MODEL.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'mqm\'') AUTHADD(DSP, GET)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.ADMIN.COMMAND.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'mqm\'') AUTHADD(DSP, PUT)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.IMA.*\'') OBJTYPE(QUEUE) PRINCIPAL('\'mqm\'') AUTHADD(CRT, PUT, GET, BROWSE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.DEFAULT.LOCAL.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'mqm\'') AUTHADD(DSP)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.BASE.TOPIC\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
 
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.TEST.MQTT\'') TYPE(BLOCKUSER) USERLIST('\'none\'') ACTION(REPLACE)"   | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.SVT.TEST.SVRCONN\'') TYPE(BLOCKUSER) USERLIST('\'none\'') ACTION(REPLACE)"   | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'IMA.TEST.SVRCONN\'') TYPE(BLOCKUSER) USERLIST('\'none\'') ACTION(REPLACE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'*MQCLIENTS*\'') TYPE(BLOCKUSER) USERLIST('\'none\'') ACTION(REPLACE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET CHLAUTH('\'CHNLJMS\'') TYPE(BLOCKUSER) USERLIST('\'none\'') ACTION(REPLACE)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/MORAY\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/MORAY\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/MORAY\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/MORAY\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/MORAY\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/DUNDEE\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/DUNDEE\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/ANGUS\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/ANGUS\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/ANGUS\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/ANGUS\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/ANGUS\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/ARGYLL\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/ARGYLL\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/BUTE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/BUTE\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/BUTE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/BUTE\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/BUTE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'

su -l mqm -c 'echo "DELETE TOPIC('\'${MQKEY}/UK/SCOTLAND/FIFE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DEFINE TOPIC('\'${MQKEY}/UK/SCOTLAND/FIFE\'') TOPICSTR('\'${MQKEY}/UK/SCOTLAND/FIFE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'${MQKEY}/UK/SCOTLAND/FIFE\'') OBJTYPE(TOPIC) PRINCIPAL('\'mqm\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
su -l mqm -c 'echo "DISPLAY TOPIC('\'${MQKEY}/UK/SCOTLAND/FIFE\'')" | /opt/mqm/bin/runmqsc MQC_SVT_MSGCONVERSION'
