#! /bin/bash

../scripts/serverControl.sh -a killServer -i 1
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill1Then2Restart.sh FAILED to stop server"
  exit 1
fi

../scripts/serverControl.sh -a checkStatus -i 2 -t 15 -s production
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill1Then2Restart.sh FAILED to checkStatus A2"
  exit 1
fi

../scripts/serverControl.sh -a killServer -i 2
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill1Then2Restart.sh FAILED to stop server"
  exit 1
fi
sleep 20

../scripts/haFunctions.py -a startAndPickPrimary -p 2 -f serverKill1Then2Restart.log

echo Status of server 1
curl http://A1_HOST:A1_PORT/ima/service/status
  
echo Status of server 2
curl http://A2_HOST:A2_PORT/ima/service/status
