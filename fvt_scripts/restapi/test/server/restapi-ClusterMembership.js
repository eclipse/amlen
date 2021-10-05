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

var uriObject = 'ClusterMembership/';

var defaultControlPort = 9104;
var defaultMessagingPort = 9105;
var defaultDiscoveryPort = 9106;

var responseTimeout = FVT.RESETCONFIG + 5000;  // must be responseTimeout > FVT.RESETCONFIG > FVT.REBOOT

var expectDefault = '{"ClusterMembership": {"ControlExternalAddress": null,"ControlPort": '+ defaultControlPort +',"DiscoveryPort": '+ defaultDiscoveryPort +',"DiscoveryTime": 10,"EnableClusterMembership": false,"MessagingAddress": null,"MessagingExternalAddress": null,"MessagingPort": '+ defaultMessagingPort +',"MessagingUseTLS": false,"MulticastDiscoveryTTL": 1,"UseMulticastDiscovery": true}, \
                      "Version": "'+ FVT.version +'"}' ; 
var expectUsedDefault = '{"ClusterMembership": {"ClusterName": "","ControlAddress": null,"ControlExternalAddress": null,"ControlExternalPort": null,"ControlPort": '+ defaultControlPort +',"DiscoveryPort": '+ defaultDiscoveryPort +',"DiscoveryServerList": null,"DiscoveryTime": 10,"EnableClusterMembership": false,"MessagingAddress": null,"MessagingExternalAddress": null,"MessagingExternalPort": null,"MessagingPort": '+ defaultMessagingPort +',"MessagingUseTLS": false,"MulticastDiscoveryTTL": 1,"UseMulticastDiscovery": true}, \
                      "Version": "'+ FVT.version +'"}' ; 
var expectEnabledDefault = '{"ClusterMembership": {"ClusterName": "BasicCluster","ControlAddress": "'+ FVT.A1_IPv4_INTERNAL_1 +'","ControlExternalAddress": null,"ControlExternalPort": null,"ControlPort": '+ defaultControlPort +',"DiscoveryPort": '+ defaultDiscoveryPort +',"DiscoveryServerList": null,"DiscoveryTime": 10,"EnableClusterMembership": true,"MessagingAddress": null,"MessagingExternalAddress": null,"MessagingExternalPort": null,"MessagingPort": '+ defaultMessagingPort +',"MessagingUseTLS": false,"MulticastDiscoveryTTL": 1,"UseMulticastDiscovery": true}, \
                      "Version": "'+ FVT.version +'"}' ; 


var verifyConfig = {};
var verifyMessage = {};


var long65535_ipV4_String = "";
var long65535_ipV6_String = "";

// Build DiscoveryServerList for IPv4 
for ( ip4=1 ; ip4 < 256 ; ip4++ ) {
    id3 = ( '00' + ip4).slice(-3);
    if ( long65535_ipV4_String.length == 0 ) {
//        long65535_ipV4_String = 'server'+ id3 +'@111.111.111.' + ip4 +':9102';
        long65535_ipV4_String =                  '111.111.111.' + ip4 +':'+ defaultControlPort ;
        controlAddressLen = long65535_ipV4_String.length + 1;
        FVT.trace( 3, "item.length : "+ controlAddressLen );
    } else if (long65535_ipV4_String.length+controlAddressLen  <= 65535 ) {
//        long65535_ipV4_String = long65535_ipV4_String +',server'+ id3 +'@111.111.111.' + ip4 +':9102' ;
        long65535_ipV4_String = long65535_ipV4_String +                  ',111.111.111.' + ip4 +':'+ defaultControlPort  ;
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
//        long65535_ipV6_String = 'server'+ id4 +'@[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:9102';
        long65535_ipV6_String =                  '[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:'+ defaultControlPort ;
        controlAddressLen = long65535_ipV6_String.length + 1;
        FVT.trace( 3, "item.length : "+ controlAddressLen );
    } else if (long65535_ipV6_String.length+controlAddressLen  <= 65535 ) {
//        long65535_ipV6_String = long65535_ipV6_String +',server'+ id4 +'@[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:9102' ;
        long65535_ipV6_String = long65535_ipV6_String +                  ',[fe80:0000:0000:1234:1234:1234:1234:' + ip6 +']:'+ defaultControlPort  ;
        FVT.trace( 3, id4 +"  SIZEOF long65535_ipV6_String: "+ long65535_ipV6_String.length );
    } else {
        FVT.trace( 3, "MAXED  ==> long65535_ipV6_String: "+ long65535_ipV6_String );
    }
}
    FVT.trace( 2, "Final long65535_ipV6_String LENGTH:"+ long65535_ipV6_String.length +"  Value: "+ long65535_ipV6_String );
 
// Test Cases Start Here

