#!/bin/bash
#------------------------------------------------------------------------------
# ldap-config.sh
#
#
# Usage: ./ldap-config.sh -a <action> [-l <ldif_file>] [-f <log>]
#        ./ldap-config.sh -a update <origgroup> <newgroup> <user>
#    setup - configure the ldap server
#    cleanup - delete the configured ldap entries
#    start - launch the slapd process
#    stop - kill the slapd process
#    search - list objects
#    update - change a users group membership
#             <origgroup> <newgroup> and <user> apply only to this action.
#------------------------------------------------------------------------------

if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
    source  ../scripts/ISMsetup.sh
else
    echo "neither testEnv.sh or ISMsetup.sh was used"
fi

while getopts "f:l:a:" option ${OPTIONS}
  do
    case ${option} in
    f )
        LOG=${OPTARG}
        ;;
    l ) 
        LDIF_FILE=${OPTARG}
        ;;
    a ) 
        ACTION=${OPTARG}
        ;;

    esac	
  done

if [[ "${LOG}" == "" ]] ; then
    LOG=ldap-config.log
    echo "No log file specified. Using ldap-config.log" | tee -a ${LOG}
fi

echo "Entering ldap-config.sh with $# arguments: $@" | tee -a ${LOG}

RC=0
case ${ACTION} in
'setup' )
    if [[ -f "$LDIF_FILE" ]] ; then
       `ldapadd -x -c -f $LDIF_FILE -D "cn=Manager,o=IBM" -w secret >> ${LOG} 2>&1`
       RC=$?
       sleep 2
    else
       echo "Failure! Input file $LDIF_FILE does not exist" | tee -a ${LOG}
       exit 1       
    fi
    ;;
    
'cleanup' )
    if [[ -f "$LDIF_FILE" ]] ; then
       `ldapdelete -x -c -f $LDIF_FILE -D "cn=Manager,o=IBM" -w secret >> ${LOG} 2>&1`
       RC=$?
       sleep 2
    else
       echo "Failure! Input file $LDIF_FILE does not exist" | tee -a ${LOG}
       exit 1       
    fi
;;

'start' )
    # When M1 runs on CentOS 7, slapd won't start if cert files do not exist; task 229554
    echo "05" > imaCA-crt.srl
    echo "02" > rootCA-crt.srl
    cp ../common_src/imaCA*.pem ./
    cp ../common_src/rootCA-*.pem ./

    ######### NOTE: The LDAP certificate is created here because the CN must contain the ldap server host #########
    # Generate keyfile for the ldap server
    openssl genrsa -out ldapserver.key 4096
    # Create certificate request for ldap server
    openssl req -new -subj "/CN=${M1_IPv4_1}" -key ldapserver.key -out ldapserver.csr
    # Create the certificate for ldap server signed by imaCA
    echo "create ldapserver-crt.pem"
    openssl x509 -req -days 7300 -in ldapserver.csr -CA imaCA-crt.pem -CAkey imaCA-key.pem -out ldapserver-crt.pem

    # Copy imaCA, rootCA, jms-ldaps to /etc/pki/CA/certs
    cp ../common_src/imaCA-crt.pem /etc/pki/CA/certs/imaCA.crt
    cp ../common_src/imaCA-key.pem /etc/pki/CA/certs/imaCA.key
    cp ../common_src/rootCA-crt.pem /etc/pki/CA/certs/rootCA.crt
    cp ../common_src/rootCA-key.pem /etc/pki/CA/certs/rootCA.key
    cp ldapserver-crt.pem /etc/pki/CA/certs/ldapserver.crt
    cp ldapserver.key /etc/pki/CA/certs/ldapserver.key

    # Create symlinks for imaCA, rootCA
    echo "create symlinks"
    ln -s -f /etc/pki/CA/certs/rootCA.crt /etc/pki/CA/certs/`openssl x509 -hash -noout -in /etc/pki/CA/certs/rootCA.crt`.0
    ln -s -f /etc/pki/CA/certs/imaCA.crt /etc/pki/CA/certs/`openssl x509 -hash -noout -in /etc/pki/CA/certs/imaCA.crt`.0

    # Start slapd process
    slapd -h "ldap:///" -f ${M1_TESTROOT}/jms_td_tests/ldap/external.ldap.slapd.conf -F /etc/openldap/slapd.d 

    sleep 2
    pid=`ps -ef | grep -v grep | grep slapd | awk '{print $2}'`
    if [[ "${pid}" != "" ]] ; then
        echo "New slapd process: ${pid}" | tee -a ${LOG}
    else
        echo "FAILURE: slapd process failed to start" | tee -a ${LOG}
    fi
;;

'stop' )
    # Kill slapd process
    pid=`ps -ef | grep -v grep | grep slapd | awk '{print $2}'`
    if [[ "${pid}" != "" ]] ; then
        echo "Killing slapd process: ${pid}" | tee -a ${LOG}
        kill -9 ${pid}
    else
        echo "FAILURE: slapd process not running" | tee -a ${LOG}
    fi
;;

'update' )
    # Copy the update template
    cp ../jms_td_tests/ldap/update.template update.ldif

    # Replace the variables
    sed -i -e s/ORIGGROUP/$2/g update.ldif
    sed -i -e s/NEWGROUP/$3/g update.ldif
    sed -i -e s/USER/$4/g update.ldif

    # Run the command
    ldapmodify -x -f ./update.ldif -D "cn=Manager,o=IBM" -w secret
    sleep 2
;;

'search' )
    `ldapsearch -xLLL -b "o=ibm" >> ${LOG} 2>&1`
    RC=0
    sleep 2
;;

* )
    echo "Failure! Invalid action specified" | tee -a ${LOG}
    exit 1
;;
esac

if [ $RC -ne 0 ] ; then
   echo "Test result is Failure!" | tee -a ${LOG}
else
   echo "Test result is Success!" | tee -a ${LOG}
fi

exit 0
