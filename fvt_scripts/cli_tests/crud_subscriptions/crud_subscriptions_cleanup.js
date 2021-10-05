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
var connPolicyName = "CRUDSubscriptionConnectionPolicy" ;
var msgPolicySubscrName = "CRUDSubscriptionMessagingPolicy" ; 
var msgPolicyTopicName = "CRUDTopicMessagingPolicy" ;
var endpointName = "CRUDSubscriptionEndpoint" ;

//var deleteConfig = '{"MessageHub":{"'+ msgHubName +'":null},"ConnectionPolicy":{"'+ connPolicyName +'":null},"MessagingPolicy":{"'+ msgPolicySubscrName +'":null},"MessagingPolicy":{"'+ msgPolicyTopicName +'":null},"Endpoint":{"'+ endpointName +'":null}}' ;
//var deleteConfig = '{"Endpoint":{"'+ endpointName +'":null},"MessagingPolicy":{"'+ msgPolicySubscrName +'":null},"MessagingPolicy":{"'+ msgPolicyTopicName +'":null},"ConnectionPolicy":{"'+ connPolicyName +'":null},"MessageHub":{"'+ msgHubName +'":null}}' ;
var deleteConfig = '{"Endpoint":{"'+ endpointName +'":null},"MessagingPolicy":{"'+ msgPolicySubscrName +'":null,"'+ msgPolicyTopicName +'":null},"ConnectionPolicy":{"'+ connPolicyName +'":null},"MessageHub":{"'+ msgHubName +'":null}}' ;
var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// CRUD_Subscriptions test cases
describe('CRUD_Subscriptions:', function() {
	
    describe('Delete:', function() {
    
        it('should return status 200 on POST DELETE', function(done) {
            var payload =  deleteConfig ;
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 404 on GET, "'+ msgHubName +'"', function(done) {
            verifyConfig = {"status":404}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessageHub/"+msgHubName, FVT.verify404NotFound, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ connPolicyName +'"', function(done) {
            verifyConfig = {"status":404}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+"ConnectionPolicy/"+connPolicyName, FVT.verify404NotFound, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ msgPolicySubscrName +'"', function(done) {
            verifyConfig = {"status":404}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessagingPolicy/"+msgPolicySubscrName, FVT.verify404NotFound, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ msgPolicyTopicName +'"', function(done) {
            verifyConfig = {"status":404}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+"MessagingPolicy/"+msgPolicyTopicName, FVT.verify404NotFound, verifyConfig, done)
        });

        it('should return status 200 on GET, "'+ endpointName +'"', function(done) {
            verifyConfig = {"status":404}; 
            FVT.makeGetRequest( FVT.uriConfigDomain+"Endpoint/"+endpointName, FVT.verify404NotFound, verifyConfig, done)
        });
        
    });
});
