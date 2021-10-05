#! /bin/bash

echo Killing imaserver process. TYPE=A1_TYPE

../scripts/serverControl.sh -a killServer -i 1
rc=$?
if [ ${rc} -ne 0 ] ; then
  echo "serverKill.sh FAILED"
  exit 1
fi

exit 0
