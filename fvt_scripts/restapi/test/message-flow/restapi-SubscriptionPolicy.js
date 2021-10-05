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
var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLxxx","Message":"blah blah"}

var uriObject = 'SubscriptionPolicy/';

// UPDATE 10022015:  Subscriptions should NOT HAVE MaxMessageTimeToLive, only a Publisher/Producer constraint
var expectDefault = '{"Version": "' + FVT.version + '","SubscriptionPolicy": { \
"DemoSubscriptionPolicy": {"ActionList": "Receive,Control","Description": "Demo policy for shared durable subscription", "MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT","Subscription": "*"} }}' ;

var testSubscribePol = JSON.parse( '{"SubscriptionPolicy": {"TestSubscribePol": {"GroupID": "theSubscriptionGroup","ActionList": "Receive,Control","Subscription": "subscription/*","CommonNames": "","UserID": "","ClientID": "","ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10","Description": "The NEW TEST Messaging Policy.","MaxMessagesBehavior": "RejectNewMessages","Protocol": "","MaxMessages": 5000 }}}' );

var expectAllSubscriptionPolicy = '{"Version": "' + FVT.version + '","SubscriptionPolicy": { \
"DemoSubscriptionPolicy": { "Description": "Demo policy for shared durable subscription", "Subscription": "*", "Protocol": "JMS,MQTT", "ActionList": "Receive,Control", "MaxMessages": 5000, "MaxMessagesBehavior": "RejectNewMessages" }, \
"TestSubscribePol": { "GroupID": "theSubscriptionGroup", "Subscription": "/topic/*", "ActionList": "Receive,Control", "Description": "The NEW TEST Messaging Policy.", "ClientID": "", "ClientAddress": "", "MaxMessages": 5000, "UserID": "", "CommonNames": "", "Protocol": "", "MaxMessagesBehavior": "RejectNewMessages" }, \
"' + FVT.long256Name + '": { "Subscription": "*", "ActionList": "Receive,Control", "MaxMessages": 5000, "MaxMessagesBehavior": "RejectNewMessages", "Protocol": "JMS,MQTT", "Description": "", "ClientID": "", "GroupID": "", "ClientAddress": "", "UserID": "", "CommonNames": "" }}}';

//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Destination,DestinationType,ActionList,MaxMessages,MaxMessagesBehavior,MaxMessageTimeToLive,DisconnectedClientNotification,PendingAction,UseCount",
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Subscription,               ActionList,MaxMessages,MaxMessagesBehavior,MaxMessageTimeToLive,DisconnectedClientNotification",

