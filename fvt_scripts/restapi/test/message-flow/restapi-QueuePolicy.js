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

var uriObject = 'QueuePolicy/';

var expectDefault = '{"Version": "' + FVT.version + '","QueuePolicy":{} }'; 
//var expectDefault = '{"Version": "' + FVT.version + '","QueuePolicy":{  \
// "DemoQueuePolicy": {"ActionList": "Send,Receive,Browse","ClientID": "*","Description": "Demo Queue Policy","Queue": "AQUEUE", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited"}}}' ;

//var expectTestQueuePol = '{"QueuePolicy": { "TestQueuePol": { "GroupID": "theQueueGroup", "ActionList": "Send,Receive,Browse", "Queue": "iot-2/type/jms/id/*", "ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10'", "Protocol": "", "Description": "The NEW TEST Queue Policy.", "ClientID": "", "UserID": "", "CommonNames": "", "MaxMessageTimeToLive": "unlimited" }}}';
var expectTestQueuePol = '{"QueuePolicy": { "TestQueuePol": { "GroupID": "theQueueGroup", "ActionList": "Send,Receive,Browse", "Queue": "iot-2/type/jms/id/*", "ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10", "Protocol": "", "Description": "The NEW TEST Queue Policy.", "ClientID": "", "UserID": "", "CommonNames": "", "MaxMessageTimeToLive": "unlimited" }}}';

var expectAllQueuePolicies = '{ "Version": "' + FVT.version + '", "QueuePolicy": { \
 "TestQueuePol": {  "GroupID": "theQueueGroup",  "ActionList": "Send,Receive,Browse",  "Queue": "iot-2/type/jms/id/*",  "CommonNames": "",  "Protocol": "",  "Description": "The NEW TEST Queue Policy.",  "ClientID": "",  "ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10",  "UserID": "",  "MaxMessageTimeToLive": "unlimited"  }, \
 "'+ FVT.long256Name +'": {  "ActionList": "Send,Receive,Browse",  "Queue": "'+ FVT.long1024String +'",  "MaxMessageTimeToLive": "unlimited",  "Protocol": "JMS",  "CommonNames": "",  "Description": "",  "ClientID": "",  "ClientAddress": "*",  "GroupID": "",  "UserID": ""  }}}' ;
 
//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Destination,DestinationType,ActionList,MaxMessages,MaxMessagesBehavior,MaxMessageTimeToLive,DisconnectedClientNotification,PendingAction,UseCount",
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Queue,                      ActionList,                                MaxMessageTimeToLive,DisconnectedClientNotification",

