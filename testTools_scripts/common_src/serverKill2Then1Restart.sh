#! /bin/bash

../scripts/serverControl.sh -a killServer -i 2
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill2Then1Restart.sh FAILED to stop server 2"
  exit 1
fi

../scripts/serverControl.sh -a checkStatus -i 1 -t 15 -s production
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill2Then1Restart.sh FAILED to checkStatus A1"
  exit 1
fi

../scripts/serverControl.sh -a killServer -i 1
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill2Then1Restart.sh FAILED to stop server 1"
  exit 1
fi

sleep 20

../scripts/serverControl.sh -a startServer -i 1
../scripts/serverControl.sh -a startServer -i 2

echo ../scripts/cluster.py  -a verifyStatus -m 1 -v 2 -s STATUS_RUNNING -t 25 -f serverKill2Then1Restart.verify.log
../scripts/cluster.py -a verifyStatus -m 1 -v 2 -s STATUS_RUNNING -t 25 -f serverKill2Then1Restart.verify.log
../scripts/cluster.py -a verifyStatus -m 2 -v 2 -s STATUS_STANDBY -t 25 -f serverKill2Then1Restart.verify.log
echo "Done"
  
curl http://A1_HOST:A1_PORT/ima/service/status
curl http://A2_HOST:A2_PORT/ima/service/status
