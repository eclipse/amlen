#!/bin/ksh
#
# RENAME THIS FILE TO:   ISMsetup.sh
#
# Description:
#   This script defines the environment for Lion test cases.
#
# Test case files should use the following upper-case TAGS in place of hard coding machine information. 
# The tester should change this file to assign the TAGS the correct actual "values".
# There are scripts that will process the test case files in the COMMON directory and 
# replace the TAGS with the "values" supplied by this file.
#----------------------------------------------------------------------------------------------------------------------------------
# The Following TAGS are system wide TAGS that may be used in test cases.  They are ALL optional:
#
# LDAP_URI - the URL to the LDAP Server in form, ip[:port] only, the test will prepend 'protocol://' and append '/DN' used
# WAS_URI - the URL to the WAS Server in form, ip:[port] only, the test will prepend 'protocol://' and append '/path' used
# DB2_SERVER_# - the DB2 Server Alias Name used in DB2 'CONNECT TO' API, increment # by 1 for each DB2 Server listed, start with '_1'
# DB2_USER_# - the DB2 Authorized user for the DB2_SERVER
# DB2_PSWD_# - the password of the DB2_USER 
# MQSERVER#_IP - the IP address of the MQ Server that will be used.... increment # by 1 for each MQ Server listed (ie. MQSERVER1_IP, MQSERVER2_IP, etc.)
# NTPSERVER_IP - the IP address of the NTP Server used in ISMWebUI automation, in Austin Automation this is 10.10.10.10
# DNSSSERVER_IP - the IP address of the DNS Server used in ISMWebUI automation, in Austin Automation this is 10.10.10.10
# DNSSEARCHDOMAIN - the textual suffix of the a DNS HOSTNAME (i.e. test.domain.com) used in ISMWebUI automation
#
#----------------------------------------------------------------------------------------------------------------------------------
# The Following TAGS define the machine used in the test environment.  Minimally, the following are required in most cases:
#    *_COUNT,  *_HOST,  *_USER,  at least one *_IP*,  M*_TESTROOT,  and  M*_IMASDK
#
# A_COUNT - Number of Appliance Machines needed in the environment
# M_COUNT - Number of test machines needed in the environment
# P_COUNT - Number of Proxy machines needed in the environment
# THIS    - Identifies which Machine 'this' machine is in the out of M_COUNT. (OPTIONAL)
#           The user does not need to set this, it is set by prepareTestEnvParameters.sh  by the ISM Automation Framework
#
# for each member (number) in A_COUNT, you will have the following group of parameters.  When A_COUNT=2, A#_ is: A1_ and A2_
# A#_HOST     - ssh-able IP address of the appliance
# A#_USER     - ssh userid, assumes passwordless ssh is set up both directions
# A#_PW       - ssh password for the server
# A#_TYPE     - server type (DOCKER, APPLIANCE, KVM_*, ...)
# A#_REST     - TRUE|FALSE whether to use the REST API
# A#_REST_USER - UserID to use the REST API
# A#_REST_PW  - Password for UserID to use the REST API
# A#_LDAP     - True|False whether the server has internal LDAP
# A#_MQCONN   - True|False whether the server has MQ Connectivity
# A#_SRVCONTID
#             - Docker container ID of the server when running in Docker
#
# A#_NIC_1    - Network Adapter Interface label name (i.e. eth0, mgt0) used by ISMWebUI automation
# A#_IPv4_1   - IPv4 IP address of first adapter (does not include the CIDR /22 or /24 suffix)
# A#_GTWYv4_1 - Gateway IP for this Network Interface used by ISMWebUI automation
# A#_IPv4_INTERNAL_1 
#             - Private IPv4 address of the first adapter
# A#_IPv4_HOSTNAME_1
#             - Private IPv4 hostname of the first adapter
# A#_IPv6_1   - IPv6 IP address of first adapter (does not include the CIDR /10 or /11 suffix) 
# A#_GTWYv6_1 - Gateway IP for this Network Interface used by ISMWebUI automation
# A#_IPv6_INTERNAL_1
#             - Private IPv6 address of the first adapter
# A#_IPv6_HOSTNAME_1
#             - Private IPv6 hostname of the first adapter
# A#_NIC_2    - Network Adapter Interface label name (i.e. eth0, mgt0) used by ISMWebUI automation
# A#_IPv4_2   - IPv4 IP address of second adapter 
# A#_GTWYv4_2 - Gateway IP for this Network Interface used by ISMWebUI automation
# A#_IPv4_INTERNAL_2
#             - Private IPv4 address of the second adapter
# A#_IPv4_HOSTNAME_2
#             - Private IPv4 hostname of the second adapter
# A#_IPv6_2   - IPv6 IP address of second adapter 
# A#_GTWYv6_2 - Gateway IP for this Network Interface used by ISMWebUI automation
# A#_IPv6_INTERNAL_2 
#             - Private IPv6 address of the second adapter
# A#_IPv6_HOSTNAME_2
#             - Private IPv6 hostname of the second adapter
# A#_IPv4_HA0 - IPv4 IP address of first HA adapter (ha1)
# A#_IPv4_HA1 - IPv4 IP address of second HA adapter (ha2)
#  Note:  IPv4_# and IPV6_# can be repeated to match the total number of adapters in the appliance.
#
# A#_SKIP_VERIFY_SERVER_RUNNING
#             - 0|1 - Whether to run verify_imaserver_running for this appliance from run-scenarios (set to 0 in setup and to 1 where needed for specifi tests) 
#
# for each member (number) in M_COUNT, you will have the following group of parameters.  When M_COUNT=4, M#_ is: M1_, M2_, M3_, and M4_
# M#_IPv4_1 - IPv4 IP address of first adapter 
# M#_IPv6_1 - IPv6 IP address of first adapter 
# M#_IPv4_2 - IPv4 IP address of second adapter 
# M#_IPv6_2 - IPv6 IP address of second adapter 
#  Note:  IPv4_# and IPV6_# can be repeated to match the total number of adapters in the machine.
#
# M#_HOST     - ssh-able 9dot ip address of the machine (or DNS hostname)
# M#_USER     - USERID used to ssh and scp to the machine (not limited to 'root' userid)
# M#_TESTROOT - Full path to the ISM Test Root Directory on the machine (can vary by machine)
# M#_IMA_SDK  - Full path to the ISM SDK Root Directory on the machine (can vary by machine)
# M#_MQ_CLIENT_SDK - Full path to the MQ Client SDK
# M#_BROWSER_LIST  - command separated list of browsers installed on the machine (can vary by machine) chrome,firefox,safari,iexplore
# M#_IMA_PLUGIN    - Full path to the IMA Plugin directory on the machine (can vary by machine)
# SYNCPORT    - Port used for the Sync Driver
# 
##
##  General Automation Environment Server Definitions
# ISM_BUILDTYPE=development
#
# MQKEY=mar...
#
# WAS_URI=10.10.10.10
# LTPAWAS_IP=10.10.0.92
# WASIP=10.10.10.10:9083
# WASClusterIP=10.10.10.10,10.10.10.10
# WASPath=/opt/IBM/WebSphereND/AppServer/
# WASType=was85
#
# DB2_SERVER_1=telemtry
# DB2_USER_1=db2admin
# DB2_PSWD_1=password 
#
# SYSLOGServer=<ipaddress>
# SYSLOGPort=514
#
# MQSERVER1_IP=10.10.10.10
# MQSERVER2_IP=10.10.10.10
#
# NTPSERVER_IP=10.10.10.10
# DNSSSERVER_IP=10.10.10.10
#
# LDAP_URI=10.10.10.10
# LDAP config testing
#  NOTE: For automation runs, the current plan is to use
#        mar080 and mar345.
# LDAP_SERVER_1=mar080.test.example.com
# LDAP_SERVER_1_PORT=389
# LDAP_SERVER_1_SSLPORT=636
# LDAP_SERVER_2=mar080.test.example.com
# LDAP_SERVER_2_PORT=5389
# LDAP_SERVER_2_SSLPORT=5636
# LDAP_SERVER_3=mar080.test.example.com
# LDAP_SERVER_3_PORT=389
# LDAP_SERVER_3_SSLPORT=636
#
# TEST_PW=apassword
# FULLRUN=TRUE
# SERVER_LABEL=201.....-....
#
# PROXY_JAVA_HOME=/path/to/java
#
# VNC_SERVER=
# VNC_DISPLAY=
##
## ISM Server (A) and Test Machine (M) Definitions
export A_COUNT=2
export M_COUNT=2
export ISM_BUILDTYPE=manual

