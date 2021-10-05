// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
var assert = require('assert')
var request = require('supertest')
var should = require('should')

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'status/';
var theObject = '';
// Numeric Server states can be found in server_admin/admin.h   @[ism_ServerState_t;]

// Import Object:  service/import {"FileName":"required","Password":"required","DisableObjects":true,"ApplyConfig":false}
 
var aDefault = '{ "Version": "' + FVT.version + '", \
  }';
// ---  DEFAULT ITEMS     -----------------------------------------------------------------------------------------------------
// Security Items  -----------------------------------------------------------------------------------------------------
var defaultAdminUserID = '{ "Version": "' + FVT.version + '", "AdminUserID": "admin" }' ;

var defaultAdminEndpoint = '{ "Version": "' + FVT.version + '", \
  "AdminEndpoint": { "ConfigurationPolicies": "AdminDefaultConfigPolicy", "Description": "Admin Endpoint used for processing administrative and monitoring requests from clients using REST API.", \
                     "Interface": "All","Port": 9089,"SecurityProfile": "AdminDefaultSecProfile"}}' ;

var defaultConfigurationPolicy = '{ "Version": "' + FVT.version + '", \
  "ConfigurationPolicy": {"AdminDefaultConfigPolicy": {"ActionList": "Configure,View,Monitor,Manage","ClientAddress": "*","Description": "Default configuration policy for AdminEndpoint"}}}' ;

var defaultCertificateProfile = '{ "Version": "' + FVT.version + '", \
  "CertificateProfile": {"AdminDefaultCertProf": {"Certificate": "AdminDefaultCert.pem","Key": "AdminDefaultKey.pem"}}}' ;

var defaultSecurityProfile = '{ "Version": "' + FVT.version + '", \
  "SecurityProfile": {"AdminDefaultSecProfile": {"CertificateProfile": "AdminDefaultCertProf","Ciphers": "Fast","MinimumProtocolMethod": "TLSv1.2","TLSEnabled": false, \
                      "UseClientCertificate": false,"UseClientCipher": false,"UsePasswordAuthentication": false}}}' ;

var defaultClientCertificate  = '{ "Version": "' + FVT.version + '", "ClientCertificate": [] }';
var defaultTrustedCertificate = '{ "Version": "' + FVT.version + '", "TrustedCertificate": [] }';
var defaultLTPAProfile = '{ "Version": "' + FVT.version + '", "LTPAProfile": {} }';
var defaultOAuthProfile = '{ "Version": "' + FVT.version + '", "OAuthProfile": {}  }';

var defaultLDAP = '{ "Version": "' + FVT.version + '", "LDAP": \
    { "BaseDN": "", "BindDN": "", "CacheTimeout": 10, "Certificate": "", "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, \
    "GroupIdMap": "", "GroupMemberIdMap": "", "GroupSuffix": "", "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, \
    "Timeout": 30, "URL": "", "UserIdMap": "", "UserSuffix": "", "Verify": false} }';
// Logging Items  -----------------------------------------------------------------------------------------------------
var defaultAdminLog = '{ "Version": "' + FVT.version + '", "AdminLog": "NORMAL" }' ;
var defaultConnectionLog = '{ "Version": "' + FVT.version + '", "ConnectionLog": "NORMAL" }' ;
var defaultSecurityLog = '{ "Version": "' + FVT.version + '", "SecurityLog": "NORMAL" }' ;
var defaultLogLevel = '{ "Version": "' + FVT.version + '", "LogLevel": "NORMAL" }' ;
var defaultTraceLevel = '{ "Version": "' + FVT.version + '", "TraceLevel": "5" }' ;
var defaultTraceMax = '{ "Version": "' + FVT.version + '", "TraceMax": "200MB" }' ;
var defaultTraceMessageData= '{ "Version": "' + FVT.version + '", "TraceMessageData": 0 }' ;

var defaultSyslog = '{ "Version": "' + FVT.version + '", "Syslog": { "Enabled": false, "Host": "127.0.0.1", "Port": 514, "Protocol": "tcp" } }' ;

var defaultLogLocation = '{ "Version": "' + FVT.version + '",  "LogLocation": { \
  "AdminLog": { "Destination": "/var/messagesight/diag/logs/imaserver-admin.log", "LocationType": "file" }, \
  "ConnectionLog": { "Destination": "/var/messagesight/diag/logs/imaserver-connection.log", "LocationType": "file" }, \
  "DefaultLog": { "Destination": "/var/messagesight/diag/logs/imaserver-default.log", "LocationType": "file" }, \
  "MQConnectivityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-mqconnectivity.log", "LocationType": "file" },\
  "SecurityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-security.log", "LocationType": "file" } }}' ;

// Server items  -----------------------------------------------------------------------------------------------------
var disableClusterMembership = '"ClusterMembership":{"ClusterName":"'+ FVT.MQKEY +'","ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'","ControlExternalAddress":null,"ControlExternalPort":null,"ControlPort":9104,"DiscoveryPort":9106, "DiscoveryServerList":null,"DiscoveryTime":10, \
  "EnableClusterMembership":false,"MessagingAddress":null,"MessagingExternalAddress":null,"MessagingExternalPort":null,"MessagingPort":9105,"MessagingUseTLS":false,"MulticastDiscoveryTTL":1,"UseMulticastDiscovery":true}';
var defaultClusterMembership = '{ "Version": "' + FVT.version + '", \
  "ClusterMembership":{"ClusterName":"","ControlAddress":null,"ControlExternalAddress":null,"ControlExternalPort":null,"ControlPort":9104,"DiscoveryPort":9106, "DiscoveryServerList":null,"DiscoveryTime":10, \
  "EnableClusterMembership":false,"MessagingAddress":null,"MessagingExternalAddress":null,"MessagingExternalPort":null,"MessagingPort":9105,"MessagingUseTLS":false,"MulticastDiscoveryTTL":1,"UseMulticastDiscovery":true}}';

var disableHighAvailability = '"HighAvailability": {"DiscoveryTimeout": 600,"EnableHA": false,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "FVT_HAPair","HeartbeatTimeout": 10, \
  "LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'","LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'","PreferredPrimary": true,"RemoteDiscoveryNIC": "'+ FVT.A2_IPv4_HA0 +'","RemoteDiscoveryPort": 0,"ReplicationPort": 0,"StartupMode": "StandAlone", \
  "UseSecuredConnections": false}';
var defaultHighAvailability = '{ "Version": "' + FVT.version + '", \
 "HighAvailability": {"DiscoveryTimeout": 600,"EnableHA": false,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "","HeartbeatTimeout": 10, \
  "LocalDiscoveryNIC": "","LocalReplicationNIC": "","PreferredPrimary": false,"RemoteDiscoveryNIC": "","RemoteDiscoveryPort": 0,"ReplicationPort": 0,"StartupMode": "AutoDetect", \
  "UseSecuredConnections": false}}' ;
  
  

var defaultClientSet =   '{ "Version": "' + FVT.version + '", "ClientSet": {} }';

var defaultTolerateRecoveryInconsistencies =   '{ "TolerateRecoveryInconsistencies":false }';

var defaultLicensedUsage =   '{ "Version": "' + FVT.version + '", "LicensedUsage": "Developers", "Accept":true }' ;
var defaultServerName =   '{ "Version": "' + FVT.version + '", "ServerName": "'+ FVT.A1_SERVERNAME +':'+ FVT.A1_PORT +'" }' ;
var defaultSNMPEnabled =   '{ "Version": "' + FVT.version + '", "SNMPEnabled": false }' ;

