#! /bin/bash

../scripts/serverControl.sh -a killServer -i 1
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill1.sh FAILED to stop server"
  exit 1
fi

sleep 20
../scripts/serverControl.sh -a startServer -i 1

done=0
while [ ${done} -eq 0 ] ; do
  ../scripts/serverControl.sh -a checkRunning -i 1
  rc=$?
  if [ ${rc} == 0 ] ; then
    done=1
  fi
done
