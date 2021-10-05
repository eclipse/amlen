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

var FVT = require('../restapi-CommonFunctions.js');

// FVT.getInterfaceName( process.env.A1_USER, process.env.A1_HOST, process.env.A1_IPv4_1, "A1_INTFNAME_1" );
// FVT.getInterfaceName( process.env.A1_USER, process.env.A1_HOST, process.env.A1_IPv4_2, "A1_INTFNAME_2" );
console.log( "@Endpoint A1_INTFNAME_1 is: "+ process.env.A1_INTFNAME_1 + ' ' + FVT.A1_INTFNAME_1 );
console.log( "@Endpoint A1_INTFNAME_2 is: "+ process.env.A1_INTFNAME_2 + ' ' + FVT.A1_INTFNAME_2  );

var verifyConfig = {};
var verifyMessage = {};

//var uriObject = 'Endpoint/';
var uriObject = 'Endpoint';


var expectDefault = '{"Version": "' + FVT.version + '","Endpoint": { \
"DemoEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy", "Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.", "Enabled": false, "Interface": "All", "InterfaceName": "All", "MaxMessageSize": "4096KB", "MessageHub": "DemoHub", "Port": 16102, "Protocol": "All", "SubscriptionPolicies": "DemoSubscriptionPolicy", "TopicPolicies": "DemoTopicPolicy"}, \
"DemoMqttEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy", "Description": "Unsecured endpoint for demonstration use with MQTT protocol only. By default, it uses port 1883.", "Enabled": false, "Interface": "All", "InterfaceName": "All", "MaxMessageSize": "4096KB", "MessageHub": "DemoHub", "Port": 1883, "Protocol": "MQTT", "TopicPolicies": "DemoTopicPolicy"}}}' ;

var expectOneDefault = '{"Version": "' + FVT.version + '","Endpoint":{"DemoEndpoint": { "ConnectionPolicies": "DemoConnectionPolicy", "Description": "Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted.", "Enabled": false, "Interface": "All", "InterfaceName": "All", "MaxMessageSize": "4096KB", "MessageHub": "DemoHub", "Port": 16102, "Protocol": "All", "SubscriptionPolicies": "DemoSubscriptionPolicy", "TopicPolicies": "DemoTopicPolicy"}}}'

//  ====================  Test Cases Start Here  ====================  //
//"ListObjects":"Name,Enabled,Port,Protocol,Interface,SecurityProfile,ConnectionPolicies, {MessagingPolicies .vs. mochaTopicPolicies,QueuePolicies,SubscriptionPolicies} ,MaxMessageSize,MessageHub,MaxSendSize,BatchMessages,Description",

