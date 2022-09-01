#!/bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

PKCS12=p12
PFX=pfx
PEM=pem
DEFAULT_OVERWRITE=FALSE

CERTDIR=${IMA_SVR_DATA_PATH}/data/certificates
TRUSTSTORE=${IMA_SVR_DATA_PATH}/data/certificates/truststore
KEYSTORE=${IMA_SVR_DATA_PATH}/data/certificates/keystore
USERFILESDIR=${IMA_SVR_DATA_PATH}/userfiles


# Create dirs
mkdir -p -m 770 ${USERFILESDIR} > /dev/null 2>&1
mkdir -p -m 700 ${TRUSTSTORE} > /dev/null 2>&1
mkdir -p -m 700 ${KEYSTORE} > /dev/null 2>&1

if [[ $# -lt 3 || $# -gt 7 ]] ; then
    #echo "Invalid arguments. To get help execute 'imaserver apply Certificate help'."
    exit 1
fi

action=$1
type=$2
OVERWRITE=$3

if [[ $action = "apply" ]] ; then
 if [[ $type = "MQ" ]] ; then
   CERTNAME=$4
   KEYNAME=$5

   if [[ ! -f "${USERFILESDIR}/$CERTNAME" ]] ; then
    #echo "Cannot find $CERTNAME."
    exit 2
   fi
   if [[ ! -f "${USERFILESDIR}/$KEYNAME" ]] ; then
        #echo "Cannot find $KEYNAME."
        exit 3
   fi

   #check if MQC dir exist
   if [[ ! -d ${IMA_SVR_INSTALL_PATH}/certificates/MQC ]] ; then
       /bin/mkdir -p -m 700 ${IMA_SVR_INSTALL_PATH}/certificates/MQC > /dev/null 2>&1
       error=$?
       if [[ $error -ne 0 ]] ; then
          exit 5
       fi
   fi

   if [[ -f ${IMA_SVR_INSTALL_PATH}/certificates/MQC/mqconnectivity.kdb ]] &&
      [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 4;
   fi
   /bin/cp "${USERFILESDIR}/$CERTNAME" ${IMA_SVR_INSTALL_PATH}/certificates/MQC/mqconnectivity.kdb > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$CERTNAME can not be stored in the keystore."
       exit 5
   fi
   rm -rf "${USERFILESDIR}/$CERTNAME"

   /bin/cp "${USERFILESDIR}/$KEYNAME" ${IMA_SVR_INSTALL_PATH}/certificates/MQC/mqconnectivity.sth > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$KEYNAME can not be stored in the keystore."
       exit 6
   fi
   rm -rf "${USERFILESDIR}/$KEYNAME"
   exit 0
 fi

 if [[ $type = "TRUSTED" ]] ; then
   PROFILENAME=$4
   CANAME=$5

   if [[ ! -f ${USERFILESDIR}/$CANAME ]] ; then
      #echo "Cannot find $CANAME."
      exit 2
   fi

   #CERTSUFFIX=$(echo ${USERFILESDIR}/$CANAME | /usr/bin/awk -F'.' '{print $NF}')
   #if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PEM | /usr/bin/tr [:upper:] [:lower:]` ] ; then
   #    exit 15;
   #fi

   /usr/bin/openssl x509 -in "${USERFILESDIR}/$CANAME" -text -noout > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 14
   fi


   if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       ACTION="add"
   fi
   if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo "TRUE" | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       ACTION="overwrite"
   fi

   #check if it is a CA certificate
   /bin/imacli en_US validate CACert ${USERFILESDIR}/$CANAME
   error=$?
   if [[ $error -lt 1 ]] ; then
       exit 17
   fi

   export LC_CTYPE=en_US.UTF-8
   export LC_ALL=en_US.UTF-8
   ${IMA_SVR_INSTALL_PATH}/bin/ima_rehash $ACTION $PROFILENAME "$CANAME"
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 12
   fi

   rm -rf "${USERFILESDIR}/$CANAME"
   exit 0
 fi

 if [[ $type = "LDAP" ]] ; then
   CERTNAME=$4

   if [[ ! -f "${USERFILESDIR}/$CERTNAME" ]] ; then
     #echo "Cannot find $CERTNAME."
     exit 2
   fi

   #CERTSUFFIX=$(echo "${USERFILESDIR}/$CERTNAME" | /usr/bin/awk -F'.' '{print $NF}')
   #if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PEM | /usr/bin/tr [:upper:] [:lower:]` ] ; then
   #    exit 15;
   #fi

   #check if LDAP dir exist
   if [[ ! -d ${IMA_SVR_INSTALL_PATH}/certificates/LDAP ]] ; then
       /bin/mkdir -p -m 700 ${IMA_SVR_INSTALL_PATH}/certificates/LDAP > /dev/null 2>&1
       error=$?
       if [[ $error -ne 0 ]] ; then
          exit 5
       fi
   fi

   if [[ -f ${IMA_SVR_INSTALL_PATH}/certificates/LDAP/ldap.pem ]] &&
      [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 4;
   fi

   #check if it is a CA certificate
   /bin/imacli en_US validate CACert "${USERFILESDIR}/$CERTNAME"
   error=$?
   if [[ $error -lt 1 ]] ; then
       exit 17
   fi

   /usr/bin/openssl x509 -in "${USERFILESDIR}/$CERTNAME" -text -noout > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 14
   fi

   /bin/cp "${USERFILESDIR}/$CERTNAME" ${IMA_SVR_INSTALL_PATH}/certificates/LDAP/ldap.pem > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$CERTNAME can not be stored in the keystore."
       exit 5
   fi
   rm -rf "${USERFILESDIR}/$CERTNAME"

   exit 0
 fi

 if [[ $type = "Server" ]] ; then
   CERTNAME=$4
   KEYNAME=$5
   keypwdarg=$6
   certpwdarg=$7

   if [[ $keypwdarg != "KeyFilePassword=" ]] ; then
      #echo "$keypwdarg"
      KEYPWD=$(echo $keypwdarg | /usr/bin/awk -F'KeyFilePassword=' '{print $NF}')
   fi

   if [[ $certpwdarg != "CertFilePassword=" ]] ; then
      #echo "$certpwdarg"
      CERTPWD=$(echo $certpwdarg | /usr/bin/awk -F'CertFilePassword=' '{print $NF}')
   fi

   #echo "keypwd=$KEYPWD"
   #echo "certpwd=$CERTPWD"
   if [[ ! -f "${USERFILESDIR}/$CERTNAME" ]] ; then
     #echo "Cannot find $CERTNAME."
     exit 2
   fi
   if [[ ! -f "${USERFILESDIR}/$KEYNAME" ]] ; then
      #echo "Cannot find $KEYNAME."
      exit 3
   fi

   if [[ $CERTNAME = $KEYNAME ]] ; then
       isKeyCertSame="true"
       #echo "Key and Cert are in the same file"
   fi

   KEYSUFFIX=$(echo "${USERFILESDIR}/$KEYNAME" | /usr/bin/awk -F'.' '{print $NF}')
   if [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] ||
      [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 16
   fi

   CERTSUFFIX=$(echo "${USERFILESDIR}/$CERTNAME" | /usr/bin/awk -F'.' '{print $NF}')
   if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] ||
      [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 16
   fi

   /usr/bin/openssl x509 -in "${USERFILESDIR}/$CERTNAME" -text -noout > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 14
   fi

   if [[ ! -z "$KEYPWD" ]] ; then
       if [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] &&
          [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
          if [[ $isKeyCertSame != "true" ]] ; then
               /usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -out "${USERFILESDIR}/$KEYNAME" -passin pass:"$KEYPWD" >/dev/null 2>&1
               error=$?
               if [[ $error -ne 0 ]] ; then
                   exit 11
               fi
           else
              /usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -out "${USERFILESDIR}/$KEYNAME.keypart" -passin pass:"$KEYPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                  exit 11
              fi
              /usr/bin/openssl x509 -in "${USERFILESDIR}/$CERTNAME" -text -out "${USERFILESDIR}/$CERTNAME" > /dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                  exit 14
              fi
              cat "${USERFILESDIR}/$KEYNAME.keypart" >> "${USERFILESDIR}/$CERTNAME"
              rm -rf "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
           fi
       else
           /usr/bin/openssl pkcs12 -in "${USERFILESDIR}/$KEYNAME" -nodes -out "${USERFILESDIR}/$KEYNAME.parsed.pem" -passin pass:"$KEYPWD" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               rm -rf "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
               exit 11
           fi
           /usr/bin/openssl pkcs12 -export -in "${USERFILESDIR}/$KEYNAME.parsed.pem" -out "${USERFILESDIR}/$KEYNAME" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               rm -rf "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
               exit 11
           fi
       fi
       CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -passin pass: -check -noout)
       if [[ $CHECK != "RSA key ok" ]] ; then
           #echo "$KEYNAME is either expired or bad."
           rm -rf "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
           exit 8
       fi
   else
       CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -passin pass: -check -noout)
       if [[ $CHECK != "RSA key ok" ]] ; then
           #echo "$KEYNAME has a password. need keypwd=value."
           exit 9
       fi
   fi
   if [[ ! -z "$CERTPWD" ]] ; then
       #CERTPWD=$7
       if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] &&
          [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
          if [[ $isKeyCertSame != "true" ]] ; then
              /usr/bin/openssl rsa -in "${USERFILESDIR}/$CERTNAME" -out "${USERFILESDIR}/$CERTNAME" -passin pass:"$CERTPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  #echo "CERTCHECK=$CERTCHECK failed."
                  exit 10
              fi
          else
              /usr/bin/openssl rsa -in "${USERFILESDIR}/$CERTNAME" -out "${USERFILESDIR}/$CERTNAME.keypart" -passin pass:"$CERTPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "${USERFILESDIR}/$CERTNAME.keypart" > /dev/null 2>&1
                  exit 11
              fi
              /usr/bin/openssl x509 -in "${USERFILESDIR}/$CERTNAME" -text -out "${USERFILESDIR}/$CERTNAME" > /dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "${USERFILESDIR}/$CERTNAME.keypart" > /dev/null 2>&1
                  exit 14
              fi
              cat "${USERFILESDIR}/$CERTNAME.keypart" >> "${USERFILESDIR}/$CERTNAME"
              rm -rf "${USERFILESDIR}/$CERTNAME.keypart" > /dev/null 2>&1
          fi
       else
           /usr/bin/openssl pkcs12 -in "${USERFILESDIR}/$CERTNAME" -nodes -out "${USERFILESDIR}/$CERTNAME.parsed.cert" -passin pass:"$CERTPWD" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               exit 10
           fi
           /usr/bin/openssl pkcs12 -export -in "${USERFILESDIR}/$CERTNAME.parsed.cert" -out "${USERFILESDIR}/$CERTNAME" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               exit 10
           fi
       fi
#       CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$CERTNAME" -passin pass: -check -noout)
#       if [[ $CHECK != "RSA key ok" ]] ; then
#           #echo "$CERTNAME is either expired or bad."
#           exit 10
#       fi
#   else
#      CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$CERTNAME" -passin pass: -check -noout)
#       if [[ $CHECK != "RSA key ok" ]] ; then
#           #echo "$CERTNAME has a password. need certpwd=value."
#           exit 13
#       fi

   fi

   #check if key and cert are match
   ${IMA_SVR_INSTALL_PATH}/bin/matchkeycert.sh "${USERFILESDIR}/$CERTNAME" "${USERFILESDIR}/$KEYNAME"
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$CERTNAME and $KEYNAME do not match."
       exit 7
   fi


   #check overwrite flag
   if [[ -f "${KEYSTORE}/$CERTNAME" ]] &&
      [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 4;
   fi

   /bin/cp "${USERFILESDIR}/$CERTNAME" ${KEYSTORE}/. > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
      #echo "$CERTNAME can not be stored in the keystore."
      exit 5
   fi
   rm -rf "${USERFILESDIR}/$CERTNAME"

   if [[ $isKeyCertSame != "true" ]] ; then
      /bin/cp "${USERFILESDIR}/$KEYNAME" ${KEYSTORE}/. > /dev/null 2>&1
      error=$?
      if [[ $error -ne 0 ]] ; then
         #echo "$KEYNAME can not be stored in the keystore."
         exit 6
      fi
      rm -rf "${USERFILESDIR}/$KEYNAME"
   fi
   exit 0
 fi
 #echo "Unsuported Certificate type"
 exit 12
fi

if [[ $action = "remove" ]] ; then
 if [[ $type = "TRUSTED" ]] ; then
   PROFILENAME=$4
   CANAME=$5

   export LC_CTYPE=en_US.UTF-8
   export LC_ALL=en_US.UTF-8
   ${IMA_SVR_INSTALL_PATH}/bin/ima_rehash $OVERWRITE $PROFILENAME "$CANAME"
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 12
   fi

   if [ "${CANAME}" != "" ]
   then
       cd ${USERFILESDIR} > /dev/null 2>&1
       rm -f "$CANAME" > /dev/null 2>&1
   fi
   exit 0
 fi

 if [[ $type = "LDAP" ]] ; then
     if [ "${CERTDIR}" != "" ]
     then
         cd ${CERTDIR}/LDAP > /dev/null 2>&1
         rm -f * > /dev/null 2>&1
     fi
   exit 0
 fi
fi
echo "Unsuported action"
exit 12
