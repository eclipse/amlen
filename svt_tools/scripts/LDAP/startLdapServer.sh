#! /bin/sh

# --------------------------------------------------------------------------
# Parse parameters passed in.
# --------------------------------------------------------------------------

LDAP_URI=$1
if [ "$LDAP_URI" == "" ] ; then
        LDAP_URI="ldapi://%10.10.10.10:389"
fi

BDB_LIBPATH=/usr
LDAP_HOME=/usr/
LDAP_LIBPATH=$LDAP_HOME/lib64

SLAPD_CONF=/etc/openldap
DB_HOME="/home/var/lib/ldap"

###################################Start the LDAP server ######################################
echo "Stopping LDAP Server..."
sudo pkill -9 slapd
sleep 10

echo "Reindexing the LDAP database. This can take a couple minutes..."
sudo LD_LIBRARY_PATH=$BDB_LIBPATH:$LDAP_LIBPATH $LDAP_HOME/sbin/slapindex -f ${SLAPD_CONF}/slapd.conf  ; rc=$?
if [ "$?" != "0" ] ; then
  echo "Failed to re-index the LDAP database"
  exit
fi
sleep 10

# copy DB_CONFIG from perftools to DB_HOME
sudo cp ${SLAPD_CONF}/DB_CONFIG ${DB_HOME}/
echo "Starting LDAP Server..."
#sudo LD_LIBRARY_PATH=$BDB_LIBPATH:$LDAP_LIBPATH taskset -c 0-3,6-11 $LDAP_HOME/sbin/slapd -h $LDAP_URI -g root -f ${SLAPD_CONF}/slapd.conf
sudo LD_LIBRARY_PATH=$BDB_LIBPATH:$LDAP_LIBPATH taskset -c 0-3,6-11 $LDAP_HOME/sbin/slapd              -g root -f ${SLAPD_CONF}/slapd.conf
sleep 5