describe('Endpoint:', function() {
this.timeout( FVT.defaultTimeout );

// 99809
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "Endpoint":"EP1":{..},"EP2":{...}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on get, DEFAULT of "Endpoint":"DemoEndpoint":{..}', function(done) {
            verifyConfig = JSON.parse(expectOneDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
    
    });

//  ====================   Create - Add - Update test cases  ====================  //

    describe('Name:', function() {
// DemoMessagingPolicy is gone, eventually MsgingPolicy will go away
        it('should return status 404 on post "Max Length Name" Missing MsgingPolicies', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Port":1493,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","MessagingPolicies": "DemoMessagingPolicy"}}}';
            verifyConfig = JSON.parse(payload);
//          verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
//            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object is not found. Type: MessagingPolicy Name: DemoMessagingPolicy"};
			verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: MessagingPolicy Name: DemoMessagingPolicy"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, "Max Length Name" when Missing MsgingPolicy not created', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"' + FVT.long256Name + '":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Name" Missing MsgingPolicy', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Port":1493,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "No description"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Port":1492,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on REPOST "TestEndpoint"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Port":1492,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET rePOSTed "TestEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "TestSubscribeEndpoint"', function(done) {
            var payload = '{"Endpoint":{"TestSubscribeEndpoint":{"Port":1490,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy", "SubscriptionPolicies":"DemoSubscriptionPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET POSTed "TestSubscriberEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Description:', function() {

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Description": "The TEST Endpoint."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Description": "The NEW TEST Endpoint."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Description":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Port[1--65535]:', function() {

        it('should return status 200 on post "Max Port 65535"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Port":65535}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Port 65535"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Min Port 1"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Port":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Min Port 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Protocol[JMS,MQTT-(ALL)]:', function() {
    
        it('should return status 200 on post "Protocol JMS"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol JMS"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100256
        it('should return status 200 on post "Protocol":"jms,mqtt" (lc)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol":"jms,mqtt"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, Protocol:jms,matt (lc)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol MQTT"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol":"MQTT"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol MQTT"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol ALL"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol":"ALL"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol ALL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 
        it('should return status 200 on post "Protocol:null Default"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol": null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"' + FVT.long256Name + '":{"Protocol": "JMS,MQTT"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol:null Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('MaxMessageSize[1024(1kb)-1024KB-268435456(262144KB)]:', function() {
// Defect 93271, 100008   
        it('should return status 200 on post "MaxMessageSize":"1024" (MIN)', function(done) {
            var payload = '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxMessageSize":"1024"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"1024" (MIN)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// Defect 93271 - shouldn't this be rejected - no K, KB
        it('should return status 200 on post "MaxMessageSize":"268435456" (MAX)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"268435456"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"268435456" (MAX)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"1KB"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"1KB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"1KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"2 KB"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"2 KB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"2 KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"262144KB" (MAX KB)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"262144KB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"262144KB" (MAX KB)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"262143 KB"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"262143 KB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"262143 KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"256MB" (MAX MB)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"256MB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"256MB" (MAX MB)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSiz":"255 MB"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"255 MB"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize":"255 MB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 98141
        it('should return status 200 on post "MaxMessageSize":null (Default)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"' + FVT.long256Name + '":{"MaxMessageSize":"1024KB"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageSize:null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('MaxSendSize (128-16384-65536):', function() {

    it('should return status 200 on post "MaxSendSize MIN 128"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"MaxSendSize":128}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxSendSize MIN 128"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxSendSize":null(Default)', function(done) {
            var payload = '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxSendSize":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxSendSize":16384}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxSendSize":null(Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxSendSize MAX 65536"', function(done) {
            var payload = '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxSendSize":65536}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxSendSize MAX 65536"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxSendSize":null(Default)', function(done) {
            var payload = '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxSendSize":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"'+ FVT.long256Name +'":{"MaxSendSize":16384}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxSendSize":null(Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    describe('BatchMessages(T):', function() {

        it('should return status 200 on post "BatchMessages":false ', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"BatchMessages":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "BatchMessages":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "BatchMessages":true', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"BatchMessages":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "BatchMessages":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "BatchMessages":false', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"BatchMessages":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "BatchMessages":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "BatchMessages":null (Default:T)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"BatchMessages":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"' + FVT.long256Name + '":{"BatchMessages":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "BatchMessages":null (Default:T)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Enabled(T):', function() {

        it('should return status 200 on post "Enabled":false', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Enabled":true', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Enabled":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Enabled":false', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Enabled":null (Default:T)', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long256Name + '":{"Enabled":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"' + FVT.long256Name + '":{"Enabled":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled":null (Default:T)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Interface[ipv4-all-ipv6:', function() {

        it('should return status 200 on POST "Interface":ipv4', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"'+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "Interface":i"all"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"all"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"all"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "Interface":i"All"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"All"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"All"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "Interface":i"ALL"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"ALL"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"all"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100242 116106
        it('should return status 200 on POST "Interface":ipv6', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"['+ FVT.msgServer_IPv6 +']"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":ipv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "Interface":null (DEFAULT:all)', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":null}}}';
            verifyConfig = {"Endpoint":{"TestEndpoint":{"Interface":"All"}}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":null (DEFAULT:all)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('InterfaceName[""]:', function() {
// need to use INTERFACE:IP and see InterfaceName be set to eth0 or something...
/*
ifconfig -s  gives a list of interfaces Put in an object to iterate through
ifconfig listitem | grep for A1_HOST 0-found, 1-is notfound
verify that listitem matched InterfaceName
...unless Arch makes this a settable field , then do something else  TBD
*/


        it('should return status 200 on POST "InterfaceName":"ip1"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"'+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"TestEndpoint":{"Interface":"'+ FVT.msgServer +'", "InterfaceName":"'+ FVT.A1_INTFNAME_1 +'_0"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "InterfaceName":"ip1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "InterfaceName":"ip2"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":"'+ FVT.msgServer_2 +'"}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"TestEndpoint":{"Interface":"'+ FVT.A1_msgServer_2 +'", "InterfaceName":"'+ FVT.A1_INTFNAME_2 +'_0"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "InterfaceName":"ip2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "InterfaceName":"all" (Default)', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Interface":null}}}';
            verifyConfig = JSON.parse( '{"Endpoint":{"TestEndpoint":{"Interface":"All", "InterfaceName":"All"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "InterfaceName":"all" (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });


//  ====================  Hub and Policy test cases  ====================  //
    describe('Hub and Policy Test Cases:', function() {

        it('should return status 200 on post "TestConnPol"', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":{"ClientID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestConnPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConnectionPolicy/TestConnPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestMsgPol"', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":{"GroupID":"theGroup","DestinationType":"Queue","ActionList":"Send,Receive,Browse","Destination":"AQUEUE/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestMsgPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessagingPolicy/TestMsgPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"GroupID":"theGroup","Topic":"type/+/topic","ActionList":"Publish,Subscribe"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestTopicPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "PublishTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"PublishTopicPol":{"GroupID":"theGroup","Topic":"type/+/topic","ActionList":"Publish"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PublishTopicPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/PublishTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MQTTTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"MQTTTopicPol":{"Protocol":"MQTT","Topic":"type/+/topic","ActionList":"Publish,Subscribe"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MQTTTopicPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/MQTTTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "JMSTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"JMSTopicPol":{"Protocol":"JMS","Topic":"type/+/topic","ActionList":"Publish,Subscribe"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "JMSTopicPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/JMSTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestQueuePol"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"GroupID":"theGroup","Queue":"TYPE/QUEUE/*","ActionList":"Send,Receive,Browse"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestQueuePol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueuePolicy/TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestSubscriptionPol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscriptionPol":{"GroupID":"theGroup","Subscription":"type/subscription/*","ActionList":"Receive,Control"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestSubscriptionPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/TestSubscriptionPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestMsgHub"', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestMsgHub"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/TestMsgHub', FVT.getSuccessCallback, verifyConfig, done);
        });



        it('should return status 200 on post "Change MessageHub"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"MessageHub": "TestMsgHub"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change MessageHub"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Change MessagingPolicy" with MQTT TOPIC and Subscription Policies', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"TopicPolicies": "MQTTTopicPol", "SubscriptionPolicies":"TestSubscriptionPol"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change TopicPolicy"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Change MessagingPolicy"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"TopicPolicies": "TestTopicPol"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change TopicPolicy"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Change ConnectionPolicy"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"ConnectionPolicies":"TestConnPol"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change ConnectionPolicy"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Change Port"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Port":1776}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change Port"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Change All Required Fields"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Port":1776,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Change All Required Fields"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        })
	});

//  ====================  Error test cases  ====================  //
describe('Error:', function() {
    describe('General:', function() {
//  No longer NO OP, 
        it('should return status 400 on POST "Endpoint":null - bad form', function(done) {
            var payload = '{"Endpoint":null}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"Endpoint\":null} is not valid." };
//            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully." };
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"Endpoint\":null is not valid." };
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"Endpoint\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after POST  "Endpoint":null - bad form', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST a NOOP chg to "TestEndpoint":{}', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"Endpoint\":null} is not valid." };
//            verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Port Value: null." };
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after a NOOP chg to "TestEndpoint":{}', function(done) {
            verifyConfig = JSON.parse( '{"Endpoint":{"TestEndpoint":{"Port":1776,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
// 100252 
        it('should return status 404 when deleting a "MissingEndpoint"', function(done) {
            var payload = '{"Endpoint":{"MissingEndpoint":null}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+'MissingEndpoint', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "MissingEndpoint" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET of "MissingEndpoint" not found', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MissingEndpoint":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MissingEndpoint', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });
    

    describe('Port[1--65535]:', function() {
//100253  (DISABLED Endpoint are not recognized as using the PORT, want to be able to allow use to TOGGLE between to EP's i.e. Secure/Unsecure...)
//        it('should return status 400 when DUPLICATE Port set  on Enabled:F Endpoint', function(done) {
//            var payload = '{"Endpoint":{"PortInUse-EnabledFalse":{"Port":16102,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
//            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0435","Message":"A port number is in use." };
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
//        });
//        it('should return status 404 on get, PortInUse-EnabledFalse was not created', function(done) {
//            verifyConfig = {"status":404,"Endpoint":{"PortInUse-EnabledFalse":{}}};
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'PortInUse-EnabledFalse', FVT.verify404NotFound, verifyConfig, done);
//        });
// 100253
        it('should return status 400 when DUPLICATE Port set  on Enabled:T Endpoint', function(done) {
            var payload = '{"Endpoint":{"PortInèUse-EnabledTrue":{"Port":1776,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0435","Message":"A port number is in use." };
            verifyMessage = {"status":400,"Code":"CWLNA0155","Message":"The port is in use." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, PortInUse-EnabledTrue was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"PortInUse-EnabledTrue":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'PortInUse-EnabledTrue', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when No Port set ', function(done) {
            var payload = '{"Endpoint":{"PortRequired":{"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0435","Message":"A port number must be specified." };
            verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Port Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, PortRequired was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"PortRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'PortRequired', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when Bad Port set 65536 ', function(done) {
            var payload = '{"Endpoint":{"BadPort":{"Port":65536,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0436","Message":"An invalid port number was specified." };
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Port Value: \"65536\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadPort was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"BadPort":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'BadPort', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when STRING Port set "65535" ', function(done) {
            var objName = 'StringPort' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"Port":"65535","ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: StringPort Property: Port Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, StringPort was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"StringPort":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'StringPort', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when Port:null (NO Default) ', function(done) {
            var objName = 'NullPort' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"Port":null,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: NullPort Property: Port Type: JSON_NULL" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, NullPort was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"NullPort":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'NullPort', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });
    

    describe('ConnectionPolicies:', function() {
// 100254
        it('should return status 400 when No Connection Policy set ', function(done) {
            var objName = 'ConnectionPolicyRequired' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"Port":6969,"MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0333","Message":"No connection policy is configured."};
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ConnectionPolicies Value: null."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConnectionPolicyRequired was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"ConnectionPolicyRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'ConnectionPolicyRequired', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when ConnectionPolicies misspelled ', function(done) {
            var objName = 'ConnectionPolicyMisspelled' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"ConnectionPolicy":"NotCreatedYet","Port":6969,"MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0333","Message":"No connection policy is configured." };
            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Endpoint Name: ConnectionPolicyMisspelled Property: ConnectionPolicy" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConnectionPolicyMisspelled was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"ConnectionPolicyMisspelled":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'ConnectionPolicyMisspelled', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when Connection Policy not found ', function(done) {
            var objName = 'ConnectionPolicyNotFound' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"ConnectionPolicies":"NotCreatedYet","Port":6969,"MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0433","Message":"The endpoint must contain a valid connection policy." };
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: ConnectionPolicy Name: NotCreatedYet" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: NotCreatedYet" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConnectionPolicyNotFound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"ConnectionPolicyNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'ConnectionPolicyNotFound', FVT.verify404NotFound, verifyConfig, done);
        });
    });
    

    describe('MessagingPolicies[T,Q,S]:', function() {
// RULE:  If Subscription Policy, MUST have TOPIC Policy that supports Subscribe Action	
// 108582
        it('should return status 400 when Remove TopicPolicies and SubscriptionPolicies exist', function(done) {
            var payload = '{"Endpoint":{"TestSubscribeEndpoint":{"TopicPolicies":""}}}';
            verifyConfig = {"Endpoint":{"TestSubscribeEndpoint":{"Port":1490,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy", "SubscriptionPolicies":"DemoSubscriptionPolicy"}}};
//            verifyMessage = {"status":400,"Code":"CWLNA0357","Message":"If you specify a messaging policy where the destination type is Subscription, you must specify at least one messaging policy where the destination type is Topic. The topic messaging policy must grant publish authority to the topic that is associated with the subscription name specified in the subscription messaging policy. These messaging policies must be associated with the same endpoint: TestSubscriptionPol."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0379","Message":"If you specify a subscription policy, you must also specify at least one topic policy on which the protocol is MQTT or JMS or both MQTT and JMS. The topic policy must grant publish authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint: TestSubscribeEndpoint."  };
            verifyMessage = {"status":400,"Code":"CWLNA0379","Message":"If you specify a subscription policy, you must also specify at least one topic policy. The topic policy must grant subscribe authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint."  };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, TestSubscribeEndpoint was not updated', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestSubscribeEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
// 108758
        it('should return status 400 on Post to remove all Topic Policies, yet Subscription Policy specified ', function(done) {
            var payload = '{"Endpoint":{"MQTTTopicPolicyRequired":{"TopicPolicies":"","SubscriptionPolicies":"TestSubscriptionPol"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0357","Message":"If you specify a messaging policy where the destination type is Subscription, you must specify at least one messaging policy where the destination type is Topic. The topic messaging policy must grant publish authority to the topic that is associated with the subscription name specified in the subscription messaging policy. These messaging policies must be associated with the same endpoint: MQTTTopicPolicyRequired."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0334","Message":"No messaging policy is configured."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0357","Message":"If you specify a messaging policy where the destination type is Subscription, you must specify at least one messaging policy where the destination type is Topic. The topic messaging policy must grant publish authority to the topic that is associated with the subscription name specified in the subscription messaging policy. These messaging policies must be associated with the same endpoint: MQTTTopicPolicyRequired."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0379","Message":"If you specify a subscription policy, you must also specify at least one topic policy on which the protocol is MQTT or JMS or both MQTT and JMS. The topic policy must grant publish authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint: MQTTTopicPolicyRequired."  };
            verifyMessage = {"status":400,"Code":"CWLNA0379","Message":"If you specify a subscription policy, you must also specify at least one topic policy. The topic policy must grant subscribe authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint."  };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MQTTTopicPolicyRequired was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MQTTTopicPolicyRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MQTTTopicPolicyRequired', FVT.verify404NotFound, verifyConfig, done);
        });
// THIS SHOULD BE Queue Policy with No Topic Policy
        it('should return status 400 on Post to NO Topic Policy, Have a QueuePolicy and Subscription Policy exists ', function(done) {
            var payload = '{"Endpoint":{"TestSubscribeEndpoint":{"TopicPolicies":"", "QueuePolicies":"TestQueuePol"}}}';
            verifyConfig = {"Endpoint":{"TestSubscribeEndpoint":{"Port":1490,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy", "SubscriptionPolicies":"DemoSubscriptionPolicy"}}};
            verifyMessage = {"status":400,"Code":"CWLNA0379","Message":"If you specify a subscription policy, you must also specify at least one topic policy. The topic policy must grant subscribe authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint."  };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, TestSubscribeEndpoint was not updated', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestSubscribeEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
// 112475-112806  (Deemed too complicated to enforce)
//        it('should return status 400 on Post to Publish ONLY Topic Policy, yet Subscription Policy exists ', function(done) {
//            var payload = '{"Endpoint":{"PublishOnlySubscribeEndpoint":{"Port":6669,"TopicPolicies":"PublishTopicPol", "SubscriptionPolicies":"DemoSubscriptionPolicy", "ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
//            verifyConfig = {"Endpoint":{"PublishOnlySubscribeEndpoint":{}}};
//            verifyMessage = {"status":400,"Code":"CWLNA0357","Message":"If you specify a messaging policy where the destination type is Subscription, you must specify at least one messaging policy where the destination type is Topic. The topic messaging policy must grant publish authority to the topic that is associated with the subscription name specified in the subscription messaging policy. These messaging policies must be associated with the same endpoint: MQTTTopicPolicyRequired."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0334","Message":"No messaging policy is configured."  };
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
//        });
//        it('should return status 404 on get, PublishOnlySubscribeEndpoint was not updated', function(done) {
//            verifyConfig = {"status":404,"Endpoint":{"PublishOnlySubscribeEndpoint":{}}};
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'PublishOnlySubscribeEndpoint', FVT.verify404NotFound, verifyConfig, done);
//        });
// 100254
// should eventually have msg without MsgingPolicy in list....9/17
        it('should return status 400 when No Messaging Policy set ', function(done) {
            var objName = 'MessagingPolicyRequired' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0334","Message":"No messaging policy is configured."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: MessagingPolicies Value: null."  };
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: Endpoint must have one of the properties TopicPolicies,QueuePolicies,MessagingPolicies,SubscriptionPolicies specified"  };
            verifyMessage = {"status":400,"Code":"CWLNA0334","Message":"No messaging policy is configured."  };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MessagingPolicyRequired was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MessagingPolicyRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MessagingPolicyRequired', FVT.verify404NotFound, verifyConfig, done);
        });
        it('should return status 404 when MessagingPolicy misspelled ', function(done) {
            var payload = '{"Endpoint":{"MessagingPolicyMispelled":{"MessagingPolicy":"DemoMessagingPolicy","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0334","Message":"No messaging policy is configured." };
            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Endpoint Name: MessagingPolicyMispelled Property: MessagingPolicy" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MessagingPolicyMispelled was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MessagingPolicyMispelled":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MessagingPolicyMispelled', FVT.verify404NotFound, verifyConfig, done);
        });
// should eventually go away... 9/17
        it('should return status 404 when Messaging Policy not found ', function(done) {
            var payload = '{"Endpoint":{"MessagingPolicyNotFound":{"MessagingPolicies":"NotCreatedYet","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0434","Message":"The endpoint must contain a valid messaging policy." };
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: MessagingPolicy Name: NotCreatedYet" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessagingPolicy Name: NotCreatedYet" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MessagingPolicyNotFound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MessagingPolicyNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MessagingPolicyNotFound', FVT.verify404NotFound, verifyConfig, done);
        });
        
        it('should return status 404 when Topic Policy not found ', function(done) {
            var payload = '{"Endpoint":{"TopicPolicyNotFound":{"TopicPolicies":"NotCreatedYet","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: TopicPolicy Name: NotCreatedYet" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: NotCreatedYet" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, TopicPolicyNotFound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"TopicPolicyNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TopicPolicyNotFound', FVT.verify404NotFound, verifyConfig, done);
        });
        
        it('should return status 404 when Queue Policy not found ', function(done) {
            var payload = '{"Endpoint":{"QueuePolicyNotFound":{"QueuePolicies":"NotCreatedYet","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: QueuePolicy Name: NotCreatedYet" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: NotCreatedYet" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, QueuePolicyNotFound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"QueuePolicyNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'QueuePolicyNotFound', FVT.verify404NotFound, verifyConfig, done);
        });
        
        it('should return status 404 when Subscription Policy not found ', function(done) {
            var payload = '{"Endpoint":{"SubscriptionPolicyNotFound":{"SubscriptionPolicies":"NotCreatedYet","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: SubscriptionPolicy Name: NotCreatedYet" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: NotCreatedYet" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, SubscriptionPolicyNotFound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"SubscriptionPolicyNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'SubscriptionPolicyNotFound', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

    describe('MessageHub:', function() {

        it('should return status 200 on post an extra "TestMsgHub" ', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/', FVT.getSuccessCallback, verifyConfig, done);
        });
// 100254
        it('should return status 400 when Multiple Message Hubs are set ', function(done) {
            var objName = 'OneMessageHubTooMany' ;
            var payload = '{"Endpoint":{"'+ objName +'":{"MessageHub":"DemoMessageHub,TestMsgHub","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0431","Message":"Invalid MessageHub was specified." };
//            verifyMessage = {"Code":"CWLNA0129","Message":"The endpoint must be part of a message hub." };
//            verifyMessage = {"status":400,"Code":"CWLNA0431","Message":"Invalid MessageHub was specified." };
            verifyMessage = {"status":400,"Code":"CWLNA0431","Message":"Invalid MessageHub DemoMessageHub,TestMsgHub was specified." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, OneMessageHubTooMany was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"OneMessageHubTooMany":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'OneMessageHubTooMany', FVT.verify404NotFound, verifyConfig, done);
        });
// 100254, 101153
        it('should return status 400 when No Message Hub set ', function(done) {
            var objName = 'MessageHubRequired';
            var payload = '{"Endpoint":{"'+ objName +'":{"Port":6968,"ConnectionPolicies": "DemoConnectionPolicy","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0430","Message":"A MessageHub must be specified." };
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: MessageHub Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MessageHubRequired was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MessageHubRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MessageHubRequired', FVT.verify404NotFound, verifyConfig, done);
        });
// 100254
        it('should return status 400 when Message Hub not found ', function(done) {
            var objName = 'MessageHubNotFound';
            var payload = '{"Endpoint":{"'+ objName +'":{"MessageHub":"NotCreatedYet","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0431","Message":"Invalid MessageHub was specified." };
//            verifyMessage = {"Code":"CWLNA0129","Message":"The endpoint must be part of a message hub." };
//            verifyMessage = {"status":400,"Code":"CWLNA0431","Message":"Invalid MessageHub was specified." };
            verifyMessage = {"status":400,"Code":"CWLNA0431","Message":"Invalid MessageHub NotCreatedYet was specified." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MessageHubNotfound was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MessageHubNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MessageHubNotFound', FVT.verify404NotFound, verifyConfig, done);
        });
        
/*  I want this later, why am i deleting it here, can I skip it?
        it('should return status 200 when deleting "TestMsgHub"', function(done) {
            var objName = 'TestMsgHub';
            var payload = '{"MessageHub":{"'+ objName +'":{}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'MessageHub/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestMsgHub" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestMsgHub" not found', function(done) {
            verifyConfig = {"status":404,"MessageHub":{"TestMsgHub":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/TestMsgHub', FVT.verify404NotFound, verifyConfig, done);
        });
 */       
    });
    

    describe('Name:', function() {

        it('should return status 400 on post "Too Long Name"', function(done) {
            var payload = '{"Endpoint":{"' + FVT.long257Name + '":{"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Endpoint Name: '+ FVT.long257Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Too Long Name"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"' + FVT.long257Name + '":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('Description:', function() {

        it('should return status 400 on post "TOO LONG Description"', function(done) {
            var payload = '{"Endpoint":{"TooLongDescEndpoint":{"Description": "'+ FVT.long1025String +'"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage =  JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST "Too Long Description" fails', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"TooLongDescEndpoint":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TooLongDescEndpoint', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "Description":69', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"Description":69}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage =  {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: TestEndpoint Property: Description Type: JSON_INTEGER"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "Description":69 fails', function(done) {
            verifyConfig = JSON.parse( '{"Endpoint":{"TestEndpoint":{"Port":1776,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Protocol[JMS,MQTT-(ALL)]:', function() {
// 100256
        it('should return status 400 when trying to set Invalid Protocol SCADA', function(done) {
            var payload = '{"Endpoint":{"BadProtocolScada":{"Protocol":"MQTT,JMS,SCADA","Port":6970,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT,JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocolScada was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"BadProtocolScada":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'BadProtocolScada', FVT.verify404NotFound, verifyConfig, done);
        });
// 100256
        it('should return status 400 on post "Protocol":"jms,mqtt,all"', function(done) {
            var payload = '{"Endpoint":{"Protocol_jmsmqttall":{"Protocol":"jms,mqtt,all","Port":6971,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"jms,mqtt,all\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "Protocol":"jms,mqtt,all" was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"Protocol_jmsmqttall":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'Protocol_jmsmqttall', FVT.verify404NotFound, verifyConfig, done);
        });
// 100254, 101154
        it('should return status 400 on post "Protocol":""', function(done) {
            var payload = '{"Endpoint":{"Protocol_blank":{"Protocol":"","Port":6973,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"\"\"\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Protocol_blank was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"Protocol_blank":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'Protocol_blank', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('MaxMessageSize[1024(1kb)-1024KB-268435456(262144KB)]:', function() {

        it('should return status 400 on post "MaxMessageSize":"1023"', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz1023":{"MaxMessageSize":"1023","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"1023\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz1023 was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz1023":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz1023', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":268435456 (not String)', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz268435456":{"MaxMessageSize":268435456,"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: MaxMsgSz268435456 Property: MaxMessageSize Type: JSON_INTEGER" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz268435456 was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz268435456":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz268435456', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":"3K"', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz3K":{"MaxMessageSize":"3K","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"3K\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz3K was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz3K":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz3K', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":"4 K"', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz4_K":{"MaxMessageSize":"4 K","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"4 K\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz4_K was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz4_K":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz4_K', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":"262145KB" (over MAX KB)', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz262145KB":{"MaxMessageSize":"262145KB","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"262145KB\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz262145KB was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz262145KB":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz262145KB', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":"257MB" (over MAX MB)', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz257MB":{"MaxMessageSize":"257MB","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"257MB\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz257MB was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz257MB":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz257MB', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxMessageSize":"254M"', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz254M":{"MaxMessageSize":"254M","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"254M\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz254M was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz254M":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz254M', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"253 M"', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSz253_M":{"MaxMessageSize":"253 M","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"253 M\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSz253_M was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSz253_M":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSz253_M', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageSize":"" ', function(done) {
            var payload = '{"Endpoint":{"MaxMsgSzempty":{"MaxMessageSize":"","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: \"NULL\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxMsgSzempty was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxMsgSzempty":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxMsgSzempty', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('MaxSendSize (128-16384-65536):', function() {

        it('should return status 400 on post "MaxSendSize":"127"', function(done) {
            var payload = '{"Endpoint":{"MaxSendSz127":{"MaxSendSize":"127","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: MaxSendSz127 Property: MaxSendSize Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxSendSz127 was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxSendSz127":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxSendSz127', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxSendSize":127', function(done) {
            var payload = '{"Endpoint":{"MaxSendSz127":{"MaxSendSize":127,"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxSendSize Value: \"127\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxSendSz127 was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxSendSz127":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxSendSz127', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxSendSize":"" ', function(done) {
            var payload = '{"Endpoint":{"MaxSendSzempty":{"MaxSendSize":"","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: MaxSendSzempty Property: MaxSendSize Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxSendSzempty was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxSendSzempty":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxSendSzempty', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MaxSendSize":65537"', function(done) {
            var payload = '{"Endpoint":{"MaxSendSz65537":{"MaxSendSize":"65537","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: MaxSendSz65537 Property: MaxSendSize Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, MaxSendSz65537 was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"MaxSendSz65537":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'MaxSendSz65537', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

    describe('BatchMessages(T):', function() {

        it('should return status 400 on post "BatchMessages":"false" ', function(done) {
            var payload = '{"Endpoint":{"BatchMessagesString":{"BatchMessages":"false","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: BatchMessagesString Property: BatchMessages Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BatchMessagesString was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"BatchMessagesString":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'BatchMessagesString', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "BatchMessages":0', function(done) {
            var payload = '{"Endpoint":{"BatchMessagesNumber":{"BatchMessages":0,"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: BatchMessagesNumber Property: BatchMessages Type: JSON_INTEGER" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BatchMessagesNumber was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"BatchMessagesNumber":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'BatchMessagesNumber', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('Enabled(T):', function() {

        it('should return status 400 on post "Enabled":"false" ', function(done) {
            var payload = '{"Endpoint":{"EnabledString":{"Enabled":"false","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: EnabledString Property: Enabled Type: JSON_STRING" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, EnabledString was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"EnabledString":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'EnabledString', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "Enabled":0', function(done) {
            var payload = '{"Endpoint":{"EnabledNumber":{"Enabled":0,"Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Endpoint Name: EnabledNumber Property: Enabled Type: JSON_INTEGER" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, EnabledNumber was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"EnabledNumber":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'EnabledNumber', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('Interface[ipv4-all-ipv6:', function() {
// 100242 116106, BRACKETS are only REQUIRED on HTTP URI, this is valid
        it('should return status 200 on POST "Interface":"IPv6" with no []', function(done) {
            var payload = '{"Endpoint":{"InterfaceIPv6":{"Interface":"'+ FVT.msgServer_IPv6 +'","Port":9696,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: \"'+ FVT.msgServer_IPv6 +'\"." };
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, InterfaceIPv6 no [] was created', function(done) {
//            verifyConfig = {"status":404,"Endpoint":{"InterfaceIPv6":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'InterfaceIPv6', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "Interface":"localhost"', function(done) {
            var payload = '{"Endpoint":{"InterfaceHostname":{"Interface":"localhost","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: \"localhost\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, InterfaceHostname was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"InterfaceHostname":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'InterfaceHostname', FVT.verify404NotFound, verifyConfig, done);
        });
// 101162
        it('should return status 200 on POST "Interface":""', function(done) {
            var payload = '{"Endpoint":{"InterfaceEmpty":{"Interface":"","Port":6975,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: JSON_NULL." };
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: \"\"\"\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, InterfaceEmpty was not created', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"InterfaceEmpty":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'InterfaceEmpty/', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('InterfaceName[""]:', function() {
// 107354 (no longer 404/403)
        it('should return status 200 on POST "InterfaceName":eth0 (not setable)', function(done) {
            var payload = '{"Endpoint":{"InterfaceNameNoSet":{"InterfaceName":"eth0","Port":6969,"ConnectionPolicies": "DemoConnectionPolicy","MessageHub": "DemoHub","TopicPolicies": "DemoTopicPolicy"}}}';
            verifyConfig = JSON.parse( payload );
// 107354            verifyMessage = {"status":403,"Code":"CWLNA0111","Message":"The property name is not valid: Property: InterfaceName." };
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, InterfaceNameNoSet was not created', function(done) {
//            verifyConfig = {"status":404,"Endpoint":{"InterfaceNameNoSet":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
    
  });
 

//  ====================  Delete test cases  ====================  //
    describe('Delete Test Cases:', function() {

        it('should return status 200 when deleting "InterfaceIPv6"', function(done) {
            var payload = '{"Endpoint":{"InterfaceIPv6":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+"InterfaceIPv6", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "InterfaceIPv6" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "InterfaceIPv6" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"InterfaceIPv6":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+"InterfaceIPv6/", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "InterfaceNameNoSet"', function(done) {
            var payload = '{"Endpoint":{"InterfaceNameNoSet":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+"InterfaceNameNoSet", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "InterfaceNameNoSet" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "InterfaceNameNoSet" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"InterfaceNameNoSet":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+"InterfaceNameNoSet", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"Endpoint":{"'+ FVT.long256Name +'":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Endpoint":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });
		//  need to test delete of In Use MsgHub, ConnectPol and MsgPol
// eventually have MsgingPolicy go away 9/17
        it('should return status 200 on post "TestEnpoint" to set Dependency Delete Test', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"ConnectionPolicies": "TestConnPol","MessageHub": "TestMsgHub","MessagingPolicies": "TestMsgPol", "TopicPolicies":"", "QueuePolicies":"", "SubscriptionPolicies":"" }}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestEnpoint" to set Dependency Delete Test', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when deleting "TestMsgPol" in use by TestEndpoint', function(done) {
            var objName = 'TestMsgPol' ;
            var payload = '{"MessagingPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"status":400,"Code": "CWLNA0439","Message": "The policy TestMsgPol is in use."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: MessagingPolicies, Name: TestMsgPol is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'MessagingPolicy/'+objName, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed delete, "TestMsgPol" in use', function(done) {
            verifyConfig = {"MessagingPolicy":{"TestMsgPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessagingPolicy/'+'TestMsgPol', FVT.getSuccessCallback, verifyConfig, done);
        });

		//  need to test delete of In Use MsgHub, ConnectPol and [T,Q,S]Policy
        it('should return status 200 on REPOST "TestEnpoint" to set Dependency Delete Test', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{"ConnectionPolicies": "TestConnPol","MessageHub": "TestMsgHub","MessagingPolicies": "", "TopicPolicies":"TestTopicPol", "QueuePolicies":"TestQueuePol", "SubscriptionPolicies":"TestSubscriptionPol" }}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on REGET, "TestEnpoint" to set Dependency Delete Test', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 108593
        it('should return status 400 when deleting "TestTopicPol" in use by TestEndpoint', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"status":400,"Code": "CWLNA0439","Message": "The policy TestTopicPol is in use."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: TopicPolicies, Name: TestTopicPol is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/TestTopicPol', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed delete, "TestTopicPol" in use', function(done) {
            verifyConfig = {"TopicPolicy":{"TestTopicPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });
// 108593
        it('should return status 400 when deleting "TestQueuePol" in use by TestEndpoint', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"status":400,"Code": "CWLNA0439","Message": "The policy TestQueuePol is in use."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: QueuePolicies, Name: TestQueuePol is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'QueuePolicy/TestQueuePol', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed delete, "TestQueuePol" in use', function(done) {
            verifyConfig = {"QueuePolicy":{"TestQueuePol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueuePolicy/'+'TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });
// 108593
        it('should return status 400 when deleting "TestSubscriptionPol" in use by TestEndpoint', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscriptionPol":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"status":400,"Code": "CWLNA0439","Message": "The policy TestSubscriptionPol is in use."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: SubscriptionPolicies, Name: TestSubscriptionPol is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SubscriptionPolicy/TestSubscriptionPol', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed delete, "TestSubscriptionPol" in use', function(done) {
            verifyConfig = {"SubscriptionPolicy":{"TestSubscriptionPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/TestSubscriptionPol', FVT.getSuccessCallback, verifyConfig, done);
        });
// 100557
        it('should return status 400 when deleting "TestConnPol" in use by TestEndpoint', function(done) {
            var objName = 'TestConnPol'
            var payload = '{"ConnectionPolicy":{"'+ objName +'":{}}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"Code": "CWLNA0439","Message": "The policy TestConnPol is in use."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: ConnectionPolicies, Name: TestConnPol is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'ConnectionPolicy/'+objName, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed delete, "TestConnPol" in use', function(done) {
            verifyConfig = {"ConnectionPolicy":{"TestConnPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConnectionPolicy/'+'TestConnPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when deleting "TestMsgHub" in use by TestEndpoint', function(done) {
            var objName = 'TestMsgHub'
            var payload = '{"MessageHub":{"'+ objName +'":{}}}';
            verifyConfig = JSON.parse( payload ) ;
//            verifyMessage = {"Code": "CWLNA0439","Message": "The policy TestMsgHub is in use."};
//            verifyMessage = {"Code": "CWLNA0438","Message":"MessageHub is in use. It still contains at least one endpoint."};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message":"The Object: MessageHub, Name: TestMsgHub is still being used by Object: Endpoint, Name: TestEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'MessageHub/'+objName, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after delete, "TestMsgHub" not found', function(done) {
            verifyConfig = {"MessageHub":{"TestMsgHub":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/'+'TestMsgHub', FVT.getSuccessCallback, verifyConfig, done);
        });
        

        it('should return status 200 when deleting "TestEndpoint" ', function(done) {
            var objName = 'TestEndpoint'
            var payload = '{"Endpoint":{"'+ objName +'":null}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestEndpoint" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestEndpoint" not found', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"TestEndpoint":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestEndpoint', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });



// Clean up the dependencies created to test Hub and Policies 
describe('PreReq Cleanup:', function() {
        it('should return status 404 when REdeleting "TestEndpoint"', function(done) {
            var payload = '{"Endpoint":{"TestEndpoint":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+'TestEndpoint', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after redelete, "TestEndpoint" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after redelete, "TestEndpoint" not found', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"TestEndpoint":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestEndpoint', FVT.verify404NotFound, verifyConfig, done);
        });
		
        it('should return status 200 when deleting "TestSubscribeEndpoint"', function(done) {
            var payload = '{"Endpoint":{"TestSubscribeEndpoint":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'/'+'TestSubscribeEndpoint', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after redelete, "TestSubscribeEndpoint" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after redelete, "TestSubscribeEndpoint" not found', function(done) {
            verifyConfig = {"status":404,"Endpoint":{"TestSubscribeEndpoint":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'/'+'TestSubscribeEndpoint', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "TestMsgPol"', function(done) {
            var objName = 'TestMsgPol' ;
            var payload = '{"MessagingPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'MessagingPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestMsgPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessagingPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestMsgPol" not found', function(done) {
            verifyConfig = {"status":404,"MessagingPolicy":{"TestMsgPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessagingPolicy/'+'TestMsgPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "TestTopicPol"', function(done) {
            var objName = 'TestTopicPol' ;
            var payload = '{"TopicPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestTopicPol" not found', function(done) {
            verifyConfig = {"status":404,"TopicPolicy":{"TestTopicPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/'+'TestTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "PublishTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"PublishTopicPol":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/PublishTopicPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "PublishTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "PublishTopicPol" not found', function(done) {
            verifyConfig = {"status":404,"TopicPolicy":{"PublishTopicPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/PublishTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "MQTTTopicPol"', function(done) {
            var objName = 'MQTTTopicPol' ;
            var payload = '{"TopicPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "MQTTTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "MQTTTopicPol" not found', function(done) {
            verifyConfig = {"status":404,"TopicPolicy":{"MQTTTopicPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/'+'MQTTTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "JMSTopicPol"', function(done) {
            var objName = 'JMSTopicPol' ;
            var payload = '{"TopicPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "JMSTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "JMSTopicPol" not found', function(done) {
            verifyConfig = {"status":404,"TopicPolicy":{"JMSTopicPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/'+'JMSTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "TestSubscriptionPol"', function(done) {
            var objName = 'TestSubscriptionPol' ;
            var payload = '{"SubscriptionPolicy":{"'+ objName +'":{}}}' ;
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SubscriptionPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestSubscriptionPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestSubscriptionPol" not found', function(done) {
            verifyConfig = {"status":404,"SubscriptionPolicy":{"TestSubscriptionPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/'+'TestSubscriptionPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "TestConnPol"', function(done) {
            var objName = 'TestConnPol'
            var payload = '{"ConnectionPolicy":{"'+ objName +'":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'ConnectionPolicy/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestConnPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConnectionPolicy/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestConnPol" not found', function(done) {
            verifyConfig = {"status":404,"ConnectionPolicy":{"TestConnPol":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConnectionPolicy/'+'TestConnPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "TestMsgHub"', function(done) {
            var objName = 'TestMsgHub'
            var payload = '{"MessageHub":{"'+ objName +'":{}}}';
            verifyConfig = JSON.parse( payload ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'MessageHub/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestMsgHub" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestMsgHub" not found', function(done) {
            verifyConfig = {"status":404,"MessageHub":{"TestMsgHub":{}}}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+'MessageHub/'+'TestMsgHub', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

    
});
