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

var uriObject = 'MessagingPolicy/';

// var expectDefault = '{"Version": "' + FVT.version + '","MessagingPolicy":  \
// {"DemoMPForSharedSub": {"ActionList": "Receive,Control","Description": "Demo messaging policy for shared durable subscription","Destination": "*","DestinationType": "Subscription","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT"}, \
//  "DemoMessagingPolicy": {"ActionList": "Publish,Subscribe","ClientID": "*","Description": "Demo messaging policy","Destination": "*","DestinationType": "Topic","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}' ;

// var expectOneDefault = '{"Version": "' + FVT.version + '","MessagingPolicy":{"DemoMessagingPolicy": {"ActionList": "Publish,Subscribe","ClientID": "*","Description": "Demo messaging policy","Destination": "*","DestinationType": "Topic","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}' ;

var expectDefault = '{"Version": "' + FVT.version + '","MessagingPolicy":{}}' ;

var expectOneDefault = expectDefault ;

//  ====================  Test Cases Start Here  ====================  //

describe('MessagingPolicy:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
//99809
        it('should return status 200 on get, DEFAULT of "MessagingPolicy":"MP1":{..},"MP2":{...}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// REDUNDANT now that there are no default MessagingPolicies
//        it('should return status 200 on get, DEFAULT of "MessagingPolicy":"DemoMsgPol":{..}', function(done) {
//			verifyConfig = JSON.parse(expectOneDefault);
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'DemoMessagingPolicy', FVT.getSuccessCallback, verifyConfig, done);
//        });
	
	});

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Description(1024):', function() {

        it('should return status 200 on post TestMsgPol (type:Queue)', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":{"GroupID":"theQueueGroup","DestinationType":"Queue","ActionList":"Send,Receive,Browse","Destination":"AQUEUE/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, TestMsgPolicy (type:Queue)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":{"Description": "The TEST Messaging Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":{"Description": "The NEW TEST Messaging Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Valid Range test cases  ====================  //
    describe('Various Names Max Length:', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ActionList":"Receive,Control","Destination": "*","DestinationType": "Subscription","MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length ClientID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length UserID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"UserID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length UserID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length GroupID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"GroupID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length GroupID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length CommonNames"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"CommonNames": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length CommonNames"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Protocol"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Protocol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":ipv6,ipv4', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":ipv6,ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"MessagingPolicy":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Reset to Default test cases  ====================  //
    describe('ResetToDefault:', function() {
//97501 (and many more to follow)
        it('should return status 200 on post "Reset ClientID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientID"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset ClientAddress"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientAddress"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset UserID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"UserID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset UserID"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"UserID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset GroupID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"GroupID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset GroupID"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"GroupID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset CommonNames"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"CommonNames": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"CommonNames": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return status 200 on post "Reset Protocol"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"Protocol": null,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"Protocol": "","ClientID":"*"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset Description"', function(done) {
            var payload = '{"MessagingPolicy":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Description"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"'+ FVT.long256Name +'":{"Description":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   AtLeastOneOf test cases  ====================  //
    describe('AtLeastOneOf:', function() {

        it('should return status 200 on post "Only ClientAddress":ipv4', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv4,ipv6', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4,ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only UID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"UserID": "'+ FVT.long1024String +'","ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only UID"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"'+ FVT.long1024String +'","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only GID"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"GroupID": "'+ FVT.long1024String +'","UserID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only GID"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"'+ FVT.long1024String +'","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only CommonNames"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"CommonNames": "'+ FVT.long1024String +'","GroupID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"'+ FVT.long1024String +'","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only Protocol"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS","CommonNames":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT,JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   MaxLength test cases  ====================  //
    describe('MAX Names[256] of each TYPE:', function() {
// 100425
        it('should return status 200 on post "Long Destination and Type (Subscription)"', function(done) {
            var payload = '{"MessagingPolicy":{"S'+ FVT.maxFileName255 + '":{"ClientAddress":"*","ActionList":"Receive,Control","DestinationType":"Subscription","Destination":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Subscription)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100432
        it('should return status 200 on post "Long Destination and Type (Topic)"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255 + '":{"ClientAddress":"*","ActionList": "Publish,Subscribe","DestinationType":"Topic","Destination":"'+ FVT.long1024TopicName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Topic)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Long Destination and Type (Queue)"', function(done) {
            var payload = '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 + '":{"ClientAddress":"*","ActionList": "Send,Receive,Browse","DestinationType":"Queue","Destination":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Long Destination and Type (Queue)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Range test cases  ====================  //
    describe('MaxMessages[1-5000-20000000]:', function() {

        it('should return status 200 on post "MaxMessage: MIN 1"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessages":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: MIN 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100433
        it('should return status 200 on post "MaxMessage: Default 5000"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessages":null}}}';
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: Default 5000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessage: MAX: 20000000"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
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
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100434
        it('should return status 200 on post "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":null}}}';
            verifyConfig =  JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: RejectNewMessages"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   MaxMsgTTL test cases  ====================  //
    describe('MaxMsgTTL[1-unlimited-2147483647]:', function() {

        it('should return status 200 on post "MaxMessageTimeToLive MIN: 1"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MIN: 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive Default"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":null}}}';
            verifyConfig =  JSON.parse( '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"2147483647"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive unlimited"', function(done) {
            var payload = '{"MessagingPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive unlimited"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   DisconnClt test cases  ====================  //
    describe('DisconnectClient[F]:', function() {

        it('should return status 200 on post "DisconnectedClientNotification true"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255+ '":{"DisconnectedClientNotification":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100437
        it('should return status 200 on post "DisconnectedClientNotification False(Default)"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255+ '":{"DisconnectedClientNotification":null}}}';
            verifyConfig =  JSON.parse( '{"MessagingPolicy":{"T'+ FVT.maxFileName255+ '":{"DisconnectedClientNotification":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification false(Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "DisconnectedClientNotification true"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255+ '":{"DisconnectedClientNotification":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "DisconnectedClientNotification false"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255+ '":{"DisconnectedClientNotification":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"MessagingPolicy":{"'+ FVT.long256Name +'":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

// 96657
        it('should return status 400 on POST DELETE of "TestMsgPol" (not in Angel)', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":null}}' ;
            verifyConfig = JSON.parse( '{"MessagingPolicy":{"TestMsgPol":{}}}'  );
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestMsgPol\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestMsgPol" is NOT gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// POST DELETE is not supported yet
        it('should return status 200 when deleting "TestMsgPol"', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestMsgPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestMsgPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestMsgPol" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"TestMsgPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestMsgPol', FVT.verify404NotFound, verifyConfig, done);
        });
        // it('should return status 200 on GET after POST DELETE, "TestMsgPol" gone', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        // });
        // it('should return status 404 on GET after POST DELETE, "TestMsgPol" not found', function(done) {
		    // verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"TestMsgPol":{}}}' );
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestMsgPol', FVT.verify404NotFound, verifyConfig, done);
        // });
		
        it('should return status 404 on RE-DELETE of "TestMsgPol"', function(done) {
            var payload = '{"MessagingPolicy":{"TestMsgPol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestMsgPol',  FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestMsgPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after POST DELETE, "TestMsgPol" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"TestMsgPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestMsgPol', FVT.verify404NotFound, verifyConfig, done);
        });
    });

//  ====================  Error test cases  ====================  //
    describe('General Error:', function() {

        it('should return status 400 when trying to Delete All Messaging Policies, this is just bad form', function(done) {
            var payload = '{"MessagingPolicy":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"MessagingPolicy\":null} is not valid." };
			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: \"MessagingPolicy\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Messaging Policies, bad form and a Policy should be in use', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 400 when trying to set Invalid Protocol SCADA', function(done) {
            var objName = 'BadProtocolScada'
            var payload = '{"MessagingPolicy":{"'+ objName +'":{"Protocol":"MQTT,JMS,SCADA","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
		    verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT,JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocol was not created', function(done) {
		    verifyConfig = {"status":404,"MessagingPolicy":{"BadProtocolScada":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadProtocolScada", FVT.verify404NotFound, verifyConfig, done);
        });
// 100824
        it('should return status 404 on post "CommonName(s) is misspelled"', function(done) {
            var objName = 'BadParamCommonName'
            var payload = '{"MessagingPolicy":{"'+ objName +'":{"CommonName": "Missing_S_inNames","ActionList": "Publish,Subscribe","ClientID": "*","Destination": "*","DestinationType": "Topic","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: CommonName."};
            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessagingPolicy Name: BadParamCommonName Property: CommonName"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "CommonName(s) is misspelled did not create"', function(done) {
		    verifyConfig = {"status":404,"MessagingPolicy":{"BadParamCommonName":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamCommonName", FVT.verify404NotFound, verifyConfig, done);
        });
// 100425
        it('should return status 400 on post "MissingMinOne" parameter', function(done) {
            var payload = '{"MessagingPolicy":{"MissingMinOne":{"ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
//??            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties CommonNames,UserID,ClientAddress,GroupID,ClientID,Protocol specified."};
//??order   verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties UserID,ClientID,CommonNames,ClientAddress,GroupID,Protocol specified"};
//??random  verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties GroupID,ClientID,ClientAddress,Protocol,UserID,CommonNames specified"};
			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties GroupID,ClientID,ClientAddress,Protocol,UserID,CommonNames specified"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MissingMinOne" parameter did not create', function(done) {
		    verifyConfig = {"status":404,"MessagingPolicy":{"MissingMinOne":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MissingMinOne", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Name Error test cases  ====================  //
    describe('Error Name:', function() {

        it('should return status 400 on POST  "Name too Long"', function(done) {
            var payload = '{"MessagingPolicy":{"'+ FVT.long257Name +'":{"CommonNames": "Jim","ActionList": "Publish,Subscribe","ClientID": "*","Destination": "*","DestinationType": "Topic","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
//			verifyMessage = {"status":400,"Code":"CWLNA0445","Message":"The length of the name must be less than 256."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Name too Long" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"FVT.long257Name", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Description Error test cases  ====================  //
    describe('Error Desc:', function() {

        it('should return status 400 on POST  "DescTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"DescTooLong":{"Description":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DescTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"DescTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DescTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   ClientID Error test cases  ====================  //
    describe('Error ClientID:', function() {

        it('should return status 400 on POST  "ClientIDTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"ClientIDTooLong":{"ClientID":"'+ FVT.long1025String +'","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientIDTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ClientIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    });
	
//  ====================   ClientAddr Error test cases  ====================  //
    describe('Error ClientAddress:', function() {

        it('should return status 400 on POST  "ClientAddressTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"ClientAddressTooLong":{"ClientAddress":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ClientAddressTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   UserID Error test cases  ====================  //
    describe('Error UserID:', function() {

        it('should return status 400 on POST  "UserIDTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"UserIDTooLong":{"UserID":"'+ FVT.long1025String +'","ClientAddress": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "UserIDTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"UserIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"UserIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   GroupID Error test cases  ====================  //
    describe('Error GroupID:', function() {

        it('should return status 400 on POST  "GroupIDTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"GroupIDTooLong":{"GroupID":"'+ FVT.long1025String +'","UserID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "GroupIDTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"GroupIDTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"GroupIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   CommonNames Error test cases  ====================  //
    describe('Error CommonNames:', function() {

        it('should return status 400 on POST  "CommonNamesTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"CommonNamesTooLong":{"CommonNames":"'+ FVT.long1025String +'","GroupID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: CommonNames Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CommonNamesTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"CommonNamesTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"CommonNamesTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   Destination Error test cases  ====================  //
    describe('Error Destination:', function() {

        it('should return status 400 on POST  "DestinationTooLong"', function(done) {
            var payload = '{"MessagingPolicy":{"DestinationTooLong":{"Destination":"'+ FVT.long1025TopicName +'","CommonNames": "*","ActionList": "Publish,Subscribe","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: \\\"'+ FVT.long1025TopicName +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DestinationTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"DestinationTooLong":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DestinationTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   DestinationType Error test cases  ====================  //
    describe('Error DestinationType:', function() {

        it('should return status 400 on POST  "DestinationTypeVacation"', function(done) {
            var payload = '{"MessagingPolicy":{"DestinationTypeVacation":{"DestinationType":"Vacation","Destination": "*","ActionList": "Publish,Subscribe","Destination": "*"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: DestinationType Value: \"Vacation\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DestinationTypeVacation" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"DestinationTypeVacation":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DestinationTypeVacation", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Queue to Topic"', function(done) {
            var payload = '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 +'":{"DestinationType":"Topic","Destination": "*","ActionList": "Publish,Subscribe"}}}';
		    verifyConfig = JSON.parse( payload );
			// Updates to DestinationType config item are not allowed. v1.2
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a QUEUE not Topic', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 + '":{"ActionList": "Send,Receive,Browse","DestinationType":"Queue","Destination":"'+ FVT.long1024String +'"}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'Q' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Queue to Subscription"', function(done) {
            var payload = '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 +'":{"DestinationType":"Subscription","Destination": "*","ActionList": "Receive,Control"}}}'  ;
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a QUEUE not Subscription', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 + '":{"ActionList": "Send,Receive,Browse","DestinationType":"Queue","Destination":"'+ FVT.long1024String +'"}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'Q' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Topic to Subscription"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255 +'":{"DestinationType":"Subscription","Destination": "*","ActionList": "Receive,Control"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a Topic not Subscription', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"T'+ FVT.maxFileName255 + '":{"ActionList": "Publish,Subscribe","DestinationType":"Topic","Destination":"'+ FVT.long1024TopicName +'"}}}') ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'T' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Topic to Queue"', function(done) {
            var payload = '{"MessagingPolicy":{"T'+ FVT.maxFileName255 +'":{"DestinationType":"Queue","Destination": "aQueue","ActionList": "Send,Receive,Browse"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a Topic not Queue', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"T'+ FVT.maxFileName255 + '":{"ActionList": "Publish,Subscribe","DestinationType":"Topic","Destination":"'+ FVT.long1024TopicName +'"}}}') ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'T' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Subscription to Topic"', function(done) {
            var payload = '{"MessagingPolicy":{"S'+ FVT.maxFileName255 +'":{"DestinationType":"Topic","Destination": "*","ActionList": "Publish,Subscribe"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a Subscription not Topic', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"S'+ FVT.maxFileName255 + '":{"ClientAddress":"*","ActionList":"Receive,Control","DestinationType":"Subscription","Destination":"'+ FVT.long1024String +'"}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'S' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST  change "DestinationType": Subscription to Queue"', function(done) {
            var payload = '{"MessagingPolicy":{"S'+ FVT.maxFileName255 +'":{"DestinationType":"Queue","Destination": "*","ActionList": "Send,Receive,Browse"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: DestinationType." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "DestinationType" still a Subscription not Queue', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"S'+ FVT.maxFileName255 + '":{"ClientAddress":"*","ActionList":"Receive,Control","DestinationType":"Subscription","Destination":"'+ FVT.long1024String +'"}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'S' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });
	
    });
//  ====================   ActionList Error test cases  ====================  //
    describe('Error ActionList:', function() {

        it('should return status 400 on POST  "ActionListRead"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListRead":{"ActionList":"Read","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Read\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListRead" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListRead":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListRead", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoSend"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListTopicNoSend":{"ActionList":"Publish,Subscribe,Send","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoSend" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListTopicNoSend":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoSend", FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 on POST  "ActionListTopicNoReceive"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListTopicNoReceive":{"ActionList":"Publish,Subscribe,Receive","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Receive\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoReceive" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListTopicNoReceive":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoReceive", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoBrowse"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListTopicNoBrowse":{"ActionList":"Publish,Subscribe,Browse","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoBrowse" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListTopicNoBrowse":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoControl"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListTopicNoControl":{"ActionList":"Publish,Subscribe,Control","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoControl" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListTopicNoControl":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoControl", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoPublish"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListQueueNoPublish":{"ActionList":"Send,Receive,Browse,Publish","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoPublish" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListQueueNoPublish":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoSubscribe"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListQueueNoSubscribe":{"ActionList":"Send,Receive,Browse,Subscribe","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoSubscribe" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListQueueNoSubscribe":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoControl"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListQueueNoControl":{"ActionList":"Send,Receive,Browse,Control","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoControl" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListQueueNoControl":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoControl", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoPublish"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListSubscriptionNoPublish":{"ActionList":"Receive,Control,Publish","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage =  {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoPublish" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListSubscriptionNoPublish":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoSubscribe"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListSubscriptionNoSubscribe":{"ActionList":"Receive,Control,Subscribe","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoSubscribe" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListSubscriptionNoSubscribe":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoSend"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListSubscriptionNoSend":{"ActionList":"Receive,Control,Send","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoSend" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListSubscriptionNoSend":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoSend", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListSubscriptionNoBrowse"', function(done) {
            var payload = '{"MessagingPolicy":{"ActionListSubscriptionNoBrowse":{"ActionList":"Receive,Control,Browse","DestinationType": "*","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive,Control,Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListSubscriptionNoBrowse" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"ActionListSubscriptionNoBrowse":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListSubscriptionNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });

//  ====================   MaxMessages Error test cases  ====================  //
    describe('Error MaxMessages:', function() {

        it('should return status 400 on POST  "MaxMessagesTooLow"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessagesTooLow":{"MaxMessages":0,"ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessagesTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesTooHigh"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessagesTooHigh":{"MaxMessages":20000001,"ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"20000001\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooHigh" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessagesTooHigh":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on post "MismatchMaxMsgsQueue"', function(done) {
            var payload = '{"MessagingPolicy":{"MismatchMaxMsgsQueue":{"ActionList": "Send,Receive,Browse","DestinationType":"Queue","Destination":"'+ FVT.long1024String +'","MaxMessages":50000}}}';
            verifyConfig = JSON.parse(payload);
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the MessagingPolicy DestinationType is \"Queue\".." }' );
//			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the MessagingPolicy DestinationType is \"Queue\".." } 
			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: MessagingPolicy must have one of the properties ClientAddress,UserID,ClientID,GroupID,CommonNames,Protocol specified" } 
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after fail "MismatchMaxMsgsQueue)"', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MismatchMaxMsgsQueue":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MismatchMaxMsgsQueue", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   MaxMessagesBehavior Error test cases  ====================  //
    describe('Error MaxMessagesBehavior:', function() {

        it('should return status 400 on POST  "MaxMessageBehavior":"TooLow"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessageBehaviorTooLow":{"MaxMessageBehavior":"TooLow","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MaxMessageBehavior." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessagingPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessagingPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageBehaviorTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessageBehaviorTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesBehaviorTooLow"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessagesBehaviorTooLow":{"MaxMessagesBehavior":"TooLow","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessagesBehavior Value: \"TooLow\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesBehaviorTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessagesBehaviorTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   MaxMessageTimeToLive Error test cases  ====================  //
    describe('Error MaxMessageTimeToLive:', function() {
//98282
        it('should return status 400 on POST  "MaxMessageTimeToLiveTooLow"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessageTimeToLiveTooLow":{"MaxMessageTimeToLive":"0","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooLow" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessageTimeToLiveTooLow":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveTooHigh"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessageTimeToLiveTooHigh":{"MaxMessageTimeToLive":"2147483648","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"2147483648\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooHigh" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveLimited"', function(done) {
            var payload = '{"MessagingPolicy":{"MaxMessageTimeToLiveLimited":{"MaxMessageTimeToLive":"Limited","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"Limited\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveLimited" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"MaxMessageTimeToLiveLimited":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveLimited", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   DisconnectedClientNotification Error test cases  ====================  //
    describe('Error DisconnectedClientNotification:', function() {
// 100007
        it('should return status 400 on POST  "DisconnectedClientNotificationInvalid"', function(done) {
            var payload = '{"MessagingPolicy":{"DisconnectedClientNotificationInvalid":{"DisconnectedClientNotification":"True","ClientID": "*","ActionList": "Publish,Subscribe","Destination": "*","DestinationType": "Topic"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: MessagingPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Value: JSON_STRING." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: MessagingPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Type: JSON_STRING" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DisconnectedClientNotificationInvalid" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"DisconnectedClientNotificationInvalid":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DisconnectedClientNotificationInvalid", FVT.verify404NotFound, verifyConfig, done);
        });

	});

describe('Cleanup:', function() {

	it('should return status 200 on Delete QUEUE Policy', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"Q'+ FVT.maxFileName255 + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ 'Q' + FVT.maxFileName255, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Queue Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"Q'+ FVT.maxFileName255 + '":null}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Q'+ FVT.maxFileName255, FVT.verify404NotFound, verifyConfig, done);
        });

	it('should return status 200 on Delete Subscription Policy', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"S'+ FVT.maxFileName255 + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ 'S' + FVT.maxFileName255, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Subscription Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"S'+ FVT.maxFileName255 + '":null}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'S'+ FVT.maxFileName255, FVT.verify404NotFound, verifyConfig, done);
        });

	it('should return status 200 on Delete Topic Policy', function(done) {
		    verifyConfig = JSON.parse( '{"MessagingPolicy":{"T'+ FVT.maxFileName255 + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ 'T' + FVT.maxFileName255, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Topic Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"MessagingPolicy":{"T'+ FVT.maxFileName255 + '":null}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'T'+ FVT.maxFileName255, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
});
