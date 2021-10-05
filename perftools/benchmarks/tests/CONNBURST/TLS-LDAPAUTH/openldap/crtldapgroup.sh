#!/bin/bash
#--------------------------------------------------------------------------
# This script will create a group of users in LDAP at the base DN passed
# to the script.  This script assumes the following scheme in LDAP:
# BASEDN
#   ou=groups
#   ou=users
#
# The group will be created in ou=groups and the users in ou=users.  Users
# will be added as members of the group.  See usage function for more help.
#
# NOTE: only tested on CentOS/RHEL 6
# History
#
# 07/31/15 - Initial script creation
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Functions
#--------------------------------------------------------------------------
# Print usage for this script
function usage
{
  echo "usage: $0 -D <binddn> -w <bindpw> -b <basedn> --groupname <name> [--nummembers <# of members>] [--memberprefix <prefix>] [--dbnum <ldap db number>] [--ldapdb <directory>] [--help]"
  echo " --dbnum the ldab database number used by slapadd and slapindex commands (default: 2)"
  echo " --memberprefix is the prefix to which a counter (based on --nummembers) will be appended (default: \"c\")"
  echo " --nummembers is the number of ldap user entries to create and become members of the group specified by --groupname (default: 100)"
  echo " --ldapdb is the directory location of the LDAP database (default: /disk1/openldap-data)"
  echo " -b <basedn> , this script assumes ou=groups and ou=users exist under the basedn specified"
  echo "  e.g. $0 -D \"cn=manager,o=imadev,dc=ibm,dc=com\" -b \"ou=svtpvt,o=imadev,dc=ibm,dc=com\" -w secret --groupname cars --nummembers 1000000 --memberprefix car"
  echo "  e.g. $0 -D \"cn=manager,o=imadev,dc=ibm,dc=com\" -b \"ou=svtpvt,o=imadev,dc=ibm,dc=com\" -w secret --groupname homes --nummembers 1000000 --memberprefix home --dbnum 2 --ldapdb /disk1/openldap-data"
  echo "  e.g. $0 --help"
  echo " "
}

# Parse the arguments passed to this script
function parseargs {
  OPTSPEC=":h?D:w:b:-:"
  while getopts ${OPTSPEC} opt; do
    case ${opt} in
      -)
         case "${OPTARG}" in
           groupname)
             GROUP="${!OPTIND}" ; OPTIND=$((OPTIND + 1))         ;;
           nummembers)
             NUMMEMBERS="${!OPTIND}" ; OPTIND=$((OPTIND + 1))    ;;
           memberprefix)
             MEMBERPREFIX="${!OPTIND}" ; OPTIND=$((OPTIND + 1))  ;;
           dbnum)
             LDAPDBNUM="${!OPTIND}" ; OPTIND=$((OPTIND + 1)) ;;
           ldapdb)
             LDAPDB="${!OPTIND}" ; OPTIND=$((OPTIND + 1)) ;;
           help)
             usage ; exit 1 ;;
           *)
             if [ "$OPTERR" = 1 ] && [ "${optspec:0:1}" != ":" ]; then
               echo "Unknown option --${OPTARG}" >&2
             fi
             usage ; exit 1 ;;
         esac ;;
      D)
         BINDDN=${OPTARG} ;;
      b)
         BASEDN=${OPTARG} ;;
      w)
         BINDPW=${OPTARG} ;;
      h)
         usage ; exit 1   ;;
      *)
         if [ "${OPTERR}" != 1 ] || [ "${OPTSPEC:0:1}" = ":" ]; then
           echo "Non-option argument: '-${OPTARG}'" >&2
         fi
         usage ; exit 1   ;;
    esac
  done
}

