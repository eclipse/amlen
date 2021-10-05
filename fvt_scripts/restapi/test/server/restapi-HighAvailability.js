/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */
var assert = require('assert')
var request = require('supertest')
var should = require('should')

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'HighAvailability/';
var expectDefault = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": false, "Group":"", "HeartbeatTimeout": 10, "PreferredPrimary": false, "StartupMode": "AutoDetect", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "", "LocalReplicationNIC": "", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, "RemoteDiscoveryNIC": "", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 


var expectA1EnabledHostnameLocalDiscoNIC = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": true, "Group":"FVT_HAPair", "HeartbeatTimeout": 10, "PreferredPrimary": true, "StartupMode": "StandAlone", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "'+ FVT.A1_HOSTNAME_OS +'", "LocalReplicationNIC": "'+ FVT.A1_HOSTNAME_OS +'", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, \
		"RemoteDiscoveryNIC": "'+ FVT.A2_HOSTNAME_OS +'", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 

var expectA1EnabledHostnameLocalRepNIC = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": true, "Group":"FVT_HAPair", "HeartbeatTimeout": 10, "PreferredPrimary": true, "StartupMode": "StandAlone", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC": "'+ FVT.A1_HOSTNAME_OS +'", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, \
		"RemoteDiscoveryNIC": "'+ FVT.A2_HOSTNAME_OS +'", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 

var expectA1EnabledHostnameRemoteDiscoNIC = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": true, "Group":"FVT_HAPair", "HeartbeatTimeout": 10, "PreferredPrimary": true, "StartupMode": "StandAlone", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, \
		"RemoteDiscoveryNIC": "'+ FVT.A2_HOSTNAME_OS +'", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 

var expectA1Enabled = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": true, "Group":"FVT_HAPair", "HeartbeatTimeout": 10, "PreferredPrimary": true, "StartupMode": "StandAlone", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC": "'+ FVT.A1_IPv4_HA1 +'", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, \
		"RemoteDiscoveryNIC": "'+ FVT.A2_IPv4_HA0 +'", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 

var expectA2Enabled = '{"HighAvailability": {"DiscoveryTimeout": 600, "EnableHA": true, "Group":"FVT_HAPair", "HeartbeatTimeout": 10, "PreferredPrimary": false, "StartupMode": "AutoDetect", "UseSecuredConnections": false, \
        "LocalDiscoveryNIC": "'+ FVT.A2_IPv4_HA0 +'", "LocalReplicationNIC": "'+ FVT.A2_IPv4_HA1 +'", "ReplicationPort": 0, "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0, \
		"RemoteDiscoveryNIC": "'+ FVT.A1_IPv4_HA0 +'", "RemoteDiscoveryPort": 0 }, \
        "Version": "'+ FVT.version +'"}' ; 

var verifyConfig = {};
var verifyMessage = {};
/*
var long65535_ipV4_String = "";
var long65535_ipV6_String = "";

// Build DiscoveryServerList for IPv4 
for ( ip4=1 ; ip4 < 256 ; ip4++ ) {
    id3 = ( '00' + ip4).slice(-3);
    if ( long65535_ipV4_String.length == 0 ) {
        long65535_ipV4_String = 'server'+ id3 +'@111.111.111.' + ip4 +':9102';
        controlAddressLen = long65535_ipV4_String.length + 1;
        FVT.trace( 3, "item.length : "+ controlAddressLen );
    } else if (long65535_ipV4_String.length+controlAddressLen  <= 65535 ) {
        long65535_ipV4_String = long65535_ipV4_String +',server'+ id3 +'@111.111.111.' + ip4 +':9102' ;
        FVT.trace( 3, id3 +"  SIZEOF long65535_ipV4_String: "+ long65535_ipV4_String.length );
    } else {
        FVT.trace( 3, "MAXED  ==> long65535_ipV4_String: "+ long65535_ipV4_String );
    }
}
    FVT.trace( 2, "Final long65535_ipV4_String LENGTH:"+ long65535_ipV4_String.length +"  Value: "+ long65535_ipV4_String );

// Build DiscoveryServerList for IPv6 
for ( ip6= 0x0000 ; ip6 <= 0xffff ; ip6++ ) {
    id4 = ( '000' + ip6).slice(-4);
    if ( long65535_ipV6_String.length == 0 ) {
        long65535_ipV6_String = 'server'+ id4 +'@[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:9102';
        controlAddressLen = long65535_ipV6_String.length + 1;
        FVT.trace( 3, "item.length : "+ controlAddressLen );
    } else if (long65535_ipV6_String.length+controlAddressLen  <= 65535 ) {
        long65535_ipV6_String = long65535_ipV6_String +',server'+ id4 +'@[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:9102' ;
        FVT.trace( 3, id4 +"  SIZEOF long65535_ipV6_String: "+ long65535_ipV6_String.length );
    } else {
        FVT.trace( 3, "MAXED  ==> long65535_ipV6_String: "+ long65535_ipV6_String );
    }
}
    FVT.trace( 2, "Final long65535_ipV6_String LENGTH:"+ long65535_ipV6_String.length +"  Value: "+ long65535_ipV6_String );
*/ 
// Test Cases Start Here

