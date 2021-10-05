#!/bin/sh
##  Create the CA KeyRing
export SSLDIR="SSL"
mkdir -p "${SSLDIR}/WMQ_CA"

export WMQ_CA_PATH="${SSLDIR}/WMQ_CA"

export QMGR_Prefix="/var/mqm"
export Qmgr_Name="QM_MQJMS"

if [[ $# -gt 1 ]] ; then
	if [[ $# -gt 2 ]] ; then
  		$QMGR_Prefix = $1
  		$Qmgr-Name = $2
	else
 	  	$QMGR_Prefix = $1
fi
elif [[ $# -eq 1 ]] ; then
  $QMGR_Prefix = $1
fi

export QMGR_PATH="${QMGR_Prefix}/qmgrs/${Qmgr_Name}/ssl"

export WMQ_USER_PATH="${SSLDIR}/IMASERVER"
export IMAUSER="admin"

echo 'creating the CA keyring'
##create the CA keydb
runmqckm -keydb -create -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password -type cms -stash

##  Create the self signed CA
runmqckm -cert -create -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password -label "WMQ_CA" -dn "CN=WMQ_CA,O=IBM,C=US" -expire 7300

##  Extract from the SSL CA
runmqckm -cert -extract -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password -label "WMQ_CA" -target ${WMQ_CA_PATH}/WMQ_CA.crt -format ascii

##  List the Certificates in the CA KeyRing
runmqckm -cert -list -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password

echo 'creating the queue manager keyring'
##create the Qmgr keydb
runmqckm -keydb -create -db ${QMGR_PATH}/key.kdb -pw password -type cms -stash
##  Add the SSL CA certificate
runmqckm -cert -add -db ${QMGR_PATH}/key.kdb -pw password -label WMQ_CA -file ${WMQ_CA_PATH}/WMQ_CA.crt -format ascii
##  Create the Qmgr cert request
runmqckm -certreq -create -db ${QMGR_PATH}/key.kdb -pw password -label "ibmwebspheremq${Qmgr_Name}" -dn "CN=${Qmgr_Name},O=IBM,C=US" -file ${QMGR_PATH}/${Qmgr_Name}.arm
##  Sign Qmgr cert
runmqckm -cert -sign -file ${QMGR_PATH}/${Qmgr_Name}.arm -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password -label WMQ_CA -target ${QMGR_PATH}/${Qmgr_Name}.crt -format ASCII -expire 7299
##  Receive/import Qmgr cert
runmqckm -cert -receive -db ${QMGR_PATH}/key.kdb -pw password -file ${QMGR_PATH}/${Qmgr_Name}.crt -format ascii
##  List the Certificates in the QMGR KeyRing
runmqckm -cert -list -db ${QMGR_PATH}/key.kdb -pw password

echo "refreshing the queue manager's SSL cache"

echo "REFRESH SECURITY TYPE(SSL)" | runmqsc ${Qmgr_Name} 

echo "creating the SSL channeL"
echo "DEFINE CHANNEL(CHNLJMS_SSL) CHLTYPE(SVRCONN) TRPTYPE(TCP) SSLCAUTH(REQUIRED) SSLCIPH(TLS_RSA_WITH_DES_CBC_SHA)" | runmqsc ${Qmgr_Name}

echo 'creating the ima server keyring'
##  Create the IMASERVER KeyRing
mkdir -p ${WMQ_USER_PATH}
runmqckm -keydb -create -db ${WMQ_USER_PATH}/${IMAUSER}.kdb -pw password -type cms -stash

##  Add the SSL CA certificate (that signed the Qmgr)
runmqckm -cert -add -db ${WMQ_USER_PATH}/${IMAUSER}.kdb -pw password -label "WMQ_CA"  -file ${WMQ_CA_PATH}/WMQ_CA.crt -format ascii

##  Create IMASERVER cert request
##  Note the following command only works when your ID is in all
##  lower case. Substitute your ID in all lower case if it is not
##  lower case

runmqckm -certreq -create -db ${WMQ_USER_PATH}/${IMAUSER}.kdb -pw password -label ibmwebspheremq${IMAUSER} -dn "CN=${IMAUSER},O=IBM,C=US" -file ${WMQ_USER_PATH}/${IMAUSER}.arm

##  Sign IMASERVER User cert
runmqckm -cert -sign -file ${WMQ_USER_PATH}/${IMAUSER}.arm -db ${WMQ_CA_PATH}/WMQ_CA.kdb -pw password -label WMQ_CA -target ${WMQ_USER_PATH}/${IMAUSER}.crt -format ASCII -expire 7299

##  Receive/import IMASERVER User cert
runmqckm -cert -receive -db ${WMQ_USER_PATH}/${IMAUSER}.kdb -pw password -file ${WMQ_USER_PATH}/${IMAUSER}.crt -format ascii

##  List the Certificates in the IMASERVER KeyRing
runmqckm -cert -list -db ${WMQ_USER_PATH}/${IMAUSER}.kdb -pw password

echo 'IMAServer keystore is '${WMQ_USER_PATH}/${IMAUSER}'.kdb'
echo 'IMAServer stash file is '${WMQ_USER_PATH}/${IMAUSER}'.sth'
 
