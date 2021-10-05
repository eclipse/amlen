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
var defaultClusterMembership = '{ "Version": "' + FVT.version + '", \
  "ClusterMembership":{"ClusterName":"","ControlAddress":null,"ControlExternalAddress":null,"ControlExternalPort":null,"ControlPort":9104,"DiscoveryPort":9106, "DiscoveryServerList":null,"DiscoveryTime":10, \
  "EnableClusterMembership":false,"MessagingAddress":null,"MessagingExternalAddress":null,"MessagingExternalPort":null,"MessagingPort":9105,"MessagingUseTLS":false,"MulticastDiscoveryTTL":1,"UseMulticastDiscovery":true}}';

var defaultHighAvailability = '{ "Version": "' + FVT.version + '", \
  "HighAvailability": {"DiscoveryTimeout": 600,"EnableHA": false,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "","HeartbeatTimeout": 10, \
  "LocalDiscoveryNIC": "","LocalReplicationNIC": "","PreferredPrimary": false,"RemoteDiscoveryNIC": "","RemoteDiscoveryPort": 0,"ReplicationPort": 0,"StartupMode": "AutoDetect", \
  "UseSecuredConnections": false}}';

var defaultClientSet =   '{ "Version": "' + FVT.version + '", "ClientSet": {} }';

var defaultLicensedUsage =   '{ "Version": "' + FVT.version + '", "LicensedUsage": "Production", "Accept":true  }' ;
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

var defaultEndpoint = '{ "Version": "' + FVT.version + '", \
  "Endpoint": { \
    "DemoEndpoint": {"ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.", \
        "EnableAbout": true,"Enabled": false,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 16102,"Protocol": "All", \
        "SubscriptionPolicies": "DemoSubscriptionPolicy","TopicPolicies": "DemoTopicPolicy"}, \
    "DemoMqttEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy","Description": "Unsecured endpoint for demonstration use with MQTT protocol only. By default, it uses port 1883.", \
        "EnableAbout": true,"Enabled": false,"Interface": "All","InterfaceName": "All","MaxMessageSize": "4096KB", "MessageHub": "DemoHub","Port": 1883,"Protocol": "MQTT", \
        "TopicPolicies": "DemoTopicPolicy"}}}';

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
  
  
// Logging Items  -----------------------------------------------------------------------------------------------------
var configAdminLog = ' "AdminLog": "MIN" ';
var configConnectionLog = ' "ConnectionLog": "MAX" ';
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
if ( FVT.A_COUNT > 1 ) {
  var configHighAvailability = ' "HighAvailability": \
    {"DiscoveryTimeout": 600,"EnableHA": true,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "FVT_HAPair","HeartbeatTimeout": 10,"LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'",\
    "LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'","PreferredPrimary": false,"RemoteDiscoveryNIC": "'+ FVT.A2_IPv4_HA0 +'","RemoteDiscoveryPort": 0,"ReplicationPort": 0, \
    "StartupMode": "StandAlone","UseSecuredConnections": false }';
} else {
  var configHighAvailability = ' "HighAvailability": \
    {"DiscoveryTimeout": 600,"EnableHA": true,"ExternalReplicationNIC": "","ExternalReplicationPort": 0,"Group": "FVT_HAPair","HeartbeatTimeout": 10,"LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'",\
    "LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'","PreferredPrimary": false,"RemoteDiscoveryNIC": "'+ FVT.M1_HOST +'","RemoteDiscoveryPort": 0,"ReplicationPort": 0, \
    "StartupMode": "StandAlone","UseSecuredConnections": false }';
}
var configClientSet =   ' "ClientSet": {} ';

