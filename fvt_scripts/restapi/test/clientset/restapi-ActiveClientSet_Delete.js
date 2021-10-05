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
var fs = require('fs')
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var expectDefault = '' ;
var expectDefaultMQTTClient = '{ "MQTTClient": [], "Version": "'+ FVT.version +'" }' ;
var expectDefaultSubscription = '{ "Subscription": [], "Version": "'+ FVT.version +'" }' ;
var expectDefaultConnection = '{ "Connection": [], "Version": "'+ FVT.version +'" }' ;


// Results after Durable Subs and Pubs (no receives)
var expectA1AllSubscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/1","TopicString":"/CD/001/1","ClientID":"a-ian-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/2","TopicString":"/CD/001/2","ClientID":"a-ian-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-ian-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-ian-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-ian-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/1","TopicString":"/CD/001/1","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/2","TopicString":"/CD/001/2","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/9","TopicString":"/CD/001/9","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/10","TopicString":"/CD/001/10","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/11","TopicString":"/CD/001/11","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

var expectA1AllMQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-ian-uid11","IsConnected":false}, \
 {"ClientID":"a-ian-uid12","IsConnected":false}, \
 {"ClientID":"a-gro-uid11","IsConnected":false}, \
 {"ClientID":"d:org:pub:cid1","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

var expectA1AllConnection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "a-gro-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-ian-uid12",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-ian-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;



var expectA2AllSubscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-ian-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-ian-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-ian-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/6","TopicString":"/CD/001/6","ClientID":"a-ian-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/7","TopicString":"/CD/001/7","ClientID":"a-ian-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-ian-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-ian-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/6","TopicString":"/CD/001/6","ClientID":"a-ian-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/7","TopicString":"/CD/001/7","ClientID":"a-ian-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-ian-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

var expectA2AllMQTTClient = '{ "MQTTClient": [ \
{"ClientID":"a-ian-uid21","IsConnected":false}, \
{"ClientID":"a-ian-uid22","IsConnected":false}, \
{"ClientID":"d:org:pub:cid1","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

var expectA2AllConnection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid2", "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-ian-uid22",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-ian-uid21",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;



// Results after Delete #1 on A2 where ClientSet?ClientID=^a-ian-uid2&RETAIN=^ (Subscriptions = default)
//var expectA2Delete1MQTTClient = '{ "MQTTClient": [ {"ClientID":"d:org:pub:cid2","IsConnected":false} ], "Version": "'+ FVT.version +'" }' ;
var expectA2Delete1MQTTClient = expectDefaultMQTTClient ;
var expectA2Delete1Connection = '{ "Connection": [{ "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid2", "Port": 16102, "Protocol": "mqtt", "UserId": "" }], "Version": "'+ FVT.version +'" }' ;

// really expect no change on A1, delete was on A2.



// Results after Delete #2 on A1 where ClientSet?ClientID=^a-ian-uid11&Retain=^ 
var expectA1Delete2Subscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/1","TopicString":"/CD/001/1","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/2","TopicString":"/CD/001/2","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-ian-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/9","TopicString":"/CD/001/9","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/10","TopicString":"/CD/001/10","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/11","TopicString":"/CD/001/11","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

//var expectA1Delete2MQTTClient = '{ "MQTTClient": [ \
// {"ClientID":"a-ian-uid12","IsConnected":false}, \
// {"ClientID":"a-gro-uid11","IsConnected":false}, \
// {"ClientID":"d:org:pub:cid1","IsConnected":false} \
//], "Version": "'+ FVT.version +'" }' ;
var expectA1Delete2MQTTClient = '{ "MQTTClient": [], "Version": "'+ FVT.version +'" }' ;


var expectA1Delete2Connection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "a-gro-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-ian-uid12",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;

// RESTART of Server will cause Durable MQTTClient to get disconnected and all Connection lost
var expectA1Restart1MQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-ian-uid12","IsConnected":false}, \
 {"ClientID":"a-gro-uid11","IsConnected":false}, \
 {"ClientID":"d:org:pub:cid1","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

var expectA1Restart1Connection = '{"Connection": [], "Version": "v1" }' ;

// Results after Delete #3 on A1 where ClientSet?ClientID=^a-ian&Retain=^/CD/001
var expectA1Delete3Subscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/9","TopicString":"/CD/001/9","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/10","TopicString":"/CD/001/10","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/11","TopicString":"/CD/001/11","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

var expectA1Delete3MQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-gro-uid11","IsConnected":false}, \
 {"ClientID":"d:org:pub:cid1","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

// Restart KILLs all Connections
//var expectA1Delete3Connection = '{"Connection": [ \
// { "Endpoint": "DemoEndpoint", "Name": "a-gro-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
// { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
//], "Version": "v1" }' ;



// Results after Delete #4 on A2 where ClientSet?ClientID=^d:org&Retain=^/CD  
// (defaults for A2 Subscription, MQTTClient and Connection ; A1 no change)

// Results after Delete #5 on A1 where ClientSet?ClientID=^&Retain=^  (defaults for A1 Subscription and MQTTClient)
// defaults for A1 and A2 Subscription, MQTTClient, and Connection



var uriObject = 'ClientSet/' ;

