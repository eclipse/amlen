#! /bin/bash

../scripts/haFunctions.py -a startStandby -f serverRestart1Kill2.log

../scripts/serverControl.sh -a killServer -i 2

curl -sSf http://A1_HOST:A1_PORT/ima/v1/service/status
curl -sSf http://A2_HOST:A2_PORT/ima/v1/service/status

date
echo Exiting
exit 0