// ClusterMembership test cases
describe('ClusterMembership:', function() {
this.timeout( responseTimeout );

    // Pristine Get
    describe('Pristine Get:', function() {
    
        it('should return status 200 on get, Expected DEFAULT "ClusterMembership"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // EnableClusterMembership test cases
    describe('EnableClusterMembership:', function() {

        it('should return status 200 on POST with EnableClusterMembership:true with ClusterName and ControlAddress', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":true,"ClusterName":"RESTAPI#Test+Cluster","ControlAddress":"127.0.0.1"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableClusterMembership:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with EnableClusterMembership:false', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":false}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableClusterMembership:false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with EnableClusterMembership:true', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":true}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"EnableClusterMembership":true,"ClusterName":"RESTAPI#Test+Cluster","ControlAddress":"127.0.0.1"}}' ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET EnableClusterMembership:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99130
        it('should return status 200 on POST with ControlAddress/ClusterName/EnableClusterMembership:null (false)', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":null,"ClusterName":null,"ControlAddress":""}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"EnableClusterMembership":false,"ClusterName":"","ControlAddress":""}}' ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  after RESET of ControlAddress/ClusterName/EnableClusterMembership:null (false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ClusterName test cases
    describe('ClusterName:', function() {
    
        it('should return status 200 on POST with MaxLength ClusterName', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET MaxLength ClusterName', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99131, 99141
        it('should return status 200 on POST with ClusterName "" (Reset to Original, Disable Cluster)', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":"","EnableClusterMembership":false}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET MaxLength ClusterName "" (Reset to Original, Disable Cluster)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//?? should I reENABLE ClusterMembership??    
        it('should return status 200 on POST with ClusterName my_Cluster-99', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":"RESTAPI_Cluster-99"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET MaxLength ClusterName my_Cluster-99', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99131
        it('should return status 200 on POST with ClusterName null (DEFAULT)', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":null}}';
            // cause an empty string is the default, that's why I can't use payload here.
            verifyConfig = JSON.parse( '{"ClusterMembership":{"ClusterName":""}}' ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET ClusterName null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // UseMulticastDiscovery test cases
    describe('UseMulticastDiscovery:', function() {

        it('should return status 200 on POST with UseMulticastDiscovery:false with 111.111.111.1', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false,"DiscoveryServerList":"111.111.111.1:'+ defaultControlPort +'"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "UseMulticastDiscovery:false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with UseMulticastDiscovery:true', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":true}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "UseMulticastDiscovery:true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with UseMulticastDiscovery:true', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":true,"DiscoveryServerList":"111.111.111.3:9102"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "UseMulticastDiscovery:true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with UseMulticastDiscovery:false with 111.111.111.x- 1&2', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false,"DiscoveryServerList":"111.111.111.1:'+ defaultControlPort +',111.111.111.2:'+ defaultControlPort +'"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "UseMulticastDiscovery:false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99129
        it('should return status 200 on POST with UseMulticastDiscovery:null (true)', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":null,"DiscoveryServerList":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"UseMulticastDiscovery":true, "DiscoveryServerList":null}}' ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "UseMulticastDiscovery:null (true)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MulticastDiscoveryTTL test cases
    describe('MulticastDiscoveryTTL(1:1:256):', function() {
    
        it('should return status 200 on POST with MulticastDiscoveryTTL:2', function(done) {
            var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":2}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MulticastDiscoveryTTL:2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with MulticastDiscoveryTTL:1', function(done) {
            var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":1}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MulticastDiscoveryTTL:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with MulticastDiscoveryTTL:256', function(done) {
            var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":256}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MulticastDiscoveryTTL:256"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with MulticastDiscoveryTTL:255', function(done) {
            var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":255}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MulticastDiscoveryTTL:255"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99144
        it('should return status 200 on POST with MulticastDiscoveryTTL:null (1)', function(done) {
            var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"MulticastDiscoveryTTL":1}}' ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MulticastDiscoveryTTL:null (1)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // DiscoveryServerList test cases (Length 65535 string of comma separated members, required when UseMcastDisco=F
    describe('DiscoveryServerList:[null]MAX:65535', function() {
    
        it('should return status 200 on POST with "DiscoveryServerList":"111.111.111.240:65535"', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryServerList":"111.111.111.240:65535"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DiscoveryServerList":"111.111.111.240:65535"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with "DiscoveryServerList" dependency on UseMultiCastDiscovery (was MAX NAME)', function(done) {
            var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false,"DiscoveryServerList":"111.111.111.111:10240"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DiscoveryServerList":"(was MaxName1024@)111.111.111.111:10240" and UseMCaseDisco:F', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with "DiscoveryServerList":""', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryServerList":"","UseMulticastDiscovery":true}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DiscoveryServerList":"" and UseMcastDisco:T', function(done) {
            FVT.sleep( FVT.RESETCONFIG );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99146
        it('should return status 200 on POST with "DiscoveryServerList":"lengthy list IPv4"', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryServerList":"'+ long65535_ipV4_String +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryServerList":"lengthy list IPv4"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

// DiscoveryServerList test cases (Length 65535 string of comma separated members, required when UseMcastDisco=F
    describe('DiscoveryServerList:[null]MAX:65535', function() {

        if( FVT.A1_HOST_OS.substring(0, 3) != "EC2") {
// 99146 117729
            it('should return status 200 on POST with "DiscoveryServerList":"lengthy list IPv6"', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryServerList":"'+ long65535_ipV6_String +'"}}';
                verifyConfig = JSON.parse( payload ) ;
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
    // GET IS KNOWN TO FAIL _ MOCHA: Unexpected End of Input -- processing the response chunks (many defects, but R. says it's not IMA's fault.  can not repro with CURL
    //        it('should return status 200 on GET "DiscoveryServerList":"lengthy list IPv6"', function(done) {
    //            FVT.makeGetRequestChunked( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
    //        });
            
        }
    
        it('should return status 200 on POST with "DiscoveryServerList":null ("")', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryServerList":null}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryServerList":null ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ControlAddress test cases
    describe('ControlAddress:', function() {

        if( FVT.A1_HOST_OS.substring(0, 3) != "EC2") {
        
            it('should return status 200 on POST with ControlAddress IPv6', function(done) {
                var payload = '{"ClusterMembership":{"ControlAddress":"'+ FVT.A1_IPv6_INTERNAL_1 +'"}}';
                verifyConfig = JSON.parse( payload ) ;
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET "ControlAddress IPv6"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        }
// 99141
        it('should return status 200 on POST with ControlAddress:"" (empty string, Cluster Not Enabled)', function(done) {
            var payload = '{"ClusterMembership":{"ControlAddress":"","EnableClusterMembership":false}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlAddress:""  (empty string, Cluster Not Enabled)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlAddress:"'+ FVT.A1_IPv4_INTERNAL_1 +'" (Cluster Not Enabled)', function(done) {
            var payload = '{"ClusterMembership":{"ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'","EnableClusterMembership":false}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlAddress:"'+ FVT.A1_IPv4_INTERNAL_1 +'"  (Cluster Not Enabled)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlAddress:"'+ FVT.A1_IPv4_INTERNAL_1 +'" (Cluster NOW Enabled)', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":true,"ClusterName":"IPv4_TestCluster"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET ControlAddress:"'+ FVT.A1_IPv4_INTERNAL_1 +'"  (Cluster NOW Enabled)', function(done) {
            FVT.sleep( FVT.RESETCONFIG );   // keep getting ECONN Refused
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99130
        it('should return status 200 on POST with ControlAddress:null (Cluster NOT Enabled-F)', function(done) {
            var payload = '{"ClusterMembership":{"EnableClusterMembership":null,"ControlAddress":null,"ClusterName":null}}';
            verifyConfig = {"ClusterMembership":{"EnableClusterMembership":false,"ControlAddress":null,"ClusterName":""}} ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
            verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET ControlAddress:null (Cluster NOT Enabled-F)', function(done) {
            FVT.sleep( FVT.RESETCONFIG );     // keep getting ECONN Refused
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
                
    });

    // ControlPort test cases
    describe('ControlPort(1:'+ defaultControlPort +':65535):', function() {
    
        it('should return status 200 on POST with ControlPort MIN', function(done) {
            var payload = '{"ClusterMembership":{"ControlPort":1}}';
            verifyConfig = JSON.parse( payload ) ;
// why does these keep changing rtned msg?
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ControlPort MIN"', function(done) {
//            FVT.sleep( FVT.RESETCONFIG );   
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with ControlPort 9100', function(done) {
            var payload = '{"ClusterMembership":{"ControlPort":9100}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlPort 9100"', function(done) {
//            FVT.sleep( FVT.RESETCONFIG );   
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with ControlPort MAX', function(done) {
            var payload = '{"ClusterMembership":{"ControlPort":65535}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ControlPort MAX"', function(done) {
//            FVT.sleep( FVT.RESETCONFIG );   
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99145
        it('should return status 200 on POST with ControlPort null (DEFAULT)', function(done) {
            var payload = '{"ClusterMembership":{"ControlPort":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"ControlPort":'+ defaultControlPort +'}}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ControlPort null (DEFAULT)"', function(done) {
//            FVT.sleep( FVT.RESETCONFIG );   
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ControlExternalAddress test cases
    describe('ControlExternalAddress:', function() {

        it('should return status 200 on POST with ControlExternalAddress '+ FVT.msgServer, function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalAddress":"'+ FVT.msgServer +'"}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlExternalAddress '+ FVT.msgServer, function(done) {
//            FVT.sleep( FVT.RESETCONFIG );   
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalAddress:""', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalAddress":""}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlExternalAddress:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalAddress:"localhost"', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalAddress":"localhost"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlExternalAddress:"localhost"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalAddress Bogus IPv6', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":"NotValidateExternal","ControlExternalAddress":"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlExternalAddress BOGUS IPv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        if( FVT.A1_HOST_OS.substring(0, 3) != "EC2") {

            it('should return status 200 on POST with ControlExternalAddress valid IPv6', function(done) {
                var payload = '{"ClusterMembership":{"ControlExternalAddress":"'+ FVT.msgServer_IPv6 +'"}}';
                verifyConfig = JSON.parse( payload ) ;
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET ControlExternalAddress valid IPv6', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        }
    
        it('should return status 200 on POST with ControlExternalAddress null (Delete)', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalAddress":null}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET ControlExternalAddress null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ControlExternalPort test cases
    describe('ControlExternalPort(1::65535:', function() {

        it('should return status 200 on POST with ControlExternalPort:1', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalPort":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlExternalPort:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalPort:2', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalPort":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlExternalPort:2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalPort:65534', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalPort":65534}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlExternalPort:65534"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with ControlExternalPort:65535', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalPort":65535}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlExternalPort:65535"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// there is no default -- will just unset it
        it('should return status 200 on POST with ControlExternalPort:null (no default - delete?)', function(done) {
            var payload = '{"ClusterMembership":{"ControlExternalPort":null}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ControlExternalPort:null" (no default - delete?)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MessagingAddress test cases
    describe('MessagingAddress:', function() {
// ec2 must use internal address
//        it('should return status 200 on POST with MessagingAddress '+ FVT.msgServer, function(done) {
//            var payload = '{"ClusterMembership":{"MessagingAddress":"'+ FVT.msgServer +'"}}';
        it('should return status 200 on POST with MessagingAddress '+ FVT.A1_IPv4_INTERNAL_1, function(done) {
            var payload = '{"ClusterMembership":{"MessagingAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingAddress '+FVT.A1_IPv4_INTERNAL_1, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingAddress:""', function(done) {
            var payload = '{"ClusterMembership":{"MessagingAddress":""}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingAddress:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        if( FVT.A1_HOST_OS.substring(0, 3) != "EC2") {
// 99262    
			it('should return status 200 on POST with MessagingAddress IPv6 address', function(done) {
				var payload = '{"ClusterMembership":{"MessagingAddress":"'+ FVT.msgServer_IPv6 +'"}}';
				verifyConfig = JSON.parse( payload ) ;
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			it('should return status 200 on GET MessagingAddress  IPv6 address', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		};
		
        it('should return status 200 on POST with MessagingAddress:null', function(done) {
            var payload = '{"ClusterMembership":{"MessagingAddress":null}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingAddress:null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        
    });

    // MessagingPort test cases
    describe('MessagingPort(1:'+ defaultMessagingPort +':65535):', function() {
    
        it('should return status 200 on POST with MessagingPort:2', function(done) {
            var payload = '{"ClusterMembership":{"MessagingPort":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingPort:2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingPort:1 MIN', function(done) {
            var payload = '{"ClusterMembership":{"MessagingPort":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingPort:1 MIN', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingPort:65535 MAX', function(done) {
            var payload = '{"ClusterMembership":{"MessagingPort":65535}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingPort:65535 MAX', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingPort:65534', function(done) {
            var payload = '{"ClusterMembership":{"MessagingPort":65534}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingPort:65534', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingPort:null ('+ defaultMessagingPort +')', function(done) {
            var payload = '{"ClusterMembership":{"MessagingPort":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"MessagingPort":'+ defaultMessagingPort +'}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingPort:null ('+ defaultMessagingPort +')', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MessagingExternalAddress test cases
    describe('MessagingExternalAddress:', function() {

        it('should return status 200 on POST with MessagingExternalAddress '+ FVT.msgServer, function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalAddress":"'+ FVT.msgServer +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress '+FVT.msgServer, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingExternalAddress:""', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalAddress":""}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 99262

        it('should return status 200 on POST with MessagingExternalAddress Bogus IPv6 address', function(done) {
            var payload = '{"ClusterMembership":{"ClusterName":"NotValidateExternal","MessagingExternalAddress":"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress Bogus IPv6 address', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
            
        it('should return status 200 on POST with MessagingExternalAddress good IPv6 address', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalAddress":"'+ FVT.msgServer_IPv6 +'"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress good IPv6 address', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with MessagingExternalAddress:"localhost"', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalAddress":"localhost"}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress:"localhost"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST with MessagingExternalAddress:null', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalAddress":null, "ClusterName":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"MessagingExternalAddress":null, "ClusterName":""}}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalAddress:null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MessagingExternalPort test cases
    describe('MessagingExternalPort(1::65535):', function() {

        it('should return status 200 on POST with MessagingExternalPort:2', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalPort":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalPort:2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingExternalPort:1 MIN', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalPort":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalPort:1 MIN', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingExternalPort:65535 MAX', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalPort":65535}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalPort:65535 MAX', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingExternalPort:65534', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalPort":65534}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalPort:65534', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingExternalPort:null (null)', function(done) {
            var payload = '{"ClusterMembership":{"MessagingExternalPort":null}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MessagingExternalPort:null (null)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MessagingUseTLS test cases
    describe('MessagingUseTLS(F):', function() {
    
        it('should return status 200 on POST with MessagingUseTLS:true', function(done) {
            var payload = '{"ClusterMembership":{"MessagingUseTLS":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MessagingUseTLS":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingUseTLS:false', function(done) {
            var payload = '{"ClusterMembership":{"MessagingUseTLS":false}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MessagingUseTLS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingUseTLS:true', function(done) {
            var payload = '{"ClusterMembership":{"MessagingUseTLS":true}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MessagingUseTLS":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with MessagingUseTLS:null (DEFAULT-false)', function(done) {
            var payload = '{"ClusterMembership":{"MessagingUseTLS":null}}';
            verifyConfig = {"ClusterMembership":{"MessagingUseTLS":false}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MessagingUseTLS":null (DEFAULT-false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // DiscoveryPort test cases
    describe('DiscoveryPort(1:'+ defaultDiscoveryPort +':65535):', function() {

        it('should return status 200 on POST with DiscoveryPort:2', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryPort":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryPort:2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryPort:1 MIN', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryPort":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryPort:1 MIN', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryPort:65535 MAX', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryPort":65535}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryPort:65535 MAX', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryPort:65534', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryPort":65534}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryPort:65534', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryPort:null ('+ defaultDiscoveryPort +')', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryPort":null}}';
            verifyConfig = JSON.parse( '{"ClusterMembership":{"DiscoveryPort":'+ defaultDiscoveryPort +'}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET DiscoveryPort:null ('+ defaultDiscoveryPort +')', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // DiscoveryTime test cases
    describe('DiscoveryTime(1:10:2147483647):', function() {
    
        it('should return status 200 on POST with DiscoveryTime:2', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":2}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryTime:1 MIN', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":1}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":1 MIN', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryTime:10', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":10}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":10', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryTime:2147483647 MAX', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":2147483647}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":2147483647 MAX', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST with DiscoveryTime:2147483646', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":2147483646}}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":2147483646', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
 //99270   
        it('should return status 200 on POST with DiscoveryTime:null (10)', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":null}}';
            verifyConfig = {"ClusterMembership":{"DiscoveryTime":10}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":null (10)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
 //99270   workaround
        it('should return status 200 on POST with DiscoveryTime:null (10)', function(done) {
            var payload = '{"ClusterMembership":{"DiscoveryTime":10}}';
            verifyConfig = {"ClusterMembership":{"DiscoveryTime":10}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "DiscoveryTime":null (10)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ClusterMembership Delete test cases
    describe('Delete', function() {
//99153    
        it('should return status 403 on DELETE ClusterMembership', function(done) {
            verifyConfig = JSON.parse( expectUsedDefault ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Name."};
//99153 want         verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/ClusterMembership/ is not valid."};
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null."};
         verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for ClusterMembership object."};
         FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 112650
/* KILLs IMA
        it('should return status 400 on DELETE ClusterMembership "cluster"', function(done) {
            verifyConfig = JSON.parse( expectUsedDefault ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'Cluster', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE "Cluster"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
*/
//  99154    
        it('should return status 400 on POST "ClusterMembership":null(Bad Form Post Delete)', function(done) {
            var payload = '{"ClusterMembership":null}';
            verifyConfig = JSON.parse( expectUsedDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for ClusterMembership object."};
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"ClusterMembership\":null is not valid."};
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"ClusterMembership\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed Bad Form POST to Default of "ClusterMembership"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    //Error test cases
    describe('Error:', function() {
        describe('General:', function() {

            it('should return status 400 on get "Service"', function(done) {
                verifyConfig = JSON.parse( expectUsedDefault ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The RESTful request: '+ FVT.uriServiceDomain +' is not valid."}' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ClusterMembership is not valid."}' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ClusterMembership is not valid."}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ClusterMembership/ is not valid."}' );
                FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getFailureCallback, verifyMessage, done);
            });
            it('should return status 200 on get, "ClusterMembership"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        describe('Not Post ServiceDomain:', function() {
        
            it('should return status 400 on post "ClusterMembership" to Service Domain', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"mochaTestServer"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
//                verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/ is not valid."};
                verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ is not valid."};
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ClusterMembership": is NOT settable', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // Name test cases
        describe('Name:', function() {
        
            it('should return status 404 on POST with Name not settable', function(done) {
                var payload = '{"ClusterMembership":{"Name":"'+ FVT.long256Name+'"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
//                verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Name."} ;
                verifyMessage = {"status":400,"Code":"CWLNA6208","Message":"Updates to the Name configuration item are not allowed."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after "Name" fails, not settable', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // EnableClusterMembership test cases
        describe('EnableClusterMembership:', function() {
// 99141
            it('should return status 400 on POST with EnableClusterMembership:true without ClusterName', function(done) {
                var payload = '{"ClusterMembership":{"EnableClusterMembership":true,"ClusterName":null,"ControlAddress":"'+ FVT.A1_IPv6_INTERNAL_1 +'"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClusterName Value: \"NULL\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET EnableClusterMembership:true without ClusterName', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with EnableClusterMembership:true without ControlAddress', function(done) {
                var payload = '{"ClusterMembership":{"EnableClusterMembership":true,"ClusterName":"StillMissingControlAddress"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlAddress Value: \"NULL\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET EnableClusterMembership:true without ControlAddress', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // ClusterName test cases
        describe('ClusterName:', function() {
        
            it('should return status 400 on POST with too long ClusterName', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"'+ FVT.long257Name +'"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
//                verifyMessage = JSON.parse ( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClusterName Value: \\\"'+ FVT.long256Name +'7\\\"."}' );
                 verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ClusterMembership Property: ClusterName Value: '+ FVT.long257Name +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            
            it('should return status 200 on GET too long ClusterName', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
            if (FVT.A_COUNT === '2') {
            	
                it('should return status 200 POST to Enable with long ClusterName', function(done) {
                    var payload = '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'", "EnableClusterMembership":true, "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'", "UseMulticastDiscovery": false, "DiscoveryServerList":"'+ FVT.A2_IPv4_INTERNAL_1 +':9104"}}';
                    verifyConfig = JSON.parse( payload ) ;
                    FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
                });
                
                it('should return status 200 on GET Enabled with long ClusterName', function(done) {
                    FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
                });
                
            } else {
            	
            	it('should return status 200 POST to Enable with long ClusterName', function(done) {
                    var payload = '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'", "EnableClusterMembership":true, "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'" }}';
                    verifyConfig = JSON.parse( payload ) ;
                    FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
                });
            	
                it('should return status 200 on GET Enabled with long ClusterName', function(done) {
                    FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
                });
            
            }

            if (FVT.A_COUNT === '2') {
            	
//              Actually Join the cluster name and RESTART A1 And A2, the prior cases don't do a restart hence we have never actually joined the cluster yet.
                it('should return status 200 on POST "Cluster of 2: restart A1 to join cluster"', function(done) {
                    var payload = '{"Service":"Server"}';
                    verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                    FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
                }); 
                
                it('should return status 200 on GET "Cluster of 2: Cluster Membership after restart A1"', function(done) {
                    this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                    FVT.sleep( FVT.MAINTENANCE ) ;
                    FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
                });
               
                it('should return status 200 on POST "Cluster of 2: A2 enabling cluster"', function(done) {
                	var payload = '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'", "EnableClusterMembership":true, "ControlAddress":"'+ FVT.A2_IPv4_INTERNAL_1 +'", "UseMulticastDiscovery": false, "DiscoveryServerList":"'+ FVT.A1_IPv4_INTERNAL_1 +':9104"}}';
                    verifyConfig = JSON.parse( payload ) ;
                    verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
                  	FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
                });
                
                
                it('should return status 200 on POST "Cluster of 2: restart A2 to join cluster"', function(done) {
                    var payload = '{"Service":"Server"}';
                    verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                    FVT.makePostRequestWithURLVerify( 'http://' + FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
                }); 
                
            	it('should return status 200 on GET "Cluster of 2: Cluster Membership after restart A2"', function(done) {
                    this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                    FVT.sleep( FVT.MAINTENANCE ) ;
                    FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
                });
            }
            
// 135037 (new restriction, can not change ClusterName if Enabled OR in MaintenanceMode ...
    
            it('should return status 200 on POST "A1 restart in Maintenance Mode"', function(done) {
                var payload = '{"Service":"Server","Maintenance":"start"}';
                verifyConfig = JSON.parse( FVT.expectMaintenanceWithCluster ) ;
                verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
            });
            
            it('should return status 200 on GET "Status" after A1 restart in Maintenance Mode', function(done) {
                this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                FVT.sleep( FVT.MAINTENANCE ) ;
                FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
            });
            
            it('should return status 400 on POST can not change ClusterName on A1 when Maintenance Mode', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"ClusterNameInUse"}}';
                verifyConfig = JSON.parse( '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'", "EnableClusterMembership":true, "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'"  }}' ) ;
                 verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0384","Message":"You must disable the object ClusterMembership to update or set configuration item ClusterName." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            
            it('should return status 200 on GET can not change ClusterName when Maintenance Mode', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
   
            it('should return status 200 on POST "restart A1 in Production Mode"', function(done) {
                var payload = '{"Service":"Server","Maintenance":"stop"}';
                verifyConfig = JSON.parse( FVT.expectClusterConfigedStatus ) ;
                verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
            });
            
            it('should return status 200 on GET "Status" after restart A1 in Production Mode', function(done) {
                this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                FVT.sleep( FVT.MAINTENANCE ) ;
                FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
            });
            
            it('should return status 400 on POST can not change ClusterName when Active', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"ClusterNameInUse"}}';
                verifyConfig = JSON.parse( '{"ClusterMembership":{"ClusterName":"'+ FVT.long256Name +'", "EnableClusterMembership":true, "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'"  }}' ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0384","Message":"You must disable the object ClusterMembership to update or set configuration item ClusterName." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            
            it('should return status 200 on GET can not change ClusterName when Active', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

// return to the expected state 
            it('should return status 200 POST to DisEnable Cluster on A1', function(done) {
                var payload = expectUsedDefault ;
                verifyConfig = JSON.parse( payload ) ;
//                verifyMessage = { "status":200, "Code":"CWLNA6168","Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes.","text":null} ;
                verifyMessage = { "status":200, "Code":"CWLNA6168","Message":"The ${IMA_SVR_COMPONENT_NAME} will be restarted to complete the configuration changes.","text":null} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
            });
            
            it('should return status 200 on GET DisEnabled Cluster on A1', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
   
            it('should return status 200 on POST "restart Disable Cluster on A1"', function(done) {
                var payload = '{"Service":"Server" }';
                verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
                verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
            });
            
            it('should return status 200 on GET "Status" after restart Disable Cluster on A1', function(done) {
                this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                FVT.sleep( FVT.MAINTENANCE ) ;
                FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
            });
            
            if (FVT.A_COUNT === '2') {
            	
                it('should return status 200 POST to DisEnable Cluster on A2', function(done) {
                    var payload = expectUsedDefault ;
                    verifyConfig = JSON.parse( payload ) ;
//                  verifyMessage = { "status":200, "Code":"CWLNA6168","Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes.","text":null} ;
                    verifyMessage = { "status":200, "Code":"CWLNA6168","Message":"The ${IMA_SVR_COMPONENT_NAME} will be restarted to complete the configuration changes.","text":null} ;
                    FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
                });
                
                it('should return status 200 on GET DisEnabled Cluster on A2', function(done) {
                    FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
                });
       
                it('should return status 200 on POST "restart Disable Cluster on A2"', function(done) {
                    var payload = '{"Service":"Server" }';
                    verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
                    verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
                    FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
                });
                
                it('should return status 200 on GET "Status" after restart Disable Cluster on A2', function(done) {
                    this.timeout( FVT.MAINTENANCE + 5000 ) ; 
                    FVT.sleep( FVT.MAINTENANCE ) ;
                    FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint, FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done);
                });
            }
            
        });

        // UseMulticastDiscovery test cases
        describe('UseMulticastDiscovery:', function() {
// 99131
            it('should return status 200 on POST with UseMulticastDiscovery:false without DiscoveryServerList when ENABLED is false' , function(done) {
                var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false}}';
                verifyConfig = JSON.parse( payload ) ;
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET "UseMulticastDiscovery:false without DiscoveryServerList" OK when not Enabled', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// Since not Enabled the previous command was successful, now get the desired error
            it('should return status 400 on POST when UseMulticastDiscovery:false without DiscoveryServerList THEN ENABLE', function(done) {
                var payload = '{"ClusterMembership":{"EnableClusterMembership": true, "ClusterName":"MisMatchMCast-DiscoveryList", "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'"}}';
                verifyConfig = JSON.parse( '{"ClusterMembership":{"EnableClusterMembership": false, "ClusterName":"", "ControlAddress":null}}' ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryServerList Value: \"NULL\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "UseMulticastDiscovery:false without DiscoveryServerList" Then ENABLED', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// Now reset and try same command when ENABLED
            it('should return status 200 on POST with UseMulticastDiscovery:true without DiscoveryServerList when ENABLED is true' , function(done) {
                var payload = '{"ClusterMembership":{"UseMulticastDiscovery":true, "DiscoveryServerList":null, "EnableClusterMembership": true,  "ClusterName":"BasicCluster", "ControlAddress":"'+ FVT.A1_IPv4_INTERNAL_1 +'" }}';
                verifyConfig = JSON.parse( expectEnabledDefault ) ;
    //            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
                verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "UseMulticastDiscovery":true without "DiscoveryServerList" and Enabled', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            it('should return status 400 on POST with UseMulticastDiscovery:false without DiscoveryServerList and ENABLED', function(done) {
                var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false}}';
                verifyConfig = JSON.parse( expectEnabledDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryServerList Value: \"NULL\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "UseMulticastDiscovery:false without DiscoveryServerList" and ENABLED', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            
            it('should return status 400 on POST with UseMulticastDiscovery:"false" as a STRING', function(done) {
                var payload = '{"ClusterMembership":{"UseMulticastDiscovery":"false"}}';
                verifyConfig = JSON.parse( expectEnabledDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: UseMulticastDiscovery Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "UseMulticastDiscovery:"false" as a STRING', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
            it('should return status 200 on POST return values to known Defaults', function(done) {
                var payload = '{"ClusterMembership":{"UseMulticastDiscovery":null, "ClusterName":null, "ControlAddress":null, "EnableClusterMembership": false }}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
    //            verifyMessage = { "Status":200, "Code":"CWLNA6168", "Message":"The IBM IoT MessageSight server will be restarted to complete the configuration changes." } ;
                verifyMessage = { "Status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." } ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET return values to known Default', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // MulticastDiscoveryTTL test cases
        describe('MulticastDiscoveryTTL(1:1:256):', function() {
// 99362
            it('should return status 400 on POST with MulticastDiscoveryTTL:257', function(done) {
                var payload = '{"ClusterMembership":{"MulticastDiscoveryTTL":257}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MulticastDiscoveryTTL Value: \"257\"."};
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "MulticastDiscoveryTTL:257"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // DiscoveryServerList test cases (Length 65535, comma separated list of members)
        //describe('DiscoveryServerList:', function() {
// dependency with UseMcastDisco:F tested above, must be ENABLED to be caught
        
            // it('should return status 400 on POST with "DiscoveryServerList" dependency on UseMultiCastDiscovery', function(done) {
                // var payload = '{"ClusterMembership":{"UseMulticastDiscovery":false,"DiscoveryServerList":""}}';
                // verifyConfig = JSON.parse( expectUsedDefault ) ;
                // verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryServerList Value: \"NULL\"."};
                // FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            // });
            // it('should return status 200 on GET after failed POST with "DiscoveryServerList" dependency on UseMultiCastDiscovery', function(done) {
                // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            // });

            //it('should return status 400 on POST with "DiscoveryServerList":toolong', function(done) {
            //    var payload = '{"ClusterMembership":{"DiscoveryServerList":"'+ long65535_ipV6_String  +','+ long65535_ipV6_String +'"}}';
            //    verifyConfig = JSON.parse( expectUsedDefault ) ;
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryServerList Value: \\\"'+ long65535_ipV6_String  +','+ long65535_ipV6_String +'\\\"."}' );
            //    verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ClusterMembership Property: DiscoveryServerList Value: '+ long65535_ipV6_String  +','+ long65535_ipV6_String +'." }' );
            //    FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            //});
// GET IS KNOWN TO FAIL _ MOCHA: Unexpected End of Input -- processing the response chunks (many defects, but R. says it's not IMA's fault.  can not repro with CURL
            //it('should return status 200 on GET "DiscoveryServerList":toolong', function(done) {
            //    FVT.makeGetRequestChunked( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            //});
            
        //});

        // ControlAddress test cases
        describe('ControlAddress:', function() {

            it('should return status 400 on POST with ControlAddress:"localhost" ', function(done) {
                var payload = '{"ClusterMembership":{"ControlAddress":"localhost"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlAddress Value: \"localhost\"."};
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ControlAddress":"localhost"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
         
            it('should return status 400 on POST with ControlAddress [IPv6]', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"NoBracketsWhenNoPorts","ControlAddress":"['+ FVT.A1_IPv6_INTERNAL_1+']"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlAddress Value: \\\"['+ FVT.A1_IPv6_INTERNAL_1+']\\\"."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET ControlAddress [IPv6]', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
         
            it('should return status 400 on POST with ControlAddress BOGUS IPv6', function(done) {
            this.timeout(5000);
                var payload = '{"ClusterMembership":{"ClusterName":"NotValidateIPv6","ControlAddress":"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlAddress Value: \"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET ControlAddress BOGUS IPv6', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
           
        });

        // ControlPort test cases
        describe('ControlPort(1:'+ defaultControlPort +':65535):', function() {
// 99147 - kills IMA, 99363 
            it('should return status 400 on POST with "ControlPort":65536', function(done) {
                var payload = '{"ClusterMembership":{"ControlPort":65536}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlPort Value: \"65536\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after "ControlPort":65536 fails', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
//99363
            it('should return status 400 on POST with "ControlPort":0', function(done) {
                var payload = '{"ClusterMembership":{"ControlPort":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlPort Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after "ControlPort":0 fails', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

        });

        // ControlExternalAddress test cases
        describe('ControlExternalAddress:', function() {
// 99273
            it('should return status 400 on POST with ControlExternalAddress [IPv6]', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"NoBracketsWhenNoPorts","ControlExternalAddress":"['+ FVT.msgServer_IPv6+']"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlExternalAddress Value: \\\"['+ FVT.msgServer_IPv6+']\\\"."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET ControlExternalAddress [IPv6]', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

        });

        // ControlExternalPort test cases
        describe('ControlExternalPort(1::65535:', function() {
// 99152
            it('should return status 400 on POST with ControlExternalPort:"" (Not Empty String, NUMBERS ONLY)', function(done) {
                var payload = '{"ClusterMembership":{"ControlExternalPort":""}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: ControlExternalPort Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET ControlExternalPort:"" (Not Empty String, NUMBERS ONLY)', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });
            
            it('should return status 400 on POST with ControlExternalPort is NOT STRING', function(done) {
                var payload = '{"ClusterMembership":{"ControlExternalPort":"6969"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: ControlExternalPort Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET ControlExternalPort is NOT STRING', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99147 -made it die, 99363 
            it('should return status 400 on POST with ControlExternalPort:65536', function(done) {
                var payload = '{"ClusterMembership":{"ControlExternalPort":65536}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlExternalPort Value: \"65536\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ControlExternalPort:65536"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with ControlExternalPort:0', function(done) {
                var payload = '{"ClusterMembership":{"ControlExternalPort":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ControlExternalPort Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ControlExternalPort:0"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // MessagingAddress test cases
        describe('MessagingAddress:', function() {

            it('should return status 400 on POST with MessagingAddress:"localhost"', function(done) {
                var payload = '{"ClusterMembership":{"MessagingAddress":"localhost"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingAddress Value: \"localhost\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingAddress:"localhost"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
     
            it('should return status 400 on POST with MessagingAddress [IPv6] address', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"NoBracketsWhenNoPorts","MessagingAddress":"['+ FVT.msgServer_IPv6 +']"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingAddress Value: \\\"['+ FVT.msgServer_IPv6 +']\\\"."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingAddress  [IPv6] address', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
     
            it('should return status 400 on POST with MessagingAddress BOGUS IPv6 address', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"NotValidateIPv6","MessagingAddress":"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingAddress Value: \"fe80:0000:0000:ffff:ffff:6aff:fe9e:d42b\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingAddress Bogus IPv6 address', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
           
        });

        // MessagingPort test cases
        describe('MessagingPort(1:'+ defaultMessagingPort +':65535):', function() {
        
            it('should return status 400 on POST with MessagingPort:"2"', function(done) {
                var payload = '{"ClusterMembership":{"MessagingPort":"2"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: MessagingPort Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingPort:"2"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with MessagingPort:65536', function(done) {
                var payload = '{"ClusterMembership":{"MessagingPort":65536}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingPort Value: \"65536\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after Post fails MessagingPort:65536', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with MessagingPort:0', function(done) {
                var payload = '{"ClusterMembership":{"MessagingPort":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingPort Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after Post fails MessagingPort:0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // MessagingExternalAddress test cases
        describe('MessagingExternalAddress:', function() {
    //99273     
            it('should return status 400 on POST with MessagingExternalAddress [IPv6] address', function(done) {
                var payload = '{"ClusterMembership":{"ClusterName":"NoBracketsWhenNoPorts","MessagingExternalAddress":"['+ FVT.msgServer_IPv6 +']"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingExternalAddress Value: \\\"['+ FVT.msgServer_IPv6 +']\\\"."}' ) ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingExternalAddress  [IPv6] address', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // MessagingExternalPort test cases
        describe('MessagingExternalPort(1::65535):', function() {

            it('should return status 400 on POST with MessagingExternalPort:"2"', function(done) {
                var payload = '{"ClusterMembership":{"MessagingExternalPort":"2"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: MessagingExternalPort Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET MessagingExternalPort:"2"', function(done) {
                FVT.sleep( FVT.RESETCONFIG );  // sometimes get socket dropped after the POST is processed or ECONNRefused...
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with MessagingExternalPort:65536', function(done) {
                var payload = '{"ClusterMembership":{"MessagingExternalPort":65536}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingExternalPort Value: \"65536\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after Post fails MessagingExternalPort:65536', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with MessagingExternalPort:0', function(done) {
                var payload = '{"ClusterMembership":{"MessagingExternalPort":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MessagingExternalPort Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after Post fails MessagingExternalPort:0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // MessagingUseTLS test cases
        describe('MessagingUseTLS:', function() {

            it('should return status 400 on POST with MessagingUseTLS:"true"', function(done) {
                var payload = '{"ClusterMembership":{"MessagingUseTLS":"true"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: MessagingUseTLS Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after POST fails "MessagingUseTLS":"true"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with MessagingUseTLS:1', function(done) {
                var payload = '{"ClusterMembership":{"MessagingUseTLS":1}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: MessagingUseTLS Type: JSON_INTEGER"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after POST fails "MessagingUseTLS":1', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // DiscoveryPort test cases
        describe('DiscoveryPort(1:'+ defaultDiscoveryPort +':65535):', function() {

            it('should return status 400 on POST with DiscoveryPort:"2"', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryPort":"2"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: DiscoveryPort Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET DiscoveryPort:"2"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with DiscoveryPort:65536', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryPort":65536}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryPort Value: \"65536\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET after Post fails DiscoveryPort:65536', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 99363
            it('should return status 400 on POST with DiscoveryPort:0', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryPort":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryPort Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "DiscoveryPort":0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });

        // DiscoveryTime test cases
        describe('DiscoveryTime(1:10:2147483647):', function() {

            it('should return status 400 on POST with DiscoveryTime:0', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryTime":0}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTime Value: \"0\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "DiscoveryTime":0', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTime:2147483648', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryTime":2147483648}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage ={"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DiscoveryTime Value: \"2147483648\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "DiscoveryTime":2147483648', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST with DiscoveryTime:"69"', function(done) {
                var payload = '{"ClusterMembership":{"DiscoveryTime":"69"}}';
                verifyConfig = JSON.parse( expectUsedDefault ) ;
                verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ClusterMembership Name: cluster Property: DiscoveryTime Type: JSON_STRING"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "DiscoveryTime":"69"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });
        
    });

    // ClusterMembership Reset test cases
    describe('Reset', function() {
    
        it('should return status 200 on POST Restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET All Services Status', function(done) {
            this.timeout( FVT.REBOOT + 5000 );
            FVT.sleep( FVT.REBOOT );
            verifyConfig = JSON.parse( FVT.expectDefaultStatus );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Cluster Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectClusterDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Cluster", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service HighAvailability Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectHighAvailabilityDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/HighAvailability", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service MQConnectivity Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Plugin Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Plugin", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service SNMP Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectSNMPDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/SNMP", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Server Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectServerDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done)
        });

        
    });

});
