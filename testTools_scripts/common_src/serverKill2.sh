#! /bin/bash

../scripts/serverControl.sh -a killServer -i 1
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill2.sh FAILED to stop server 1"
  exit 1
fi

../scripts/serverControl.sh -a killServer -i 2
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill2.sh FAILED to stop server 2"
  exit 1
fi

sleep 20

../scripts/serverControl.sh -a startServer -i 1
../scripts/serverControl.sh -a startServer -i 2

done=0
while [ ${done} -eq 0 ] ; do
  ../scripts/serverControl.sh -a checkRunning -i 1
  rc=$?
  if [ ${rc} == 0 ] ; then
    done=1
  fi
done
