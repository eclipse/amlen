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


# Create dirs
mkdir -p -m 770 /tmp/userfiles > /dev/null 2>&1
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

   if [[ ! -f "/tmp/userfiles/$CERTNAME" ]] ; then
    #echo "Cannot find $CERTNAME."
    exit 2
   fi
   if [[ ! -f "/tmp/userfiles/$KEYNAME" ]] ; then
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
   /bin/cp "/tmp/userfiles/$CERTNAME" ${IMA_SVR_INSTALL_PATH}/certificates/MQC/mqconnectivity.kdb > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$CERTNAME can not be stored in the keystore."
       exit 5
   fi
   rm -rf "/tmp/userfiles/$CERTNAME"

   /bin/cp "/tmp/userfiles/$KEYNAME" ${IMA_SVR_INSTALL_PATH}/certificates/MQC/mqconnectivity.sth > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$KEYNAME can not be stored in the keystore."
       exit 6
   fi
   rm -rf "/tmp/userfiles/$KEYNAME"
   exit 0
 fi

 if [[ $type = "TRUSTED" ]] ; then
   PROFILENAME=$4
   CANAME=$5

   if [[ ! -f /tmp/userfiles/$CANAME ]] ; then
      #echo "Cannot find $CANAME."
      exit 2
   fi

   #CERTSUFFIX=$(echo /tmp/userfiles/$CANAME | /usr/bin/awk -F'.' '{print $NF}')
   #if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PEM | /usr/bin/tr [:upper:] [:lower:]` ] ; then
   #    exit 15;
   #fi

   /usr/bin/openssl x509 -in "/tmp/userfiles/$CANAME" -text -noout > /dev/null 2>&1
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
   /bin/imacli en_US validate CACert /tmp/userfiles/$CANAME
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

   rm -rf "/tmp/userfiles/$CANAME"
   exit 0
 fi

 if [[ $type = "LDAP" ]] ; then
   CERTNAME=$4

   if [[ ! -f "/tmp/userfiles/$CERTNAME" ]] ; then
     #echo "Cannot find $CERTNAME."
     exit 2
   fi

   #CERTSUFFIX=$(echo "/tmp/userfiles/$CERTNAME" | /usr/bin/awk -F'.' '{print $NF}')
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
   /bin/imacli en_US validate CACert "/tmp/userfiles/$CERTNAME"
   error=$?
   if [[ $error -lt 1 ]] ; then
       exit 17
   fi

   /usr/bin/openssl x509 -in "/tmp/userfiles/$CERTNAME" -text -noout > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 14
   fi

   /bin/cp "/tmp/userfiles/$CERTNAME" ${IMA_SVR_INSTALL_PATH}/certificates/LDAP/ldap.pem > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       #echo "$CERTNAME can not be stored in the keystore."
       exit 5
   fi
   rm -rf "/tmp/userfiles/$CERTNAME"

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
   if [[ ! -f "/tmp/userfiles/$CERTNAME" ]] ; then
     #echo "Cannot find $CERTNAME."
     exit 2
   fi
   if [[ ! -f "/tmp/userfiles/$KEYNAME" ]] ; then
      #echo "Cannot find $KEYNAME."
      exit 3
   fi

   if [[ $CERTNAME = $KEYNAME ]] ; then
       isKeyCertSame="true"
       #echo "Key and Cert are in the same file"
   fi

   KEYSUFFIX=$(echo "/tmp/userfiles/$KEYNAME" | /usr/bin/awk -F'.' '{print $NF}')
   if [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] ||
      [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 16
   fi

   CERTSUFFIX=$(echo "/tmp/userfiles/$CERTNAME" | /usr/bin/awk -F'.' '{print $NF}')
   if [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] ||
      [ `echo $CERTSUFFIX | /usr/bin/tr [:upper:] [:lower:]` =  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
       exit 16
   fi

   /usr/bin/openssl x509 -in "/tmp/userfiles/$CERTNAME" -text -noout > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
       exit 14
   fi

   if [[ ! -z "$KEYPWD" ]] ; then
       if [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PKCS12 | /usr/bin/tr [:upper:] [:lower:]` ] &&
          [ `echo $KEYSUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $PFX | /usr/bin/tr [:upper:] [:lower:]` ] ; then
          if [[ $isKeyCertSame != "true" ]] ; then
               /usr/bin/openssl rsa -in "/tmp/userfiles/$KEYNAME" -out "/tmp/userfiles/$KEYNAME" -passin pass:"$KEYPWD" >/dev/null 2>&1
               error=$?
               if [[ $error -ne 0 ]] ; then
                   exit 11
               fi
           else
              /usr/bin/openssl rsa -in "/tmp/userfiles/$KEYNAME" -out "/tmp/userfiles/$KEYNAME.keypart" -passin pass:"$KEYPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "/tmp/userfiles/$KEYNAME.keypart" > /dev/null 2>&1
                  exit 11
              fi
              /usr/bin/openssl x509 -in "/tmp/userfiles/$CERTNAME" -text -out "/tmp/userfiles/$CERTNAME" > /dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "/tmp/userfiles/$KEYNAME.keypart" > /dev/null 2>&1
                  exit 14
              fi
              cat "/tmp/userfiles/$KEYNAME.keypart" >> "/tmp/userfiles/$CERTNAME"
              rm -rf "/tmp/userfiles/$KEYNAME.keypart" > /dev/null 2>&1
           fi
       else
           /usr/bin/openssl pkcs12 -in "/tmp/userfiles/$KEYNAME" -nodes -out "/tmp/userfiles/$KEYNAME.parsed.pem" -passin pass:"$KEYPWD" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               rm -rf "/tmp/userfiles/$KEYNAME.parsed.pem"  >/dev/null 2>&1
               exit 11
           fi
           /usr/bin/openssl pkcs12 -export -in "/tmp/userfiles/$KEYNAME.parsed.pem" -out "/tmp/userfiles/$KEYNAME" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               rm -rf "/tmp/userfiles/$KEYNAME.parsed.pem"  >/dev/null 2>&1
               exit 11
           fi
       fi
       CHECK=$(/usr/bin/openssl rsa -in "/tmp/userfiles/$KEYNAME" -passin pass: -check -noout)
       if [[ $CHECK != "RSA key ok" ]] ; then
           #echo "$KEYNAME is either expired or bad."
           rm -rf "/tmp/userfiles/$KEYNAME.parsed.pem"  >/dev/null 2>&1
           exit 8
       fi
   else
       CHECK=$(/usr/bin/openssl rsa -in "/tmp/userfiles/$KEYNAME" -passin pass: -check -noout)
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
              /usr/bin/openssl rsa -in "/tmp/userfiles/$CERTNAME" -out "/tmp/userfiles/$CERTNAME" -passin pass:"$CERTPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  #echo "CERTCHECK=$CERTCHECK failed."
                  exit 10
              fi
          else
              /usr/bin/openssl rsa -in "/tmp/userfiles/$CERTNAME" -out "/tmp/userfiles/$CERTNAME.keypart" -passin pass:"$CERTPWD" >/dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "/tmp/userfiles/$CERTNAME.keypart" > /dev/null 2>&1
                  exit 11
              fi
              /usr/bin/openssl x509 -in "/tmp/userfiles/$CERTNAME" -text -out "/tmp/userfiles/$CERTNAME" > /dev/null 2>&1
              error=$?
              if [[ $error -ne 0 ]] ; then
                  rm -rf "/tmp/userfiles/$CERTNAME.keypart" > /dev/null 2>&1
                  exit 14
              fi
              cat "/tmp/userfiles/$CERTNAME.keypart" >> "/tmp/userfiles/$CERTNAME"
              rm -rf "/tmp/userfiles/$CERTNAME.keypart" > /dev/null 2>&1
          fi
       else
           /usr/bin/openssl pkcs12 -in "/tmp/userfiles/$CERTNAME" -nodes -out "/tmp/userfiles/$CERTNAME.parsed.cert" -passin pass:"$CERTPWD" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               exit 10
           fi
           /usr/bin/openssl pkcs12 -export -in "/tmp/userfiles/$CERTNAME.parsed.cert" -out "/tmp/userfiles/$CERTNAME" -passout pass:  >/dev/null 2>&1
           error=$?
           if [[ $error -ne 0 ]] ; then
               exit 10
           fi
       fi
#       CHECK=$(/usr/bin/openssl rsa -in "/tmp/userfiles/$CERTNAME" -passin pass: -check -noout)
#       if [[ $CHECK != "RSA key ok" ]] ; then
#           #echo "$CERTNAME is either expired or bad."
#           exit 10
#       fi
#   else
#      CHECK=$(/usr/bin/openssl rsa -in "/tmp/userfiles/$CERTNAME" -passin pass: -check -noout)
#       if [[ $CHECK != "RSA key ok" ]] ; then
#           #echo "$CERTNAME has a password. need certpwd=value."
#           exit 13
#       fi

   fi

   #check if key and cert are match
   ${IMA_SVR_INSTALL_PATH}/bin/matchkeycert.sh "/tmp/userfiles/$CERTNAME" "/tmp/userfiles/$KEYNAME"
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

   /bin/cp "/tmp/userfiles/$CERTNAME" ${KEYSTORE}/. > /dev/null 2>&1
   error=$?
   if [[ $error -ne 0 ]] ; then
      #echo "$CERTNAME can not be stored in the keystore."
      exit 5
   fi
   rm -rf "/tmp/userfiles/$CERTNAME"

   if [[ $isKeyCertSame != "true" ]] ; then
      /bin/cp "/tmp/userfiles/$KEYNAME" ${KEYSTORE}/. > /dev/null 2>&1
      error=$?
      if [[ $error -ne 0 ]] ; then
         #echo "$KEYNAME can not be stored in the keystore."
         exit 6
      fi
      rm -rf "/tmp/userfiles/$KEYNAME"
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
       cd /tmp/userfiles > /dev/null 2>&1
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