describe('SubscriptionPolicy:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
//99809
        it('should return status 200 on get, DEFAULT of "SubscriptionPolicy":"MP1":{..},"MP2":{...}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Description(1024):', function() {

        it('should return status 200 on post TestSubscribePol ', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"GroupID":"theSubscriptionGroup","ActionList": "Receive,Control","Subscription":"/topic/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, TestSubscribePol (type:Queue)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"Description": "The TEST Messaging Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"Description": "The NEW TEST Messaging Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Valid Range test cases  ====================  //
    describe('Name(2560, IDs(1024) Max Length:', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ActionList": "Receive,Control","Subscription": "*", "MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
			verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		this.timeout( FVT.REBOOT + 5000 ) ; 
    		FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectDefaultStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "SubscriptionPolicy" persists', function(done) {
            verifyConfig = JSON.parse( expectAllSubscriptionPolicy ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
//101151
        it('should return status 200 on post "Max Length ClientID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length UserID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"UserID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length UserID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length GroupID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"GroupID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length GroupID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length CommonNames"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"CommonNames": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length CommonNames"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Protocol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Protocol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":ipv6,ipv4', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":ipv6,ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Reset to Default test cases  ====================  //
    describe('ResetToDefault:', function() {
// 103152
        it('should return status 200 on post "Reset ClientID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientID"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset ClientAddress"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientAddress"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset UserID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"UserID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset UserID"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"UserID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset GroupID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"GroupID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset GroupID"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"GroupID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset CommonNames"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"CommonNames": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"CommonNames": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Protocol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": null,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "","ClientID":"*"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Description"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Description"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"'+ FVT.long256Name +'":{"Description":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   AtLeastOneOf test cases  ====================  //
    describe('AtLeastOneOf:', function() {

        it('should return status 200 on post "Only ClientAddress":ipv4', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv4,ipv6', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4,ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only UID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"UserID": "'+ FVT.long1024String +'","ClientAddress":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only UID"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"'+ FVT.long1024String +'","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only GID"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"GroupID": "'+ FVT.long1024String +'","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only GID"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"'+ FVT.long1024String +'","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only CommonNames"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"CommonNames": "'+ FVT.long1024String +'","GroupID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"'+ FVT.long1024String +'","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only Protocol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT,JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   Protocol test cases  ====================  //
    describe('Protocol["", MQTT, JMS]:', function() {

        it('should return status 200 on post "Protocol":"JMS" only', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"JMS"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"" ', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "","CommonNames":"Buddy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":""', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"Buddy","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"MQTT" only', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"MQTT"', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 107785 DOC error
        it('should return status 400 on post "Protocol":"ALL"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "All"}}}';
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT"}}}' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"All\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, POST "Protocol":"ALL" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"jms,mqtt" (lc)', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"Protocol": "jms,mqtt"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"mqtt" (lc', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"jms,mqtt"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   ClientAddress test cases  ====================  //
    describe('ClientAddress:', function() {

        it('should return status 200 on post "ClientAddress":MaxLengthList', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":MaxLengthList', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSubscribePol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":IP-Range', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"ClientAddress":"'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10'+'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":IP-Range', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSubscribePol', FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   Subscription Destination test cases  ====================  //
    describe('Subscription:', function() {
//   /iot-2/type/+/id/dog/event/+/fmt/json 
        it('should return status 200 on post "Subscription":wildcard', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"Subscription":"subscription/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Subscription":wildcard', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSubscribePol', FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   MaxLength test cases  ====================  //
    describe('MAX Names[256] of each Msging TYPE:', function() {
// 100425
        it('should return status 200 on post "Long Destination and Type (Subscription)"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+  FVT.long256Name + '":{"ClientAddress":"*","ActionList":"Receive,Control","Subscription":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Subscription)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy', FVT.getSuccessCallback, verifyConfig, done);
        });
// 100432
        it('should return status 200 on post "Long Destination and Type (Topic)"', function(done) {
            var payload = '{"TopicPolicy":{"'+  FVT.long256Name + '":{"ClientAddress":"*","ActionList": "Publish,Subscribe","Topic":"'+ FVT.long1024TopicName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Topic)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+ 'TopicPolicy', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Long Destination and Type (Queue)"', function(done) {
            var payload = '{"QueuePolicy":{"'+ FVT.long256Name + '":{"ClientAddress":"*","ActionList": "Send,Receive,Browse","Queue":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Queue)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+ 'QueuePolicy', FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Range test cases  ====================  //
    describe('MaxMessages[1-5000-20000000]:', function() {

        it('should return status 200 on post "MaxMessage: MIN 1"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessages":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: MIN 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100433, 103153
        it('should return status 200 on post "MaxMessage: Default 5000"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessages":null}}}';
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: Default 5000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessage: MAX: 20000000"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: MAX: 20000000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   MaxMsgBehave test cases  ====================  //
    describe('MaxMsgBehave[RejectNewMessages,DiscardOldMessages]:', function() {

        it('should return status 200 on post "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100434
        it('should return status 200 on post "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":null}}}';
            verifyConfig =  JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: RejectNewMessages"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   MaxMsgTTL test cases  ====================  //
// UPDATE 10022015:  Subscriptions should NOT HAVE MaxMessageTimeToLive, only a Publisher/Producer constraint:  Schema Error

    describe('MaxMsgTTL[1-unlimited-2147483647]:', function() {
// 120951
        it('should return status 400 on post "MaxMessageTimeToLive MIN: 1"', function(done) {
            var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"1"}}}';
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}' );
//			verifyMessage = JSON.parse( '{"status":404, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: ' + FVT.long256Name + ' Property: MaxMessageTimeToLive" }' ) ;
			verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: ' + FVT.long256Name + ' Property: MaxMessageTimeToLive" }' ) ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, WITHOUT! "MaxMessageTimeToLive MIN: 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        // it('should return status 200 on post "MaxMessageTimeToLive Default"', function(done) {
            // var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":null}}}';
            // verifyConfig =  JSON.parse( '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}' );
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "MaxMessageTimeToLive Default"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        // });

        // it('should return status 200 on post "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            // var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"2147483647"}}}';
            // verifyConfig = JSON.parse(payload);
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        // });

        // it('should return status 200 on post "MaxMessageTimeToLive unlimited"', function(done) {
            // var payload = '{"SubscriptionPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}';
            // verifyConfig = JSON.parse( payload );
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "MaxMessageTimeToLive unlimited"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        // });

	});
//  ====================   DisconnClt test cases  ====================  //
// ONLY valid for TOPIC and MQTT.

    describe('DisconnectClient[F]:', function() {
// 120951
        it('should return status 400 on post "DisconnectedClientNotification true"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"DisconnectedClientNotification":true}}}';
            verifyConfig = testSubscribePol ;
//			verifyMessage = {"status":404, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: TestSubscribePol Property: DisconnectedClientNotification" } ;
			verifyMessage = {"status":400, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: TestSubscribePol Property: DisconnectedClientNotification" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, POST fails "DisconnectedClientNotification true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "DisconnectedClientNotification false"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":{"DisconnectedClientNotification":false}}}';
            verifyConfig = testSubscribePol ;
//			verifyMessage = {"status":404, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: TestSubscribePol Property: DisconnectedClientNotification" } ;
			verifyMessage = {"status":400, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: SubscriptionPolicy Name: TestSubscribePol Property: DisconnectedClientNotification" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, POST fails "DisconnectedClientNotification false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+ FVT.long256Name +'":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"'+ FVT.long256Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{ "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: '+ FVT.long256Name +'","SubscriptionPolicy":{"'+ FVT.long256Name +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

// 96657
        it('should return status 400 on POST DELETE of "TestSubscribePol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":null}}' ;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestSubscribePol\":null is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: \"TestSubscribePol\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestSubscribePol" FAILS ', function(done) {
		    verifyConfig = {"SubscriptionPolicy":{"TestSubscribePol":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//        it('should return status 200 on GET after POST DELETE, "TestSubscribePol" gone ', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
//        });
//        it('should return status 404 on GET after POST DELETE, "TestSubscribePol" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"TestSubscribePol":{}}}' );
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSubscribePol', FVT.verify404NotFound, verifyConfig, done);
//        });
//		
//        it('should return status 404 on RE-DELETE of "TestSubscribePol"', function(done) {
//            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":null}}' ;
//            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code": "CWLNA6011","Message": "The requested configuration object was Not Found."};
//            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestSubscribePol',  FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
//        });
        it('should return status 200 on DELETE of "TestSubscribePol"', function(done) {
            var payload = '{"SubscriptionPolicy":{"TestSubscribePol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestSubscribePol',  FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestSubscribePol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after POST DELETE, "TestSubscribePol" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"TestSubscribePol":{}}}' );
		    verifyConfig = { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: TestSubscribePol","SubscriptionPolicy":{"TestSubscribePol":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSubscribePol', FVT.verify404NotFound, verifyConfig, done);
        });
    });

//  ====================  Error test cases  ====================  //
    describe('General Error:', function() {

        it('should return status 400 when trying to Delete All Messaging Policies, this is just bad form', function(done) {
            var payload = '{"SubscriptionPolicy":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"SubscriptionPolicy\":null} is not valid." };
			verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"SubscriptionPolicy\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Messaging Policies, bad form', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 400 when trying to set Invalid Protocol SCADA', function(done) {
            var objName = 'BadProtocolScada'
            var payload = '{"SubscriptionPolicy":{"'+ objName +'":{"Protocol":"MQTT,JMS,SCADA","ActionList": "Receive,Control","Subscription": "*", "DisconnectedClientNotification": false,"MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
		    verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT,JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocol was not created', function(done) {
//		    verifyConfig = {"status":404,"SubscriptionPolicy":{"BadProtocolScada":{}}} ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: BadProtocolScada","SubscriptionPolicy":{"BadProtocolScada":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadProtocolScada", FVT.verify404NotFound, verifyConfig, done);
        });
// 100824 120951
        it('should return status 400 on post "CommonName(s) is misspelled"', function(done) {
            var objName = 'BadParamCommonName'
            var payload = '{"SubscriptionPolicy":{"'+ objName +'":{"CommonName": "Missing_S_inNames","ActionList": "Receive,Control","ClientID": "*","Subscription": "*", "DisconnectedClientNotification": false, "MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: CommonName."};
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: BadParamCommonName Property: CommonName"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: BadParamCommonName Property: CommonName"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "CommonName(s) is misspelled did not create"', function(done) {
//		    verifyConfig = {"status":404,"SubscriptionPolicy":{"BadParamCommonName":{}}} ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: BadParamCommonName","SubscriptionPolicy":{"BadParamCommonName":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamCommonName", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "NoDestinationType" no longer needed', function(done) {
            var payload = '{"SubscriptionPolicy":{"NoDestinationType":{"DestinationType":"Subscription", "ActionList":"subscription actions here", "ClientID":"*", "Subscription": "*","DisconnectedClientNotification": false, "MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: NoDestinationType Property: DestinationType"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: NoDestinationType Property: DestinationType"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDestinationType" no longer needed"', function(done) {
//		    verifyConfig = {"status":404,"SubscriptionPolicy":{"NoDestinationType":{}}} ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: NoDestinationType","SubscriptionPolicy":{"NoDestinationType":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDestinationType", FVT.verify404NotFound, verifyConfig, done);
        });
//120 951
        it('should return status 400 on POST NoDisconnectedClientNotification it is not in "Subscription"', function(done) {
            var payload = '{"SubscriptionPolicy":{"NoDisconnectedClientNotification":{"CommonNames": "NoDestNeeded","ActionList": "Receive,Control","ClientID": "*","Subscription": "*","DisconnectedClientNotification": false, "MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: NoDisconnectedClientNotification Property: DisconnectedClientNotification"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: NoDisconnectedClientNotification Property: DisconnectedClientNotification"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDisconnectedClientNotification" no longer needed"', function(done) {
//		    verifyConfig = {"status":404,"SubscriptionPolicy":{"NoDisconnectedClientNotification":{}}} ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: NoDisconnectedClientNotification","SubscriptionPolicy":{"NoDisconnectedClientNotification":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDisconnectedClientNotification", FVT.verify404NotFound, verifyConfig, done);
        });
// 100425
        it('should return status 400 on post "MissingMinOne" parameter', function(done) {
            var payload = '{"SubscriptionPolicy":{"MissingMinOne":{"ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
//??            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties CommonNames,UserID,ClientAddress,GroupID,ClientID,Protocol specified."};
//??order   verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties UserID,ClientID,CommonNames,ClientAddress,GroupID,Protocol specified"};
//??random  verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties GroupID,ClientID,ClientAddress,Protocol,UserID,CommonNames specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties CommonNames,UserID,ClientID,ClientAddress,GroupID,Protocol specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties ClientAddress,UserID,ClientID,GroupID,CommonNames,Protocol specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties Protocol,ClientID,ClientAddress,CommonNames,UserID,GroupID specified"};
			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: SubscriptionPolicy must have one of the properties ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol specified"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "MissingMinOne" parameter did not create"', function(done) {
//		    verifyConfig = {"status":404,"SubscriptionPolicy":{"MissingMinOne":{}}} ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MissingMinOne","SubscriptionPolicy":{"MissingMinOne":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MissingMinOne", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Name Error test cases  ====================  //
    describe('Error Name:', function() {

        it('should return status 400 on POST  "Name too Long"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+ FVT.long257Name +'":{"CommonNames": "Jim","ActionList": "Receive,Control","ClientID": "*","Subscription": "*", "DisconnectedClientNotification": false,"MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
//			verifyMessage = {"status":400,"Code":"CWLNA0445","Message":"The length of the name must be less than 256."} ;
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: SubscriptionPolicy Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Name too Long" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: '+ FVT.long257Name +'","SubscriptionPolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Description Error test cases  ====================  //
    describe('Error Desc:', function() {

        it('should return status 400 on POST  "DescTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"DescTooLong":{"Description":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: Description Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DescTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"DescTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: DescTooLong","SubscriptionPolicy":{"DescTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DescTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   ClientID Error test cases  ====================  //
    describe('Error ClientID:', function() {

        it('should return status 400 on POST  "ClientIDTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ClientIDTooLong":{"ClientID":"'+ FVT.long1025String +'","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: ClientID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientIDTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ClientIDTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ClientIDTooLong","SubscriptionPolicy":{"ClientIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    });
	
//  ====================   ClientAddr Error test cases  ====================  //
    describe('Error ClientAddress:', function() {

        it('should return status 400 on POST  "ClientAddressTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ClientAddressTooLong":{"ClientAddress":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: ClientAddress Value: '+ FVT.long1025String +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ClientAddressTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ClientAddressTooLong","SubscriptionPolicy":{"ClientAddressTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ClientAddressListTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ClientAddressListTooLong":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1)  +'","ClientID": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//                verifyMessage = {"status":400, "Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." } ;
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: ClientAddress Value: '+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressListTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ClientAddressListTooLong","SubscriptionPolicy":{"ClientAddressListTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressListTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   UserID Error test cases  ====================  //
    describe('Error UserID:', function() {

        it('should return status 400 on POST  "UserIDTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"UserIDTooLong":{"UserID":"'+ FVT.long1025String +'","ClientAddress": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: UserID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "UserIDTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ClientAddressTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: UserIDTooLong","SubscriptionPolicy":{"UserIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"UserIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   GroupID Error test cases  ====================  //
    describe('Error GroupID:', function() {

        it('should return status 400 on POST  "GroupIDTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"GroupIDTooLong":{"GroupID":"'+ FVT.long1025String +'","UserID": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: GroupID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "GroupIDTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"GroupIDTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: GroupIDTooLong","SubscriptionPolicy":{"GroupIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"GroupIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   CommonNames Error test cases  ====================  //
    describe('Error CommonNames:', function() {

        it('should return status 400 on POST  "CommonNamesTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"CommonNamesTooLong":{"CommonNames":"'+ FVT.long1025String +'","GroupID": "*","ActionList": "Receive,Control","Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: CommonNames Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: CommonNames Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CommonNamesTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"CommonNamesTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: CommonNamesTooLong","SubscriptionPolicy":{"CommonNamesTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"CommonNamesTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   Destination Error test cases  ====================  //
    describe('Error Destination:', function() {

        it('should return status 400 on POST  "DestinationTooLong"', function(done) {
            var payload = '{"SubscriptionPolicy":{"DestinationTooLong":{"Subscription":"'+ FVT.long1025TopicName +'","CommonNames": "*","ActionList": "Receive,Control" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: '+ FVT.long1025TopicName +'." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Subscription Value: \\\"'+ FVT.long1025TopicName +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SubscriptionPolicy Property: Subscription Value: '+ FVT.long1025TopicName +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DestinationTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"DestinationTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: DestinationTooLong","SubscriptionPolicy":{"DestinationTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DestinationTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
// 120951
        it('should return status 400 on POST  with Destination Target is a Queue not Subscription and ActionList valid"', function(done) {
            var payload = '{"SubscriptionPolicy":{"Q_Destination":{"ClientID":"*", "Queue": "aQueue", "ActionList": "Receive,Control"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: Q_Destination Property: Queue" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET Destination Target is a QUEUE not Subscription', function(done) {
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: Q_Destination","SubscriptionPolicy":{"Q_Destination":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'Q_Destination', FVT.verify404NotFound, verifyConfig, done);
        });
// 120951
        it('should return status 400 on POST  change Destination is a Topic not Subscription and ActionList valid', function(done) {
            var payload = '{"SubscriptionPolicy":{"T_Destination":{"ClientID":"*",  "Topic": "/topic/a/*", "ActionList": "Receive,Control"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: T_Destination Property: Topic" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET Destination is a Topic not Subscription', function(done) {
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: T_Destination","SubscriptionPolicy":{"T_Destination":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'T_Destination', FVT.verify404NotFound, verifyConfig, done);
        });

    });
//  ====================   ActionList Error test cases  ====================  //
    describe('Error ActionList:', function() {

        it('should return status 400 on POST  "ActionListRead"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListRead":{"ActionList":"Read", "Subscription": "*" }}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Read\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListRead" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListRead":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListRead","SubscriptionPolicy":{"ActionListRead":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListRead", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoSend"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListNoSend":{"ActionList":"Send","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoSend" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListNoSend":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListNoSend","SubscriptionPolicy":{"ActionListNoSend":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoSend", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoBrowse"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListNoBrowse":{"ActionList":"Browse","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoBrowse" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListNoBrowse":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListNoBrowse","SubscriptionPolicy":{"ActionListNoBrowse":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoPublish"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListNoPublish":{"ActionList":"Publish","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoPublish" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListNoPublish":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListNoPublish","SubscriptionPolicy":{"ActionListNoPublish":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoSubscribe"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListNoSubscribe":{"ActionList":"Subscribe","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: Publish,Subscribe,Control." }' );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoSubscribe" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListNoSubscribe":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListNoSubscribe","SubscriptionPolicy":{"ActionListNoSubscribe":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoPublish"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListSubscriptionNoPublish":{"ActionList":"Receive,Control,Publish","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoPublish" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListSubscriptionNoPublish":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListSubscriptionNoPublish","SubscriptionPolicy":{"ActionListSubscriptionNoPublish":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoSubscribe"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListSubscriptionNoSubscribe":{"ActionList":"Receive,Control,Subscribe","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoSubscribe" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListSubscriptionNoSubscribe":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListSubscriptionNoSubscribe","SubscriptionPolicy":{"ActionListSubscriptionNoSubscribe":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoSend"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListSubscriptionNoSend":{"ActionList":"Receive,Control,Send","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoSend" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListSubscriptionNoSend":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListSubscriptionNoSend","SubscriptionPolicy":{"ActionListSubscriptionNoSend":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoSend", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoBrowse"', function(done) {
            var payload = '{"SubscriptionPolicy":{"ActionListSubscriptionNoBrowse":{"ActionList":"Receive,Control,Browse","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoBrowse" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"ActionListSubscriptionNoBrowse":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: ActionListSubscriptionNoBrowse","SubscriptionPolicy":{"ActionListSubscriptionNoBrowse":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });

//  ====================   MaxMessages Error test cases  ====================  //
    describe('Error MaxMessages:', function() {

        it('should return status 400 on POST  "MaxMessagesTooLow"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessagesTooLow":{"MaxMessages":0,"ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessagesTooLow":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessagesTooLow","SubscriptionPolicy":{"MaxMessagesTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesTooHigh"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessagesTooHigh":{"MaxMessages":20000001,"ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"20000001\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooHigh" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessagesTooHigh":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessagesTooHigh","SubscriptionPolicy":{"MaxMessagesTooHigh":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MismatchMaxMsgsSubscription"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MismatchMaxMsgsSubscription":{"ActionList": "Receive,Control","DestinationType":"Queue","Subscription":"'+ FVT.long1024String +'","MaxMessages":50000}}}';
            verifyConfig = JSON.parse(payload);
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the SubscriptionPolicy DestinationType is \"Queue\".." }' );
//			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the SubscriptionPolicy DestinationType is \"Queue\".." } 
//			verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MismatchMaxMsgsSubscription Property: DestinationType" } 
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MismatchMaxMsgsSubscription Property: DestinationType" } 
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after fail "MismatchMaxMsgsSubscription)"', function(done) {
	//	    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MismatchMaxMsgsSubscription":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MismatchMaxMsgsSubscription","SubscriptionPolicy":{"MismatchMaxMsgsSubscription":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MismatchMaxMsgsSubscription", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   MaxMessagesBehavior Error test cases  ====================  //
    describe('Error MaxMessagesBehavior:', function() {
// 120951
        it('should return status 400 on POST  "MaxMessageBehavior":"TooLow"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessageBehaviorTooLow":{"MaxMessageBehavior":"TooLow","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MaxMessageBehavior." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageBehaviorTooLow" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessageBehaviorTooLow":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessageBehaviorTooLow","SubscriptionPolicy":{"MaxMessageBehaviorTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesBehaviorTooLow"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessagesBehaviorTooLow":{"MaxMessagesBehavior":"TooLow","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessagesBehavior Value: \"TooLow\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesBehaviorTooLow" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessagesBehaviorTooLow":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessagesBehaviorTooLow","SubscriptionPolicy":{"MaxMessagesBehaviorTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   MaxMessageTimeToLive Error test cases  ====================  //
// UPDATE 10-02-2015:  THIS IS ALWAYS AN ERROR!  Change to 404 
    describe('Error MaxMessageTimeToLive:', function() {
//98282, 120951
        it('should return status 400 on POST  "MaxMessageTimeToLiveTooLow"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessageTimeToLiveTooLow":{"MaxMessageTimeToLive":"0","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: 0." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveTooLow Property: MaxMessageTimeToLive" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveTooLow Property: MaxMessageTimeToLive" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessageTimeToLiveTooLow":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessageTimeToLiveTooLow","SubscriptionPolicy":{"MaxMessageTimeToLiveTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveTooHigh"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessageTimeToLiveTooHigh":{"MaxMessageTimeToLive":"2147483648","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: 2147483648." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveTooHigh Property: MaxMessageTimeToLive" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveTooHigh Property: MaxMessageTimeToLive" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooHigh" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessageTimeToLiveTooHigh","SubscriptionPolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveLimited"', function(done) {
            var payload = '{"SubscriptionPolicy":{"MaxMessageTimeToLiveLimited":{"MaxMessageTimeToLive":"Limited","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: Limited." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveLimited Property: MaxMessageTimeToLive" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: MaxMessageTimeToLiveLimited Property: MaxMessageTimeToLive" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveLimited" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"MaxMessageTimeToLiveLimited":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: MaxMessageTimeToLiveLimited","SubscriptionPolicy":{"MaxMessageTimeToLiveLimited":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveLimited", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   DisconnectedClientNotification Error test cases  ====================  //
    describe('Error DisconnectedClientNotification:', function() {
// 100007, 120951
        it('should return status 404 on POST  "DisconnectedClientNotificationInvalid"', function(done) {
            var payload = '{"SubscriptionPolicy":{"DisconnectedClientNotificationInvalid":{"DisconnectedClientNotification":"True","ClientID": "*","ActionList": "Receive,Control","Subscription": "*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SubscriptionPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Value: JSON_STRING." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SubscriptionPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Type: JSON_STRING" }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SubscriptionPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DisconnectedClientNotificationInvalid" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"DisconnectedClientNotificationInvalid":{}}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: DisconnectedClientNotificationInvalid","SubscriptionPolicy":{"DisconnectedClientNotificationInvalid":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DisconnectedClientNotificationInvalid", FVT.verify404NotFound, verifyConfig, done);
        });

	});

    describe('Cleanup:', function() {

	    it('should return status 200 on Delete QUEUE Policy', function(done) {
		    verifyConfig = JSON.parse( '{"QueuePolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'QueuePolicy/' + FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Queue Policy gone', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: '+ FVT.long256Name + '","QueuePolicy":{"'+ FVT.long256Name + '":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueuePolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

	    it('should return status 404 on Delete Subscription Policy', function(done) {
		    verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SubscriptionPolicy/' + FVT.long256Name, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Subscription Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: '+ FVT.long256Name + '","SubscriptionPolicy":{"'+ FVT.long256Name + '":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

	    it('should return status 200 on Delete Topic Policy', function(done) {
		    verifyConfig = JSON.parse( '{"TopicPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/' + FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Topic Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"T'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse(' { "status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: '+ FVT.long256Name + '","TopicPolicy":{"'+ FVT.long256Name + '":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
});