# Create members in LDAP (python subscript for improved performance)
function createmembers {
  echo "creating members ..."
  rm -f $TMPLDIF

  # Python subscript to create ldif file
python - <<END
#!/usr/bin/python

import sys
import subprocess
import os

def writeldif(file,index):
  file.write("DN: uid=${MEMBERPREFIX}" + format(index,'$PAD') + ",ou=users,${BASEDN}\n")
  file.write("objectClass: inetorgperson\n")
  file.write("objectClass: organizationalPerson\n")
  file.write("objectClass: person\n")
  file.write("objectClass: top\n")
  file.write("cn: user\n")
  file.write("description: User ${MEMBERPREFIX}" + format(index,'$PAD') + " of group $GROUP\n")
  file.write("sn: " + format(index,'$PAD') + "\n")
  file.write("uid: ${MEMBERPREFIX}" + format(index,'$PAD') + "\n")
  file.write("userPassword: svtpvtpass\n")
  file.write("employeeNumber: " + format(index,'$PAD') + "\n\n")

ldif=open("$TMPLDIF",'w')
for i in range($NUMMEMBERS):
  writeldif(ldif,i)

ldif.close()

END

  slapadd -F /etc/openldap/slapd.d -l ${TMPLDIF} -n ${LDAPDBNUM}
}

# Create group and add members in LDAP
function addmembers {
  echo "creating group and adding members ..."
  rm -f $TMPLDIF

  # Python subscript to create ldif file
python - <<END
#!/usr/bin/python

import sys
import subprocess
import os

# Create a group of names in LDAP
def creategroup(file):
  file.write("DN: cn=${GROUP},ou=groups,${BASEDN}\n")
  file.write("objectClass: groupOfNames\n")
  file.write("objectClass: top\n")
  file.write("cn: ${GROUP}\n")
  file.write("description: LDAP group of names ${GROUP}\n")

ldif=open("$TMPLDIF",'w')
creategroup(ldif)

for i in range($NUMMEMBERS):
  ldif.write("member: uid=${MEMBERPREFIX}" + format(i,'$PAD') + ",ou=users,${BASEDN}\n")

ldif.close()

END

  slapadd -F /etc/openldap/slapd.d -l ${TMPLDIF} -n ${LDAPDBNUM}
}

##########
## Main ##
##########
LOGFILE=${0%%.sh}.log
ERRORLOG=${0%%.sh}.err.log
LDAPDB="/disk1/openldap-data"
NUMMEMBERS=100
LDAPDBNUM=2
MEMBERPREFIX="c"
PAD="08d"
TMPLDIF=tmp.ldif

rm -f $LOGFILE $ERRORLOG

parseargs $@

# Check required parameters are passed to the script
if [ "$BINDDN" == "" -o "$BINDPW" == "" -o "$BASEDN" == "" -o "$GROUP" == "" -o "$LDAPDB" == "" -o ! e $LDAPDB ] ; then
  echo "-D, -w ,-b, and --groupname are all required parameters and $LDAPDB directory must exist"
  usage
  exit 1
fi

echo "Creating LDAP group ${GROUP} with ${NUMMEMBERS} members (member prefix: ${MEMBERPREFIX}) in ou=groups and ou=users @ basedn ${BASEDN}"

echo "Stopping slapd ..."
service slapd stop

createmembers           2>&1 | tee $LOGFILE
addmembers              2>&1 | tee -a $LOGFILE

# Flush BDB to disk and checkpoint the transaction logs (after which it is safe to delete existing transaction logs)
db_checkpoint -1 -h ${LDAPDB}

# Reindex the database
echo "indexing the database ..."
slapindex -n ${LDAPDBNUM}

# Chown the database files to ldap user and group
chown -R ldap:ldap ${LDAPDB}

echo "Starting slapd"
service slapd start

ldapsearch -x -w $BINDPW -D $BINDDN -b "cn=${GROUP},ou=groups,${BASEDN}" | tail -n 10 ; rc=$?
if [ "$rc" != "0" -a "$rc" != "4" ] ; then
  echo "make sure URI in /etc/openldap/ldap.conf is configured to point to an LDAP server is up and running (rc=$rc)"
  exit 1
fi