// HighAvailability test cases
//       "ListObjects":"EnableHA,StartupMode,Group,RemoteDiscoveryNIC,LocalReplicationNIC,LocalDiscoveryNIC,DiscoveryTimeout,HeartbeatTimeout,PreferredPrimary",
// added:  "ExternalReplicationNIC": "",  "ExternalReplicationPort": 0,  "ReplicationPort": 0,  "RemoteDiscoveryPort": 0,  "UseSecuredConnections": false

describe('HighAvailability:', function() {
this.timeout( FVT.defaultTimeout );

    // Pristine Get
    describe('Pristine Get:', function() {
    
        it('should return status 200 on get, Expected DEFAULT "HighAvailability"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
	
//  ====================   Required Group and NICs ====================  //
    describe('MinimalConfig:', function() {
// If EnableHA is true, must specify a GROUP name  + 04/2016 AND NOW the three NICs are also required....
// use to be valid with no NICs...
        it('should return status 400 on POST with EnableHA:true, Group, yet no NICs', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true,"Group":"haconfig" }}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: \"\"." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \"\"." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableHA:true, Group, yet no NICs fails', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true without local Disco NIC', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true,"Group":"haconfig", "RemoteDiscoveryNIC":"9.9.9.9", "LocalReplicationNIC":"9.9.9.9"}}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: LocalDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableHA:true without local Disco NIC', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true without local Relication NIC', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true,"Group":"haconfig", "RemoteDiscoveryNIC":"9.9.9.9", "LocalDiscoveryNIC":"9.9.9.9" }}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: LocalReplicationNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableHA:true without local Relication NIC', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true without remote Disco NIC', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true,"Group":"haconfig", "LocalDiscoveryNIC":"9.9.9.9", "LocalReplicationNIC":"9.9.9.9"}}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableHA:true without remote Disco NIC', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true without Group', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "RemoteDiscoveryNIC":"9.9.9.9", "LocalDiscoveryNIC":"9.9.9.9", "LocalReplicationNIC":"9.9.9.9"}}';
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: Group Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableHA:true without Group', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

