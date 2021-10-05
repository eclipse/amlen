#!/bin/bash
#This script is used to accept the license agreement

# Source the ISMsetup file to get the appliance IP address (A1_HOST)
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

if [ -z "${M1_BROWSER_LIST}" ]; then
	FIRST_BROWSER="firefox"
else
	#IFS=. read -a BROWSERS << "${M1_BROWSER_LIST}"
	BROWSERS=(${M1_BROWSER_LIST//,/ })
	FIRST_BROWSER="${BROWSERS[0]}"
fi
## FIXME: Selenium requires that we setup PATH for iexplore to work
if [ "${FIRST_BROWSER}" == "iexplore" ]; then
    FIRST_BROWSER="firefox"
fi

CLASSNAME="com.ibm.ism.test.testcases.license.LicenseAgreementTest"
URL="http://${A1_HOST}:9080/ISMWebUI"
USER="${A1_USER}"
PASSWORD="${USER}"
ARG1="URL=${URL}"
ARG2="USER=${USER}"
ARG3="PASSWORD=${PASSWORD}"
ARG4="BROWSER=${FIRST_BROWSER}"
ARG5="DEBUG=true"

SCRIPT=`basename $0`
outFile="${M1_TESTROOT}/setup/${SCRIPT}.log"
echo "M1_BROWSER_LIST=${M1_BROWSER_LIST}" >> ${outFile}
ostype=`uname`
echo "${ostype}" >> ${outFile}
case ${ostype} in
    CYGWIN*)
        echo "This machine ${M1_HOST} is Windows" >> ${outFile}
        ;;
    *)
        echo "This machine ${M1_HOST} is Linux" >> ${outFile}
		vncserver -kill :3
        vncserver :3 -localhost &>/dev/null
        export DISPLAY="localhost:3"
        echo "DISPLAY=${DISPLAY}" >> ${outFile}
        ;;
esac

cmd="java ${CLASSNAME} ${ARG1} ${ARG2} ${ARG3} ${ARG4} ${ARG5}"
echo "Running: ${cmd}" >> ${outFile} 
output=`$cmd`
RC=$?
echo ${output} >> "${outFile}"
if [ "$RC" -eq "0" ]; then
   echo "PASSED: Accept License Agreement"
else
   echo "FAILED: Accept License Agreement"
fi