// MQ Items  -----------------------------------------------------------------------------------------------------
var defaultMQConnectivityEnabled = '{  "MQConnectivityEnabled": false, "Version": "' + FVT.version + '" }';
var defaultMQConnectivityLog = '{ "Version": "' + FVT.version + '", "MQConnectivityLog": "NORMAL" }' ;
var defaultQueueManagerConnection = '{ "Version": "' + FVT.version + '", "QueueManagerConnection": {} }' ;
var defaultDestinationMappingRule = '{ "Version": "' + FVT.version + '", "DestinationMappingRule": {} }' ;

// Message Flow Items  -----------------------------------------------------------------------------------------------------
var defaultTopicMonitor = '{ "Version": "' + FVT.version + '", \
  "TopicMonitor": [] }'

var defaultMessageHub = '{ "Version": "' + FVT.version + '", \
  "MessageHub": {"DemoHub": {"Description": "Demo Message Hub."}}}';

var defaultTopicPolicy = '{ "Version": "' + FVT.version + '", \
  "TopicPolicy": {"DemoTopicPolicy": {"ActionList": "Publish,Subscribe","ClientID": "*","Description": "Demo topic policy","DisconnectedClientNotification": false, \
  "MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages","Topic": "*"}}}';

var defaultSubscriptionPolicy = '{ "Version": "' + FVT.version + '", \
  "SubscriptionPolicy": {"DemoSubscriptionPolicy": {"ActionList": "Receive,Control","Description": "Demo policy for shared durable subscription","MaxMessages": 5000, \
  "MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT","Subscription": "*"}}}';

var disableEndpoint = '"Endpoint": { \
    "DemoEndpoint": {"ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.", \
        "EnableAbout": true,"Enabled": false,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 16102,"Protocol": "All", \
        "SubscriptionPolicies": "DemoSubscriptionPolicy","TopicPolicies": "DemoTopicPolicy"}, \
    "DemoMqttEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use with MQTT protocol only. By default, it uses port 1883.", \
        "EnableAbout": true,"Enabled": false,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 1883,"Protocol": "MQTT", \
        "TopicPolicies": "DemoTopicPolicy"}}';
var defaultEndpoint = '{ "Version": "' + FVT.version + '", '+ disableEndpoint +'}' ;

var defaultQueue = '{ "Queue":{}, "Version": "' + FVT.version + '" }';
var defaultQueuePolicy = '{ "QueuePolicy":{}, "Version": "' + FVT.version + '" }';

var defaultProtocol = '{ "Version": "' + FVT.version + '", "Protocol": { \
  "jms": { "UseTopic": true, "UseQueue": true, "UseShared": true, "UseBrowse": true }, \
  "mqtt": { "UseTopic": true, "UseQueue": false, "UseShared": true, "UseBrowse": false } }}';
  
var defaultPlugin = '{ "Plugin":{}, "Version": "' + FVT.version + '" }';
var defaultPluginServer = '{ "PluginServer":"127.0.0.1", "Version": "' + FVT.version + '" }';
var defaultPluginPort = '{ "PluginPort":9103, "Version": "' + FVT.version + '" }';
var defaultPluginDebugServer = '{ "PluginDebugServer":"", "Version": "' + FVT.version + '" }';
var defaultPluginDebugPort = '{ "PluginDebugPort":0, "Version": "' + FVT.version + '" }';
var defaultPluginMaxHeapSize = '{ "PluginMaxHeapSize":512, "Version": "' + FVT.version + '" }';

//   --- CONFIGURED ITEMS   ---------------------------------------------------------------------------------------------------------------------------
// Security Items  -----------------------------------------------------------------------------------------------------
var configAdminUserID = ' "AdminUserID": "admin" ';

var updateAdminEndpoint = '"AdminEndpoint": { "ConfigurationPolicies": "BatchConfigPolicy", "Interface": "All","SecurityProfile": "AdminDefaultSecProfile"}' ;
var configAdminEndpoint = '"AdminEndpoint": { "ConfigurationPolicies": "BatchConfigPolicy", "Interface": "All","Port": 9089,"SecurityProfile": "AdminDefaultSecProfile"}' ;

var configConfigurationPolicy = '"ConfigurationPolicy": {"BatchConfigPolicy": {"ActionList": "Configure,View,Monitor,Manage","ClientAddress": "*","Description": "Batch policy for AdminEndpoint"}} ';
    
var configCertificateProfile = '"CertificateProfile": \
    {"BatchCertProf": {"Certificate": "BatchCert.pem","Key": "BatchKey.pem"}, \
     "ClientCertCertProf": {"Certificate": "ClientCert.pem","Key": "ClientKey.pem"}, \
     "TrustedCertCertProf": {"Certificate": "TrustCert.pem","Key": "TrustKey.pem"} } ' ;

var configSecurityProfile = '"SecurityProfile": \
  {"ClientCertSecProf": {"CertificateProfile": "ClientCertCertProf","Ciphers": "Medium","MinimumProtocolMethod": "TLSv1.1","TLSEnabled": false, \
                      "UseClientCertificate": true,"UseClientCipher": false,"UsePasswordAuthentication": false}, \
  "TrustedCertSecProf": {"CertificateProfile": "TrustedCertCertProf","Ciphers": "Fast","MinimumProtocolMethod": "TLSv1.2","TLSEnabled": false, \
                      "UseClientCertificate": true,"UseClientCipher": false,"UsePasswordAuthentication": false}, \
  "BatchSecProf": {"CertificateProfile": "BatchCertProf","Ciphers": "Fast","MinimumProtocolMethod": "TLSv1","TLSEnabled": false, \
                      "UseClientCertificate": true,"UseClientCipher": false,"UsePasswordAuthentication": false} \
                      }' ;
    
var configClientCertificate  = ' "ClientCertificate": [{"SecurityProfileName":"ClientCertSecProf","CertificateName": "ClientCertificate.pem"}]  ';
var configTrustedCertificate = ' "TrustedCertificate": [{"SecurityProfileName":"TrustedCertSecProf","TrustedCertificate": "TrustedCertificate.pem"}] ';
var configOAuthProfile = ' "OAuthProfile": {"BatchOAuthProf":{"ResourceURL":"http://oauth.com","KeyFileName":"OAuthKeyfile" }} ';

var updateLTPAProfile = ' "LTPAProfile": {"BatchLTPAProf":{"KeyFileName":"LTPAKeyfile","Password":"imasvtest"}} ';         
var configLTPAProfile = ' "LTPAProfile": {"BatchLTPAProf":{"KeyFileName":"LTPAKeyfile" }} ';

var updateLDAP = '"LDAP": { "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"ima4test", "CacheTimeout": 10, "Certificate": "LDAPCert.pem", \
            "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, "GroupIdMap": "cn", "GroupMemberIdMap":"member", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", \
            "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, "Timeout": 30, \
            "URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "UserIdMap":"uid", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "Verify": false} ';
var configLDAP = '"LDAP": { "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "CacheTimeout": 10, "Certificate": "ldap.pem", \
            "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, "GroupIdMap": "cn", "GroupMemberIdMap":"member", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", \
            "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, "Timeout": 30, \
            "URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "UserIdMap":"uid", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "Verify": false} ';
// took out BindPassword to verify Import Preview
var importLDAP = '"LDAP": { "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "CacheTimeout": 10, "Certificate": "ldap.pem", \
            "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, "GroupIdMap": "cn", "GroupMemberIdMap":"member", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", \
            "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, "Timeout": 30, \
            "URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "UserIdMap":"uid", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "Verify": false} ';  
  
