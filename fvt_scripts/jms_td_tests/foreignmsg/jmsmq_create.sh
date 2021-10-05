#!/bin/bash
# 

su -l mqm -c '/opt/mqm/bin/crtmqm QM_MQJMS'
su -l mqm -c '/opt/mqm/bin/strmqm QM_MQJMS'
su -l mqm -c 'echo "define channel ('\'CHNLJMS\'') chltype (SVRCONN)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "define listener ('\'MQ_LISTENER\'') TRPTYPE(TCP) PORT(1415) CONTROL(QMGR)" | /opt/mqm/bin/runmqsc QM_MQJMS'
su -l mqm -c 'echo "start listener ('\'MQ_LISTENER\'')" | /opt/mqm/bin/runmqsc QM_MQJMS'