var configLicensedUsage =   ' "LicensedUsage": "Non-Production", "Accept":true  ' ;
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
//  iterate[i] thru config item list appending a prefix [default|config] to the verifyEnum[i] string,
//  then get the value of the compound string via  eval( <prefix> + verifyEnum[i] )
// NOTE:  config has the outter {}'s stripped so they can be built into batches easier, 
//  need to be added back when retrieve individual values
// ----------------------------------------------------------------------------------------------------- 
// LicensedUsage is problem in BATCH, has to handle seperately 
var verifyENUM = [ "AdminUserID", "AdminEndpoint", "ConfigurationPolicy", "CertificateProfile", "SecurityProfile", "ClientCertificate", 
   "TrustedCertificate", "LTPAProfile", "OAuthProfile", "LDAP", "AdminLog", "ConnectionLog", "LogLevel", "TraceLevel", "TraceMax", 
   "TraceMessageData", "Syslog", "LogLocation", "ClusterMembership", "HighAvailability", "LicensedUsage", "ServerName", "SNMPEnabled", 
   "MQConnectivityEnabled", "MQConnectivityLog", "QueueManagerConnection", "DestinationMappingRule", "MessageHub", "TopicMonitor", "TopicPolicy", 
   "SubscriptionPolicy", "Endpoint", "Queue", "QueuePolicy", "Protocol", "Plugin", "PluginServer", "PluginPort", 
   "PluginDebugServer", "PluginDebugPort", "PluginMaxHeapSize" ]; 


var enableSecurityConfig = '{'+ configConfigurationPolicy +', '+ configCertificateProfile +', '+ configSecurityProfile +', '+ updateAdminEndpoint +', '+ configAdminUserID +', '+ configClientCertificate +', '+ configTrustedCertificate +', '+ updateLTPAProfile +', '+ configOAuthProfile +', '+ updateLDAP +'}';
// (temp subset) 134436 var enableSecurityConfig = '{'+ configConfigurationPolicy +', '+ configCertificateProfile +', '+ configSecurityProfile +', '+ updateAdminEndpoint +', '+ configAdminUserID +', '+ configClientCertificate +', '+ configTrustedCertificate +', '+ updateLTPAProfile +', '+ configOAuthProfile +'}';

var enableLoggingConfig = '{'+ configAdminLog +', '+ configConnectionLog +', '+ configLogLevel +', '+ configTraceLevel +', '+ configTraceMax +', '+ configTraceMessageData +', '+ updateSyslog +', '+ configLogLocation +'}';
// (temp subset) 134556 - having to spilt to work around a MOCHA error parsing
var enableLoggingConfig_1 = '{'+ configAdminLog +', '+ configConnectionLog +', '+ configLogLevel +', '+ configTraceLevel +', '+ configTraceMax +', '+ configTraceMessageData +', '+ configLogLocation +'}';
// updateSyslog seems to fail in MOCHA consistently, yet is successful in CURL.  
var enableLoggingConfig_2 = '{'+ updateSyslog +'}';

// configSNMP and configLicensedUsage done separately
//var enableServerConfig = '{'+ configServerName +', '+ configClusterMembership +', '+ configHighAvailability +', '+ configClientSet +'}';
var enableServerConfig = '{'+ configServerName +', '+ configClusterMembership +', '+ configHighAvailability   +'}';

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
        var object = verifyENUM[i];
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
// 1. Post the Updated Config.
// 2. Verify GET Updated Config
// 3. RESTART Server   (Janet's addition, verify the CONFIG got replicated/persisted all though the Server before you blow it away...)
// 4. Verify GET Updated Config
// 5. Config Reset
// 6. Verify GET Default Config
// 7. RESTART Server  
// 8. Verify GET Default Config

