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

var uriObject = 'status/';

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here
function getDynamicFieldsCallback( err, res, done ){
    FVT.trace( 0, "@getDynamicFieldsCallback RESPONSE:"+ JSON.stringify( res, null, 2 ) );
	jsonResponse = JSON.parse( res.text );
	expectJSON = JSON.parse( FVT.expectDefaultStatus );
    thisName = jsonResponse.Server.Name ;
    thisVersion = jsonResponse.Server.Version ;
	FVT.trace( 1, 'The Server Name: '+ thisName +' and the Version is: '+ thisVersion );
	expectJSON.Server[ "Name" ] = thisName ;
	expectJSON.Server[ "Version" ] = thisVersion ;
	FVT.expectDefaultStatus = JSON.stringify( expectJSON, null, 0 );

//	console.log( "PARSE: req" );
	res.req.method.should.equal('GET');
//	res.req.url.should.equal( 'http://' + FVT.IMA_AdminEndpoint +'ima/v1/service/Status/');

	FVT.trace( 0, "NEW FVT.expectDefaultStatus:"+ JSON.stringify( FVT.expectDefaultStatus, null, 2 ) );
	res.statusCode.should.equal(200);
	FVT.getSuccessCallback( err, res, done )

	}

// Service - Reset Config test cases
describe('Service-ResetConfig:', function() {
this.timeout( FVT.defaultTimeout );

    // Status Get test cases
    describe('Pristine Get Status:', function() {
	
        it('should return 200 on get, DEFAULT of "Status"', function(done) {
            verifyConfig = JSON.parse( FVT.expectServerDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, getDynamicFieldsCallback, verifyConfig, done)
        });

    });

    // Status Update test cases
    describe('ResetConfig:', function() {

        it('should return 200 POST "Reset:config"', function(done) {
            var payload = '{ "Service":"Server","Reset":"config" }';
			var expectNewJSON = JSON.parse( FVT.expectDefaultStatus );
			expectNewJSON.Server[ "Name" ] = FVT.A1_SERVERNAME +":"+ FVT.A1_PORT ;
			FVT.expectDefaultStatus = JSON.stringify( expectNewJSON, null, 0 );
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET "Status" after restart', function(done) {
            this.timeout( FVT.REBOOT + 5000 );
            FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Cluster Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectClusterDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Cluster", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service HighAvailability Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectHighAvailabilityDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/HighAvailability", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service MQConnectivity Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Plugin Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Plugin", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service SNMP Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectSNMPDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/SNMP", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET Service Server Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectServerDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done)
        });

    });

});
