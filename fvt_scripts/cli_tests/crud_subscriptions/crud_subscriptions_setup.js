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

var msgHubName = "SubscriptionCRUDHub" ;
var msgHub = '"MessageHub":{"'+ msgHubName +'":{"Description":"Subscription hub for create update delete tests"}}' ;
var connPolicyName = "CRUDSubscriptionConnectionPolicy" ;
var connPolicy = '"ConnectionPolicy":{"'+ connPolicyName +'":{"Protocol":"JMS","Description":"Subscription connection policy for create update delete tests"}}' ;
var msgPolicySubscrName = "CRUDSubscriptionMessagingPolicy" ; 
var msgPolicySubscr = '"MessagingPolicy":{"'+ msgPolicySubscrName +'":{"Destination":"CRUDSubscription1","DestinationType":"Subscription","ActionList":"Receive","Protocol":"JMS","Description":"Subscription messaging policy for create update delete tests","MaxMessages":5}}' ;
var msgPolicyTopicName = "CRUDTopicMessagingPolicy" ;
var msgPolicyTopic = '"MessagingPolicy":{"'+ msgPolicyTopicName +'":{"Destination":"CRUDTopic","DestinationType":"Topic","ActionList":"Subscribe","Protocol":"JMS","Description":"Topic messaging policy for subscription for create update delete tests","MaxMessages":3}}' ;
var endpointName = "CRUDSubscriptionEndpoint" ;
var endpoint = '"Endpoint":{"'+ endpointName +'":{"Enabled":true,"Port":28412,"Protocol":"JMS","ConnectionPolicies":"CRUDSubscriptionConnectionPolicy","MessagingPolicies":"CRUDSubscriptionMessagingPolicy,CRUDTopicMessagingPolicy","MessageHub":"SubscriptionCRUDHub","Port":18412,"MaxMessageSize":"4MB" }}' ;
// BOOLEAN is getting changed to string in interium Admin code
var endpoint = '"Endpoint":{"'+ endpointName +'":{"Enabled":"true","Port":28412,"Protocol":"JMS","ConnectionPolicies":"CRUDSubscriptionConnectionPolicy","MessagingPolicies":"CRUDSubscriptionMessagingPolicy,CRUDTopicMessagingPolicy","MessageHub":"SubscriptionCRUDHub","Port":18412,"MaxMessageSize":"4MB" }}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// CRUD_Subscriptions test cases
describe('CRUD_Subscriptions:', function() {

    describe('Create:', function() {
    
        it('should return status 200 on POST', function(done) {
            var payload = '{'+ msgHub +','+ connPolicy +','+ msgPolicySubscr +','+ msgPolicyTopic +','+ endpoint +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on GET, "'+ msgHubName +'"', function(done) {
		    verifyConfig = JSON.parse( '{'+ msgHub +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessageHub/"+msgHubName, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ connPolicyName +'"', function(done) {
		    verifyConfig = JSON.parse( '{'+ connPolicy +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"ConnectionPolicy/"+connPolicyName, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ msgPolicySubscrName +'"', function(done) {
		    verifyConfig = JSON.parse( '{'+ msgPolicySubscr +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessagingPolicy/"+msgPolicySubscrName, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ msgPolicyTopicName +'"', function(done) {
		    verifyConfig = JSON.parse( '{'+ msgPolicyTopic +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessagingPolicy/"+msgPolicyTopicName, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ endpointName +'"', function(done) {
		    verifyConfig = JSON.parse( '{'+ endpoint +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Endpoint/"+endpointName, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