export A1_HOST=10.10.3.x         
export A1_PORT=9089         
export IMA_AdminEndpoint=${A1_HOST}:${A1_PORT}         
export A1_USER=root
export A1_PW=password
export A1_TYPE=DOCKER
export A1_REST_USER=admin
export A1_REST_PW=admin
export A1_REST=TRUE
export A1_SRVCONTID=IMA
export A1_LDAP=FALSE
export A1_MQCONN=FALSE
export A1_NIC_1=eth0
export A1_IPv4_1=10.10.3.x     
export A1_GTWYv4_1=10.10.3.1
export A1_IPv6_1=fc00::10:10:3:x
export A1_GTWYv6_1=fc00::10:10:3:1
export A1_IPv4_HOSTNAME_1=marxxx.IPv4_1.com
export A1_IPv6_HOSTNAME_1=marxxx.IPv6_1.com
export A1_NIC_2=mgt0
export A1_IPv4_2=10.10.4.x         
export A1_GTWYv4_2=10.10.4.1
export A1_IPv6_2=fc00::10:10:4:x      
export A1_GTWYv6_2=fc00::10:10:4:1
export A1_IPv4_HOSTNAME_2=marxxx.IPv4_2.com
export A1_IPv6_HOSTNAME_2=marxxx.IPv6_2.com
export A1_IPv4_HA1=10.11.3.x     
export A1_IPv6_HA1=fc00::10:11:3:x     
export A1_IPv4_HA2=10.12.3.x     
export A1_IPv6_HA2=fc00::10:12:3:x    
export A1_SKIP_VERIFY_SERVER_RUNNING=0  