//  ====================   EnableHA: Specify if High Availability is enabled. (Required) ====================  //
    describe('EnableHA[F]:', function() {

        it('should return status 200 on POST with EnableHA:true with Groups and NICs', function(done) {
            var payload = expectA1Enabled ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET EnableHA:true with Groups and NICs', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 100977
        it('should return status 200 on POST with EnableHA:null (false)', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":null,"Group":null}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":null}}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  after RESET of EnableHA:null (false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with EnableHA:true', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true,"Group":"haconfig"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET EnableHA:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with EnableHA:false', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":false,"Group":""}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET EnableHA:false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// NICs can be NULL if EnableHA:false
        it('should return status 200 on POST with EnableHA:null (false) and NICs:null', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":null, "Group":null, "LocalDiscoveryNIC":null, "LocalReplicationNIC":null, "RemoteDiscoveryNIC":null }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":null, "LocalDiscoveryNIC":null, "LocalReplicationNIC":null, "RemoteDiscoveryNIC":null }}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  after RESET of EnableHA:null (false) and NICs:null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// NICs can be "" if EnableHA:false
        it('should return status 200 on POST with EnableHA:false and NICs:""', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  after RESET of EnableHA:false and NICs:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
// NIC can not be "" or NULL, must have value if enabled
        it('should return status 400 on POST with EnableHA:true and LocalDiscoveryNIC:""', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoEmptyString", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and LocalDiscoveryNIC:"" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true and LocalDiscoveryNIC:null', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoNull", "LocalDiscoveryNIC":null, "LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: NULL." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalDiscoveryNIC Value: null." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and LocalDiscoveryNIC:null fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

// NIC can not be "" or NULL, must have value if enabled
        it('should return status 400 on POST with EnableHA:true and LocalReplicationNIC:""', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoEmptyString", "LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalReplicationNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalReplicationNIC Value: \"\"." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalReplicationNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and LocalReplicationNIC:"" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true and LocalReplicationNIC:null', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoNull", "LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC":null, "RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalReplicationNIC Value: NULL." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalReplicationNIC Value: null." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalReplicationNIC Value: NULL." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and LocalReplicationNIC:null fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

// NIC can not be "" or NULL, must have value if enabled
        it('should return status 400 on POST with EnableHA:true and RemoteDiscoveryNIC:""', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoEmptyString", "LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "RemoteDiscoveryNIC":"" }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: \"\"." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and RemoteDiscoveryNIC:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with EnableHA:true and RemoteDiscoveryNIC:null', function(done) {
            var payload = '{"HighAvailability":{"EnableHA":true, "Group":"NicNoNull", "LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'", "LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "RemoteDiscoveryNIC":null }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"EnableHA":false, "Group":"InactiveHA", "LocalDiscoveryNIC":"", "LocalReplicationNIC":"", "RemoteDiscoveryNIC":"" }}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: NULL." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: null." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: NULL." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after EnableHA:true and RemoteDiscoveryNIC:null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    
        it('should return status 200 on POST with EnableHA:true', function(done) {
            var payload = expectA1Enabled ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET EnableHA:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    });
    
    
//  ====================   StartupMode: The high availability mode at which the node will start (Required) ====================  //
    describe('StartupMode:[AutoDetect,StandAlone]:', function() {

        it('should return status 200 on POST with StartupMode:StandAlone', function(done) {
            var payload = '{"HighAvailability":{"StartupMode":"StandAlone"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET of StartupMode:StandAlone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 100990
        it('should return status 200 on POST with StartupMode:null (AutoDetect)', function(done) {
            var payload = '{"HighAvailability":{"StartupMode":null}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"StartupMode":"AutoDetect"}}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  after RESET of StartupMode:null (AutoDetect)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with StartupMode:StandAlone', function(done) {
            var payload = '{"HighAvailability":{"StartupMode":"StandAlone"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET of StartupMode:StandAlone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		
    // Config persists Check
        it('should return status 400 POST restart HighAvailability only', function(done) {
            var payload = '{"Service":"HighAvailability"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Service Value: HighAvailability."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Service:Server', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
			var verifyStatus = JSON.parse( FVT.expectHARunningStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "StartupMode:StandAlone" persists', function(done) {
            verifyConfig = {"HighAvailability":{"StartupMode":"StandAlone"}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// LEAVE AT StandAlone for expectA!Enabled default
//        it('should return status 200 on POST with StartupMode:AutoDetect', function(done) {
//            var payload = '{"HighAvailability":{"StartupMode":"AutoDetect"}}';
//            verifyConfig = JSON.parse( payload ) ;
//            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
//        it('should return status 200 on GET  after RESET of StartupMode:AutoDetect', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//        });

    });


//  ====================  RemoteDiscoveryNIC: The discovery interface on the remote node in the HA Pair.  ====================  //
    describe('RemoteDiscoveryNIC[IPAddress]REQUIRED:', function() { 

        it('should return status 200 on POST with RemoteDiscoveryNIC:ipv4', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with RemoteDiscoveryNIC:"" (NOT VALID When Enabled)', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"", "EnableHA":true }}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'"}}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: \"\"." } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: RemoteDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:"" (NOT VALID)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with RemoteDiscoveryNIC:ipv6', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv6_HA0 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:ipv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with RemoteDiscoveryNIC:null (Reset to Default)', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":null, "EnableHA":false }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:null (Reset to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    

        it('should return status 200 on POST with RemoteDiscoveryNIC:ipv4', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +'", "EnableHA":true }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
 
// RESET to enabled value
        it('should return status 200 on POST with RemoteDiscoveryNIC:A2_IPv4_HA1', function(done) {
            var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA1 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET RemoteDiscoveryNIC:A2_IPv4_HA1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

//  ====================  LocalReplicationNIC: IPv4 or IPv6 address of the replication interface  ====================  //
    describe('LocalReplicationNIC[IPAddress]REQUIRED:', function() { 

        it('should return status 200 on POST with LocalReplicationNIC:ipv4', function(done) {
            var payload = '{"HighAvailability":{"LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "StartupMode":"StandAlone", "PreferredPrimary":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalReplicationNIC:ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
// Local Nics can not be same adapter unless server.cfg is manually updated to ALLOWSINGLENIC  IF I SET IT, does this go successful in Production?
// Added StartupMode and PreferredPrimary to avoid StoreStarting forever ... finally just changed the expectA1Enabled to default to those values
        it('should return status 200 on POST Service Restart in Maintenance', function(done) {
            var payload = '{"Service":"Server", "CleanStore":true }' ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET Service Status NOT in Maintenance WITH HA.AllowSingleNIC=1', function(done) {
    		this.timeout( FVT.START_HA + 3000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
// NO HA.AllowSingleNIC=1 in server.cfg
//		    verifyConfig = JSON.parse( '{ "Server": {  "Status": "Running",  "State": 9,  "StateDescription": "Running (maintenance)",  "ErrorCode": 510,  "ErrorMessage": "An HA configuration parameter is not valid: parameter name=HA.LocalDiscoveryNIC value='+ FVT.A1_IPv4_HA1 +'." }}'  );
		    verifyConfig = JSON.parse( '{ "Server": {  "Status": "Running",  "State": 1,  "StateDescription": "Running (production)",  "ErrorCode": 0,  "ErrorMessage": "" }}'  );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done)
        });
    		

        it('should return status 400 on POST with LocalReplicationNIC:"" (NOT VALID)', function(done) {
            var payload = '{"HighAvailability":{"LocalReplicationNIC":""}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'"}}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalReplicationNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalReplicationNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET LocalReplicationNIC:"" (NOT VALID)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with LocalReplicationNIC:ipv6', function(done) {
            var payload = '{"HighAvailability":{"LocalReplicationNIC":"'+ FVT.A1_IPv6_HA1 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalReplicationNIC:ipv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with LocalReplicationNIC:null (Reset to Default)', function(done) {
            var payload = '{"HighAvailability":{"LocalReplicationNIC":null, "EnableHA":false }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalReplicationNIC:null (Reset to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
 
// RESET to enabled value
        it('should return status 200 on POST with LocalReplicationNIC:A1_IPv6_HA1', function(done) {
            var payload = '{"HighAvailability":{"LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +'", "EnableHA":true }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalReplicationNIC:A1_IPv6_HA1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
   
        it('should return status 200 on POST Service Restart out of Maintenance', function(done) {
            var payload = '{"Service":"Server", "CleanStore":true }' ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET Service Status out of Maintenance', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
		    verifyConfig = JSON.parse( FVT.expectHARunningStatus );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
       
    });

//  ====================  LocalDiscoveryNIC: IPv4 or IPv6 address of the discovery interface  ====================  //
    describe('LocalDiscoveryNIC[IPAddress]REQUIRED:', function() { 

        it('should return status 200 on POST with LocalDiscoveryNIC:ipv4', function(done) {
            var payload = '{"HighAvailability":{"LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalDiscoveryNIC:ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with LocalDiscoveryNIC:"" (NOT VALID)', function(done) {
            var payload = '{"HighAvailability":{"LocalDiscoveryNIC":""}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'"}}' ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \"\"." } ;
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: LocalDiscoveryNIC Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET LocalDiscoveryNIC:"" (NOT VALID)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with LocalDiscoveryNIC:ipv6', function(done) {
            var payload = '{"HighAvailability":{"LocalDiscoveryNIC":"'+ FVT.A1_IPv6_HA0 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalDiscoveryNIC:ipv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with LocalDiscoveryNIC:null (Reset to Default)', function(done) {
            var payload = '{"HighAvailability":{"LocalDiscoveryNIC":null, "EnableHA":false }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalDiscoveryNIC:null (Reset to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

// RESET to enabled value
        it('should return status 200 on POST with LocalDiscoveryNIC:A1_IPv4_HA0', function(done) {
            var payload = '{"HighAvailability":{"LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +'", "EnableHA":true }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET LocalDiscoveryNIC:A1_IPv4_HA0', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});

//  ====================  DiscoveryTimeout: Time in seconds the node will attempt to discover a HA-Pair node. (Required) ====================  //
    describe('DiscoveryTimeout[10-600-2147483647]:', function() { 

        it('should return status 200 on POST with DiscoveryTimeout:10 (MIN)', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":10}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:10 (MIN)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with DiscoveryTimeout:11', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":11}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:11', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with DiscoveryTimeout:null (DEFAULT)', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":null}}';
            verifyConfig = {"HighAvailability":{"DiscoveryTimeout":600}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with DiscoveryTimeout:2147483647 (MAX)', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":2147483647}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:2147483647 (MAX)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with DiscoveryTimeout:2147483646', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":2147483646}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:2147483646', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with DiscoveryTimeout:600', function(done) {
            var payload = '{"HighAvailability":{"DiscoveryTimeout":600}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryTimeout:600', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

//  ====================  HeartbeatTimeout: Time in seconds to detect that the other node in the HA-Pair has failed. (Required) ====================  //
    describe('HeartbeatTimeout[1-10-2147483647]:', function() { 

        it('should return status 200 on POST with HeartbeatTimeout:1 (MIN)', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:1 (MIN)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with HeartbeatTimeout:2', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with HeartbeatTimeout:null (DEFAULT)', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":null}}';
            verifyConfig = {"HighAvailability":{"HeartbeatTimeout":10}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with HeartbeatTimeout:2147483647 (MAX)', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":2147483647}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:2147483647 (MAX)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with HeartbeatTimeout:2147483646', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":2147483646}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:2147483646', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with HeartbeatTimeout:10', function(done) {
            var payload = '{"HighAvailability":{"HeartbeatTimeout":10}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET HeartbeatTimeout:10', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    });

//  ====================  PreferredPrimary: When both nodes start in Auto-Detect mode, prefer to configure this node as Primary. (Required)  ====================  //
    describe('PreferredPrimary[F]:', function() { 

        it('should return status 200 on POST with PreferredPrimary:true', function(done) {
            var payload = '{"HighAvailability":{"PreferredPrimary":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET PreferredPrimary:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with PreferredPrimary:false', function(done) {
            var payload = '{"HighAvailability":{"PreferredPrimary":false}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET PreferredPrimary:false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with PreferredPrimary:true', function(done) {
            var payload = '{"HighAvailability":{"PreferredPrimary":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET PreferredPrimary:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 100977
        it('should return status 200 on POST with PreferredPrimary:null (false)', function(done) {
            var payload = '{"HighAvailability":{"PreferredPrimary":null}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"PreferredPrimary":false}}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  after RESET of PreferredPrimary:null (false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with PreferredPrimary:true for expectA1Enabled default', function(done) {
            var payload = '{"HighAvailability":{"PreferredPrimary":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET PreferredPrimary:true for expectA1Enabled default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    });


//  ====================  Group: Group name of the HA Pair. Maximum length: 128 characters. (Required)  ====================  //
    describe('Group[""]:', function() { 
// EnableHA rules tested above
       
        it('should return status 200 on POST with Group Max Length (128)', function(done) {
            var payload = '{"HighAvailability":{"Group":"'+  FVT.long128Name +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET Group Max Length (128)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with Group:null (No Default anymore) ', function(done) {
            var payload = '{"HighAvailability":{"Group":null}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"Group":"'+  FVT.long128Name +'"}}' ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: Group Value: \"NULL\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET Group:null (No Default anymore) ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with Group:msgServer name', function(done) {
            var payload = '{"HighAvailability":{"Group":"'+  FVT.msgServer +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET Group:msgServer name', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST with Group:"" ', function(done) {
            var payload = '{"HighAvailability":{"Group":""}}';
            verifyConfig = JSON.parse( '{"HighAvailability":{"Group":"'+  FVT.msgServer +'"}}' ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: Group Value: \"\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET Group:"" ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

//RESET to Enabled value
        it('should return status 200 on POST with Group:FVT_HAPair', function(done) {
            var payload = '{"HighAvailability":{"Group":"FVT_HAPair"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET Group:FVT_HAPair', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    

//  ====================  Error Tests  ====================  //
    describe('Error:', function() { 

    //  ====================   EnableHA: Specify if High Availability is enabled. (Required) ====================  //
        describe('EnableHA[F]:', function() {
// 100977
            it('should return status 400 on POST with EnableHA:"true"', function(done) {
                var payload = '{"HighAvailability":{"EnableHA":"true"}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: EnableHA Type: JSON_STRING" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET EnableHA:true not String', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 100977
            it('should return status 200 on POST with EnableHA:1', function(done) {
                var payload = '{"HighAvailability":{"EnableHA":1}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: EnableHA Type: JSON_INTEGER" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET EnableHA:true not Integer', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
        
        });
        
        
    //  ====================   StartupMode: The high availability mode at which the node will start (Required) ====================  //
        describe('StartupMode:[AutoDetect,StandAlone]:', function() {

            it('should return status 400 on POST with StartupMode:StandAloneAutoDetect', function(done) {
                var payload = '{"HighAvailability":{"StartupMode":"StandAloneAutoDetect"}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: StandAloneAutoDetect" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: StartupMode Value: StandAloneAutoDetect." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: StartupMode Value: \"StandAloneAutoDetect\"." };
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed  StartupMode:StandAloneAutoDetect', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with StartupMode:StandAlone,AutoDetect', function(done) {
                var payload = '{"HighAvailability":{"StartupMode":"StandAlone,AutoDetect"}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: StandAlone,AutoDetect" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: StartupMode Value: \\\"StandAlone,AutoDetect\\\"."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed  StartupMode:StandAlone,AutoDetect', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with StartupMode:ALL', function(done) {
                var payload = '{"HighAvailability":{"StartupMode":"ALL"}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: ALL" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: StartupMode Value: ALL." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: StartupMode Value: \"ALL\"." } ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed  StartupMode:ALL', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with StartupMode:true', function(done) {
                var payload = '{"HighAvailability":{"StartupMode":true}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: JSON_BOOLEAN" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: JSON_TRUE" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed StartupMode:true', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


    //  ====================  RemoteDiscoveryNIC: The discovery interface on the remote node in the HA Pair.  ====================  //
        describe('RemoteDiscoveryNIC[IPAddress]:', function() { 

            it('should return status 400 on POST with RemoteDiscoveryNIC:ipv4,ipv6', function(done) {
                var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ FVT.A2_IPv4_HA0 +',['+ FVT.A2_IPv6_HA0 +']"}}';
                verifyConfig = JSON.parse( expectA1Enabled ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: JSON_BOOLEAN" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: '+ FVT.A2_IPv4_HA0 +','+ FVT.A2_IPv6_HA0 +'." }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \\\"'+ FVT.A2_IPv4_HA0 +',['+ FVT.A2_IPv6_HA0 +']\\\"." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed RemoteDiscoveryNIC:ipv4,ipv6', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
// 04/2019 RemoteDiscoveryNIC can NOW BE a Hostname!
            it('should return status 200 (now) on POST with RemoteDiscoveryNIC:hostname', function(done) {
                var payload = '{"HighAvailability":{"RemoteDiscoveryNIC":"'+ process.env.A2_HOSTNAME_OS +'"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameRemoteDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: '+ process.env.A2_HOSTNAME +'" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: '+ process.env.A2_HOSTNAME +'." }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RemoteDiscoveryNIC Value: \\\"'+ process.env.A2_HOSTNAME +'\\\"." }' );
//                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
//            });
//            it('should return status 200 on GET after failed RemoteDiscoveryNIC:hostname', function(done) {
//                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//            });
	            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		    });
		    it('should return status 200 on GET with RemoteDiscoveryNIC:hostname', function(done) {
		        FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		    });
        
        });

    //  ====================  LocalReplicationNIC: IPv4 or IPv6 address of the replication interface  ====================  //
        describe('LocalReplicationNIC[IPAddress]:', function() { 

            it('should return status 400 on POST with LocalReplicationNIC:ipv4,ipv6', function(done) {
                var payload = '{"HighAvailability":{"LocalReplicationNIC":"'+ FVT.A1_IPv4_HA1 +',['+ FVT.A1_IPv6_HA1 +']"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameRemoteDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: JSON_BOOLEAN" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: '+ FVT.A1_IPv4_HA1 +','+ FVT.A1_IPv6_HA1 +'." }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: \\\"'+ FVT.A1_IPv4_HA1 +',['+ FVT.A1_IPv6_HA1 +']\\\"." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed LocalReplicationNIC:ipv4,ipv6', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

// PENDING QUESTION            
// 04/2019 LocalReplicationNIC can NOW BE a Hostname!
            it('should return status 200 (now) on POST with LocalReplicationNIC:hostname', function(done) {
                var payload = '{"HighAvailability":{"LocalReplicationNIC":"'+ process.env.A1_HOSTNAME_OS +'"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalRepNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: '+ process.env.A1_HOSTNAME +'" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: '+ process.env.A1_HOSTNAME +'." }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalReplicationNIC Value: \\\"'+ process.env.A1_HOSTNAME +'\\\"." }' );
//                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
//            });
//            it('should return status 200 on GET after failed LocalReplicationNIC:hostname', function(done) {
//                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//            });
            
	            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		    });
		    it('should return status 200 on GET with RemoteDiscoveryNIC:hostname', function(done) {
		        FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		    });
        
        });

    //  ====================  LocalDiscoveryNIC: IPv4 or IPv6 address of the discovery interface  ====================  //
        describe('LocalDiscoveryNIC[IPAddress]:', function() { 

            it('should return status 400 on POST with LocalDiscoveryNIC:ipv4,ipv6', function(done) {
                var payload = '{"HighAvailability":{"LocalDiscoveryNIC":"'+ FVT.A1_IPv4_HA0 +',['+ FVT.A1_IPv6_HA0 +']"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalRepNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: JSON_BOOLEAN" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: '+ FVT.A1_IPv4_HA0 +','+ FVT.A1_IPv6_HA0 +'." }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \\\"'+ FVT.A1_IPv4_HA0 +',['+ FVT.A1_IPv6_HA0 +']\\\"." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed LocalDiscoveryNIC:ipv4,ipv6', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 200 (now) on POST with LocalDiscoveryNIC:hostname', function(done) {
                var payload = '{"HighAvailability":{"LocalDiscoveryNIC":"'+ process.env.A1_HOSTNAME_OS +'"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: StartupMode Type: '+ process.env.A1_HOSTNAME +'" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: '+ process.env.A1_HOSTNAME +'." }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: '+ process.env.A1_HOSTNAME +'." }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LocalDiscoveryNIC Value: \\\"'+ process.env.A1_HOSTNAME +'\\\"." }' );
//                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
//            });
//            it('should return status 200 on GET after failed LocalDiscoveryNIC:hostname', function(done) {
//                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//            });
            
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
	        });
	        it('should return status 200 on GET with RemoteDiscoveryNIC:hostname', function(done) {
     	        FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
	        });
        
        });

    //  ====================  DiscoveryTimeout: Time in seconds the node will attempt to discover a HA-Pair node. (Required) ====================  //
        describe('DiscoveryTimeout[10-600-2147483647]:', function() { 

            it('should return status 400 on POST with DiscoveryTimeout:9', function(done) {
                var payload = '{"HighAvailability":{"DiscoveryTimeout":9}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: 9." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: \"9\"." };
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed DiscoveryTimeout:9', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTimeout:-1', function(done) {
                var payload = '{"HighAvailability":{"DiscoveryTimeout":-1}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: -1" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: -1." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: \"-1\"." };
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed DiscoveryTimeout:-1', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTimeout:1.0', function(done) {
                var payload = '{"HighAvailability":{"DiscoveryTimeout":1.0}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: 1.0" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: JSON_REAL" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed DiscoveryTimeout:1.0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTimeout:""', function(done) {
                var payload = '{"HighAvailability":{"DiscoveryTimeout":""}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: JSON_NULL" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: JSON_STRING" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed DiscoveryTimeout:"")', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTimeout:2147483648', function(done) {
                var payload = '{"HighAvailability":{"DiscoveryTimeout":2147483648}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: DiscoveryTimeout Type: 2147483648" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: -2147483648." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTimeout Value: \"2147483648\"." } ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed DiscoveryTimeout:2147483648', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
        
        });

    //  ====================  HeartbeatTimeout: Time in seconds to detect that the other node in the HA-Pair has failed. (Required) ====================  //
        describe('HeartbeatTimeout[1-10-2147483647]:', function() { 

            it('should return status 400 on POST with HeartbeatTimeout:-1', function(done) {
                var payload = '{"HighAvailability":{"HeartbeatTimeout":-1}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: -1" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: HeartbeatTimeout Value: -1." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: HeartbeatTimeout Value: \"-1\"." } 
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed HeartbeatTimeout:-1', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with HeartbeatTimeout:1.0', function(done) {
                var payload = '{"HighAvailability":{"HeartbeatTimeout":1.0}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: 1.0" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: JSON_REAL" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed HeartbeatTimeout:1.0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with HeartbeatTimeout:""', function(done) {
                var payload = '{"HighAvailability":{"HeartbeatTimeout":""}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: JSON_NULL" }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: JSON_STRING" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed HeartbeatTimeout:"")', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with HeartbeatTimeout:2147483648', function(done) {
                var payload = '{"HighAvailability":{"HeartbeatTimeout":2147483648}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: HeartbeatTimeout Type: 2147483648" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: HeartbeatTimeout Value: -2147483648." }' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: HeartbeatTimeout Value: \"2147483648\"." } ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after failed HeartbeatTimeout:2147483648', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
        
        });

    //  ====================  PreferredPrimary: When both nodes start in Auto-Detect mode, prefer to configure this node as Primary. (Required)  ====================  //
        describe('PreferredPrimary[F]:', function() { 
// 100977
            it('should return status 400 on POST with PreferredPrimary:"true", NOT A String', function(done) {
                var payload = '{"HighAvailability":{"PreferredPrimary":"true"}}';
                verifyConfig = {"HighAvailability":{"PreferredPrimary":true}} ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: PreferredPrimary Type: JSON_STRING" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after PreferredPrimary:"true", NOT A String', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 100977
            it('should return status 200 on POST with PreferredPrimary:1, NOT A String', function(done) {
                var payload = '{"HighAvailability":{"PreferredPrimary":1}}';
                verifyConfig = JSON.parse( payload ) ;
                verifyConfig = {"HighAvailability":{"PreferredPrimary":true}} ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: PreferredPrimary Type: JSON_INTEGER" }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after PreferredPrimary:1, NOT A String', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
        
        });


    //  ====================  Group: Group name of the HA Pair. Maximum length: 128 characters. (Required)  ====================  //
        describe('Group[""]:', function() { 
// 100989
            it('should return status 400 on POST with Group .gt.Max Length (129)', function(done) {
                var payload = '{"HighAvailability":{"Group":"'+ FVT.long129Name +'"}}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: HighAvailability Name: haconfig Property: Group Type: '+ FVT.long129Name +'" }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Group Value: '+ FVT.long129Name +'." }' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Group Value: \\\"'+ FVT.long129Name +'\\\"." }' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: HighAvailability Property: Group Value: '+ FVT.long129Name +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET Group .gt.Max Length (129)', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });
        
    
    });
    
    
    //  Delete test cases
    describe('Delete', function() {

        it('should return status 400 on DELETE HighAvailability', function(done) {
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Name."};
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"Delete is not allowed for HighAvailability object."};
            verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for HighAvailability object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "HighAvailability":null(Bad Form Post Delete)', function(done) {
            var payload = '{"HighAvailability":null}';
                verifyConfig = JSON.parse( expectA1EnabledHostnameLocalDiscoNIC ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for HighAvailability object."};
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"HighAvailability\":null is not valid."};
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"HighAvailability\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed Bad Form POST to Default of "HighAvailability"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    //  Sync a HA Pair
    describe('SyncHAPair', function() {

        it('should return status 200 on GET "Status" A1 before A2 sync', function(done) {
			var verifyStatus = JSON.parse( FVT.expectHARunningStatus ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
		
        it('should return 200 on POST with EnableHA:true A2', function(done) {
            var payload = expectA2Enabled ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET EnableHA:true A2', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 POST restart A2 sync', function(done) {
//            var payload = '{"Service":"Server", "CleanStore":true, "Maintenance":"stop"}';
            var payload = '{"Service":"Server", "CleanStore":true }';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" A2', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
			var verifyStatus = JSON.parse( FVT.expectHARunningA2Sync ) ;
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });

// shouldn't have to restart A1, it is config'ed, ready and waiting for A2?  Restarting with StartupMode:Standalone causes ReasonCode:3, ReasonString:SplitBrain
//        it('should return status 200 POST restart A1 sync', function(done) {
//            var payload = '{"Service":"Server", "CleanStore":true}';
//            var verifyPayload = JSON.parse( payload ) ;
//			verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
//            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
//        });
        it('should return status 200 on GET "Status" A1 sync', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
			var verifyStatus = JSON.parse( FVT.expectHARunningA1Sync ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });

    });

    describe('Cleanup', function() {
    
        it('should return 200 on POST A1 expectDefault', function(done) {
            var payload = expectDefault ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET A1 expectDefault', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    
        it('should return 200 on POST A2 expectDefault', function(done) {
            var payload = expectDefault ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET A2 expectDefault', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        // restart standby first - note it is brought up first in maintenance mode
        it('should return 200 on POST Service Restart A2', function(done) {
            var payload = '{"Service":"Server"}' ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET Service Status A2', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
		    verifyConfig = JSON.parse( FVT.expectMaintenance );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return 200 on POST Service Restart A1', function(done) {
            var payload = '{"Service":"Server"}' ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET Service Status A1', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
		    verifyConfig = JSON.parse( FVT.expectDefaultStatus );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
    
        // stop maintenance mode and restart back to production for the previously standby configured node
        it('should return 200 on POST Service Restart A2', function(done) {
            var payload = '{"Service":"Server","Maintenance":"stop"}' ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET Service Status A2', function(done) {
    		this.timeout( FVT.START_HA + 5000 ) ; 
    		FVT.sleep( FVT.START_HA ) ;
		    verifyConfig = JSON.parse( FVT.expectDefaultStatus );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });

		
    });
    
});
