#! /bin/bash

../scripts/serverControl.sh -a stopServer -i 1
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
