#!/bin/bash

if [[ "$1" == "setup" ]] ; then
  ldapadd -x -f ./ldap/ha.ldap.setup.ldif -D "cn=Manager,o=IBM" -w secret
elif [[ "$1" == "cleanup" ]] ; then
  ldapdelete -x -f ./ldap/ha.ldap.cleanup.ldif -D "cn=Manager,o=IBM" -w secret
fi

sleep 5
