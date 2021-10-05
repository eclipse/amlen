#!/bin/bash
#set -x

#su -l mqm -c '/opt/mqm/bin/endmqm -i QM_MQJMS'
#su -l mqm -c '/opt/mqm/bin/dltmqm QM_MQJMS'
#su -l mqm -c '/opt/mqm/bin/crtmqm QM_MQJMS'
su -l mqm -c '/opt/mqm/bin/strmqm QM_MQJMS'

su -l mqm -c 'echo "set authrec objtype(QMGR) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.DEFAULT.MODEL.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'root\'') AUTHADD(DSP, GET)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.BASE.TOPIC\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQCONN'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.ADMIN.COMMAND.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'root\'') AUTHADD(DSP, PUT)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.IMA.*\'') OBJTYPE(QUEUE) PRINCIPAL('\'root\'') AUTHADD(CRT, PUT, GET, BROWSE)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'SYSTEM.DEFAULT.LOCAL.QUEUE\'') OBJTYPE(QUEUE) PRINCIPAL('\'root\'') AUTHADD(DSP)" | /opt/mqm/bin/runmqsc QM_MQJMS'

#su -l mqm -c 'echo "DELETE CHANNEL(MQCLIENTS)" | /opt/mqm/bin/runmqsc QM_MQJMS'
#su -l mqm -c 'echo "DEFINE CHANNEL(MQCLIENTS) CHLTYPE(SVRCONN) TRPTYPE(TCP)" | /opt/mqm/bin/runmqsc QM_MQJMS'
#su -l mqm -c 'echo "DELETE CHANNEL(CHNLJMS)" | /opt/mqm/bin/runmqsc QM_MQJMS'
#su -l mqm -c 'echo "DEFINE CHANNEL(CHNLJMS) CHLTYPE(SVRCONN) TRPTYPE(TCP)" | /opt/mqm/bin/runmqsc QM_MQJMS'

#su -l mqm -c 'echo "DELETE LISTENER(MQ_LISTENER) TRPTYPE (TCP) CONTROL (QMGR) PORT (1415)" | /opt/mqm/bin/runmqsc QM_MQJMS'
#su -l mqm -c 'echo "DEFINE LISTENER(MQ_LISTENER) TRPTYPE (TCP) CONTROL (QMGR) PORT (1415)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "START LISTENER(MQ_LISTENER)" | /opt/mqm/bin/runmqsc QM_MQJMS'

