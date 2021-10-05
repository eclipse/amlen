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

var uriObject = 'ServerName/';

var expectDefault = {};
var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// ServerName test cases
describe('ServerName', function() {
this.timeout( FVT.defaultTimeout );

    // ServerName Get test cases
    describe('Pristine Get:', function() {
	    expectDefault = '{"ServerName":"'+ FVT.A1_SERVERNAME +':'+ FVT.A1_PORT +'","Version":"'+ FVT.version +'"}' ;
		
        it('should return status 200 on get, DEFAULT of "ServerName"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // ServerName Update test cases
    describe('Update Valid Range:', function() {
	
        it('should return status 200 on post "ServerName":Max Length Name', function(done) {
            var payload = '{"ServerName":"'+ FVT.long1024String +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ServerName":"Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // Config persists Check
    describe('Restart Persists:', function() {
	
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
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "ServerName":"Max Length Name" persists', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

    // ServerName Delete test cases
    describe('Delete:', function() {
	
        it('should return status 403 when deleting ServerName', function(done) {
            var payload = '{"ServerName":"'+ FVT.long1024String +'"}';
            verifyConfig = JSON.parse( payload ) ;
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: ServerName"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ServerName":"Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// Not empty String anymore, not Delete anymore, now it is ALSO a RESET to Default!
// DEFECT 134292 - no longer valid
        it('should return status 400 on POST "ServerName":""(NOT RESET to Default)', function(done) {
            var payload = '{"ServerName":""}';
            verifyConfig = JSON.parse( '{"ServerName":"'+ FVT.long1024String +'"}' ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ServerName Value: \"\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ServerName" after POST to ""(NOT RESET to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 on POST "ServerName":"hostname:port"(Manually RESET to Default)', function(done) {
            var payload = '{"ServerName":"'+ FVT.A1_SERVERNAME +':'+ FVT.A1_PORT +'"}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ServerName" after POST to "hostname:port"(Manually RESET to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // ServerName Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to Name too long', function(done) {
            var payload = '{"ServerName":"'+ FVT.long1024String + 5 +'"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = JSON.parse( '{"Code":"CWLNA0112","Message":"A property value is not valid: Property: ServerName Value: '+ FVT.long1024String +'5."}' );
//			verifyMessage = JSON.parse( '{"Code":"CWLNA0112","Message":"The property value is not valid: Property: ServerName Value: \\\"'+ FVT.long1024String +'5\\\"."}' );
//          verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The length of the value of the configuration object is too long. Object: ServerName Property: ServerName Value: '+ FVT.long1024String + 5 +'."}' );
//            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object:  Property: ServerName Value: '+ FVT.long1024String + 5 +'."}' );
            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0146","Message":"The value that is specified for the property is too long. Property: ServerName Value: '+ FVT.long1024String + 5 +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on Get after POST fails set to Name too long', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});
	
    // ServerName Reset to Default
    describe('Reset to Default', function() {
	
        it('should return status 200 on post "ServerName":"True"', function(done) {
            var payload = '{"ServerName":"True"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ServerName":"True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ServerName":null(Default)', function(done) {
            var payload = '{"ServerName":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ServerName":null(Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});

});