var verifyConfig = {};
var verifyMessage = {};
//  ====================  Test Cases Start Here  ====================  //
//  Delete Retained messages in Durable Topic /CD/001/3 for ClientID with name starting a-ian, then Delete entire a-ian-cid1 ClientSet.

describe('ClientSet:', function() {
this.timeout( 30000 );

    describe('Verify Initial Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1', function(done) {
            verifyConfig = JSON.parse( expectA1AllConnection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });


    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A2', function(done) {
            verifyConfig = JSON.parse(expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2', function(done) {
            verifyConfig = JSON.parse( expectA2AllConnection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });



    // Delete #1 on A2 where ClientSet?ClientID=^a-ian-uid2&RETAIN=^
    describe('A2.Delete 1 where ClientSet?ClientID=^a-ian-uid2&RETAIN=^:', function() {

        it('should return 200 on DELETE ClientSet to Delete#1', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 2, Clients deleted: 2, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^a-ian-uid2", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete#1 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
		    FVT.sleep( FVT.DELETE_CLIENTSET );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 2, Clients deleted: 2, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^a-ian-uid2", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });


    describe('Verify Delete#1 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics (no change expected)
        it('should return 200 on GET Subscription @ A1-Delete1', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-Delete1', function(done) {
            verifyConfig = JSON.parse( expectA1AllConnection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A2-Delete1', function(done) {
            verifyConfig = JSON.parse(expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });



    // Delete #2 on A1 where ClientSet?ClientID=^a-ian-uid11&Retain=^
    describe('A1.Delete 2 where ClientSet?ClientID=^a-ian-uid11&Retain=^:', function() {

        it('should return 200 on DELETE ClientSet to Delete#2', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^a-ian-uid11&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete#2 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
		    FVT.sleep( FVT.DELETE_CLIENTSET );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^a-ian-uid11&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify Delete#2 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-Delete2', function(done) {
            verifyConfig = JSON.parse(expectA1Delete2Subscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete2', function(done) {
            verifyConfig = JSON.parse( expectA1Delete2MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A1-Delete2', function(done) {
            verifyConfig = JSON.parse( expectA1Delete2Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1Connection   );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

			
    // Config persists Check
    describe('Restart Server, recheck Statistics:', function() {
	
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		this.timeout( FVT.REBOOT + 5000 ) ; 
    		FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectClusterConfigedStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-Delete2', function(done) {
            verifyConfig = JSON.parse(expectA1Delete2Subscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete2', function(done) {
            verifyConfig = JSON.parse( expectA1Restart1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A1-Delete2', function(done) {
            verifyConfig = JSON.parse( expectA1Restart1Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-Delete2 is Delete1', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1Connection   );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });


    // Delete #3 on A1 where ClientSet?ClientID=^a-ian&Retain=^/CD/001
    describe('A1.Delete 3 where ClientSet?ClientID=^a-ian&Retain=^/CD/001', function() {

        it('should return 200 on DELETE ClientSet to Delete#3', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^a-ian&Retain=^/CD/001", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete#3 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
		    FVT.sleep( FVT.DELETE_CLIENTSET );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^a-ian&Retain=^/CD/001", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify Delete#3 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-Delete3', function(done) {
            verifyConfig = JSON.parse(expectA1Delete3Subscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete3', function(done) {
            verifyConfig = JSON.parse( expectA1Delete3MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-Delete3', function(done) {
            verifyConfig = JSON.parse( expectA1Restart1Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete3 is Delete1 still', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete3 is Delete1 still', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A2-Delete3 is Delete1 still', function(done) {
            verifyConfig = JSON.parse( expectA2Delete1Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });



    // Delete #4 on A2 where ClientSet?ClientID=^d:org&Retain=^/CD
    describe('A2.Delete 4 where ClientSet?ClientID=^d:org&Retain=^/CD', function() {

        it('should return 200 on DELETE ClientSet to Delete#4', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^d:org&Retain=^/CD", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete#4 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
		    FVT.sleep( FVT.DELETE_CLIENTSET );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 1, Clients deleted: 1, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^d:org&Retain=^/CD", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify Delete#4 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-Delete4 is Delete3', function(done) {
            verifyConfig = JSON.parse( expectA1Delete3Subscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete4 is Delete3', function(done) {
            verifyConfig = JSON.parse( expectA1Delete3MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-Delete4 is Delete3', function(done) {
            verifyConfig = JSON.parse( expectA1Restart1Connection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete4 is default', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete4 is default', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A2-Delete4', function(done) {
//            verifyConfig = JSON.parse( expectDefaultConnection  );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

});

    // Delete #5 on A1 where ClientSet?ClientID=^&Retain=^
    describe('A1.Delete 5 where ClientSet?ClientID=^&Retain=^', function() {

        it('should return 200 on DELETE ClientSet to Delete#5', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 2, Clients deleted: 2, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete#5 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
		    FVT.sleep( FVT.DELETE_CLIENTSET );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 2, Clients deleted: 2, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify Delete#5 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-Delete5 is default', function(done) {
            verifyConfig = JSON.parse(expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete5 is default', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-Delete5 at end', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection  );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete5 at end', function(done) {
            verifyConfig = JSON.parse(expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete5 at end', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A2-Delete5 at end', function(done) {
//            verifyConfig = JSON.parse( expectDefaultConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    });


});