for ip_address in ${@:2}; do
        su -l mqm -c 'echo "SET CHLAUTH(CHNLJMS) type(ADDRESSMAP) address('\'$ip_address\'') mcauser('\'root\'') action(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
done
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'/UK/FOOTBALL/ARSENAL\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'/US/FOOTBALL/COWBOYS\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "SET CHLAUTH(MQCLIENTS) TYPE(BLOCKUSER) USERLIST('\'idontexist\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET CHLAUTH(CHNLJMS) TYPE(BLOCKUSER) USERLIST('\'idontexist\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "SET CHLAUTH(MQCLIENTS) TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET CHLAUTH(MQCLIENTS) TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET CHLAUTH(CHNLJMS) TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET CHLAUTH(CHNLJMS) TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/FOOTBALL/ARSENAL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/FOOTBALL/ARSENAL\'') TOPICSTR('\'$1/FOOTBALL/ARSENAL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/FOOTBALL/ARSENAL\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/FOOTBALL/ARSENAL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete topic('\'$1/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'$1/UK/SCOTLAND/ARGYLL\'') TOPICSTR('\'$1/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/UK/SCOTLAND/ARGYLL\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'$1/UK/SCOTLAND/ARGYLL\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete topic('\'$1/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'$1/UK/SCOTLAND/DUNDEE\'') TOPICSTR('\'$1/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/UK/SCOTLAND/DUNDEE\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'$1/UK/SCOTLAND/DUNDEE\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete topic('\'$1/UK/SCOTLAND/OBAN\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'$1/UK/SCOTLAND/OBAN\'') TOPICSTR('\'$1/UK/SCOTLAND/OBAN\'')"| /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/UK/SCOTLAND/OBAN\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'$1/UK/SCOTLAND/OBAN\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1/UK/SCOTLAND/PERTH\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1/UK/SCOTLAND/PERTH\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/UK/SCOTLAND/PERTH\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1/UK/SCOTLAND/PERTH\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/MQTopic/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/MQTopic/To/ISMTopic\'') TOPICSTR('\'$1/MQTopic/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/MQTopic/To/ISMTopic\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/MQTopic/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/MQTopic/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/MQTopic/To/ISMQueue\'') TOPICSTR('\'$1/MQTopic/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/MQTopic/To/ISMQueue\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/MQTopic/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopic/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopic/To/MQTopic\'') TOPICSTR('\'$1/ISMTopic/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopic/To/MQTopic\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopic/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics0\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics0\'') TOPICSTR('\'$1/ISMTopic/To/MultipleMQTopics0\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopic/To/MultipleMQTopics0\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopic/To/MultipleMQTopics0\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics1\'') TOPICSTR('\'$1/ISMTopic/To/MultipleMQTopics1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopic/To/MultipleMQTopics1\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopic/To/MultipleMQTopics1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics2\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopic/To/MultipleMQTopics2\'') TOPICSTR('\'$1/ISMTopic/To/MultipleMQTopics2\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopic/To/MultipleMQTopics2\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopic/To/MultipleMQTopics2\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopicSubtree/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopicSubtree/To/MQTopic\'') TOPICSTR('\'$1/ISMTopicSubtree/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopicSubtree/To/MQTopic\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopicSubtree/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMTopicSubtree/To/MQTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMTopicSubtree/To/MQTopicSubtree\'') TOPICSTR('\'$1/ISMTopicSubtree/To/MQTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMTopicSubtree/To/MQTopicSubtree\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMTopicSubtree/To/MQTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/MQTopicSubtree/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/MQTopicSubtree/To/ISMTopic\'') TOPICSTR('\'$1/MQTopicSubtree/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/MQTopicSubtree/To/ISMTopic\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/MQTopicSubtree/To/ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/MQTopicSubtree/To/ISMTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/MQTopicSubtree/To/ISMTopicSubtree\'') TOPICSTR('\'$1/MQTopicSubtree/To/ISMTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/MQTopicSubtree/To/ISMTopicSubtree\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/MQTopicSubtree/To/ISMTopicSubtree\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/ISMQueue/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/ISMQueue/To/MQTopic\'') TOPICSTR('\'$1/ISMQueue/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/ISMQueue/To/MQTopic\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/ISMQueue/To/MQTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "DELETE TOPIC('\'$1/MQTSubtree/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DEFINE TOPIC('\'$1/MQTSubtree/To/ISMQueue\'') TOPICSTR('\'$1/MQTSubtree/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET AUTHREC PROFILE('\'$1/MQTSubtree/To/ISMQueue\'') OBJTYPE(TOPIC) PRINCIPAL('\'root\'') AUTHADD(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "DISPLAY TOPIC('\'$1/MQTSubtree/To/ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.ISMTopic.To.MQQueue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.ISMTopic.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.ISMTopic.To.MQQueue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.ISMTopic.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.MQQueue.To.ISMTopic\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.MQQueue.To.ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.MQQueue.To.ISMTopic\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.MQQueue.To.ISMTopic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.ISMTopicSubtree.To.MQQueue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.ISMTopicSubtree.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.ISMTopicSubtree.To.MQQueue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.ISMTopicSubtree.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.ISMQueue.To.MQQueue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.ISMQueue.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.ISMQueue.To.MQQueue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.ISMQueue.To.MQQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.MQQueue.To.ISMQueue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.MQQueue.To.ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.MQQueue.To.ISMQueue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.MQQueue.To.ISMQueue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

