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
var objectDefault = '' ;
var uriObject = 'ClientSet/' ;

var expectDefaultSubscription = '{ "Subscription": [], "Version": "'+ FVT.version +'" }' ;
var expectDefaultMQTTClient = '{ "MQTTClient": [], "Version": "'+ FVT.version +'" }' ;


var verifyConfig = {};
var verifyMessage = {};
//  ====================  Test Cases Start Here  ====================  //

describe('ClientSet_Cleanup:', function() {
this.timeout( FVT.defaultTimeout );

        it('should return 200 on DELETE ClientSet to Delete All on A1', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on DELETE ClientSet to Delete All on A2', function(done) {
            verifyConfig = '' ;
            verifyMessage = { "Status":200, "Code":"CWLNA6197"};
            FVT.makeDeleteRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriServiceDomain + "ClientSet/?Retain=^&ClientID=^", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });


        it('should return 200 on GET MQTTClient after Delete All on A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All on A1', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.IMA_AdminEndpoint , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET MQTTClient after Delete All on A2', function(done) {
            verifyConfig = JSON.parse( expectDefaultMQTTClient );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "MQTTClient", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET Subscription after Delete All on A2', function(done) {
            verifyConfig = JSON.parse( expectDefaultSubscription );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.A2_IMA_AdminEndpoint , FVT.uriMonitorDomain + "Subscription", FVT.getSuccessCallback, verifyConfig, done);
        });


});
