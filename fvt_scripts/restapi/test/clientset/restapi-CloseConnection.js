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


// Results after Durable Subs and Pubs started
var expectA1AllSubscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/1","TopicString":"/CD/001/1","ClientID":"a-org-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/2","TopicString":"/CD/001/2","ClientID":"a-org-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-org-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-org-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-org-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/1","TopicString":"/CD/001/1","ClientID":"a-org-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/2","TopicString":"/CD/001/2","ClientID":"a-org-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-org-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-org-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-org-uid12","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/9","TopicString":"/CD/001/9","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/10","TopicString":"/CD/001/10","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/11","TopicString":"/CD/001/11","ClientID":"a-gro-uid11","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

// This is default when all subscriptions are deleted.
var expectA1AllMQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-org-uid11","IsConnected":false}, \
 {"ClientID":"a-org-uid12","IsConnected":false}, \
 {"ClientID":"a-gro-uid11","IsConnected":false}, \
 {"ClientID":"d:org:pub:cid1","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

var expectA1AllConnection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "a-gro-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-org-uid12",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-org-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;


var expectA2AllSubscription = '{ "Subscription": [ \
 {"SubName":"/CD/001/3","TopicString":"/CD/001/3","ClientID":"a-org-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-org-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-org-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/6","TopicString":"/CD/001/6","ClientID":"a-org-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/7","TopicString":"/CD/001/7","ClientID":"a-org-uid21","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/4","TopicString":"/CD/001/4","ClientID":"a-org-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/5","TopicString":"/CD/001/5","ClientID":"a-org-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/6","TopicString":"/CD/001/6","ClientID":"a-org-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/7","TopicString":"/CD/001/7","ClientID":"a-org-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"}, \
 {"SubName":"/CD/001/8","TopicString":"/CD/001/8","ClientID":"a-org-uid22","IsDurable":true,"MaxMessages":5000,"MessagingPolicy":"DemoTopicPolicy"} \
], "Version": "'+ FVT.version +'" }' ;

// This is default when all Connections are closed.
var expectA2AllMQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-org-uid21","IsConnected":false}, \
 {"ClientID":"a-org-uid22","IsConnected":false}, \
 {"ClientID":"d:org:pub:cid2","IsConnected":false} \
], "Version": "'+ FVT.version +'" }' ;

var expectA2AllConnection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid2", "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-org-uid22",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "a-org-uid21",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;


// Results after CLOSE CONNECTION  #2 on A1:  ClientID:a-org-uid*

var expectA1CloseConn2MQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-org-uid11","IsConnected":false}, \
 {"ClientID":"a-org-uid12","IsConnected":false}  \
], "Version": "'+ FVT.version +'" }' ;


var expectA1CloseConn2Connection = '{"Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "a-gro-uid11",    "Port": 16102, "Protocol": "mqtt", "UserId": "" }, \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;



// Results after CLOSE CONNECTION  #3 on A1:  ClientID:a-org-uid11, UserID=""

var expectA1CloseConn3MQTTClient = '{ "MQTTClient": [ \
 {"ClientID":"a-gro-uid11","IsConnected":false}, \
 {"ClientID":"a-org-uid11","IsConnected":false}, \
 {"ClientID":"a-org-uid12","IsConnected":false}  \
], "Version": "'+ FVT.version +'" }' ;

var expectA1CloseConn3Connection = '{ "Connection": [ \
 { "Endpoint": "DemoEndpoint", "Name": "d:org:pub:cid1", "Port": 16102, "Protocol": "mqtt", "UserId": "" }  \
], "Version": "v1" }' ;

// Results after CLOSE CONNECTION #4 on A1:  Userid:""
// Subscriptions are A1All, MQTT and Connections are Default

// Results after Delete ClientSet #5 defaults for Subscription


var uriObject = 'close/connection' ;

var verifyConfig = {};
var verifyMessage = {};
var clientAddress = "";

function getClientConnAddress ( theURL, domain, done ) {

    try {
        request( theURL )
          .get(  encodeURI(domain)  )
          .end(function(err, res) {
            if (err) {
                FVT.trace( 0, '=== ERROR in getClientConnAddress ===:'+ String(err) );
                throw err;
            } else {
                jsonResponse = JSON.parse( res.text );
                FVT.trace( 9, 'jsonResponse='+ JSON.stringify( jsonResponse ) );
                jsonArray = jsonResponse[ 'Connection' ];
                FVT.trace( 9, 'jsonArray='+ JSON.stringify( jsonArray ) );
                clientAddress = jsonArray[0]['ClientAddr'];
                FVT.trace( 1, 'clientAddress ='+ clientAddress  );

                done ();
            }
        });
    } catch ( ex ) {
        FVT.trace( 0, "Unexpected Expection in getClientConnAddress : "+ ex );
        FVT.trace( 0, JSON.stringify(ex, null, 2));
    }

};

//  ====================  Test Cases Start Here  ====================  //
//  Delete Retained messages in Durable Topic /CD/001/3 for ClientID with name starting a-org, then Delete entire a-org-cid1 ClientSet.