// Logging Items  -----------------------------------------------------------------------------------------------------
var configAdminLog = ' "AdminLog": "MIN" ';
var configConnectionLog = ' "ConnectionLog": "MAX" ';
var configSecurityLog = ' "SecurityLog": "MAX" ';
var configLogLevel = ' "LogLevel": "MIN" ';
var configTraceLevel = ' "TraceLevel": "5,admin:9,http:9" ';
var configTraceMax = ' "TraceMax": "251MB" ' ;
var configTraceMessageData= ' "TraceMessageData": 0 ' ;

var configSyslog = ' "Syslog": { "Enabled": false, "Host": "127.0.0.1", "Port": 415, "Protocol": "udp"  }' ;
var updateSyslog = ' "Syslog": { "Port": 415, "Protocol": "udp"  }' ;

var configLogLocation = ' "LogLocation": { \
  "AdminLog": { "Destination": "/var/messagesight/diag/logs/imaserver-admin.log", "LocationType": "file" }, \
  "ConnectionLog": { "Destination": "/var/messagesight/diag/logs/imaserver-connection.log", "LocationType": "file" }, \
  "DefaultLog": { "Destination": "/var/messagesight/diag/logs/imaserver-default.log", "LocationType": "file" }, \
  "MQConnectivityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-mqconnectivity.log", "LocationType": "file" },\
  "SecurityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-security.log", "LocationType": "file" }}' ;

// Server items  -----------------------------------------------------------------------------------------------------
var configClusterMembership = ' "ClusterMembership": \
    {"ControlExternalAddress": null,"ControlPort": 9104,"DiscoveryPort": 9106,"DiscoveryTime": 10,"EnableClusterMembership": true,"MessagingAddress": null, \
    "MessagingExternalAddress": null,"MessagingPort": 9105,"MessagingUseTLS": false,"MulticastDiscoveryTTL": 1,"UseMulticastDiscovery": true, "ClusterName":"'+ FVT.MQKEY +'","ControlAddress": "'+ FVT.A1_IPv4_INTERNAL_1 +'"} ';

var configHighAvailability = ' "HighAvailability": \
    {"DiscoveryTimeout": 600,"EnableHA": true,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "FVT_HAPair","HeartbeatTimeout": 10,"LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'",\
    "LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'","PreferredPrimary": true,"RemoteDiscoveryNIC": "'+ FVT.A2_IPv4_HA0 +'","RemoteDiscoveryPort": 0,"ReplicationPort": 0, \
    "StartupMode": "StandAlone","UseSecuredConnections": false }';

var configClientSet =   ' "ClientSet": {} ';

var configTolerateRecoveryInconsistencies =   ' "TolerateRecoveryInconsistencies":true ';

var configLicensedUsage =   ' "LicensedUsage": "Non-Production", "Accept":true ' ;
var configServerName =   ' "ServerName": "' + FVT.A1_HOSTNAME + '" ' ;
var configSNMPEnabled =   ' "SNMPEnabled": true ' ;

// MQ Items  -----------------------------------------------------------------------------------------------------
var configMQConnectivityEnabled = ' "MQConnectivityEnabled": true ';
var configMQConnectivityLog = ' "MQConnectivityLog": "MAX" ' ;
var configQueueManagerConnection = ' "QueueManagerConnection":{"BatchQmgr":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1416)","QueueManagerName":"QM_MQJMS","ChannelName":"CHNLJMS"}}' ;
var configDestinationMappingRule = ' "DestinationMappingRule":{"BatchDMapRule":{"Destination":"MQ_QUEUE","Enabled":false,"MaxMessages":5000,"QueueManagerConnection":"BatchQmgr","RetainedMessages":"None","RuleType":1,"Source":"/ms_topic"}}' ;

// Message Flow Items  -----------------------------------------------------------------------------------------------------
var configTopicMonitor = ' "TopicMonitor":["/topic/#"] ';

var configMessageHub = ' "MessageHub": {"DemoHub": {"Description": "Demo Message Hub."}, "BatchHub":{} }' ;

var configTopicPolicy = ' "TopicPolicy": \
    {"DemoTopicPolicy": {"ActionList": "Publish,Subscribe","ClientID": "*","Description": "Demo topic policy","DisconnectedClientNotification": false, \
    "MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages","Topic": "*"}}' ;

var configSubscriptionPolicy = ' "SubscriptionPolicy": \
    {"DemoSubscriptionPolicy": {"ActionList": "Receive,Control","Description": "Demo policy for shared durable subscription","MaxMessages": 5000, \
    "MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT","Subscription": "*"}}' ;

var configEndpoint = ' "Endpoint": { \
    "DemoEndpoint": {"ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.", \
         "EnableAbout": true,"Enabled": true,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 16102,"Protocol": "All", \
         "SubscriptionPolicies": "DemoSubscriptionPolicy","TopicPolicies": "DemoTopicPolicy"}, \
    "DemoMqttEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use with MQTT protocol only. By default, it uses port 1883.", \
         "EnableAbout": true,"Enabled": true,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 1883,"Protocol": "MQTT", \
         "TopicPolicies": "DemoTopicPolicy"}}' ;

var configQueue = ' "Queue":{"BatchQueue":{}} ';
var configQueuePolicy = ' "QueuePolicy":{ "BatchQueuePol": { "ActionList": "Send,Receive,Browse", "Queue": "BatchQueue/*", "ClientID": "'+ FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 +'" }}';
var configProtocol = ' "Protocol": { \
  "jms": { "UseTopic": true, "UseQueue": true, "UseShared": true, "UseBrowse": true }, \
  "mqtt": { "UseTopic": true, "UseQueue": false, "UseShared": true, "UseBrowse": false } }';
  
var configPlugin = ' "Plugin":{} ';
var configPluginServer = ' "PluginServer":"127.0.0.1" ';
var configPluginPort = ' "PluginPort":3019 ';
var configPluginDebugServer = ' "PluginDebugServer":"" ';
var configPluginDebugPort = ' "PluginDebugPort":0 ';
var configPluginMaxHeapSize = ' "PluginMaxHeapSize":2048 ';
var configPluginVMArgs = ' "PluginVMArgs":"-Xms2G -Xmx4G -Xss1024m" ';


// ----------------------------------------------------------------------------------------------------- 
//  ENUM Config   
//  iterate[i] thru config item list appending a prefix [default|config] to the verifyExportImportENUM[i] string,
//  then get the value of the compound string via  eval( <prefix> + verifyExportImportENUM[i] )
// NOTE:  config has the outter {}'s stripped so they can be built into batches easier, 
//  need to be added back when retrieve individual values
// ----------------------------------------------------------------------------------------------------- 
//REMOVED:  LicenseUsage, has to handle seperately
// LicensedUsage is problem in Import, has to handle seperately 
var verifyExportImportENUM = [ "ConfigurationPolicy", "CertificateProfile", "SecurityProfile", "ClientCertificate", 
   "TrustedCertificate", "LTPAProfile", "OAuthProfile", "LDAP", "AdminLog", "ConnectionLog", "SecurityLog", "LogLevel", "TraceMax", 
   "TraceMessageData", "Syslog", "LogLocation", "ClusterMembership", "HighAvailability", "ServerName", "SNMPEnabled", 
   "MQConnectivityEnabled", "MQConnectivityLog", "QueueManagerConnection", "DestinationMappingRule", "MessageHub", "TopicMonitor", "TopicPolicy", 
   "SubscriptionPolicy", "Endpoint", "Queue", "QueuePolicy", "Protocol", "Plugin", "PluginServer", "PluginPort", 
   "PluginDebugServer", "PluginDebugPort", "PluginMaxHeapSize", "TolerateRecoveryInconsistencies" ]; 

