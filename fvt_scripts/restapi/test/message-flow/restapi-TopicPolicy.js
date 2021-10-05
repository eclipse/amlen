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
var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLxxx","Message":"blah blah"}

var uriObject = 'TopicPolicy/';

var expectDefault = '{"Version": "' + FVT.version + '","TopicPolicy": { \
"DemoTopicPolicy": {"ActionList": "Publish,Subscribe","ClientID": "*","Description": "Demo topic policy","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages","Topic": "*"}}}' ;

var expectAllTopicPolicies = '{ "Version": "' + FVT.version + '", "TopicPolicy": { \
 "DemoTopicPolicy": {  "Description": "Demo topic policy",  "Topic": "*",  "ClientID": "*",  "ActionList": "Publish,Subscribe",  "MaxMessages": 5000,  "MaxMessagesBehavior": "RejectNewMessages",  "MaxMessageTimeToLive": "unlimited",  "DisconnectedClientNotification": false  }, \
 "TestTopicPol": {  "GroupID": "theTopicGroup",  "ActionList": "Publish,Subscribe",  "Topic": "'+ FVT.topicLevel32 +'",  "MaxMessageTimeToLive": "unlimited",  "ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10",  "Description": "The NEW TEST Topic Policy.",  "ClientID": "",  "CommonNames": "",  "UserID": "",  "Protocol": "",  "MaxMessages": 5000,  "MaxMessagesBehavior": "RejectNewMessages",  "DisconnectedClientNotification": false  }, \
 "'+ FVT.long256Name +'": {  "ActionList": "Subscribe",  "MaxMessageTimeToLive": "unlimited",  "Topic": "'+ FVT.long1024TopicName +'",  "MaxMessages": 20000000,  "MaxMessagesBehavior": "RejectNewMessages",  "Protocol": "MQTT",  "ClientAddress": "*",  "Description": "",  "GroupID": "",  "ClientID": "",  "CommonNames": "",  "UserID": "",  "DisconnectedClientNotification": false}}}' ;

//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Destination,DestinationType,ActionList,MaxMessages,MaxMessagesBehavior,MaxMessageTimeToLive,DisconnectedClientNotification,PendingAction,UseCount",
//      "ListObjects":"Name,Description,ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol,Topic,                      ActionList,MaxMessages,MaxMessagesBehavior,MaxMessageTimeToLive,DisconnectedClientNotification",