describe('ResetConfig:', function() {
//this.timeout( FVT.defaultTimeout );
this.timeout( 15000 );

// GET Default Config
    for( i=0 ; i < verifyENUM.length ; i++ ) {
        theObject = verifyENUM[i];
        getConfig( i, 'GET Initial Config:', 'default' )
    };


// Upload Certs 
    describe('PUT Certs:', function() {

    
        it('should return status 200 on PUT "BatchCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'BatchCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "BatchKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'BatchKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "TrustedCertificate.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'TrustedCertificate.pem' ;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on PUT "TrustCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'TrustCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "TrustKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'TrustKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "ClientCertificate.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'ClientCertificate.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on PUT "ClientCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'ClientCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "ClientKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'ClientKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LDAPCert.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'LDAPCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "OAuthKeyfile"', function(done) {
            sourceFile = 'mar400.wasltpa.keyfile';
            targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LTPAKeyfile"', function(done) {
            sourceFile = 'mar400.wasltpa.keyfile';
            targetFile = 'LTPAKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

    });


    
// POST UPDATE CONFIG
    describe('POST Updated Config:', function() {
    // Update LicensedUsage outside of the Batch Post
        it('should return status 200 POST LicensedUsage:Non-Production', function(done) {
            var payload =  '{ '+ configLicensedUsage +'}' ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET LicensedUsage:Non-Production ', function(done) {
		    this.timeout( FVT.REBOOT + 2000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain +"LicensedUsage", FVT.getSuccessCallback, verifyConfig, done)
        });        
        it('should return status 200 GET Server Status ', function(done) {
            verifyConfig = JSON.parse( FVT.expectServerDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain +"status/Server", FVT.getSuccessCallback, verifyConfig, done)
        });
	// Update LicensedUsage outside of the Batch Post

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
    // Update SNMP outside of the Batch Post

    // Update with the Batch Post
        postUpdatedConfig();
		
	// NOW have to restart the server to get Cluster Config to Start
        it('should return status 200 POST to Restart for Cluster and HA', function(done) {
            var payload =  '{"Service":"Server", "CleanStore":true}' ;
            verifyConfig = JSON.parse( FVT.expectAllConfigedStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after Restart for Cluster and HA', function(done) {
            this.timeout( FVT.START_HA + 3000 );
            FVT.sleep( FVT.START_HA );
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        
    });

// GET Update CONFIG #1  
    for( i=0 ; i < verifyENUM.length ; i++ ) {
        theObject = verifyENUM[i];
        getConfig( i, 'GET Updated Config1:', 'config' )
    };


// RESTART Server
    describe('RestartServerUpdate:', function() {
    
        it('should return status 200 POST "Restart Server" get updated config', function(done) {
            var payload = '{ "Service":"Server" }';
            verifyConfig = JSON.parse( FVT.expectAllConfigedStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Update Config 1', function(done) {
            this.timeout( FVT.START_HA + 3000 );
            FVT.sleep( FVT.START_HA );
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// GET Update CONFIG #2  
    for( i=0 ; i < verifyENUM.length ; i++ ) {
        theObject = verifyENUM[i];
        getConfig( i, 'GET Updated Config after Restart:', 'config' )
    };



// RESET CONFIG
    describe('ResetConfig:', function() {
    
        it('should return status 200 POST "Reset Config"', function(done) {
            var payload = '{ "Service":"Server","Reset":"config" }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart to reset config', function(done) {
            this.timeout( FVT.RESETCONFIG + 5000 );
            FVT.sleep( FVT.RESETCONFIG );
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
// LicensedUsage IS NOT reset, have to manually reset it...
        it('should return status 200 POST "LicensedUsage" to default', function(done) {
            var payload = defaultLicensedUsage;
            verifyConfig = JSON.parse( defaultLicensedUsage ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "LicensedUsage', function(done) {
            this.timeout( FVT.REBOOT + 5000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+"LicensedUsage", FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// GET Default Config after RESET
    for( i=0 ; i < verifyENUM.length ; i++ ) {
        theObject = verifyENUM[i];
        getConfig( i, 'GET Default Config after RESET:', 'default' )
    };


// RESTART Server
    describe('RestartServerDefault:', function() {
    
        it('should return status 200 POST "Restart Server" for defaults', function(done) {
            var payload = '{ "Service":"Server" }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Status" after restart after Default 1', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


// GET Default Config after RESET
    for( i=0 ; i < verifyENUM.length ; i++ ) {
        theObject = verifyENUM[i];
        getConfig( i, 'GET Default Config RESET/RESTART:', 'default' )
    };


});
