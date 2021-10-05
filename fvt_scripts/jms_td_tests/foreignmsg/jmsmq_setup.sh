#!/bin/bash
#------------------------------------------------------------------------------
# jmsmq_setup.sh
#------------------------------------------------------------------------------
#
# This script sets up the queues and topics in MQ that are used by the JMS
# Foreign Message tests.
# 
# Root must have authorizations to the entire queue manager QM_MQJMS.
# Each client machine IP must have authorization to the channel CHNLJMS.
# Root must have authorizations for each topic and queue.
#
# If this script is run before the FVT_MQConnectivity, then there will be
# a problem. That script deletes and recreates the Queue Manager QM_MQJMS
# but it does not redefine the channel CHNLJMS.
#
#------------------------------------------------------------------------------

#set -x

# This script assumes QM_MQJMS is already created.
su -l mqm -c '/opt/mqm/bin/strmqm QM_MQJMS'
su -l mqm -c 'echo "set authrec objtype(QMGR) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'

# Timing issue with multiple runs hitting this MQ Server. Cannot delete and recreate things in
# one run that might cause another run to fail.
#su -l mqm -c 'echo "DELETE CHANNEL('\'CHNLJMS\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
#su -l mqm -c 'echo "DEFINE CHANNEL ('\'CHNLJMS\'') CHLTYPE (SVRCONN)" | /opt/mqm/bin/runmqsc QM_MQJMS'

# This for loop shouldn't be needed anymore after adding the 2 lines below it...
# Instead of authorizing individual automation client IP's, just allow all 9. and 10. addresses
for ip_address in ${@:2}; do
	su -l mqm -c 'echo "set chlauth(CHNLJMS) type(ADDRESSMAP) address('\'$ip_address\'') mcauser('\'root\'') action(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
done

su -l mqm -c 'echo "SET CHLAUTH(MQCLIENTS) TYPE(ADDRESSMAP) ADDRESS('\'9.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "SET CHLAUTH(MQCLIENTS) TYPE(ADDRESSMAP) ADDRESS('\'10.*\'') MCAUSER('\'root\'') ACTION(REPLACE)"  | /opt/mqm/bin/runmqsc QM_MQJMS'


su -l mqm -c 'echo "delete topic('\'jmsT_$1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'jmsT_$1\'') TOPICSTR('\'jmsT_$1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'jmsT_$1\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'jmsT_$1\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete topic('\'$1/send/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'$1/send/Topic\'') TOPICSTR('\'$1/send/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/send/Topic\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'$1/send/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete topic('\'$1/reply/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define topic('\'$1/reply/Topic\'') TOPICSTR('\'$1/reply/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1/reply/Topic\'') objtype(TOPIC) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display topic('\'$1/reply/Topic\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.send.Queue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.send.Queue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.send.Queue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.send.Queue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'

su -l mqm -c 'echo "delete qlocal('\'$1.reply.Queue\'') purge" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define qlocal('\'$1.reply.Queue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "set authrec profile('\'$1.reply.Queue\'') objtype(QUEUE) principal('\'root\'') authadd(ALL)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "display qlocal('\'$1.reply.Queue\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