// 141433 finally defined what is not in the EXPORT and thus not changed on IMPORT
// also ServerUID, AdminUserPassword, AdminDefaultCertProfile, AdminDefaultSecProfile.
var verifyExportImportUnchangedENUM = [ "AdminUserID", "AdminEndpoint", "TraceLevel", "LicensedUsage"  ];



var enableSecurityConfig = '{'+ configConfigurationPolicy +', '+ configCertificateProfile +', '+ configSecurityProfile +', '+ updateAdminEndpoint +', '+ configAdminUserID +', '+ configClientCertificate +', '+ configTrustedCertificate +', '+ updateLTPAProfile +', '+ configOAuthProfile +', '+ updateLDAP +'}';
// (temp subset) 134436 var enableSecurityConfig = '{'+ configConfigurationPolicy +', '+ configCertificateProfile +', '+ configSecurityProfile +', '+ updateAdminEndpoint +', '+ configAdminUserID +', '+ configClientCertificate +', '+ configTrustedCertificate +', '+ updateLTPAProfile +', '+ configOAuthProfile +'}';

var enableLoggingConfig = '{'+ configAdminLog +', '+ configConnectionLog +', '+ configSecurityLog +', '+ configLogLevel +', '+ configTraceLevel +', '+ configTraceMax +', '+ configTraceMessageData +', '+ updateSyslog +', '+ configLogLocation +'}';
// (temp subset) 134556 - having to spilt to work around a MOCHA error parsing
var enableLoggingConfig_1 = '{'+ configAdminLog +', '+ configConnectionLog +', '+ configSecurityLog +', '+ configLogLevel +', '+ configTraceLevel +', '+ configTraceMax +', '+ configTraceMessageData +', '+ configLogLocation +'}';
// updateSyslog seems to fail in MOCHA consistently, yet is successful in CURL.  
var enableLoggingConfig_2 = '{'+ updateSyslog +'}';

// configSNMPEnabled done separately
var enableServerConfig = '{'+ configLicensedUsage +', '+ configServerName +', '+ configClusterMembership +', '+ configHighAvailability +', '+ configClientSet +', '+ configTolerateRecoveryInconsistencies   +'}';
// (temp subset) var enableServerConfig = '{'+ configLicensedUsage +', '+ configServerName +', '+ configClusterMembership +'}';

var enableMessageFlowConfig = '{'+ configMessageHub +', '+ configTopicMonitor +', '+ configTopicPolicy +', '+ configSubscriptionPolicy +', '+ configEndpoint +', '+ configQueue +', '+ configQueuePolicy +'}';

var enablePluginConfig = '{'+  configPlugin +', '+ configPluginPort +', '+ configPluginDebugServer +', '+ configPluginDebugPort +', '+ configPluginMaxHeapSize +', '+ configPluginVMArgs +', '+ configProtocol +'}';
var enablePluginConfig = '{'+  configPluginServer +', '+ configPluginPort +', '+ configPluginDebugServer +', '+ configPluginDebugPort +', '+ configPluginMaxHeapSize +', '+ configPluginVMArgs +'}';
// configMQConnectivityEnabled done separately
var enableMQConfig = '{'+ configMQConnectivityLog +', '+ configQueueManagerConnection +', '+ configDestinationMappingRule +'}';

  
// ??? Should I remove SERVER.NAME, 2 UPTIMEs and VERSION; HA.PrimaryLastTime?  or add special handling?
//  {"HighAvailability": {"PrimaryLastTime": "2015-07-08 23:59:29Z"}}
//  {"Server": {"Name": "GIXZj6Zb","UpTimeDescription": "0 days 1 hours 23 minutes 1 seconds","UpTimeSeconds": 4981,"Version": "V.next Beta 20150708-0400"}}

var verifyConfig = {};
var verifyMessage = {};

// Functions Start Here
function getDynamicFieldsCallback( res, done ){
    FVT.trace( 0, "@getDynamicFieldsCallback RESPONSE:"+ JSON.stringify( res, null, 2 ) );
    jsonResponse = JSON.parse( res.text );
    expectJSON = JSON.parse( FVT.expectDefaultStatus );
    thisName = jsonResponse.Server.Name ;
    thisVersion = jsonResponse.Server.Version ;
    FVT.trace( 1, 'The Server Name: '+ thisName +' and the Version is: '+ thisVersion );
    expectJSON.Server[ "Name" ] = thisName ;
    expectJSON.Server[ "Version" ] = thisVersion ;
    FVT.expectDefaultStatus = JSON.stringify( expectJSON, null, 0 );

//    console.log( "PARSE: req" );
    res.req.method.should.equal('GET');
//    res.req.url.should.equal( 'http://' + FVT.IMA_AdminEndpoint +'ima/v1/service/Status/');

//    console.log( "PARSE: header" );
    res.header.server.should.equal( FVT.responseHeaderProductName );
//    res.header.access-control-allow-origin.should.equal('*');
//    res.header.access-control-allow-credentials.should.equal('true');

//    res.header.connection.should.equal('keep-alive');
    res.header.connection.should.equal('close');

    //    res.header.keep-alive.should.equal('timeout=60');
//    res.header.cache-control.should.equal('no-cache');
//    res.header.content-type.should.equal('application/json');
//    res.header.content-length.should.equal('552');

    
    FVT.trace( 0, "NEW FVT.expectDefaultStatus:"+ JSON.stringify( FVT.expectDefaultStatus, null, 2 ) );
    res.statusCode.should.equal(200);
    FVT.getSuccessCallback( res, done )
//    done();
}

