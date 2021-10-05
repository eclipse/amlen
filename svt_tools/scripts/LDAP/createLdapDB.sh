#! /bin/sh

UIDPREFIX="u"
HOST=`hostname`
AddDir=0
AddUsr=0
AddMem=0
copy2gsacache=1
USR=""
LDAP_HOME=/usr
LDAP_LIBPATH=$LDAP_HOME/lib64
BDB_HOME=/usr
BDB_LIBPATH=$BDB_HOME/lib64

BASE_DN="ou=SVT,O=IBM,c=US"

SLAPD_CONF=/etc/openldap
DB_HOME="/home/var/lib/ldap"
DN_NAME="cn=root,O=IBM,C=US"
DN_PSWD="ima4test"
LDAP_URI="10.10.10.10"
LDAP_PORT="389"
topFile="/niagara/test/scripts/LDAP/SVT_TOP_OBJECTS.mar032.ldif"

# -------------------------------------------------------------------------
# Function clean_logs
# -------------------------------------------------------------------------
function clean_logs
{
        cd  ${DB_HOME}/
        # find the last log file and remove the older log files
        list=`ls | grep log.0`
        for file in $list
        do
                tmp=${file}
        done
        sudo mv ${tmp} log1
        sudo rm log.000*
        sudo mv log1 ${tmp}
        cd ${SLAPD_CONF}
}

