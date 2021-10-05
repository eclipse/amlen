#!/bin/bash

source cluster.cfg

i=$1


command() {
echo curl -X POST -d '{ "LDAP":{"Enabled":true,"URL":"ldap://10.142.70.151:389","BaseDN":"ou=svtpvt,o=imadev,dc=ibm,dc=com","BindDN":"cn=manager,o=imadev,dc=ibm,dc=com","BindPassword":"secret","UserSuffix":"ou=users,ou=svtpvt,o=imadev,dc=ibm,dc=com","GroupSuffix":"ou=groups,ou=svtpvt,o=imadev,dc=ibm,dc=com","UserIdMap":"uid","GroupIdMap":"cn","GroupMemberIdMap":"member","EnableCache":true,"CacheTimeout":10,"GroupCacheTimeout":300,"Timeout":30,"MaxConnections":100,"IgnoreCase":false,"NestedGroupSearch":false}}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
curl -X POST -d '{ "LDAP":{"Enabled":true,"URL":"ldap://10.142.70.151:389","BaseDN":"ou=svtpvt,o=imadev,dc=ibm,dc=com","BindDN":"cn=manager,o=imadev,dc=ibm,dc=com","BindPassword":"secret","UserSuffix":"ou=users,ou=svtpvt,o=imadev,dc=ibm,dc=com","GroupSuffix":"ou=groups,ou=svtpvt,o=imadev,dc=ibm,dc=com","UserIdMap":"uid","GroupIdMap":"cn","GroupMemberIdMap":"member","EnableCache":true,"CacheTimeout":10,"GroupCacheTimeout":300,"Timeout":30,"MaxConnections":100,"IgnoreCase":false,"NestedGroupSearch":false}}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration

# curl -X POST -d '{ "LDAP":{"Enabled":"True","URL":"ldap://10.113.50.14:389","BaseDN":"ou=SVT,o=IBM,c=US","BindDN":"cn=root,o=IBM,c=US","BindPassword":"ima4test","UserSuffix":"ou=users,ou=SVT,o=IBM,c=US","GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US","UserIdMap":"uid","GroupIdMap":"cn","GroupMemberIdMap":"member","EnableCache":"True","CacheTimeout":"10","GroupCacheTimeout":"300","Timeout":"30","MaxConnections":"100","IgnoreCase":"False","NestedGroupSearch":"False"}}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration

# curl -X POST -d '{ "ConnectionPolicy":{ "SVTSecureCarsConnectPolicy":{"Protocol":"MQTT,JMS","Description":"SVTSecureConnectionPolicyForCars","GroupID":"Cars"}}}' http://${SERVER[$i]}:${PORT}/ima/v1/configuration

}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