describe('CloseConn:', function() {
this.timeout( FVT.defaultTimeout );

    describe('Verify Initial Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1', function(done) {
            verifyConfig = JSON.parse( expectA1AllConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A2', function(done) {
            verifyConfig = JSON.parse(expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });
// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A2', function(done) {
//            verifyConfig = JSON.parse( expectA2AllConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });


        it('should return 200 on GET A2.Clients ClientAddress', function(done) {
            verifyConfig = JSON.parse( expectA2AllConnection );
            getClientConnAddress( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", done);
        });

    });


    // Close Connections #1 on an A2: All connected Sub and Pub clients from ClientAddress
    describe('A2.CloseConnection ClientAddress', function() {

        it('should return 400 on verify A2.CloseConnection on ClientAddress:"" (Invalid)', function(done) {
            var payload = '{"ClientAddress":""}' ;
            verifyConfig = '' ;
//            verifyMessage = { "status":400, "Code":"CWLNA8618", "Message":"Cannot run the \"close Connection\" command. Specify one or more\nof the following items: UserID, ClientID, ClientAddress."};
            verifyMessage = { "status":400, "Code":"CWLNA6204", "Message":"Cannot process \"close/connection\" REST call. Specify one or more of the following items: UserID, ClientID, ClientAddress."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on verify A2.CloseConnection on ClientAddr:"ip" (Invalid Key)', function(done) {
            var payload = '{"ClientAddr":"'+ clientAddress +'"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: ClientAddr."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });


        it('should return 200 on A2.CloseConnection on ClientAddress:'+ clientAddress, function(done) {
            var payload = '{"ClientAddress":"'+ clientAddress +'"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on verify A2.CloseConnection on ClientAddress:'+ clientAddress, function(done) {
            var payload = '{"ClientAddress":"'+ clientAddress +'"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on A2.CloseConnection on ClientAddress:MaxLengthListOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            var payload = '{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST +'"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on A2.CloseConnection on ClientAddress: EXCEEDMaxLengthListOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            var payload = '{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ', ' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });


    describe('Verify CloseConn #1 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-CloseConn#1', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn#1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A1-CloseConn#1', function(done) {
//            verifyConfig = JSON.parse( expectA1AllConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A2-CloseConn#1', function(done) {
            verifyConfig = JSON.parse(expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn#1', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A2-CloseConn#1', function(done) {
//            verifyConfig = JSON.parse( expectDefaultConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    });


    // Close Connection #2a on A2 - should already be gone
    describe('A2.CloseConn #2a where ClientID:a-org-uid22', function() {

        it('should return 400 when ClientID already closed', function(done) {
            var payload = '{"ClientID":"a-org-uid22"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });

    // Close Connection #2b on A1 where ClientID=^a-org-uid  NO REGEX
    describe('A1.CloseConn #2b where ClientID=^a-org-uid - forbidden REGEX', function() {

        it('should return 400 on CloseConn #2b NO REGEX', function(done) {
            var payload = '{"ClientID":"^a-org-uid"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });


    // Close Connection #2 on A1 where ClientID=a-org-uid*  WILDCARDS are Valid - defect 129339  (close: a-org-uid11 and uid12)
    describe('A1.CloseConn #2 where ClientID:a-org-uid* - Wildcard', function() {

        it('should return 200 on CloseConn #2 A1:a-org-uid WILDCARD', function(done) {
            var payload = '{"ClientID":"a-org-uid*"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });


    // Close Connection #2 on A1 where ClientID=a-org-uid*  WILDCARDS are Valid - defect 129339  (close: a-org-uid11 and uid12)
    describe('A1.CloseConn #2 where ClientID:a-org-uid* - Wildcard', function() {

        it('should return 200 on CloseConn #2 A1:a-org-uid WILDCARD', function(done) {
            var payload = '{"ClientID":"a-org-uid*"}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify CloseConn #2 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Server Statistics (no change expected)
        it('should return 200 on GET Subscription @ A1-CloseConn #2', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn #2', function(done) {
            verifyConfig = JSON.parse( expectA1CloseConn2MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A1-CloseConn #2', function(done) {
//            verifyConfig = JSON.parse( expectA1CloseConn2Connection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    // Verify Expected A2 Subscriptions, MQTTClient and Server Statistics
        it('should return 200 on GET Subscription @ A2-CloseConn #2', function(done) {
            verifyConfig = JSON.parse(expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn #2', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A2-CloseConn #2', function(done) {
//            verifyConfig = JSON.parse( expectDefaultConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    });

		
    // Config persists Check
    describe('Restart Server A2, recheck Statistics:', function() {
	
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		this.timeout( FVT.REBOOT + 5000 ) ; 
    		FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectClusterConfigedStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequestWithURL(  'http://'+ FVT.A2_IMA_AdminEndpoint ,FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
	
    // Verify Expected A1 Subscriptions, MQTTClient and Server Statistics (no change expected)
        it('should return 200 on GET Subscription @ A1-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse( expectA1CloseConn2MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse( expectA1CloseConn2Connection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Server Statistics
        it('should return 200 on GET Subscription @ A2-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse(expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A2-CloseConn #2 after restart A2', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

	
    // Close Connection #3 on A1 where ClientID=a-gro-uid11, userID=""
    describe('A1.CloseConn #3 where ClientID=a-gro-uid11, userID=""', function() {

        it('should return 200 on CloseConn #3 - ClientID:a-gro-uid11, userID:""', function(done) {
            var payload = '{"ClientID":"a-gro-uid11", "UserID":""}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on verify CloseConn #3 - ClientID:a-gro-uid11, userID:""', function(done) {
            var payload = '{"ClientID":"a-gro-uid11", "UserID":""}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });


    describe('Verify CloseConn #3 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Connection Statistics
        it('should return 200 on GET Subscription @ A1-CloseConn3', function(done) {
            verifyConfig = JSON.parse(expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn3', function(done) {
            verifyConfig = JSON.parse( expectA1CloseConn3MQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A1-CloseConn3', function(done) {
            verifyConfig = JSON.parse( expectA1CloseConn3Connection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Connection Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-CloseConn3', function(done) {
            verifyConfig = JSON.parse( expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn3', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A2-CloseConn3', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });



    // CloseConn #4 on A1 where Userid=""  -- should be d:org:cid1
    describe('A1.CloseConn #4 where Userid=""', function() {
// defect 131766
        it('should return 200 on CloseConn #4, UserID:""', function(done) {
            var payload = '{"UserID":""}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 400 on verify CloseConn #4, UserID:""', function(done) {
            var payload = '{"UserID":""}' ;
            verifyConfig = '' ;
            verifyMessage = { "status":400, "Code":"CWLNA6136", "Message":"The specified connection was not closed. The specified connection cannot be found."};
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "close/connection", payload,  FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify CloseConn#4 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Server Statistics
        it('should return 200 on GET Subscription @ A1-CloseConn #4', function(done) {
            verifyConfig = JSON.parse( expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn #4', function(done) {
            verifyConfig = JSON.parse( expectA1AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

// 133756 Connection Stats are not updated in RealTime
//        it('should return 200 on GET Connection @ A1-CloseConn #4', function(done) {
//            verifyConfig = JSON.parse( expectDefaultConnection );
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
//        });

    // Verify Expected A2 Subscriptions, MQTTClient and Server Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-CloseConn #4', function(done) {
            verifyConfig = JSON.parse( expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn #4', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-CloseConn #4', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });
});


		
    // Config persists Check
    describe('Restart Server A1, recheck Statistics:', function() {
	
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		this.timeout( FVT.REBOOT + 5000 ) ; 
    		FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectClusterConfigedStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequestWithURL(  'http://'+ FVT.IMA_AdminEndpoint ,FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
	

    // Verify Expected A1 Subscriptions, MQTTClient and Server Statistics
        it('should return 200 on GET Subscription @ A1-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectA1AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectA1AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection @ A1-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Server Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectA2AllSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectA2AllMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-CloseConn #4 after restart A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });
});

    // Delete #5 on A1 and A2 where ClientSet?ClientID=^&Retain=^
    describe('A1.Delete 5 where ClientSet?ClientID=^&Retain=^', function() {
// defect 129380
        it('should return 200 on DELETE ClientSet to A1.Delete#5', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 4, Clients deleted: 4, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to A1.Delete#5 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
            FVT.sleep( FVT.DELETE_CLIENTSET  );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 4, Clients deleted: 4, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });
	
    describe('A2.Delete 5 where ClientSet?ClientID=^&Retain=^', function() {

        it('should return 200 on DELETE ClientSet to A2.Delete#5', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 3, Clients deleted: 3, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to A2.Delete#5 confirm', function(done) {
		    this.timeout( FVT.DELETE_CLIENTSET + 3000 );
            FVT.sleep( FVT.DELETE_CLIENTSET  );
            verifyConfig = '' ;
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 2, Clients deleted: 2, Deletion errors: 0"};
//            verifyMessage = { "Status":200, "Code":"CWLNA8751", "Message":"The request is complete. Clients found: 3, Clients deleted: 3, Deletion errors: 0"};
            verifyMessage = { "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 0, Clients deleted: 0, Deletion errors: 0"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet?ClientID=^&Retain=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

    });

    describe('Verify Delete#5 Statistics:', function() {

    // Verify Expected A1 Subscriptions, MQTTClient and Server Statistics
        it('should return 200 on GET Subscription @ A1-Delete5 is default', function(done) {
            verifyConfig = JSON.parse(expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A1-Delete5 is default', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A1-Delete5 at end', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    // Verify Expected A2 Subscriptions, MQTTClient and Server Statistics (unchanged)
        it('should return 200 on GET Subscription @ A2-Delete5 at end', function(done) {
            verifyConfig = JSON.parse(expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain+"Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient @ A2-Delete5 at end', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Connection@ A2-Delete5 at end', function(done) {
            verifyConfig = JSON.parse( expectDefaultConnection );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Connection", FVT.getSuccessCallback, verifyConfig, done);
        });

    });


});
