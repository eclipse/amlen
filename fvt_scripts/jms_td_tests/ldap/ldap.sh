#!/bin/bash
#------------------------------------------------------------------------------
# ldap.sh
#
# Usage: ./ldap.sh <action> [origgroup] [newgroup] [user] -f <log>
#    setup - configure the ldap server
#    cleanup - delete the configured ldap entries
#    start - launch the slapd process
#    stop - kill the slapd process
#    update - change a users group membership
#             [origgroup] [newgroup] and [user] apply only to this action.
#------------------------------------------------------------------------------

function print_usage {
	echo -e 'USAGE: ./ldap.sh -a <action> -f <log> [-o <old group> -g <new group> -u <user>]\n'
	echo -e '  action:    start|stop|setup|cleanup|update\n'
	echo -e '  log:       Log file name\n'
	echo -e '  old group: Name of original group to remove user from\n'
	echo -e '  new group: Name of new group to add user to\n'
	echo -e '  user:      Name of user to move from one group to another\n'
	exit 0
}

echo "Entering ldap.sh with $# arguments: $@" | tee -a ${LOG}

# Parse arguments
while getopts "a:f:g:h:o:u:" opt ; do
	case ${opt} in
	a )
		ACTION=${OPTARG}
		;;
	f )
		LOG=${OPTARG}
		;;
	g )
		NEWGROUP=${OPTARG}
		;;
	h )
		print_usage
		;;
	o )
		OLDGROUP=${OPTARG}
		;;
	u )
		USER=${OPTARG}
		;;
	* )
		exit 1
		;;
	esac
done

# Source environment file
if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
    source  ../scripts/ISMsetup.sh
else
    echo "neither testEnv.sh nor ISMsetup.sh was used"
fi

case ${ACTION} in
"setup" )
    ldapadd -x -f ${M1_TESTROOT}/jms_td_tests/ldap/external.ldap.ldif -D "cn=Manager,o=IBM" -w secret
    sleep 2
	;;
"cleanup" )
    ldapdelete -x -f ${M1_TESTROOT}/jms_td_tests/ldap/external.delete.ldif -D "cn=Manager,o=IBM" -w secret
    sleep 2
	;;
"start" )
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
    slapd -h "ldap:/// ldaps://:6636/" -f ${M1_TESTROOT}/jms_td_tests/ldap/external.ldap.slapd.conf -F /etc/openldap/slapd.d &

	# Give slapd a couple seconds to start
    sleep 2
    
    echo  "Get some debug info to make sure the process is running and the ports are in use."
    netstat -tulpn | grep 6636
    netstat -tulpn | grep 389
    ps -ef | grep slapd

    pid=`ps -ef | grep -v grep | grep slapd | awk '{print $2}'`
    if [[ "${pid}" != "" ]] ; then
        echo "New slapd process: ${pid}" | tee -a ${LOG}
    else
        echo "ldap.sh Test result is Failure! slapd process failed to start" | tee -a ${LOG}
        exit 0
    fi
	;;
"stop" )
    # Kill slapd process
    pid=`ps -ef | grep -v grep | grep slapd | awk '{print $2}'`
    if [[ "${pid}" != "" ]] ; then
        echo "Killing slapd process: ${pid}" | tee -a ${LOG}
        kill -9 ${pid}
        rm /etc/pki/CA/certs/ldapserver*
        rm /etc/pki/CA/certs/ima*
        rm /etc/pki/CA/certs/root*
        rm /etc/pki/CA/certs/*.0
    else
        echo "slapd process not running" | tee -a ${LOG}
    fi
	;;
"update" )
	# Copy the update template
    cp ldap/update.template update.ldif

    # Replace the variables
    sed -i -e s/ORIGGROUP/${OLDGROUP}/g update.ldif
    sed -i -e s/NEWGROUP/${NEWGROUP}/g update.ldif
    sed -i -e s/USER/${USER}/g update.ldif

    # Run the command
    ldapmodify -x -f ./update.ldif -D "cn=Manager,o=IBM" -w secret
    sleep 2
    ;;
"search" )
    # List the contents of the LDAP
    ldapsearch -xLLL -b "o=IBM"
    sleep 2
    ;;
* )
	echo "ldap.sh Test result is Failure! Invalid action specified" | tee -a ${LOG}
	exit 0
	;;
esac

echo "ldap.sh Test result is Success!" | tee -a ${LOG}
exit 0
