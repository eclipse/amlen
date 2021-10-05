#! /bin/bash

../scripts/serverControl.sh -a restartServer -i 1

../scripts/serverControl.sh -a checkStatus -i 1 -s production -t 25
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverRestart.sh FAILED to check status for server 1"
  exit 1
fi
