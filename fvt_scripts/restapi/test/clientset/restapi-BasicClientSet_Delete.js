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

var expect5Subscription = '{ "Subscription": [ \
  { "TopicString": "/CD/001/1", "ClientID": "a-org-uid1", "Consumers": 0, "IsDurable": true, "IsShared": false, "MaxMessages": 5000, "MessagingPolicy": "DemoTopicPolicy", "SubName": "/CD/001/1" }, \
  { "TopicString": "/CD/001/2", "ClientID": "a-org-uid1", "Consumers": 0, "IsDurable": true, "IsShared": false, "MaxMessages": 5000, "MessagingPolicy": "DemoTopicPolicy", "SubName": "/CD/001/2" }, \
  { "TopicString": "/CD/001/3", "ClientID": "a-org-uid1", "Consumers": 0, "IsDurable": true, "IsShared": false, "MaxMessages": 5000, "MessagingPolicy": "DemoTopicPolicy", "SubName": "/CD/001/3" }, \
  { "TopicString": "/CD/001/4", "ClientID": "a-org-uid1", "Consumers": 0, "IsDurable": true, "IsShared": false, "MaxMessages": 5000, "MessagingPolicy": "DemoTopicPolicy", "SubName": "/CD/001/4" }, \
  { "TopicString": "/CD/001/5", "ClientID": "a-org-uid1", "Consumers": 0, "IsDurable": true, "IsShared": false, "MaxMessages": 5000, "MessagingPolicy": "DemoTopicPolicy", "SubName": "/CD/001/5" }  \
], "Version": "'+ FVT.version +'" }' ;

var expect2MQTTClient = '{ "MQTTClient": [ \
  { "ClientID": "a-org-uid1", "IsConnected": false }, \
  { "ClientID": "d:org:pub:cid1", "IsConnected": false } ], "Version": "'+ FVT.version +'" }' ;


var expect1MQTTClient = '{ "MQTTClient": [ { "ClientID": "d:org:pub:cid1", "IsConnected": false } ], "Version": "'+ FVT.version +'" }' ;


var uriObject = 'ClientSet/' ;

var verifyConfig = {};
var verifyMessage = {};
//  ====================  Test Cases Start Here  ====================  //
//  Delete Retained messages in Durable Topic /CD/001/3 for ClientID with name starting a-org, then Delete entire a-org-cid1 ClientSet.

describe('ClientSet:', function() {
this.timeout( FVT.defaultTimeout );

    // Verify Expected Subscriptions exist
    describe('Verify Subscriptions:', function() {

        it('should return 200 on GET Subscriptions', function(done) {
            verifyConfig = JSON.parse(expect5Subscription );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    // Verify Expected MQTT Durable Clients have connected
    describe('Verify MQTTClient:', function() {

        it('should return 200 on GET MQTTClients', function(done) {
            verifyConfig = JSON.parse( expect2MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    // Delete  'a-org-uid1' ClientID (everything) and Verify
    describe('Delete All ClientID a-org-uid1:', function() {

        it('should return 200 on DELETE ClientSet to Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197"};
            FVT.makeDeleteRequest( FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^a-org-uid1$", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });


    });

    // Verify Expected MQTT Durable Subscriber is deleted and Subscriptions gone
    describe('Verify Sub Statistics:', function() {
	
        it('should return 200 on GET MQTTClients after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expect1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

		
    // Config persists Check
    describe('Restart Server, check Sub Statistics:', function() {
	
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
	
        it('should return 200 on GET MQTTClients after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expect1MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });


    // Delete  'd:org' ClientID (everything) and Verify
    describe('Delete All ClientID d:org', function() {

        it('should return 200 on DELETE ClientSet to Delete All Topics for ^d:org', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197"};
            FVT.makeDeleteRequest( FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^d:org", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
    });

    // Verify Expected MQTT Durable Subscriber and Publisher are deleted and Subscriptions gone
    describe('Verify Sub Statistics:', function() {
	
        it('should return 200 on GET MQTTClients after Delete All Topics for d:org', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All Topics for d:org', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

		
    // Config persists Check
    describe('Restart Server, check d:org Statistics:', function() {
	
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
	
        it('should return 200 on GET MQTTClients after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All Topics for a-org-uid1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });


});