describe('TopicPolicy:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
//99809
        it('should return status 200 on get, DEFAULT of "TopicPolicy":"MP1":{..},"MP2":{...}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Description(1024):', function() {

        it('should return status 200 on post TestTopicPol ', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"GroupID":"theTopicGroup","ActionList":"Publish,Subscribe","Topic":"/topic/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, TestTopicPol (type:Queue)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 107777
        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"Description": "The TEST Topic Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"Description": "The NEW TEST Topic Policy."}}}';
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
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ActionList":"Publish,Subscribe","Topic": "*", "MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages","Protocol": "JMS,MQTT"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length ClientID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//101151
        it('should return status 200 on post "Max Length UserID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"UserID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length UserID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length GroupID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"GroupID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length GroupID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length CommonNames"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"CommonNames": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length CommonNames"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Protocol"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Protocol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":ipv6,ipv4', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":ipv6,ipv4', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"TopicPolicy":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
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
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientID"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset ClientAddress"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientAddress"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset UserID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"UserID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset UserID"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"UserID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset GroupID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"GroupID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset GroupID"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"GroupID": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset CommonNames"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"CommonNames": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"CommonNames": ""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Protocol"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": null,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "","ClientID":"*"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103152
        it('should return status 200 on post "Reset Description"', function(done) {
            var payload = '{"TopicPolicy":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Description"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"'+ FVT.long256Name +'":{"Description":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   AtLeastOneOf test cases  ====================  //
    describe('AtLeastOneOf:', function() {

        it('should return status 200 on post "Only ClientAddress":ipv4', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv4,ipv6', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4,ipv6"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +','+ FVT.msgServer_IPv6 +'","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only UID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"UserID": "'+ FVT.long1024String +'","ClientAddress":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only UID"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"'+ FVT.long1024String +'","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only GID"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"GroupID": "'+ FVT.long1024String +'","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only GID"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"'+ FVT.long1024String +'","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only CommonNames"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"CommonNames": "'+ FVT.long1024String +'","GroupID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"'+ FVT.long1024String +'","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only Protocol"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT,JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    //  ====================   Protocol test cases  ====================  //
    describe('Protocol["", MQTT, JMS, (All)]:', function() {

        it('should return status 200 on post "Protocol":"JMS" only', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"JMS"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"" ', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "","CommonNames":"Buddy"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":""', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"Buddy","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"MQTT" only', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT","CommonNames":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"MQTT"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 107785 DOC error
        it('should return status 400 on post "Protocol":"All"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "All"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"All\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });        
        it('should return status 200 on get, Post fails "Protocol":"All"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on post (lc) "Protocol":"mqtt,jms"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "mqtt,jms"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get (lc) "Protocol":"mqtt,jms"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"mqtt,jms"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Protocol":"MQTT,JMS"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Protocol":"MQTT,JMS"', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "","ClientID":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT,JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    //  ====================   ClientAddress test cases  ====================  //
    describe('ClientAddress:', function() {
// 107786
        it('should return status 200 on post "ClientAddress":MaxLengthList compressed', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":MaxLengthList compresses', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":MaxLengthList', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"ClientAddress":"'+ FVT.CLIENT_ADDRESS_LIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":MaxLengthList', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ClientAddress":IP-Range', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"ClientAddress":"'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +',10.10.10.10-10.10.10.10'+'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ClientAddress":IP-Range', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    //  ====================   Topic Destination test cases  ====================  //
    describe('Topic:', function() {
//   /iot-2/type/+/id/dog/event/+/fmt/json 
        it('should return status 200 on post "Topic":wildcard', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"Topic":"/iot-2/type/*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Topic":wildcard', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Topic":32Levels', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":{"Topic":"'+ FVT.topicLevel32 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Topic":32Levels', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    //  ====================   MaxLength test cases  ====================  //
    describe('MAX Names[256] of each Msging TYPE:', function() {
// 100425
        it('should return status 200 on post "Long Destination and Type (Subscription)"', function(done) {
            var payload = '{"SubscriptionPolicy":{"'+ FVT.long256Name + '":{"ClientAddress":"*","ActionList":"Receive,Control","Subscription":"'+ FVT.long1024String +'"}}}';
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
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessages":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: MIN 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100433, 103153
        it('should return status 200 on post "MaxMessage: Default 5000"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessages":null}}}';
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Message: Default 5000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessage: MAX: 20000000"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
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
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100434
        it('should return status 200 on post "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":null}}}';
            verifyConfig =  JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: Default(Reject New)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"DiscardOldMessages"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessagesBehavior: DiscardOldMessages"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessagesBehavior: RejectNewMessages"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessagesBehavior":"RejectNewMessages"}}}';
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
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MIN: 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive Default"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":null}}}';
            verifyConfig =  JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"2147483647"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive MAX: 2147483647"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessageTimeToLive unlimited"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"MaxMessageTimeToLive":"unlimited"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessageTimeToLive unlimited"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   DisconnClt test cases  ====================  //
// ONLY valid for TOPIC and MQTT with Subscribe authority.

    describe('DisconnectClient[F]:', function() {

        it('should return status 200 on post "DisconnectedClientNotification true"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 100437
        it('should return status 200 on post "DisconnectedClientNotification False(Default)"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":null}}}';
            verifyConfig =  JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification false(Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "DisconnectedClientNotification":true and "Protocol":"jms"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true,"Protocol":"jms"}}}';
//             verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0380","Message":"DisconnectedClientNotification cannot be enabled, if MQTT protocol is not allowed in the policy."};
            verifyMessage = {"status":400,"Code":"CWLNA0380","Message":"DisconnectedClientNotification cannot be enabled if MQTT protocol is not allowed in the topic policy."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, POST Fails "DisconnectedClientNotification":true and "Protocol":"jms"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "DisconnectedClientNotification":true and "Protocol":"MQTT,..."', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true,"Protocol":"MQTT,JMS"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification":true and "Protocol":"MQTT,..."', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "DisconnectedClientNotification":true, "Protocol":"MQTT, "ActionList":"Subscribe"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true,"Protocol":"MQTT", "ActionList":"Subscribe"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification":true, "Protocol":"MQTT, "ActionList":"Subscribe"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// MUST have Subscribe authority
        it('should return status 400 on post "DisconnectedClientNotification":true, "Protocol":"MQTT, "ActionList":"Publish"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true,"Protocol":"MQTT", "ActionList":"Publish"}}}';
            verifyConfig = JSON.parse( '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":true,"Protocol":"MQTT", "ActionList":"Subscribe"}}}' );
//            verifyMessage = {"status":400,"Code":"CWLNA0381","Message":"DisconnectedClientNotification cannot be enabled, if policy does not grant subscribe authority."};
            verifyMessage = {"status":400,"Code":"CWLNA0381","Message":"DisconnectedClientNotification cannot be enabled if topic policy does not grant subscribe authority."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "DisconnectedClientNotification":true, "Protocol":"MQTT, "ActionList":"Publish" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "DisconnectedClientNotification false"', function(done) {
            var payload = '{"TopicPolicy":{"' + FVT.long256Name + '":{"DisconnectedClientNotification":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
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
        it('should return status 200 on get, "TopicPolicies" persists', function(done) {
            verifyConfig = JSON.parse( expectAllTopicPolicies ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

      });


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"TopicPolicy":{"'+ FVT.long256Name +'":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"'+ FVT.long256Name +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: '+ FVT.long256Name +'","TopicPolicy":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

// 96657
// 09/11: No longer NO OP a Post Delete, should flag as an error.
// 107787
        it('should return status 200 on POST DELETE of "TestTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":null}}' ;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"TestTopicPol\":null is not valid."};
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"TestTopicPol\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
//        it('should return status 200 on GET after POST DELETE, "TestTopicPol" gone', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
//        });
        it('should return status 200 on GET after POST DELETE, "TestTopicPol" fails', function(done) {
            verifyConfig = {"TopicPolicy":{"TestTopicPol":{"GroupID": "theTopicGroup"}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestTopicPol', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on REAL-DELETE of "TestTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestTopicPol',  FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after REAL DELETE, "TestTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after REAL DELETE, "TestTopicPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"TestTopicPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: TestTopicPol","TopicPolicy":{"TestTopicPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on RE-DELETE of "TestTopicPol"', function(done) {
            var payload = '{"TopicPolicy":{"TestTopicPol":null}}' ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestTopicPol',  FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after RE-DELETE, "TestTopicPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after RE-DELETE, "TestTopicPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"TestTopicPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: TestTopicPol","TopicPolicy":{"TestTopicPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestTopicPol', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

//  ====================  Error test cases  ====================  //
    describe('General Error:', function() {

        it('should return status 400 when trying to Delete All Messaging Policies, this is just bad form', function(done) {
            var payload = '{"TopicPolicy":null}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"TopicPolicy\":null} is not valid." };
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: \"TopicPolicy\":null is not valid." };
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"TopicPolicy\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Messaging Policies, bad form', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 400 when trying to set Invalid Protocol SCADA', function(done) {
            var objName = 'BadProtocolScada'
            var payload = '{"TopicPolicy":{"'+ objName +'":{"Protocol":"MQTT,JMS,SCADA","ActionList": "Publish,Subscribe","Topic": "*", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT,JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocol was not created', function(done) {
//            verifyConfig = {"status":404,"TopicPolicy":{"BadProtocolScada":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: BadProtocolScada","TopicPolicy":{"BadProtocolScada":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadProtocolScada", FVT.verify404NotFound, verifyConfig, done);
        });
// 100824
        it('should return status 404 on post "CommonName(s) is misspelled"', function(done) {
            var objName = 'BadParamCommonName'
            var payload = '{"TopicPolicy":{"'+ objName +'":{"CommonName": "Missing_S_inNames","ActionList": "Publish,Subscribe","ClientID": "*","Topic": "*", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: CommonName."};
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: BadParamCommonName Property: CommonName"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: BadParamCommonName Property: CommonName"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "CommonName(s) is misspelled did not create"', function(done) {
//            verifyConfig = {"status":404,"TopicPolicy":{"BadParamCommonName":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: BadParamCommonName","TopicPolicy":{"BadParamCommonName":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamCommonName", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST "NoDestinationType" no longer needed', function(done) {
            var payload = '{"TopicPolicy":{"NoDestinationType":{"DestinationType":"Topic", "ActionList":"Publish,Subscribe", "ClientID":"*", "Topic": "*","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: NoDestinationType Property: DestinationType"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: NoDestinationType Property: DestinationType"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDestinationType" no longer needed"', function(done) {
//            verifyConfig = {"status":404,"TopicPolicy":{"NoDestinationType":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: NoDestinationType","TopicPolicy":{"NoDestinationType":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDestinationType", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST "Destination" is now "Topic"', function(done) {
            var payload = '{"TopicPolicy":{"NoDestinationParam":{"CommonNames": "NoDestNeeded","ActionList": "Publish,Subscribe","ClientID": "*","Destination": "*","DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages": 5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: NoDestinationParam Property: Destination"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: NoDestinationParam Property: Destination"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on Get "NoDestinationParam" no longer needed"', function(done) {
//            verifyConfig = {"status":404,"TopicPolicy":{"NoDestinationParam":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: NoDestinationParam","TopicPolicy":{"NoDestinationParam":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoDestinationParam", FVT.verify404NotFound, verifyConfig, done);
        });
// 100425
        it('should return status 400 on post "MissingMinOne" parameter', function(done) {
            var payload = '{"TopicPolicy":{"MissingMinOne":{"ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
//??            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties CommonNames,UserID,ClientAddress,GroupID,ClientID,Protocol specified."};
//??order   verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties UserID,ClientID,CommonNames,ClientAddress,GroupID,Protocol specified"};
//??random  verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties GroupID,ClientID,ClientAddress,Protocol,UserID,CommonNames specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties GroupID,ClientAddress,ClientID,UserID,CommonNames,Protocol specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties CommonNames,UserID,Protocol,ClientID,ClientAddress,GroupID specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties CommonNames,UserID,ClientID,ClientAddress,GroupID,Protocol specified"};
//            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties Protocol,ClientID,ClientAddress,CommonNames,UserID,GroupID specified"};
            verifyMessage = {"status":400,"Code":"CWLNA0139","Message":"The object: TopicPolicy must have one of the properties ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol specified"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "MissingMinOne" parameter did not create"', function(done) {
//            verifyConfig = {"status":404,"TopicPolicy":{"MissingMinOne":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MissingMinOne","TopicPolicy":{"MissingMinOne":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MissingMinOne", FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
//  ====================   Name Error test cases  ====================  //
    describe('Error Name:', function() {

        it('should return status 400 on POST  "Name too Long"', function(done) {
            var payload = '{"TopicPolicy":{"'+ FVT.long257Name +'":{"CommonNames": "Jim","ActionList": "Publish,Subscribe","ClientID": "*","Topic": "*", "DisconnectedClientNotification": false,"MaxMessageTimeToLive": "unlimited","MaxMessages":5000,"MaxMessagesBehavior": "RejectNewMessages"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0445","Message":"The length of the name must be less than 256."} ;
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: TopicPolicy Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Name too Long" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: '+ FVT.long257Name +'","TopicPolicy":{"'+ FVT.long257Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
//  ====================   Description Error test cases  ====================  //
    describe('Error Desc:', function() {

        it('should return status 400 on POST  "DescTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"DescTooLong":{"Description":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: Description Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DescTooLong" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"DescTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: DescTooLong","TopicPolicy":{"DescTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DescTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

    });


//  ====================   Topic Error test cases  ====================  //
    describe('Error Topic:', function() {
// 120906 - not enforcing
        it('should return status 200 on post "Topic":33Levels', function(done) {
            var payload = '{"TopicPolicy":{"TestTopic33Levels":{"Topic":"'+ FVT.topicLevel33 +'","ActionList": "Publish,Subscribe","ClientID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Topic":33Levels', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestTopic33Levels', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on Delete TestTopic33Levels', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"TestTopic33Levels":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'TopicPolicy/TestTopic33Levels', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete TestTopic33Levels gone', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: TestTopic33Levels","TopicPolicy":{"TestTopic33Levels":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'TopicPolicy/TestTopic33Levels', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
//  ====================   ClientID Error test cases  ====================  //
    describe('Error ClientID:', function() {

        it('should return status 400 on POST  "ClientIDTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"ClientIDTooLong":{"ClientID":"'+ FVT.long1025String +'","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: ClientID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientIDTooLong" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ClientIDTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ClientIDTooLong","TopicPolicy":{"ClientIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    });
    
//  ====================   ClientAddr Error test cases  ====================  //
// List is 100 MAX, Length of the Value is 8K  (3/2016)
    describe('Error ClientAddress:', function() {

        it('should return status 400 on POST  "ClientAddressListTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"ClientAddressListTooLong":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1)  +'","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: ClientAddress Value: '+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1)  +'." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressListTooLong" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ClientAddressListTooLong","TopicPolicy":{"ClientAddressListTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressListTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ClientAddressTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"ClientAddressTooLong":{"ClientAddress":"'+ FVT.long1025String +'","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: ClientAddress Value: '+ FVT.long1025String +'." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ClientAddressTooLong" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ClientAddressTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ClientAddressTooLong","TopicPolicy":{"ClientAddressTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ClientAddressTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    
    });
//  ====================   UserID Error test cases  ====================  //
    describe('Error UserID:', function() {

        it('should return status 400 on POST  "UserIDTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"UserIDTooLong":{"UserID":"'+ FVT.long1025String +'","ClientAddress": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: UserID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "UserIDTooLong" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"UserIDTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: UserIDTooLong","TopicPolicy":{"UserIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"UserIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    
    });
//  ====================   GroupID Error test cases  ====================  //
    describe('Error GroupID:', function() {

        it('should return status 400 on POST  "GroupIDTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"GroupIDTooLong":{"GroupID":"'+ FVT.long1025String +'","UserID": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupID Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: GroupID Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "GroupIDTooLong" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"GroupIDTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: GroupIDTooLong","TopicPolicy":{"GroupIDTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"GroupIDTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    
    });
//  ====================   CommonNames Error test cases  ====================  //
    describe('Error CommonNames:', function() {

        it('should return status 400 on POST  "CommonNamesTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"CommonNamesTooLong":{"CommonNames":"'+ FVT.long1025String +'","GroupID": "*","ActionList": "Publish,Subscribe","Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: CommonNames Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: CommonNames Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CommonNamesTooLong" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"CommonNamesTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: CommonNamesTooLong","TopicPolicy":{"CommonNamesTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"CommonNamesTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
    
    });
//  ====================   Destination (Topic) Error test cases  ====================  //
    describe('Error Destination:', function() {

        it('should return status 400 on POST  "DestinationTooLong"', function(done) {
            var payload = '{"TopicPolicy":{"DestinationTooLong":{"Topic":"'+ FVT.long1025TopicName +'","CommonNames": "*","ActionList": "Publish,Subscribe" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: '+ FVT.long1025TopicName +'." }' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Topic Value: \\\"'+ FVT.long1025TopicName +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TopicPolicy Property: Topic Value: '+ FVT.long1025TopicName +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DestinationTooLong" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"DestinationTooLong":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: DestinationTooLong","TopicPolicy":{"DestinationTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DestinationTooLong", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST  with "Destination": Queue not Topic"', function(done) {
            var payload = '{"TopicPolicy":{"Q_Destination":{"ClientID":"*", "Queue": "aQueue", "ActionList": "Publish,Subscribe"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Destination is  a Queue." }' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: ." }' );
//            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: Q_Destination Property: Queue" }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: Q_Destination Property: Queue" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Destination" is a QUEUE not Topic', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"Q_Destination":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: Q_Destination","TopicPolicy":{"Q_Destination":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'Q_Destination', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST "Destination": Subscription" not Topic', function(done) {
            var payload = '{"TopicPolicy":{"S_Destination":{"ClientID":"*",  "Subscription": "subscription1", "ActionList": "Publish,Subscribe"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Destination." }' );
//            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: S_Destination Property: Subscription" }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: S_Destination Property: Subscription" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "Destination"  is a Subscription not Topic', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"S_Destination":{}}}') ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: S_Destination","TopicPolicy":{"S_Destination":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'S_Destination', FVT.verify404NotFound, verifyConfig, done);
        });

    });
//  ====================   ActionList Error test cases  ====================  //
    describe('Error ActionList:', function() {

        it('should return status 400 on POST  "ActionListRead"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListRead":{"ActionList":"Read", "Topic": "*" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Read\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListRead" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListRead":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListRead","TopicPolicy":{"ActionListRead":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListRead", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoSend"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListTopicNoSend":{"ActionList":"Publish,Subscribe,Send","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoSend" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListTopicNoSend":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListTopicNoSend","TopicPolicy":{"ActionListTopicNoSend":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoSend", FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 on POST  "ActionListTopicNoReceive"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListTopicNoReceive":{"ActionList":"Publish,Subscribe,Receive","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage =  {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Receive\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoReceive" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListTopicNoReceive":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListTopicNoReceive","TopicPolicy":{"ActionListTopicNoReceive":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoReceive", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoBrowse"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListTopicNoBrowse":{"ActionList":"Publish,Subscribe,Browse","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoBrowse" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListTopicNoBrowse":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListTopicNoBrowse","TopicPolicy":{"ActionListTopicNoBrowse":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListTopicNoControl"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListTopicNoControl":{"ActionList":"Publish,Subscribe,Control","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Publish,Subscribe,Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListTopicNoControl" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListTopicNoControl":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListTopicNoControl","TopicPolicy":{"ActionListTopicNoControl":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListTopicNoControl", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoSend"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListNoSend":{"ActionList":"Send","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: Send,Receive,Browse,Publish." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoSend" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListNoSend":{}}}' ) ;
              verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListNoSend","TopicPolicy":{"ActionListNoSend":{}}}' );
          FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoSend", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoReceive"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListNoReceive":{"ActionList":"Receive","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: Send,Receive,Browse,Subscribe." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Receive\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoReceive" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListNoReceive":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListNoReceive","TopicPolicy":{"ActionListNoReceive":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoReceive", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoControl"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListNoControl":{"ActionList":"Control","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: Send,Receive,Browse,Control." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Control\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoControl" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListNoControl":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListNoControl","TopicPolicy":{"ActionListNoControl":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoControl", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "ActionListNoBrowse"', function(done) {
            var payload = '{"TopicPolicy":{"ActionListNoBrowse":{"ActionList":"Browse","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: Receive,Control,Publish." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Browse\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "ActionListNoBrowse" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"ActionListNoBrowse":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: ActionListNoBrowse","TopicPolicy":{"ActionListNoBrowse":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ActionListNoBrowse", FVT.verify404NotFound, verifyConfig, done);
        });
    
    });

//  ====================   MaxMessages Error test cases  ====================  //
    describe('Error MaxMessages:', function() {

        it('should return status 400 on POST  "MaxMessagesTooLow"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessagesTooLow":{"MaxMessages":0,"ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooLow" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessagesTooLow":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessagesTooLow","TopicPolicy":{"MaxMessagesTooLow":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesTooHigh"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessagesTooHigh":{"MaxMessages":20000001,"ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"20000001\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesTooHigh" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessagesTooHigh":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessagesTooHigh","TopicPolicy":{"MaxMessagesTooHigh":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });
// DestinationType being invalid must be after ActionList?    Not valid and duplicate?
        it('should return status 404 on post "MismatchMaxMsgsQueue"', function(done) {
            var payload = '{"TopicPolicy":{"MismatchMaxMsgsQueue":{"ActionList": "Send,Receive,Browse","DestinationType":"Queue","Topic":"'+ FVT.long1024String +'","MaxMessages":50000}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the TopicPolicy DestinationType is \"Queue\".." }' );
            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: The \"MaxMessages\" is not a valid argument because the TopicPolicy DestinationType is \"Queue\".." } 
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Send,Receive,Browse\"." } 
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after fail "MismatchMaxMsgsQueue)"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MismatchMaxMsgsQueue":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MismatchMaxMsgsQueue","TopicPolicy":{"MismatchMaxMsgsQueue":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MismatchMaxMsgsQueue", FVT.verify404NotFound, verifyConfig, done);
        });

    });

//  ====================   MaxMessagesBehavior Error test cases  ====================  //
    describe('Error MaxMessagesBehavior:', function() {

        it('should return status 400 on POST  "MaxMessageBehavior":"TooLow"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessageBehaviorTooLow":{"MaxMessageBehavior":"TooLow","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MaxMessageBehavior." }' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
//            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: TopicPolicy Name: MaxMessageBehaviorTooLow Property: MaxMessageBehavior" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageBehaviorTooLow" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessageBehaviorTooLow":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessageBehaviorTooLow","TopicPolicy":{"MaxMessageBehaviorTooLow":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessagesBehaviorTooLow"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessagesBehaviorTooLow":{"MaxMessagesBehavior":"TooLow","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessagesBehavior Value: \"TooLow\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessagesBehaviorTooLow" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessagesBehaviorTooLow":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessagesBehaviorTooLow","TopicPolicy":{"MaxMessagesBehaviorTooLow":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessagesBehaviorTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

    });

//  ====================   MaxMessageTimeToLive Error test cases  ====================  //
    describe('Error MaxMessageTimeToLive:', function() {
//98282
        it('should return status 400 on POST  "MaxMessageTimeToLiveTooLow"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessageTimeToLiveTooLow":{"MaxMessageTimeToLive":"0","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"0\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooLow" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessageTimeToLiveTooLow":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessageTimeToLiveTooLow","TopicPolicy":{"MaxMessageTimeToLiveTooLow":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooLow", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveTooHigh"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessageTimeToLiveTooHigh":{"MaxMessageTimeToLive":"2147483648","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"2147483648\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveTooHigh" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessageTimeToLiveTooHigh","TopicPolicy":{"MaxMessageTimeToLiveTooHigh":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveTooHigh", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST  "MaxMessageTimeToLiveLimited"', function(done) {
            var payload = '{"TopicPolicy":{"MaxMessageTimeToLiveLimited":{"MaxMessageTimeToLive":"Limited","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageTimeToLive Value: \"Limited\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "MaxMessageTimeToLiveLimited" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"MaxMessageTimeToLiveLimited":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: MaxMessageTimeToLiveLimited","TopicPolicy":{"MaxMessageTimeToLiveLimited":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"MaxMessageTimeToLiveLimited", FVT.verify404NotFound, verifyConfig, done);
        });

    });

//  ====================   DisconnectedClientNotification Error test cases  ====================  //
    describe('Error DisconnectedClientNotification:', function() {
// 100007
        it('should return status 400 on POST  "DisconnectedClientNotificationInvalid"', function(done) {
            var payload = '{"TopicPolicy":{"DisconnectedClientNotificationInvalid":{"DisconnectedClientNotification":"True","ClientID": "*","ActionList": "Publish,Subscribe","Topic": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: TopicPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Value: JSON_STRING." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: TopicPolicy Name: DisconnectedClientNotificationInvalid Property: DisconnectedClientNotification Type: JSON_STRING" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "DisconnectedClientNotificationInvalid" did not create', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"DisconnectedClientNotificationInvalid":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: DisconnectedClientNotificationInvalid","TopicPolicy":{"DisconnectedClientNotificationInvalid":{}}}' );
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
//            verifyConfig = JSON.parse( '{"status":404,"QueuePolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueuePolicy Name: '+ FVT.long256Name + '","QueuePolicy":{"'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueuePolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on Delete Subscription Policy', function(done) {
            verifyConfig = JSON.parse( '{"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SubscriptionPolicy/' + FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Subscription Policy gone', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"SubscriptionPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SubscriptionPolicy Name: '+ FVT.long256Name + '","SubscriptionPolicy":{"'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SubscriptionPolicy/'+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on re-Delete Topic Policy', function(done) {
            verifyConfig = JSON.parse( '{"TopicPolicy":{"'+ FVT.long256Name + '":null}}' ) ;
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject + FVT.long256Name, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get all after delete Topic Policy gone', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TopicPolicy":{"T'+ FVT.long256Name + '":null}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TopicPolicy Name: '+ FVT.long256Name + '","TopicPolicy":{"'+ FVT.long256Name + '":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
});
