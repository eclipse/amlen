#!/bin/bash

while getopts "a:b:m:n:f:" opt; do
  case ${opt} in
  a )
    aLOG=${OPTARG}
    ;;
  b )
    bLOG=${OPTARG}
    ;;
  m )
    mMsg=${OPTARG}
    ;;
  n )
    nMsg=${OPTARG}
    ;;
  f )
    LOG=${OPTARG}
    ;;
  * )
    exit 1
    ;;
  esac
done

echo "aLOG=${aLOG}"
echo "bLOG=${bLOG}"
echo "mMSG=${mMsg}"
echo "nMSG=${nMsg}"

numA=`grep -c "${mMsg}" ${aLOG}`
numB=`grep -c "${nMsg}" ${bLOG}`

if [ ${numA} -eq ${numB} ] ; then
  echo "Found ${numA} occurrences in both logs" | tee -a ${LOG}
  echo "Test result is Success!" | tee -a ${LOG}
else
  echo "Found ${numA} occurrences in ${aLOG} and ${numB} occurrences in ${bLOG}"
  echo "Test result is Failure!" | tee -a ${LOG}
fi