function postUpdatedConfig(){

    it('should return status 200 POST Security Configuration Block', function(done) {
        var payload =  enableSecurityConfig ;
        verifyConfig = JSON.parse( enableSecurityConfig );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Logging Configuration Block #1', function(done) {
        var payload =  enableLoggingConfig_1 ;
        verifyConfig = JSON.parse( enableLoggingConfig_1 );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Logging Configuration Block #2', function(done) {
        var payload =  enableLoggingConfig_2 ;
        verifyConfig = JSON.parse( enableLoggingConfig_2 );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Logging Configuration Block #2', function(done) {
        var payload =  '{'+ configLogLocation +'}';
        verifyConfig = JSON.parse( payload );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Logging Configuration Block #3', function(done) {
        var payload =  '{'+ configLogLocation +'}';
        verifyConfig = JSON.parse( payload );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Server Configuration Block', function(done) {
        var payload =  enableServerConfig ;
        verifyConfig = JSON.parse( enableServerConfig );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST MessageFlow Configuration Block', function(done) {
        var payload =  enableMessageFlowConfig ;
        verifyConfig = JSON.parse( enableMessageFlowConfig );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

    it('should return status 200 POST Plugin Configuration Block', function(done) {
        var payload =  enablePluginConfig ;
        verifyConfig = JSON.parse( enablePluginConfig );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });

// 124276
//* comment out until fixed
    it('should return status 200 POST MQ Configuration Block', function(done) {
        var payload =  enableMQConfig ;
        verifyConfig = JSON.parse( enableMQConfig );
        FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
    });
//124276 END */
}

// GET Default Config
function getConfig( i, theText, thePrefix ) {
    describe( theText , function() {
        var object = verifyExportImportENUM[i];
//console.log( '====DEBUG: thePrefix: '+ thePrefix +'  theObject: '+ object + '   the Value: '+ eval( thePrefix + object ) );
        it('should return status 200 on '+ theText +': '+ object , function(done) {
            if( thePrefix === "default" ) {
		        verifyConfig = JSON.parse( eval( thePrefix + object ) ) ;
			} else {
		        verifyConfig = JSON.parse( "{" + eval(  "config" + object ) +"}" ) ;
			}
            FVT.makeGetRequest( FVT.uriConfigDomain + object, FVT.getSuccessCallback, verifyConfig, done);
        });
    });
}        



// Test Cases Start Here, validate initial config, then:

// Steps are done outside this test case restapi-Export.js, see ism-RESTAPI-01.sh
//  ---------------------------------------------------------------------
// 1. Post the Updated Config.
// 2. Verify GET Updated Config
// 3. RESTART Server   (Janet's addition, verify the CONFIG got replicated/persisted all though the Server before you blow it away...)
// 4. Verify GET Updated Config (persisted)
// 5. EXPORT

// Steps are done outside this test case, either CLI or restapi-SvcResetConfig.js, see ism-RESTAPI-01.sh
//  ---------------------------------------------------------------------
// 1. SAVE the Export file (/tmp/userfiles/ is erased with Reset:config
// 2. Config Reset
// 3. Verify GET Default Config
// 4. RESTART Server  
// 5. Verify GET Default Config
// 6. REPLACE the Export file in /tmp/userfile

// Steps are done HERE
//  ---------------------------------------------------------------------
// 0. [PREREQ] need to ENABLE services before the IMPORT
// 1. IMPORT the Exported config file
// 2. Verify GET Updated Config #1
// 3. RESTART Server  
// 4. Verify GET Updated Config #2 restart
// 5. Config Reset
// 6. Verify GET Default Config
// 7. RESTART Server  
// 8. GET Default Config after RESET/RESTART


describe('Import:', function() {
//this.timeout( FVT.defaultTimeout );
this.timeout( 15000 );

// 0. [PREREQ] need to ENABLE services before the IMPORT

    describe('Enable SubServices:', function() {
    // Update MQConnectivity outside of the Batch Post
        it('should return status 200 POST MQConnEnabled:true', function(done) {
            var payload =  '{ '+ configMQConnectivityEnabled +'}' ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET MQConnectivityEnabled:true ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain +"MQConnectivityEnabled", FVT.getSuccessCallback, verifyConfig, done)
        });        
        it('should return status 400 GET mqconnectivity Status ', function(done) {
		    this.timeout( FVT.START_MQ + 2000 + 2000 );
            FVT.sleep( FVT.START_MQ + 2000 );
            verifyConfig = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: mqconnectivity is not valid." }  ;
            FVT.makeGetRequest( FVT.uriServiceDomain +"status/mqconnectivity", FVT.getFailureCallback, verifyConfig, done)
        });        
        it('should return status 200 GET MQConnectivity Status ', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning )  ;
            FVT.makeGetRequest( FVT.uriServiceDomain +"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });        
    // Update MQConnectivity outside of the Batch Post

    // Update SNMP outside of the Batch Post
        it('should return status 200 POST SNMPEnabled:true', function(done) {
            var payload =  '{ '+ configSNMPEnabled +'}' ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET SNMPEnabled:true ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain +"SNMPEnabled", FVT.getSuccessCallback, verifyConfig, done)
        });        
        it('should return status 400 GET snmp Status ', function(done) {
            FVT.sleep( FVT.START_SNMP );
            verifyConfig = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: snmp is not valid." }  ;
            FVT.makeGetRequest( FVT.uriServiceDomain +"status/snmp", FVT.getFailureCallback, verifyConfig, done)
        });        
        it('should return status 200 GET SNMP Status ', function(done) {
            verifyConfig = JSON.parse( FVT.expectSNMPRunning )  ;
            FVT.makeGetRequest( FVT.uriServiceDomain +"status/SNMP", FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });        

// 1. IMPORT the Exported config file 
	describe('Import Configuration:', function() {
// Verify my VERIFYTESTS have expect value before I attempt  
        it('should return status 200 GET MQConnLog:Normal, verify before any Imports', function(done) {
		    verifyConfig = { "MQConnectivityLog":"NORMAL" };
            FVT.makeGetRequest( FVT.uriConfigDomain +"MQConnectivityLog", FVT.getSuccessCallback, verifyConfig, done)
        });        
        it('should return status 200 GET DemoEndpoint, verify before any Imports', function(done) {
		    verifyConfig = { "Endpoint":{"DemoEndpoint":{ "Enabled":false }}};
            FVT.makeGetRequest( FVT.uriConfigDomain +"Endpoint/DemoEndpoint", FVT.getSuccessCallback, verifyConfig, done)
        });        
	
// Preview the IMPORT 0 ApplyConfig defaults to false, DisableObjects defaults to true
        it('should return status 200 POST "Import" config Preview 0', function(done) {
            var payload = '{ "FileName":"ExportServerConfig", "Password":"ima4test" }';
 			verifyConfigString = '{\n  \"Version\": "'+ FVT.version +'",\n  '+ configServerName +',\n  '+ configLogLevel +',\n  '+ configConnectionLog +',\n  '+ configSecurityLog +',\n  '+ configAdminLog +',\n  '+ configMQConnectivityLog +',\n  '+ configTraceMax +',\n  '+ configTraceMessageData +',\n  \"TraceSelected\": \"\",\n  \"TraceOptions\": \"time,thread,where\",\n  \"TraceConnection\": \"\",\n  \"TraceBackup\": 1,\n  \"TraceBackupCount\": 3,\n  \"TraceBackupDestination\": \"\",\n  \"FIPS\": false,\n  '+ configSNMPEnabled +',\n  '+ configLogLocation +',\n  '+ configSecurityProfile +',\n  '+ configCertificateProfile +',\n  '+ configMQConnectivityEnabled +',\n  '+ configMessageHub +',\n  '+ disableEndpoint +',\n  \"EnableDiskPersistence\": true,\n  '+ configConfigurationPolicy +',\n  "ConnectionPolicy": { 		"DemoConnectionPolicy": { 			"Description": "Demo connection policy", 			"ClientID": "*" 		} 	},\n  '+ configTopicPolicy +',\n  '+ configSubscriptionPolicy +',\n  '+ configPluginServer +',\n  '+ configPluginPort +',\n  \"PluginDebugServer\": \"\",\n  \"PluginDebugPort\": 0,\n  '+ configPluginMaxHeapSize +',\n  '+ configPluginVMArgs +',\n  '+ configTopicMonitor +',\n  '+ disableHighAvailability +',\n  '+ importLDAP +',\n  '+ disableClusterMembership +',\n  '+ configSyslog +'\n}\n' ;
 			console.log( verifyConfigString );
            verifyConfig = JSON.parse( verifyConfigString );
            verifyMessage = JSON.parse( '{ "status":200 }' ) ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"import", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET MQConnLog:Normal, verify not updated Preview Import 0 ', function(done) {
		    verifyConfig = { "MQConnectivityLog":"NORMAL" };
            FVT.makeGetRequest( FVT.uriConfigDomain +"MQConnectivityLog", FVT.getSuccessCallback, verifyConfig, done)
        });        
	
// Preview the IMPORT 1 ApplyConfig defaults to false, DisableObjects:true
        it('should return status 200 POST "Import" config Preview 1', function(done) {
            var payload = '{ "FileName":"ExportServerConfig", "Password":"ima4test", "DisableObjects":true }';
			verifyConfig = JSON.parse( '{\n  \"Version\": "'+ FVT.version +'",\n  '+ configServerName +',\n  '+ configLogLevel +',\n  '+ configConnectionLog +',\n  '+ configSecurityLog +',\n  '+ configAdminLog +',\n  '+ configMQConnectivityLog +',\n  '+ configTraceMax +',\n  '+ configTraceMessageData +',\n  \"TraceSelected\": \"\",\n  \"TraceOptions\": \"time,thread,where\",\n  \"TraceConnection\": \"\",\n  \"TraceBackup\": 1,\n  \"TraceBackupCount\": 3,\n  \"TraceBackupDestination\": \"\",\n  \"FIPS\": false,\n  '+ configSNMPEnabled +',\n  '+ configLogLocation +',\n  '+ configSecurityProfile +',\n  '+ configCertificateProfile +',\n  '+ configMQConnectivityEnabled +',\n  '+ configMessageHub +',\n  '+ disableEndpoint +',\n  \"EnableDiskPersistence\": true,\n  '+ configConfigurationPolicy +',\n  "ConnectionPolicy": { 		"DemoConnectionPolicy": { 			"Description": "Demo connection policy", 			"ClientID": "*" 		} 	},\n  '+ configTopicPolicy +',\n  '+ configSubscriptionPolicy +',\n  '+ configPluginServer +',\n  '+ configPluginPort +',\n  \"PluginDebugServer\": \"\",\n  \"PluginDebugPort\": 0,\n  '+ configPluginMaxHeapSize +',\n  '+ configPluginVMArgs +',\n  '+ configTopicMonitor +',\n  '+ disableHighAvailability +',\n  '+ importLDAP +',\n  '+ disableClusterMembership +',\n  '+ configSyslog +'\n}\n' );
            verifyMessage = JSON.parse( '{ "status":200 }' ) ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"import", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET MQConnLog:Normal, verify not updated Preview Import 1 ', function(done) {
		    verifyConfig = { "MQConnectivityLog":"NORMAL" };
            FVT.makeGetRequest( FVT.uriConfigDomain +"MQConnectivityLog", FVT.getSuccessCallback, verifyConfig, done)
        });        

// Preview the IMPORT 2 ApplyConfig:false, DisableObject:false
        it('should return status 200 POST "Import" config Preview 2', function(done) {
            var payload = '{ "FileName":"ExportServerConfig", "Password":"ima4test", "ApplyConfig":false, "DisableObjects":false }';
			verifyConfig = JSON.parse( '{\n  \"Version\": "'+ FVT.version +'",\n  '+ configServerName +',\n  '+ configLogLevel +',\n  '+ configConnectionLog +',\n  '+ configSecurityLog +',\n  '+ configAdminLog +',\n  '+ configMQConnectivityLog +',\n  '+ configTraceMax +',\n  '+ configTraceMessageData +',\n  \"TraceSelected\": \"\",\n  \"TraceOptions\": \"time,thread,where\",\n  \"TraceConnection\": \"\",\n  \"TraceBackup\": 1,\n  \"TraceBackupCount\": 3,\n  \"TraceBackupDestination\": \"\",\n  \"FIPS\": false,\n  '+ configSNMPEnabled +',\n  '+ configLogLocation +',\n  '+ configSecurityProfile +',\n  '+ configCertificateProfile +',\n  '+ configMQConnectivityEnabled +',\n  '+ configMessageHub +',\n  '+ configEndpoint +',\n  \"EnableDiskPersistence\": true,\n  '+ configConfigurationPolicy +',\n  "ConnectionPolicy": { 		"DemoConnectionPolicy": { 			"Description": "Demo connection policy", 			"ClientID": "*" 		} 	},\n  '+ configTopicPolicy +',\n  '+ configSubscriptionPolicy +',\n  '+ configPluginServer +',\n  '+ configPluginPort +',\n  \"PluginDebugServer\": \"\",\n  \"PluginDebugPort\": 0,\n  '+ configPluginMaxHeapSize +',\n  '+ configPluginVMArgs +',\n  '+ configTopicMonitor +',\n  '+ configHighAvailability +',\n  '+ importLDAP +',\n  '+ configClusterMembership +',\n  '+ configSyslog +'\n}\n' );
            verifyMessage = JSON.parse( '{ "status":200 }' ) ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"import", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET DemoEndpoint, verify Enabled Preview Import 2 ', function(done) {
		    verifyConfig = { "Endpoint":{"DemoEndpoint":{ "Enabled":false }}};
            FVT.makeGetRequest( FVT.uriConfigDomain +"Endpoint/DemoEndpoint", FVT.getSuccessCallback, verifyConfig, done)
        });        

//DO THE Import 3 with Objects Disabled TRUE!
        it('should return status 200 POST "Import" 3', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            var payload = '{ "FileName":"ExportServerConfig", "Password":"ima4test", "ApplyConfig":true, "DisableObjects":true }';
			verifyConfig = JSON.parse( '{\n  \"Version\": "'+ FVT.version +'",\n  '+ configServerName +',\n  '+ configLogLevel +',\n  '+ configConnectionLog +',\n  '+ configSecurityLog +',\n  '+ configAdminLog +',\n  '+ configMQConnectivityLog +',\n  '+ configTraceMax +',\n  '+ configTraceMessageData +',\n  \"TraceSelected\": \"\",\n  \"TraceOptions\": \"time,thread,where\",\n  \"TraceConnection\": \"\",\n  \"TraceBackup\": 1,\n  \"TraceBackupCount\": 3,\n  \"TraceBackupDestination\": \"\",\n  \"FIPS\": false,\n  '+ configSNMPEnabled +',\n  '+ configLogLocation +',\n  '+ configSecurityProfile +',\n  '+ configCertificateProfile +',\n  '+ configMQConnectivityEnabled +',\n  '+ configMessageHub +',\n  '+ disableEndpoint +',\n  \"EnableDiskPersistence\": true,\n  '+ configConfigurationPolicy +',\n  "ConnectionPolicy": { 		"DemoConnectionPolicy": { 			"Description": "Demo connection policy", 			"ClientID": "*" 		} 	},\n  '+ configTopicPolicy +',\n  '+ configSubscriptionPolicy +',\n  '+ configPluginServer +',\n  '+ configPluginPort +',\n  \"PluginDebugServer\": \"\",\n  \"PluginDebugPort\": 0,\n  '+ configPluginMaxHeapSize +',\n  '+ configPluginVMArgs +',\n  '+ configTopicMonitor +',\n  '+ disableHighAvailability +',\n  '+ importLDAP +',\n  '+ disableClusterMembership +',\n  '+ configSyslog +'\n}\n' );
            FVT.makePostRequest( FVT.uriServiceDomain+"import", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET DemoEndpoint, verify Enabled Import 3 ', function(done) {
		    verifyConfig = { "Endpoint":{"DemoEndpoint":{ "Enabled":false }}};
            FVT.makeGetRequest( FVT.uriConfigDomain +"Endpoint/DemoEndpoint", FVT.getSuccessCallback, verifyConfig, done)
        });        
// IMPORT will put into Maintenance Mode 4/25
        it('should return status 200 on GET "Status" after Import 3, Maintenance mode', function(done) {
            verifyConfig = JSON.parse( FVT.expectMaintenanceWithDisableObjects) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
		
// Restart and STOP Maintenance Mode...		
        it('should return status 200 POST restart Server after Import 3', function(done) {
            var payload = '{ "Service":"Server", "Maintenance":"stop" }';
//temp            verifyConfig = JSON.parse( FVT.expectAllConfigedStatus ) ;
            verifyConfig = JSON.parse( FVT.expectServerDefault ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Import 3', function(done) {
            this.timeout( FVT.START_HA + 3000 );
            FVT.sleep( FVT.START_HA );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });

// random error thown in for wrong DOMAIN/HTTP ACTION		
        it('should return status 200 on "GET" IMPORT status incorrectly', function(done) {
            verifyConfig = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/import is not valid." }' ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"import", FVT.getFailureCallback, verifyConfig, done)
        });
		

//DO THE Import 4 with Objects Disabled FALSE!
        it('should return status 200 POST "Import" 4', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            var payload = '{ "FileName":"ExportServerConfig", "Password":"ima4test", "ApplyConfig":true, "DisableObjects":false }';
//            verifyConfig = JSON.parse( "Status":200, "Code":"CWLNA6011","Message":"COMMAND COMPELTED SUCCESSFULLY" ) ;
			verifyConfig = {  "Version": "v1",  "ServerName": "'+ FVT.A1_HOSTNAME +'",  "LogLevel": "MIN",  "ConnectionLog": "MAX",  "SecurityLog": "NORMAL",  "AdminLog": "MIN",  "MQConnectivityLog": "MAX",  "TraceMax": "251MB",  "TraceMessageData": 0,  "TraceSelected": "",  "TraceOptions": "time,thread,where",  "TraceConnection": "",  "TraceBackup": 1,  "TraceBackupCount": 3,  "TraceBackupDestination": "",  "FIPS": false,  "SNMPEnabled": true,  "LogLocation": {    "DefaultLog": {      "LocationType": "file",      "Destination": "/var/messagesight/diag/logs/imaserver-default.log"    },    "ConnectionLog": {      "LocationType": "file",      "Destination": "/var/messagesight/diag/logs/imaserver-connection.log"    },    "AdminLog": {      "LocationType": "file",      "Destination": "/var/messagesight/diag/logs/imaserver-admin.log"    },    "SecurityLog": {      "LocationType": "file",      "Destination": "/var/messagesight/diag/logs/imaserver-security.log"    },    "MQConnectivityLog": {      "LocationType": "file",      "Destination": "/var/messagesight/diag/logs/imaserver-mqconnectivity.log"    }  },  "SecurityProfile": {    "ClientCertSecProf": {      "CertificateProfile": "ClientCertCertProf",      "UseClientCipher": false,      "UseClientCertificate": true,      "MinimumProtocolMethod": "TLSv1.1",      "Ciphers": "Medium",      "TLSEnabled": false,      "UsePasswordAuthentication": false,      "OAuthProfile": "",      "LTPAProfile": "",      "CRLProfile": ""    },    "BatchSecProf": {      "CertificateProfile": "BatchCertProf",      "UseClientCipher": false,      "UseClientCertificate": true,      "MinimumProtocolMethod": "TLSv1",      "Ciphers": "Fast",      "TLSEnabled": false,      "UsePasswordAuthentication": false,      "OAuthProfile": "",      "LTPAProfile": "",      "CRLProfile": ""    },    "TrustedCertSecProf": {      "CertificateProfile": "TrustedCertCertProf",      "UseClientCipher": false,      "UseClientCertificate": true,      "MinimumProtocolMethod": "TLSv1.2",      "Ciphers": "Fast",      "TLSEnabled": false,      "UsePasswordAuthentication": false,      "OAuthProfile": "",      "LTPAProfile": "",      "CRLProfile": ""    }  },  "CertificateProfile": {    "BatchCertProf": {      "Certificate": "BatchCert.pem",      "Key": "BatchKey.pem"    },    "ClientCertCertProf": {      "Certificate": "ClientCert.pem",      "Key": "ClientKey.pem"    },    "TrustedCertCertProf": {      "Certificate": "TrustCert.pem",      "Key": "TrustKey.pem"    }  },  "MQConnectivityEnabled": true,  "MessageHub": {    "DemoHub": {      "Description": "Demo Message Hub."    },    "BatchHub": {      "Description": ""    }  },  "Endpoint": {    "DemoEndpoint": {      "Port": 16102,      "Enabled": true,      "Protocol": "All",      "Interface": "All",      "InterfaceName": "All",      "ConnectionPolicies": "DemoConnectionPolicy",      "TopicPolicies": "DemoTopicPolicy",      "SubscriptionPolicies": "DemoSubscriptionPolicy",      "MaxMessageSize": "4096KB",      "MessageHub": "DemoHub",      "EnableAbout": true,      "Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.",      "BatchMessages": true,      "SecurityProfile": "",      "MaxSendSize": 16384,      "QueuePolicies": null    },    "DemoMqttEndpoint": {      "Port": 1883,      "Enabled": true,      "Protocol": "MQTT",      "Interface": "All",      "InterfaceName": "All",      "ConnectionPolicies": "DemoConnectionPolicy",      "TopicPolicies": "DemoTopicPolicy",      "MaxMessageSize": "4096KB",      "MessageHub": "DemoHub",      "EnableAbout": true,      "Description": "Unsecured endpoint for demonstration use with MQTT protocol only. By default, it uses port 1883.",      "SubscriptionPolicies": null,      "BatchMessages": true,      "SecurityProfile": "",      "MaxSendSize": 16384,      "QueuePolicies": null    }  },  "EnableDiskPersistence": true,  "ConfigurationPolicy": {    "AdminDefaultConfigPolicy": {      "Description": "Default configuration policy for AdminEndpoint",      "ClientAddress": "*",      "ActionList": "Configure,View,Monitor,Manage"    },    "BatchConfigPolicy": {      "ActionList": "Configure,View,Monitor,Manage",      "ClientAddress": "*",      "Description": "Batch policy for AdminEndpoint",      "CommonNames": "",      "UserID": "",      "GroupID": ""    }  },  "ConnectionPolicy": {    "DemoConnectionPolicy": {      "Description": "Demo connection policy",      "ClientID": "*"    }  },  "TopicPolicy": {    "DemoTopicPolicy": {      "Description": "Demo topic policy",      "Topic": "*",      "ClientID": "*",      "ActionList": "Publish,Subscribe",      "MaxMessages": 5000,      "MaxMessagesBehavior": "RejectNewMessages",      "MaxMessageTimeToLive": "unlimited",      "DisconnectedClientNotification": false,      "CommonNames": "",      "Protocol": "",      "ClientAddress": "",      "UserID": "",      "GroupID": ""    }  },  "SubscriptionPolicy": {    "DemoSubscriptionPolicy": {      "Description": "Demo policy for shared durable subscription",      "Subscription": "*",      "Protocol": "JMS,MQTT",      "ActionList": "Receive,Control",      "MaxMessages": 5000,      "MaxMessagesBehavior": "RejectNewMessages",      "ClientID": "",      "ClientAddress": "",      "CommonNames": "",      "UserID": "",      "GroupID": ""    }  },  "PluginServer": "127.0.0.1",  "PluginPort": 3019,  "PluginDebugServer": "",  "PluginDebugPort": 0,  "PluginMaxHeapSize": 2048,  "PluginVMArgs": "-Xms2G -Xmx4G -Xss1024m",  "TopicMonitor": [],  "LTPAProfile": {    "BatchLTPAProf": {      "KeyFileName": "LTPAKeyfile",      "Password": "c90ed5e7af84c020c515c1f86ec9e10b"    }  },  "OAuthProfile": {    "BatchOAuthProf": {      "KeyFileName": "OAuthKeyfile",      "ResourceURL": "http://oauth.com",      "GroupInfoKey": "",      "AuthKey": "access_token",      "UserInfoURL": null,      "UserInfoKey": ""    }  },  "QueuePolicy": {    "BatchQueuePol": {      "ActionList": "Send,Receive,Browse",      "Queue": "BatchQueue/*",      "ClientID": "'+ FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 +'",      "Description": "",      "CommonNames": "",      "Protocol": "",      "ClientAddress": "",      "UserID": "",      "GroupID": "",      "MaxMessageTimeToLive": "unlimited"    }  },  "Queue": {    "BatchQueue": {      "Description": "",      "AllowSend": true,      "ConcurrentConsumers": true,      "MaxMessages": 5000    }  },  "QueueManagerConnection": {    "BatchQmgr": {      "ConnectionName": "'+ FVT.MQSERVER1_IP +'(1416)",      "ChannelName": "CHNLJMS",      "QueueManagerName": "QM_MQJMS",      "ChannelUserName": "",      "Verify": false,      "SSLCipherSpec": "",      "Force": false,      "ChannelUserPassword": ""    }  },  "DestinationMappingRule": {    "BatchDMapRule": {      "Source": "/ms_topic",      "Destination": "MQ_QUEUE",      "MaxMessages": 5000,      "Enabled": false,      "RuleType": 1,      "RetainedMessages": "None",      "QueueManagerConnection": "BatchQmgr"    }  },  "HighAvailability": {    "EnableHA": false,    "Group": "FVT_HAPair",    "RemoteDiscoveryNIC": "",    "StartupMode": "AutoDetect",    "HeartbeatTimeout": 10,    "PreferredPrimary": false,    "LocalDiscoveryNIC": "",    "DiscoveryTimeout": 600,    "LocalReplicationNIC": "",    "ExternalReplicationNIC": "",    "ExternalReplicationPort": 0,    "ReplicationPort": 0,    "RemoteDiscoveryPort": 0,    "UseSecuredConnections": false  },  "LDAP": {    "NestedGroupSearch": false,    "CacheTimeout": 10,    "MaxConnections": 100,    "Certificate": "ldap.pem",    "BindDN": "cn=root,o=IBM,c=US",    "BindPassword": "0207633694fab658610905ec0202e315",    "Enabled": false,    "EnableCache": true,    "GroupCacheTimeout": 300,    "URL": "ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'",    "Timeout": 30,    "UserSuffix": "ou=users,ou=SVT,o=IBM,c=US",    "GroupIdMap": "cn",    "GroupMemberIdMap": "member",    "UserIdMap": "uid",    "BaseDN": "ou=SVT,o=IBM,c=US",    "GroupSuffix": "ou=groups,ou=SVT,o=IBM,c=US",    "IgnoreCase": true,    "Verify": false  },  "ClusterMembership": {    "MessagingAddress": null,    "MulticastDiscoveryTTL": 1,    "EnableClusterMembership": false,    "ClusterName": "",    "UseMulticastDiscovery": true,    "DiscoveryServerList": null,    "DiscoveryTime": 10,    "MessagingPort": 9105,    "ControlPort": 9104,    "ControlAddress": null,    "MessagingUseTLS": false,    "DiscoveryPort": 9106,    "ControlExternalAddress": null,    "ControlExternalPort": null,    "MessagingExternalPort": null,    "MessagingExternalAddress": null  },  "Syslog": {    "Host": "127.0.0.1",    "Enabled": false,    "Port": 415,    "Protocol": "udp"  }} ;
            FVT.makePostRequest( FVT.uriServiceDomain+"import", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET DemoEndpoint, verify Enabled Import 4 ', function(done) {
		    verifyConfig = { "Endpoint":{"DemoEndpoint":{ "Enabled":true }}};
            FVT.makeGetRequest( FVT.uriConfigDomain +"Endpoint/DemoEndpoint", FVT.getSuccessCallback, verifyConfig, done)
        });
// IMPORT will put into Maintenance Mode 4/25
        it('should return status 200 on GET "Status" after Import 4, Maintenance mode', function(done) {
            verifyConfig = JSON.parse( FVT.expectMaintenanceWithServices ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
		
// Restart and STOP Maintenance Mode...		
        it('should return status 200 POST restart Server after Import 4', function(done) {
            var payload = '{ "Service":"Server", "Maintenance":"stop" }';
//temp            verifyConfig = JSON.parse( FVT.expectAllConfigedStatus ) ;
            verifyConfig = JSON.parse( FVT.expectHAFailedNICsStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Import 4', function(done) {
            this.timeout( FVT.START_HA + 3000 );
            FVT.sleep( FVT.START_HA );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
    });	


// 2. Verify GET Updated Config #1  
    for( i=0 ; i < verifyExportImportENUM.length ; i++ ) {
        theObject = verifyExportImportENUM[i];
        getConfig( i, 'GET IMPORTED Config#1:', 'config' )
    };


// 3. RESTART Server
    describe('RestartImportedServerCfg:', function() {
    
        it('should return status 200 POST "Restart Server" get updated config', function(done) {
            var payload = '{ "Service":"Server" }';
            verifyConfig = JSON.parse( FVT.expectHAFailedNICsStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Update Config 1', function(done) {
            this.timeout( FVT.START_HA + 3000 );
            FVT.sleep( FVT.START_HA );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// 4. Verify GET Updated Config #2 restart  
    for( i=0 ; i < verifyExportImportENUM.length ; i++ ) {
        theObject = verifyExportImportENUM[i];
        getConfig( i, 'GET Updated Config after Restart:', 'config' )
    };



// 5. RESET CONFIG
    describe('ResetConfig:', function() {
    
        it('should return status 200 POST "Reset Config"', function(done) {
            var payload = '{ "Service":"Server","Reset":"config" }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart to reset config', function(done) {
            this.timeout( FVT.RESETCONFIG + 5000 );
            FVT.sleep( FVT.RESETCONFIG );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// 6. GET Default Config after RESET
    for( i=0 ; i < verifyExportImportENUM.length ; i++ ) {
        theObject = verifyExportImportENUM[i];
        getConfig( i, 'GET Default Config after RESET:', 'default' )
    };


// 7. RESTART Server
    describe('RestartServerResetCfg:', function() {
    
        it('should return status 200 POST "Restart Server" for defaults', function(done) {
            var payload = '{ "Service":"Server" }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Default 1', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
// License is NOT RESET, must do manually
    
        it('should return status 200 RESET License to default', function(done) {
            var payload = '{ "LicensedUsage":null, "Accept":true }';
            verifyConfig = JSON.parse( defaultLicensedUsage ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RESET License to default', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+"LicensedUsage", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET "Status" after RESET License to default', function(done) {
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// 8. GET Default Config after RESET/RESTART
    for( i=0 ; i < verifyExportImportENUM.length ; i++ ) {
        theObject = verifyExportImportENUM[i];
        getConfig( i, 'GET Default Config RESET/RESTART:', 'default' )
    };


});
