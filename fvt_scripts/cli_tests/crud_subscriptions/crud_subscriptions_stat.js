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
var subscriptName = 'CRUDSubscription1' ;

var deleteConfig = '{"MessageHub":{"'+ msgHubName +'":null},"ConnectionPolicy":{"'+ connPolicyName +'":null},"MessagingPolicy":{"'+ msgPolicySubscrName +'":null},"MessagingPolicy":{"'+ msgPolicyTopicName +'":null},"Endpoint":{"'+ endpointName +'":null}}' ;
var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// CRUD_Subscriptions test cases
describe('CRUD_Subscriptions:', function() {
	
    describe('STAT:', function() {
    
        it('should return status 200 on GET Stats', function(done) {
            verifyConfig = JSON.parse( '{"Subscription": []}' ) ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+'Subscription/'+subscriptName, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
});
