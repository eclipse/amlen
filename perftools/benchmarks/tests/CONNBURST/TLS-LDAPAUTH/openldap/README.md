# Installing and configuring OpenLDAP server for MessageSight Performance benchmark testing
As user `root`, perform the following steps to install and configure OpenLDAP for MessageSight performance benchmark testing.

## 1) yum install openldap*
## 2) Update /etc/rsyslog.conf, add local4.* with line below, and restart rsyslog

```
local4.*                                                /var/log/openldap.log
```

<i>NOTE: if the file /var/log/openldap.log is manually removed, then you need to service restart rsyslog in that case as well</i>

## 3) Update /etc/sysconfig/ldap and set the local interface to listen on

```
SLAPD_LDAP=no
SLAPD_LDAPI=no
SLAPD_LDAPS=no
SLAPD_URLS="ldaps://svtpvt-bench-ldap1.priv:689 ldap://svtpvt-bench-ldap1.priv:389 ldapi://%2Fvar%2Flib%2Fldap%2Fds"
```

NOTE: svtpvt-bench-ldap1.priv resolves to the IPv4 address of the private interface on the LDAP system

### 3b) Update certs so slapd will start: 
#### 3b.1) update certs : 
* scp files from /workspace/perftools/benchmarks/certs/key-ldap.pem , /workspace/perftools/benchmarks/certs/cert-ldap.pem to /etc/openldap/certs 
* chown -R ldap:ldap /etc/openldap/certs/*pem

#### 3b.2) manually update /etc/openldap/slapd.d/cn=config.ldif with these entries for cert files:

```
olcTLSCertificateFile: /etc/openldap/certs/cert-ldap.pem
olcTLSCertificateKeyFile: /etc/openldap/certs/key-ldap.pem
```

## 4) chkconfig slapd on
## 5) service slapd start
## 6) set URI and BASE in /etc/openldap/ldap.conf
See below for example:

```
BASE  dc=ibm,dc=com
URI	  ldap://svtpvt-bench-ldap1.priv
```

## 7) slappasswd -s secret (copy hash into the following files)

output should be: "{SSHA}GVdrwrLRJNdvMkggXalQCqXsly1fTREo" if not , then update the ldif files also in step 9, as they have they have this value hard coded in them.

 - /etc/openldap/slapd.d/cn=config/olcDatabase={0}config.ldif
 - /etc/openldap/slapd.d/cn=config/olcDatabase={2}bdb.ldif    

<i>NOTE: delete the #CRC32 comment at the top of the ldif files that you edit so that slapd won't complain on startup that checksum is no longer valid (note: update 10.5.18 : not required. This will be corrected later by commands run.)</i>

### 7a.1) after updating config files with new password, restart ldap before adding missing schemas, use password "secret" from step 7 

service slapd restart

### 7a.2)  add missing schemas, use password "secret" from step 7

ldapadd -x -w secret -D 'cn=config' -f ./schema/cosine.ldif

ldapadd -x -w secret -D 'cn=config' -f ./schema/inetorgperson.ldif

ldapadd -x -w secret -D 'cn=config' -f ./schema/nis.ldif 

## 8) Editing the cn=config ldif file(s)...

Use the input .ldif files from where the README.md is located and run commands below to finish up the configuration. There is a samples directory inside the directory where the README.md is for openldap if you want to see example of how these files should look after configuration.

### 8a.) Update: /etc/openldap/slapd.d/cn\=config.ldif

  ldapmodify -Y external -H  ldapi://%2Fvar%2Flib%2Fldap%2Fds  -f 0config.ldif 

### 8b.) Update: /etc/openldap/slapd.d/cn\=config/olcDatabase\=\{0\}config.ldif  

  ldapmodify -Y external -H  ldapi://%2Fvar%2Flib%2Fldap%2Fds  -f olc0config.ldif 

### 8c.) Update /etc/openldap/slapd.d/cn=config/olcDatabase={1}monitor.ldif

  ldapmodify -Y external -H  ldapi://%2Fvar%2Flib%2Fldap%2Fds  -f olc1monitor.ldif 

### 8d.) Update /etc/openldap/slapd.d/cn=config/olcDatabase={2}mdb.ldif  

  ldapmodify -Y external -H  ldapi://%2Fvar%2Flib%2Fldap%2Fds  -f olc2mdb.ldif 

  <i>NOTE: check it this file exists first, if it does not exist but an hdb or bdb file is there, then rename that file from "hdb" to mdb. mdb memory mapped database is faster for performance. Then edit the old file and change any occurence of "hdb" or "Hdb" etc.. to mdb/Mdb etc... as needed. Then run the command below to complete configuration.</i>
  
  <i>NOTE member index is crucial for group membership lookup performance</i>

## 9 ) Create data directory for OpenLDAP 
mkdir -p /disk1/openldap-data
    
## 10) Create top level ldap objects (use "secret" from step 7)
ldapmodify -x -w secret -D "cn=manager,o=imadev,dc=ibm,dc=com" -f base.ldif

## 11) Create the LDAP entries used in the benchmark test (use "secret" from step 7)
time ./crtldapgroup.sh -D "cn=manager,o=imadev,dc=ibm,dc=com" -w secret -b "ou=PERF,o=imadev,dc=ibm,dc=com" --groupname Homes --nummembers 1000000 --memberprefix h --dbnum 2
	

# Reference 
The following video provides helpful background information on OpenLDAP: https://www.youtube.com/watch?v=Kyqr58CQ_wc
