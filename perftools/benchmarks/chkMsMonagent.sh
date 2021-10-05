#!/bin/bash

LOGFILE=/var/log/chkMsMonagent.log
bond0_ip=`/usr/sbin/ip addr show dev bond0 2>> $LOGFILE| grep -o "inet [0-9]*\.[0-9]*\.[0-9]*\.[0-9]*" | cut -d' ' -f2` 
ps -C "python /usr/bin/ms-monagent.py" -o pid= >/dev/null 2>/dev/null
chkpid=$?

if [ "$chkpid" == "1" ] ; then
  echo "$(date): ms-monagent.py was not running, starting it now" >> $LOGFILE
  python /usr/bin/ms-monagent.py --adminEndpoint $bond0_ip &>> $LOGFILE &
fi
