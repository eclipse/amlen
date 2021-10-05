#! /bin/bash
service slapd status
echo " "
echo "Restart LDAP, y/n"
read y
if [[ ${y} == "y"* ]]; then
	service slapd stop
	service slapd start
	service slapd status
fi