describe('QueuePolicy:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
//99809
        it('should return status 200 on get, DEFAULT of "QueuePolicy":"MP1":{..},"MP2":{...}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Description(1024):', function() {

        it('should return status 200 on post TestQueuePol ', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"GroupID":"theQueueGroup","ActionList":"Send,Receive,Browse","Queue":"QUEUE/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, TestQueuePol (type:Queue)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"Description": "The TEST Queue Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"Description": "The NEW TEST Queue Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Valid Range test cases  ====================  //
    describe('Name(256), IDs(1024) Max Length:', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ActionList":"Send,Receive,Browse","Queue": "AQUEUE", "MaxMessageTimeToLive": "unlimited","Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length ClientID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length UserID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"UserID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length UserID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length GroupID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"GroupID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length GroupID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length CommonNames"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"CommonNames": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length CommonNames"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Protocol"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Protocol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":ipv6,ipv4', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":ipv6,ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 108083
        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"QueuePolicy":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
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
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientID"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset ClientAddress"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientAddress"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset UserID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"UserID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset UserID"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"UserID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset GroupID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"GroupID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset GroupID"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"GroupID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset CommonNames"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"CommonNames": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"CommonNames": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Protocol"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": null,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "","ClientID":"*"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Description"', function(done) {
            var payload = '{"QueuePolicy":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Description"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"'+ FVT.long256Name +'":{"Description":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   AtLeastOneOf test cases  ====================  //
    describe('AtLeastOneOf:', function() {

        it('should return status 200 on post "Only ClientAddress":ipv4', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv4,ipv6', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4,ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only UID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"UserID": "'+ FVT.long1024String +'","ClientAddress":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only UID"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"'+ FVT.long1024String +'","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 107789
        it('should return status 200 on post "Only GID"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"GroupID": "'+ FVT.long1024String +'","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only GID"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"'+ FVT.long1024String +'","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only CommonNames"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"CommonNames": "'+ FVT.long1024String +'","GroupID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"'+ FVT.long1024String +'","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only Protocol"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   Protocol test cases  ====================  //
    describe('Protocol["", MQTT, JMS]:', function() {

        it('should return status 200 on post "Protocol":"JMS" only', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"JMS"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"" ', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "","CommonNames":"Buddy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":""', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"Buddy","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// MQTT does not support QUEUEs
        it('should return status 400 on post "Protocol":"MQTT" only', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT","CommonNames":""}}}';
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "","CommonNames":"Buddy"}}}' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Protocol":"MQTT" failed', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"Buddy","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 107785 Doc Error
        it('should return status 400 on post "Protocol":"All"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "All","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"All\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Protocol":"All" failed', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"Buddy","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"JMS"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"JMS"', function(done) {
            verifyConfig = JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   ClientAddress test cases  ====================  //
    describe('ClientAddress:', function() {

        it('should return status 200 on post "ClientAddress":MaxLengthList compressed', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":MaxLengthList compressed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":MaxLengthList', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"ClientAddress":"'+ FVT.CLIENT_ADDRESS_LIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":MaxLengthList', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":IP-Range', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"ClientAddress":"'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10'+'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":IP-Range', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	//  ====================   Queue Destination test cases  ====================  //
    describe('Queue:', function() {
//   /iot-2/type/+/id/+/event/+/fmt/json 
        it('should return status 200 on post "Queue":wildcard', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"Queue":"iot-2/type/jms/id/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Queue":wildcard', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestQueuePol', FVT.getSuccessCallback, verifyConfig, done);
        });

	});

//  ====================   MaxLength test cases  ====================  //
    describe('MAX Names[256] of each Msging TYPE Concurrent:', function() {
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
 
 //  ====================   MaxMsgTTL test cases  ====================  //
    describe('MaxMsgTTL[1-unlimited-2147483647]:', function() {

        it('should return status 200 on post "MaxMessageTimeToLive MIN: 1"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MIN: 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive Default"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":null}}}';
            verifyConfig =  JSON.parse( '{"QueuePolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"2147483647"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive unlimited"', function(done) {
            var payload = '{"QueuePolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive unlimited"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   DisconnClt test cases  ====================  //
// ONLY valid for TOPIC and MQTT.
    describe('DisconnectClient[F]:', function() {
//120951
        it('should return status 400 on post "DisconnectedClientNotification true"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"DisconnectedClientNotification":true}}}';
            verifyConfig = JSON.parse( expectTestQueuePol );
//			verifyMessage = {"status":404, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: QueuePolicy Name: TestQueuePol Property: DisconnectedClientNotification" } ;
			verifyMessage = {"status":400, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: QueuePolicy Name: TestQueuePol Property: DisconnectedClientNotification" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "DisconnectedClientNotification false"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":{"DisconnectedClientNotification":false}}}';
            verifyConfig = JSON.parse( expectTestQueuePol );
//			verifyMessage = {"status":404, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: QueuePolicy Name: TestQueuePol Property: DisconnectedClientNotification" } ;
			verifyMessage = {"status":400, "Code":"CWLNA0138", "Message":"The property name is invalid. Object: QueuePolicy Name: TestQueuePol Property: DisconnectedClientNotification" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

      describe('Config Persists:', function() {
	
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
        it('should return status 200 on get, "QueuePolicies" persists', function(done) {
            verifyConfig = JSON.parse( expectAllQueuePolicies ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

      });


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"QueuePolicy":{"'+ FVT.long256Name +'":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"'+ FVT.long256Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: '+ FVT.long256Name +'","QueuePolicy":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

// 96657
//   --  107787  --  September 2015 - POST Delete not be in 1st release.
        it('should return status 400 on POST DELETE of "TestQueuePol"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":null}}' ;
//            verifyConfig = JSON.parse( payload );
            verifyConfig = JSON.parse( expectTestQueuePol );
//            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request \"TestQueuePol\":null is not valid."};
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestQueuePol\":null is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: \"TestQueuePol\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestQueuePol" fails (still there for now)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        // it('should return status 200 on GET after POST DELETE, "TestQueuePol" gone', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        // });
        // it('should return status 404 on GET after POST DELETE, "TestQueuePol" not found', function(done) {
		    // verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"TestQueuePol":{}}}' );
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestQueuePol', FVT.verify404NotFound, verifyConfig, done);
        // });

// added to really delete since POST Delete does not, so the RE_DELETE test will succeed
        it('should return status 200 on DELETE of "TestQueuePol"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestQueuePol',  FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "TestQueuePol" is really gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
		
        it('should return status 404 on RE-DELETE of "TestQueuePol"', function(done) {
            var payload = '{"QueuePolicy":{"TestQueuePol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestQueuePol',  FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after (re)DELETE, "TestQueuePol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after (re)DELETE, "TestQueuePol" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"TestQueuePol":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: TestQueuePol","QueuePolicy":{"TestQueuePol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestQueuePol', FVT.verify404NotFound, verifyConfig, done);
        });
    });

//  ====================  Error test cases  ====================  //
    describe('General Error:', function() {

        it('should return status 400 when trying to Delete All Messaging Policies, this is just bad form', function(done) {
            var payload = '{"QueuePolicy":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"QueuePolicy\":null} is not valid." };
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: \"QueuePolicy\":null is not valid." };
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"QueuePolicy\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Messaging Policies, bad form', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 120951
        it('should return status 400 when trying to set Invalid MaxMessages On a Queue', function(done) {
            var payload = {"QueuePolicy": {"NoMaxMessage": { "MaxMessages":20000000, "Protocol":"JMS", "ActionList":"Send,Receive,Browse", "Queue": "AQUEUE", "DisconnectedClientNotification": false, "MaxMessageTimeToLive": "unlimited" }}} ;
		    verifyConfig =  payload ;
//		    verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: 20000000." };
		    verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoMaxMessage Property: MaxMessages" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, NoMaxMessage was not created', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"NoMaxMessage":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: NoMaxMessage","QueuePolicy":{"NoMaxMessage":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoMaxMessage", FVT.verify404NotFound, verifyConfig, done);
        });
    
        it('should return status 400 when trying to set Invalid MaxMessageBehavior On a Queue', function(done) {
            var payload = {"QueuePolicy":{"NoMaxMsgBehavior":{"MaxMessageBehavior": "RejectNewMessages","Protocol":"JMS","ActionList": "Send,Receive,Browse","Queue": "AQUEUE", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited"}}} ;
		    verifyConfig =  payload ;
//		    verifyMessage = {"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: 20000000." };
//			verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoMaxMsgBehavior Property: MaxMessageBehavior" } ;
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoMaxMsgBehavior Property: MaxMessageBehavior" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, NoMaxMsgBehavior was not created', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"NoMaxMsgBehavior":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: NoMaxMsgBehavior","QueuePolicy":{"NoMaxMsgBehavior":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoMaxMsgBehavior", FVT.verify404NotFound, verifyConfig, done);
        });
    
        it('should return status 400 when trying to set Invalid Protocol SCADA', function(done) {
            var payload = {"QueuePolicy":{"BadProtocolScada":{"Protocol":"JMS,SCADA","ActionList": "Send,Receive,Browse","Queue": "AQUEUE", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited" }}};
		    verifyConfig =  payload ;
		    verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocolScada was not created', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"BadProtocolScada":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: BadProtocolScada","QueuePolicy":{"BadProtocolScada":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadProtocolScada", FVT.verify404NotFound, verifyConfig, done);
        });
// 100824 120951
        it('should return status 400 on post "CommonName(s) is misspelled"', function(done) {
            var objName = 'BadParamCommonName'
            var payload = '{"QueuePolicy":{"'+ objName +'":{"CommonName": "Missing_S_inNames","ActionList": "Send,Receive,Browse","ClientID": "*","Queue": "AQUEUE", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited" }}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: CommonName."};
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: BadParamCommonName Property: CommonName"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: BadParamCommonName Property: CommonName"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "CommonName(s) is misspelled did not create"', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"BadParamCommonName":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: BadParamCommonName","QueuePolicy":{"BadParamCommonName":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamCommonName", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "NoDestinationType" no longer needed', function(done) {
            var payload = '{"QueuePolicy":{"NoDestinationType":{"DestinationType":"Queue", "ActionList":"Send,Receive,Browse", "ClientID":"*", "Queue": "AQUEUE","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited" }}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoDestinationType Property: DestinationType"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoDestinationType Property: DestinationType"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDestinationType" no longer needed"', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"NoDestinationType":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: NoDestinationType","QueuePolicy":{"NoDestinationType":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDestinationType", FVT.verify404NotFound, verifyConfig, done);
        });
// NOT SURE IF test matches comment valid?
        it('should return status 400 on POST "DisconnectedClientNotification" is not in "Queue"', function(done) {
            var payload = '{"QueuePolicy":{"NoDisconnectedClientNotification":{"CommonNames": "NoDestNeeded","ActionList": "Send,Receive,Browse","ClientID": "*","Queue": "AQUEUE","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited" }}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoDestinationType Property: Destination"};
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoDisconnectedClientNotification Property: DisconnectedClientNotification"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: NoDisconnectedClientNotification Property: DisconnectedClientNotification"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDisconnectedClientNotification" in Queue"', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"NoDisconnectedClientNotification":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: NoDisconnectedClientNotification","QueuePolicy":{"NoDisconnectedClientNotification":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDisconnectedClientNotification", FVT.verify404NotFound, verifyConfig, done);
        });
// 100425, 112795
        it('should return status 400 on post "MissingMinOne" parameter', function(done) {
            var payload = '{"QueuePolicy":{"MissingMinOne":{"ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
//??            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties CommonNames,UserID,ClientAddress,GroupID,ClientID,Protocol specified."};
//??order   verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties UserID,ClientID,CommonNames,ClientAddress,GroupID,Protocol specified"};
//??random  verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties GroupID,ClientID,ClientAddress,Protocol,UserID,CommonNames specified"};
//			verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties ClientAddress,Protocol,GroupID,ClientID,UserID,CommonNames specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties CommonNames,UserID,Protocol,ClientID,ClientAddress,GroupID specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties CommonNames,UserID,ClientID,ClientAddress,Protocol,GroupID specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties Protocol,ClientID,ClientAddress,CommonNames,UserID,GroupID specified"};
            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: QueuePolicy must have one of the properties ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol specified"};
			FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "MissingMinOne" parameter did not create"', function(done) {
//		    verifyConfig = {"status":404,"QueuePolicy":{"MissingMinOne":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: MissingMinOne","QueuePolicy":{"MissingMinOne":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MissingMinOne", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Name Error test cases  ====================  //
    describe('Error Name:', function() {

        it('should return status 400 on POST  "Name too Long"', function(done) {
            var payload = '{"QueuePolicy":{"'+ FVT.long257Name +'":{"CommonNames": "Jim","ActionList": "Send,Receive,Browse","ClientID": "*","Queue": "AQUEUE", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
//			verifyMessage = {"status":400,"Code":"CWLNA0445","Message":"The length of the name must be less than 256."} ;
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: QueuePolicy Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Name too Long" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: '+ FVT.long257Name +'","QueuePolicy":{"'+ FVT.long257Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   Description Error test cases  ====================  //
    describe('Error Desc:', function() {

        it('should return status 400 on POST  "DescTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"DescTooLong":{"Description":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: Description Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DescTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"DescTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: DescTooLong","QueuePolicy":{"DescTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DescTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

	});
	
//  ====================   ClientID Error test cases  ====================  //
    describe('Error ClientID:', function() {

        it('should return status 400 on POST  "ClientIDTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"ClientIDTooLong":{"ClientID":"'+ FVT.long1025String +'","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: ClientID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientIDTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ClientIDTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ClientIDTooLong","QueuePolicy":{"ClientIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    });
	
//  ====================   ClientAddr Error test cases  ====================  //
    describe('Error ClientAddress:', function() {

        it('should return status 400 on POST  "ClientAddressListTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"ClientAddressListTooLong":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1)  +'","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//s			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: ClientAddress Value: '+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1)  +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressListTooLong" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ClientAddressListTooLong","QueuePolicy":{"ClientAddressListTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressListTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ClientAddressTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"ClientAddressTooLong":{"ClientAddress":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: ClientAddress Value: '+ FVT.long1025String +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ClientAddressTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ClientAddressTooLong","QueuePolicy":{"ClientAddressTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   UserID Error test cases  ====================  //
    describe('Error UserID:', function() {

        it('should return status 400 on POST  "UserIDTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"UserIDTooLong":{"UserID":"'+ FVT.long1025String +'","ClientAddress": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: UserID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "UserIDTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"UserIDTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: UserIDTooLong","QueuePolicy":{"UserIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"UserIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   GroupID Error test cases  ====================  //
    describe('Error GroupID:', function() {

        it('should return status 400 on POST  "GroupIDTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"GroupIDTooLong":{"GroupID":"'+ FVT.long1025String +'","UserID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: GroupID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "GroupIDTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"GroupIDTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: GroupIDTooLong","QueuePolicy":{"GroupIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"GroupIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   CommonNames Error test cases  ====================  //
    describe('Error CommonNames:', function() {

        it('should return status 400 on POST  "CommonNamesTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"CommonNamesTooLong":{"CommonNames":"'+ FVT.long1025String +'","GroupID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: CommonNames Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: CommonNames Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CommonNamesTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"CommonNamesTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: CommonNamesTooLong","QueuePolicy":{"CommonNamesTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"CommonNamesTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });
//  ====================   Destination Error test KeyName NOW: Queue,Topic, or Subscription  ====================  //
    describe('Error Destination:', function() {
// 120951
        it('should return status 400 on POST  "DestinationNotValid"', function(done) {
            var payload = '{"QueuePolicy":{"DestinationNotValid":{"Destination":"'+ FVT.long1025TopicName +'","CommonNames": "*","ActionList": "Send,Receive,Browse" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: '+ FVT.long1025TopicName +'." }' );
//			verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: DestinationNotValid Property: Destination"} ;
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: DestinationNotValid Property: Destination"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DestinationNotValid" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"DestinationNotValid":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: DestinationNotValid","QueuePolicy":{"DestinationNotValid":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DestinationNotValid", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST  "QueueNameTooLong"', function(done) {
            var payload = '{"QueuePolicy":{"QueueNameTooLong":{"Queue":"'+ FVT.long1025TopicName +'","CommonNames": "*","ActionList": "Send,Receive,Browse" }}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: '+ FVT.long1025TopicName +'." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: QueueNameTooLong Property: Queue Value: '+ FVT.long1025TopicName +'." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Queue Value: \\\"'+ FVT.long1025TopicName +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueuePolicy Property: Queue Value: '+ FVT.long1025TopicName +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "QueueNameTooLong" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"QueueNameTooLong":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: QueueNameTooLong","QueuePolicy":{"QueueNameTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"QueueNameTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

    });
//  ====================   ActionList Error test cases  ====================  //
    describe('Error ActionList:', function() {

        it('should return status 400 on POST  "ActionListRead"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListRead":{"ActionList":"Read", "Queue": "AQUEUE" }}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Read\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListRead" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListRead":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListRead","QueuePolicy":{"ActionListRead":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListRead", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoPublish"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListNoPublish":{"ActionList":"Publish","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoPublish" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListNoPublish":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListNoPublish","QueuePolicy":{"ActionListNoPublish":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 on POST  "ActionListNoSubscribe"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListNoSubscribe":{"ActionList":"Subscribe","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload ); 
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoSubscribe" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListNoSubscribe":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListNoSubscribe","QueuePolicy":{"ActionListNoSubscribe":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoControl"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListNoControl":{"ActionList":"Control","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoControl" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListNoControl":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListNoControl","QueuePolicy":{"ActionListNoControl":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoControl", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoPublish"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListQueueNoPublish":{"ActionList":"Send,Receive,Browse,Publish","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Publish\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoPublish" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListQueueNoPublish":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListQueueNoPublish","QueuePolicy":{"ActionListQueueNoPublish":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoPublish", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoSubscribe"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListQueueNoSubscribe":{"ActionList":"Send,Receive,Browse,Subscribe","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Subscribe\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoSubscribe" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListQueueNoSubscribe":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListQueueNoSubscribe","QueuePolicy":{"ActionListQueueNoSubscribe":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoSubscribe", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListQueueNoControl"', function(done) {
            var payload = '{"QueuePolicy":{"ActionListQueueNoControl":{"ActionList":"Send,Receive,Browse,Control","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse,Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListQueueNoControl" did not create', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"ActionListQueueNoControl":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: ActionListQueueNoControl","QueuePolicy":{"ActionListQueueNoControl":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListQueueNoControl", FVT.verify404NotFound, verifyConfig, done);
        });
	
    });

//  ====================   MaxMessageTimeToLive Error test cases  ====================  //
    describe('Error MaxMessageTimeToLive:', function() {
//98282
        it('should return status 400 on POST  "MaxMessageTimeToLiveTooLow"', function(done) {
            var payload = '{"QueuePolicy":{"MaxMessageTimeToLiveTooLow":{"MaxMessageTimeToLive":"0","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooLow" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"MaxMessageTimeToLiveTooLow":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: MaxMessageTimeToLiveTooLow","QueuePolicy":{"MaxMessageTimeToLiveTooLow":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveTooHigh"', function(done) {
            var payload = '{"QueuePolicy":{"MaxMessageTimeToLiveTooHigh":{"MaxMessageTimeToLive":"2147483648","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"2147483648\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooHigh" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: MaxMessageTimeToLiveTooHigh","QueuePolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveLimited"', function(done) {
            var payload = '{"QueuePolicy":{"MaxMessageTimeToLiveLimited":{"MaxMessageTimeToLive":"Limited","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"Limited\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveLimited" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"MaxMessageTimeToLiveLimited":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: MaxMessageTimeToLiveLimited","QueuePolicy":{"MaxMessageTimeToLiveLimited":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveLimited", FVT.verify404NotFound, verifyConfig, done);
        });

	});

//  ====================   DisconnectedClientNotification Error test cases  ====================  //
    describe('Error DisconnectedClientNotification:', function() {
// 100007 120951
        it('should return status 400 on POST  "DisconnectedClientNotificationInvalid"', function(done) {
            var payload = '{"QueuePolicy":{"DisconnectedClientNotificationInvalid":{"DisconnectedClientNotification":"True","ClientID": "*","ActionList": "Send,Receive,Browse","Queue": "AQUEUE"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: QueuePolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Value: JSON_STRING." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0127","Message":"The property type is not valid. Object: QueuePolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Type: JSON_STRING" }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueuePolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DisconnectedClientNotificationInvalid" did not create', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"DisconnectedClientNotificationInvalid":{}}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: DisconnectedClientNotificationInvalid","QueuePolicy":{"DisconnectedClientNotificationInvalid":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DisconnectedClientNotificationInvalid", FVT.verify404NotFound, verifyConfig, done);
        });

	});

    describe('Cleanup:', function() {

	    it('should return status 404 on (re)Delete MaxLen Name QUEUE Policy', function(done) {
		    verifyConfig = JSON.parse( '{"QueuePolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'QueuePolicy/' + FVT.long256Name, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Queue Policy gone', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: '+ FVT.long256Name + '","QueuePolicy":{"'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueuePolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

	    it('should return status 200 on Delete Subscription Policy', function(done) {
		    verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SubscriptionPolicy/' + FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Subscription Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: '+ FVT.long256Name + '","SubscriptionPolicy":{"'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

	    it('should return status 200 on Delete Topic Policy', function(done) {
		    verifyConfig = JSON.parse( '{"TopicPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+ 'TopicPolicy/' + FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Topic Policy gone', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"T'+ FVT.long256Name + '":null}}' ) ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: T'+ FVT.long256Name + '","TopicPolicy":{"T'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+ 'TopicPolicy/T' + FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
});
