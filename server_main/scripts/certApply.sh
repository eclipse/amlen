#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
# PEM is not used, so commenting it out
#PEM=pem
DEFAULT_OVERWRITE=FALSE

CERTDIR=${IMA_SVR_DATA_PATH}/data/certificates
export CERTDIR

USERFILESDIR=${IMA_SVR_DATA_PATH}/userfiles

mkdir -p -m 770 ${USERFILESDIR} > /dev/null 2>&1 3>&1

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
        if [[ ! -d ${CERTDIR}/MQC ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/MQC > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 5
            fi
        fi

        if [[ -f ${CERTDIR}/MQC/mqconnectivity.kdb ]] &&
            [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            exit 4;
        fi

        if [ -f ${USERFILESDIR}/${CERTNAME} ]
        then
            /bin/cp "${USERFILESDIR}/$CERTNAME" ${CERTDIR}/MQC/mqconnectivity.kdb > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                #echo "$CERTNAME can not be stored in the keystore."
                exit 5
            fi
            rm -f "${USERFILESDIR}/$CERTNAME"
        fi

        if [ -f ${USERFILESDIR}/${KEYNAME} ]
        then
            /bin/cp "${USERFILESDIR}/$KEYNAME" ${CERTDIR}/MQC/mqconnectivity.sth > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                #echo "$KEYNAME can not be stored in the keystore."
                exit 6
            fi
            rm -f "${USERFILESDIR}/$KEYNAME"
        fi

        exit 0
    fi

# ---------------------------------------- TRUSTED CERTS ----------------------------------

    if [[ $type = "TRUSTED" ]] ; then
        PROFILENAME=$4
        CANAME=$5

        if [[ ! -f ${USERFILESDIR}/$CANAME ]] ; then
           #echo "Cannot find $CANAME."
           exit 2
        fi

        # check for a valid cert
        /usr/bin/openssl x509 -in "${USERFILESDIR}/$CANAME" -text -noout > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 14
        fi

        #check if keystore dir exist
        if [[ ! -d ${CERTDIR}/truststore ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/truststore > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 5
            fi
        fi

        if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            ACTION="add"
        fi
        if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo "TRUE" | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            ACTION="overwrite"
        fi

        CERTFILEPATH=${CERTDIR}/truststore/${PROFILENAME}/${CANAME}
        if [ -f ${CERTFILEPATH} ]
        then
            if [ "${ACTION}" == "add" ]
            then
                exit 4
            fi
        fi

        # validation if it is a CA certificate
        /usr/bin/openssl x509 -in "${USERFILESDIR}/$CANAME" -text -noout | grep "CA:TRUE" > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
           exit 17
        fi

        export LC_CTYPE=en_US.UTF-8
        export LC_ALL=en_US.UTF-8
        ${IMA_SVR_INSTALL_PATH}/bin/cert_rehash.py $ACTION $PROFILENAME "$CANAME"
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 12
        fi

        rm -f "${USERFILESDIR}/$CANAME"

        exit 0
    fi

# ------------------------------- Client Certificate ---------------------------------------------

    if [[ $type = "CLIENT" ]] ; then
        PROFILENAME=$4
        CANAME=$5

        if [[ ! -f ${USERFILESDIR}/$CANAME ]] ; then
           #echo "Cannot find $CANAME."
           exit 2
        fi

        /usr/bin/openssl x509 -in "${USERFILESDIR}/$CANAME" -text -noout > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 14
        fi


        #check if keystore dir exist
        DIRNAME=${PROFILENAME}_allowedClientCerts
        /bin/mkdir -p -m 700 ${CERTDIR}/truststore/${DIRNAME} > /dev/null 2>&1
        if [[ ! -d ${CERTDIR}/truststore/${DIRNAME} ]] ; then
            exit 5
        fi

        if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            ACTION="add"
        fi
        if [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo "TRUE" | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            ACTION="overwrite"
        fi

        CERTFILEPATH=${CERTDIR}/truststore/${DIRNAME}/${CANAME}
        if [ -f ${CERTFILEPATH} ]
        then
            if [ "${ACTION}" == "add" ]
            then
                exit 4
            fi
        fi


        # Copy file to truststore
        cp ${USERFILESDIR}/$CANAME ${CERTDIR}/truststore/${DIRNAME}/. > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 12
        fi

        rm -f "${USERFILESDIR}/$CANAME"
        exit 0
    fi

# ---------------------------------------- CRL CERTS -----------------------------------
    if [[ $type = "REVOCATION" ]] ; then
        SECPROFILE=$4
        CRLPROFILE=$5
        CRLCERT=$6

        if [[ ! -d ${CERTDIR}/truststore/${SECPROFILE}_crl ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/truststore/${SECPROFILE}_crl
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 5
            fi
        fi

        /bin/cp ${CERTDIR}/CRL/${CRLPROFILE}/${CRLCERT} ${CERTDIR}/truststore/${SECPROFILE}_crl/crl.pem > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 12
        fi
        exit 0
    fi
# ---------------------------------------- LDAP CERTS ----------------------------------

    if [[ $type = "LDAP" ]] ; then
        CERTNAME=$4

        if [[ ! -f "${USERFILESDIR}/$CERTNAME" ]] ; then
            #echo "Cannot find $CERTNAME."
            exit 2
        fi

        # check if LDAP dir exist
        if [[ ! -d ${CERTDIR}/LDAP ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/LDAP > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 5
            fi
        fi

        if [[ -f ${CERTDIR}/LDAP/ldap.pem ]] &&
            [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            exit 4;
        fi

        /usr/bin/openssl x509 -in "${USERFILESDIR}/$CERTNAME" -text -noout > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 14
        fi

        /bin/cp "${USERFILESDIR}/$CERTNAME" ${CERTDIR}/LDAP/ldap.pem > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            #echo "$CERTNAME can not be stored in the keystore."
            exit 5
        fi
        rm -f "${USERFILESDIR}/$CERTNAME"

        exit 0
    fi

# ---------------------------------------- SERVER CERTS ----------------------------------

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

        if [[ "${CERTNAME}" = "${KEYNAME}" ]] ; then
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
                CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -passin pass: -check -noout)
                if [[ "$CHECK" = "RSA key ok" ]] ; then
                    echo "This key file does not require a password even though one was provided."
                else
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
                            rm -f "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                            exit 11
                        fi

                        #  defect 212137: we were not extracting all certificates from combined pem file
                        # extract pem file into crt file containing all the certs
                        #if awk -f ${IMA_SVR_INSTALL_PATH}/bin/extract_pems.awk "${USERFILESDIR}/${CERTNAME}" > /dev/null 2>&1; then
                        if ${IMA_SVR_INSTALL_PATH}/bin/extract-certs-from-pem.py "${USERFILESDIR}/${CERTNAME}"; then
                            cat "${USERFILESDIR}/${CERTNAME}.crt" >> "${USERFILESDIR}/${CERTNAME}.keypart"
                            /bin/cp -f "${USERFILESDIR}/$KEYNAME.keypart" "${USERFILESDIR}/${CERTNAME}"
                            perl -pi -e 's/\r\n$/\n/g' ${USERFILESDIR}/${CERTNAME}
                            #rm -f "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                        else
                            rm -f "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                            exit 14
                        fi
                    fi
                fi
            else
                /usr/bin/openssl pkcs12 -in "${USERFILESDIR}/$KEYNAME" -nodes -out "${USERFILESDIR}/$KEYNAME.parsed.pem" -passin pass:"$KEYPWD" -passout pass:  >/dev/null 2>&1
                error=$?
                if [[ $error -ne 0 ]] ; then
                    rm -f "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
                    exit 11
                fi
                /usr/bin/openssl pkcs12 -export -in "${USERFILESDIR}/$KEYNAME.parsed.pem" -out "${USERFILESDIR}/$KEYNAME" -passout pass:  >/dev/null 2>&1
                error=$?
                if [[ $error -ne 0 ]] ; then
                    rm -f "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
                    exit 11
                fi
            fi

            CHECK=$(/usr/bin/openssl rsa -in "${USERFILESDIR}/$KEYNAME" -passin pass: -check -noout)
            if [[ $CHECK != "RSA key ok" ]] ; then
                #echo "$KEYNAME is either expired or bad."
                rm -f "${USERFILESDIR}/$KEYNAME.parsed.pem"  >/dev/null 2>&1
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
                    #  defect 212137: we were not extracting all certificates from combined pem file
                    # extract pem file into crt file containing all the certs
                    #if awk -f ${IMA_SVR_INSTALL_PATH}/bin/extract_pems.awk "${USERFILESDIR}/${CERTNAME}" > /dev/null 2>&1; then
                    if ${IMA_SVR_INSTALL_PATH}/bin/extract-certs-from-pem.py "${USERFILESDIR}/${CERTNAME}"; then
                        cat "${USERFILESDIR}/${CERTNAME}.crt" >> "${USERFILESDIR}/${CERTNAME}.keypart"
                        /bin/cp -f "${USERFILESDIR}/$KEYNAME.keypart" "${USERFILESDIR}/${CERTNAME}"
                        perl -pi -e 's/\r\n$/\n/g' ${USERFILESDIR}/${CERTNAME}
                        #rm -f "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                    else
                        #rm -f "${USERFILESDIR}/$KEYNAME.keypart" > /dev/null 2>&1
                        exit 14
                    fi
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
        fi

        #check if key and cert are match
        ${IMA_SVR_INSTALL_PATH}/bin/matchkeycert.sh "${USERFILESDIR}/$CERTNAME" "${USERFILESDIR}/$KEYNAME"
        error=$?
        if [[ $error -ne 0 ]] ; then
            #echo "$CERTNAME and $KEYNAME do not match."
            exit 7
        fi

        #check if keystore dir exist
        if [[ ! -d ${CERTDIR}/keystore ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/keystore > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                #echo "$CERTNAME can not be stored in the keystore."
                exit 5
            fi
        fi

        #check overwrite flag
        if [[ -f "${CERTDIR}/keystore/$CERTNAME" ]] &&
            [ `echo $OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` = `echo $DEFAULT_OVERWRITE | /usr/bin/tr [:upper:] [:lower:]` ] ; then
            exit 4;
        fi

        /bin/cp "${USERFILESDIR}/$CERTNAME" ${CERTDIR}/keystore/. > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            #echo "$CERTNAME can not be stored in the keystore."
            exit 5
        fi
        rm -f "${USERFILESDIR}/$CERTNAME"

        if [[ $isKeyCertSame != "true" ]] ; then
            /bin/cp "${USERFILESDIR}/$KEYNAME" ${CERTDIR}/keystore/. > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                #echo "$KEYNAME can not be stored in the keystore."
                exit 6
            fi
            rm -f "${USERFILESDIR}/$KEYNAME"
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
        ${IMA_SVR_INSTALL_PATH}/bin/cert_rehash.py "delete" $PROFILENAME "$CANAME"
        error=$?
        if [[ $error -ne 0 ]] ; then
          exit 12
        fi

        if [[ -f ${USERFILESDIR}/${CANAME} ]] ; then
          rm -f ${USERFILESDIR}/${CANAME}
        fi

        exit 0
    fi

    if [[ $type = "CLIENT" ]] ; then
        PROFILENAME=$4
        CANAME=$5

        DIRNAME=${PROFILENAME}_allowedClientCerts
        if [[ -f ${CERTDIR}/truststore/${DIRNAME}/${CANAME} ]] ; then
            rm -f ${CERTDIR}/truststore/${DIRNAME}/${CANAME}
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 12
            fi
        fi

        if [[ -f "${USERFILESDIR}/${CANAME}" ]] ; then
           rm -f "${USERFILESDIR}/${CANAME}"
        fi

        exit 0
    fi

    if [[ $type = "LDAP" ]] ; then
        rm -f ${CERTDIR}/LDAP/* > /dev/null 2>&1
        exit 0
    fi

    if [[ $type = "REVOCATION" ]] ; then
        PROFILENAME=$4
        rm -f ${CERTDIR}/truststore/${PROFILENAME}_crl/*
        exit 0
    fi
elif [[ $action = "staging" ]] ; then
    # ------------------------------- Client Revocation List -----------------------------------------
    if [[ $type = "REVOCATION" ]] ; then
        CRLPROFNAME=$4
        CRLCERTNAME=$5

        fileOverwrite=0
        # For overwrite case check if we are indeed changing source or not and if its the same name
        if [[ $OVERWRITE = "true" ]] ; then
            if [[ ! -f ${USERFILESDIR}/$CRLCERTNAME ]] ; then
                if [[ ! -f ${CERTDIR}/CRL/${CRLPROFNAME}/${CRLCERTNAME} ]] ; then
                    exit 2
                else
                    exit 0
                fi
            else
                if [[ -f ${CERTDIR}/CRL/${CRLPROFNAME}/${CRLCERTNAME} ]] ; then
                    fileOverwrite=1;
                fi
            fi
        elif [[ ! -f ${USERFILESDIR}/$CRLCERTNAME ]] ; then
            #echo "Cannot find $CRLCERTNAME."
            exit 2
        fi

        /usr/bin/openssl crl -in "${USERFILESDIR}/$CRLCERTNAME" -text -noout > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 14
        fi

        # Create CRLProfile directory if needed, check for overwrite if a file exists
        if [[ ! -d ${CERTDIR}/CRL/${CRLPROFNAME} ]] ; then
            /bin/mkdir -p -m 700 ${CERTDIR}/CRL/${CRLPROFNAME} > /dev/null 2>&1
            error=$?
            if [[ $error -ne 0 ]] ; then
                exit 5
            fi
        fi

        if [[ $OVERWRITE = "true" ]] ; then
            rm -f ${CERTDIR}/CRL/${CRLPROFNAME}/* > /dev/null 2>&1
        fi

        # Copy cert file to CRL staging directory
        /bin/cp ${USERFILESDIR}/$CRLCERTNAME ${CERTDIR}/CRL/${CRLPROFNAME}/${CRLCERTNAME} > /dev/null 2>&1
        error=$?
        if [[ $error -ne 0 ]] ; then
            exit 12
        fi

        rm -f "${USERFILESDIR}/$CRLCERTNAME"

        # Return special rc for overwrites of file with SAME NAME
        if [[ $fileOverwrite -eq 1 ]] ; then
            exit 99
        fi

        exit 0
    fi
fi

echo "Unsupported action"
exit 12