# -------------------------------------------------------------------------
# Function copy2gsacache
# -------------------------------------------------------------------------
function copy2gsacache
{
	# find the last log file and remove the other log files 
	clean_logs
	
	cd  ${DB_HOME}/..
	
	# remove all __db.* files
	#sudo rm ${DB_HOME}/__db.*
	
	# remove the alock file
	sudo rm ${DB_HOME}/alock
	
	# compress DB_HOME directory and copy it to gsacache
	sudo tar -zcvf ldapDB.tar.gz ${DB_HOME}
	sudo scp ldapDB.tar.gz $USR:~/openldap/
	echo "The ldapDB.tar.gz file has been copied to $USR:~/openldap/, please move it to gsacache on mar145."
	
	echo -n "Do you want to remove all files in ${DB_HOME} directory? [y/N]: "
	read rmFiles
	if [ "$rmFiles" == "y" -o "$rmFiles" == "Y" ] ; then
		sudo rm ${DB_HOME}/*
	fi
	
	cd ${SLAPD_CONF}
}

# --------------------------------------------------------------------------
# Parse parameters passed in.
# --------------------------------------------------------------------------
NUM_ARGS=$#

while [ $# -gt 0 ]
do
	case "$1" in
		-d)               # Create new Directory
			shift
			topFile=$1
			AddDir=1
			;;
		-u)               # Create Users
			shift
			newUsers=$1
			AddUsr=1
			;;
		-m)                # Add members
			shift
			newMembers=$1
			AddMem=1
			;;
		--help)
			echo "-d <ldif file name> --- Add Directory"
			echo "-u <startUser>-<endUser> --- Add Users"
			echo "-m <startMember>-<endMember> --- Add Members into group"
			;;
		-nocopy)
			copy2gsacache=0
			;;
		*)
			echo "Passing unknown option ($1)"
			exit
			;;
	esac
	
	shift
done


###################################Start the LDAP server ######################################
./startLdapServer.sh ${LDAP_URI}

#####################################Create Directories##############################################
if [ ${AddDir} == 1 ]; then
	echo "Start creating directories..."
	$LDAP_HOME/bin/ldapmodify -x -D ${DN_NAME} -w ${DN_PSWD} -h ${LDAP_URI} -p ${LDAP_PORT} -f ${topFile}
	sleep 5
fi

#####################################Create Users##############################################
if [ ${AddUsr} == 1 ]; then
	argv=$(echo $newUsers | tr "-" "\n")
	i=0
	for item in ${argv}
	do 
		if [ ${i} == 0 ]; then
			startUser=$item
		else
			endUser=$item
		fi
		i=$((i+1))
	done
	NO_USERS=$((endUser-startUser+1))
	
	echo "Start creating Users..."
	#set -x
	firstloop=0
	
	for (( user=startUser ; user <= endUser ; user++ ))
	do
	   # create user LDIF file
	   USER=`printf "%07d" $user`
	   MEMBER="DN: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"
	
	   echo "version: 1"                             > uid.${UIDPREFIX}.ldif
	   echo ${MEMBER}                                >> uid.${UIDPREFIX}.ldif
	   echo "changetype: add"                        >> uid.${UIDPREFIX}.ldif
	   echo "objectClass: inetorgperson"             >> uid.${UIDPREFIX}.ldif
	   echo "objectClass: organizationalPerson"      >> uid.${UIDPREFIX}.ldif
	   echo "objectClass: person"                    >> uid.${UIDPREFIX}.ldif
	   echo "objectClass: top"                       >> uid.${UIDPREFIX}.ldif
	   echo "cn: user"                               >> uid.${UIDPREFIX}.ldif
	   echo "description: User UIDs in the Internet" >> uid.${UIDPREFIX}.ldif
	   echo "sn: "${USER}                            >> uid.${UIDPREFIX}.ldif
	   echo "uid: "${UIDPREFIX}${USER}               >> uid.${UIDPREFIX}.ldif
	   echo "userPassword: imaperftest"              >> uid.${UIDPREFIX}.ldif
	   #echo "employeeNumber: ${user}"                >> uid.${UIDPREFIX}.ldif
	
	   echo "Creating UID for: " ${MEMBER}
	
	   #/usr/bin/ldapmodify -x -D ${DN_NAME} -w ${DN_PSWD}  -h ${LDAP_URI} -p 389  -f uid.${UIDPREFIX}.ldif
	   $LDAP_HOME/bin/ldapmodify -x -D ${DN_NAME} -w ${DN_PSWD} -h ${LDAP_URI} -p ${LDAP_PORT} -f uid.${UIDPREFIX}.ldif
	
	done
	
	echo "%===> Completed creating ${NO_USERS} Application UIDs"
	cat  uid.${UIDPREFIX}.ldif
	
	sleep 3

fi

#######################################Create Group Members###########################################
if [ ${AddMem} == 1 ]; then
	echo "Start adding Members into group..."
	GROUPDN="dn: cn=perfUsersInternet,ou=groups,${BASE_DN}"
	firstloop=0
	
	argv=$(echo $newMembers | tr "-" "\n")
	i=0
	for item in ${argv}
	do 
		if [ ${i} == 0 ]; then
			startMember=$item
		else
			endMember=$item
		fi
		i=$((i+1))
	done
	NO_MEMBERS=$((endMember-startMember+1))
	
	num_mem_added=0
	for (( user=startMember ; user <= endMember ; user++ ))
	do
		# create member LDIF
		echo ${GROUPDN}           >member.${UIDPREFIX}.ldif
		echo "changetype: modify" >>member.${UIDPREFIX}.ldif
		echo "add: member"        >>member.${UIDPREFIX}.ldif
	
	    USER=`printf "%07d" $user`
	    MEMBER="member: uid=${UIDPREFIX}${USER},ou=users,${BASE_DN}"
	    echo ${MEMBER}            >>member.${UIDPREFIX}.ldif
	
	    echo "Adding Member:  ${MEMBER} to group: ${GROUPDN}"

	    num_mem_added=$((num_mem_added+1))
	    
	    $LDAP_HOME/bin/ldapmodify -x -D ${DN_NAME} -w ${DN_PSWD} -h ${LDAP_URI} -p ${LDAP_PORT}   -f member.${UIDPREFIX}.ldif
	    
	    if [ $num_mem_added == 100 ] ; then
	   	    sudo LD_LIBRARY_PATH=$BDB_LIBPATH $BDB_HOME/bin/db_checkpoint -1 -h ${DB_HOME}
	   	    clean_logs
	   	    num_mem_added=0
	    fi
	done
	
	echo "%===> Completed adding ${NO_MEMBERS} MEMBER UIDs to group:  ${GROUPDN}"
	cat  member.${UIDPREFIX}.ldif
	
	sleep 3
fi

# flush memory
echo "Flushing memory..."
sudo LD_LIBRARY_PATH=$BDB_LIBPATH $BDB_HOME/bin/db_checkpoint -1 -h ${DB_HOME}

# reindex the database
echo "stopping slapd and indexing the bdb database..."
sudo pkill -9 slapd
sudo LD_LIBRARY_PATH=$BDB_LIBPATH:$LDAP_LIBPATH $LDAP_HOME/sbin/slapindex -f ${SLAPD_CONF}/slapd.conf 

# Copy the Users to GSACache
if [ ${copy2gsacache} == 1 ]; then
	echo "Copying data to user@mar145:~/openldap..."
	copy2gsacache
fi