export A2_HOST=10.10.3.x
export A2_PORT=9089         
export A2_IMA_AdminEndpoint=${A2_HOST}:${A2_PORT}         
export A2_USER=root
export A2_PW=password
export A2_TYPE=DOCKER
export A2_REST_USER=admin
export A2_REST_PW=admin
export A2_REST=TRUE
export A2_SRVCONTID=IMA
export A2_LDAP=FALSE
export A2_MQCONN=FALSE
export A2_NIC_1=eth0
export A2_IPv4_1=10.10.3.x
export A2_GTWYv4_1=10.10.3.1
export A2_IPv6_1=fc00::10:10:3:x
export A2_GTWYv6_1=fc00::10:10:3:1
export A2_IPv4_HOSTNAME_1=marxxx.IPv4_1.com
export A2_IPv6_HOSTNAME_1=marxxx.IPv6_1.com
export A2_NIC_2=mgt0
export A2_IPv4_2=10.10.4.x
export A2_GTWYv4_2=10.10.4.1
export A2_IPv6_2=fc00::10:10:4:x
export A2_GTWYv6_2=fc00::10:10:4:1
export A2_IPv4_HOSTNAME_2=marxxx.IPv4_2.com
export A2_IPv6_HOSTNAME_2=marxxx.IPv6_2.com
export A2_IPv4_HA1=10.11.3.x     
export A2_IPv6_HA1=fc00::10:11:3:x     
export A2_IPv4_HA2=10.12.3.x     
export A2_IPv6_HA2=fc00::10:12:3:x    
export A2_SKIP_VERIFY_SERVER_RUNNING=0

export THIS=whatever

export M1_IPv4_1=10.10.1.x
export M1_IPv6_1=fc00::10:10:1:x
export M1_IPv4_2=10.10.2.x         
export M1_IPv6_2=fco0::10:10:2:x
export M1_HOST=9.3.x.x 
export M1_USER=root           
export M1_TESTROOT=/ism/test
export M1_IMA_SDK=/demo1
export M1_MQ_CLIENT_SDK=/opt/mqm/
export M1_BROWSER_LIST=firefox

export M2_IPv4_1=10.10.1.x
export M2_IPv6_1=fc00::10:10:1:x
export M2_IPv4_2=10.10.2.x
export M2_IPv6_2=fc00::10:10:2:x
export M2_HOST=9.3.x.x
export M2_USER=userid
export M2_TESTROOT=/niagara/testdir/
export M2_IMA_SDK=/demo
export M2_MQ_CLIENT_SDK=/opt/mqmV7.5/
export M2_BROWSER_LIST=firefox,chrome,safari,iexplore

export SYNCPORT=32223

# IMAProxy and IMABridge parameters
export P_COUNT=2
export B_COUNT=2

#PROXIES
export P1_NAME=proxy1.test.example.com
export P1_USER=root
export P1_HOST=169.xx.xx.xx
export P1_IPv4_1=169.xx.xx.xx
export P1_PROXYROOT=/niagara/proxy
export P1_PROXY_JAVA_HOME=/opt/ibm/java-x86_64-80

export P2_NAME=proxy2.test.example.com
export P2_USER=root
export P2_HOST=158.xx.xx.xx
export P2_IPv4_1=158.xx.xx.xx
export P2_PROXYROOT=/niagara/proxy
export P2_PROXY_JAVA_HOME=/opt/ibm/java-x86_64-80

#BRIDGES 
export B1_USER=root
export B1_HOST=169..xx.xx.xx
export B1_TYPE=DOCKER
export B1_BRIDGECONTID=imabridge
export B1_BRIDGEPORT=9961
export B1_BRIDGEHOME=/opt/ibm/imabridge/bin/
export B1_BRIDGEROOT=/mnt/A1/imabridge
export B1_REST_USER=admin
export B1_REST_PW=admin

#B1_KAFKASERVERS and B1_KAFKABROKERS are actually the same things for a list of Kafka brokers.  B1_KAFKABROKERS must have quotes
export B1_KAFKASERVERS=xx.bluemix.net:9093,yy.bluemix.net:9093
export B1_KAFKABROKERS="xx.bluemix.net:9093","yy.bluemix.net:9093"
export B1_KAFKAUSERNAME=xxxxxxxxxx
export B1_KAFKAPASSWORD=xxxxxxxxxxxxxx


export P2_USER=root
export P2_HOST=158.xx.xx.xx
export B2_TYPE=RPM
export B2_BRIDGECONTID=imabridge
export B2_BRIDGEPORT=9961
export B2_BRIDGEHOME=/opt/ibm/imabridge/bin/
export B2_BRIDGEROOT=/var/imabridge
export B2_REST_USER=admin
export B2_REST_PW=admin

