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

var uriObject = 'AdminUserID/';
var expectDefault = '{"AdminUserID":"admin","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// AdminUserID test cases
describe('AdminUserID:', function() {
this.timeout( FVT.defaultTimeout );

    // AdminUserID Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "AdminUserID"(admin)', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // AdminUserID Update test cases
    describe('Update Valid Range', function() {
	
        it('should return status 200 on post "AdminUserID":"1234567890123456" all numeric', function(done) {
            var payload = '{"AdminUserID":"1234567890123456"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AdminUserID":"1234567890123456"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "AdminUserID":Max Length Name', function(done) {
            var payload = '{"AdminUserID":"'+ FVT.long16Name +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AdminUserID":"Max Length Name"', function(done) {
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
        it('should return status 200 on get, "AdminUserID":"Max Length Name" persists', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // AdminUserID Delete test cases
    describe('Delete', function() {
	
        it('should return status 403 when deleting AdminUserID', function(done) {
            verifyConfig = JSON.parse( '{"AdminUserID":"'+ FVT.long16Name +'"}' ) ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: AdminUserID"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "AdminUserID":"Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 on post "AdminUserID":""(invalid now, not DELETE)', function(done) {
            var payload = '{"AdminUserID":""}';
            verifyConfig = JSON.parse( '{"AdminUserID":"'+ FVT.long16Name +'"}' ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminUserID Value: \"NULL\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "AdminUserID":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // AdminUserID Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to Name too long', function(done) {
            var payload = '{"AdminUserID":"'+ FVT.long16Name + 7 +'"}';
            verifyConfig = JSON.parse( '{"AdminUserID":"'+ FVT.long16Name +'"}' ) ;
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminUserID Value: \\\"'+ FVT.long16Name + 7 +'\\\"."}' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The length of the value of the configuration object is too long. Object: AdminUserID Property: AdminUserID Value: \\\"'+ FVT.long16Name + 7 +'\\\"."}' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object:  Property: AdminUserID Value: '+ FVT.long16Name + 7 +'."}' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0146","Message":"The value that is specified for the property is too long. Property: AdminUserID Value: '+ FVT.long16Name + 7 +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "AdminUserID":"Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to BOOLEAN', function(done) {
            var payload = '{"AdminUserID":true}';
            verifyConfig = JSON.parse( '{"AdminUserID":"'+ FVT.long16Name +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminUserID Property: AdminUserID Type: InvalidType:JSON_BOOLEAN"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminUserID Property: AdminUserID Type: InvalidType:JSON_TRUE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST to BOOLEAN fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to INTEGER', function(done) {
            var payload = '{"AdminUserID":666}';
            verifyConfig = JSON.parse( '{"AdminUserID":"'+ FVT.long16Name +'"}' ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminUserID Property: AdminUserID Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST to INTEGER fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});
	
    // AdminUserID Reset to Default
    describe('Reset to Default', function() {
	
        it('should return status 200 on post "AdminUserID":"True"', function(done) {
            var payload = '{"AdminUserID":"True"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AdminUserID":"True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "AdminUserID":null(Default)', function(done) {
            var payload = '{"AdminUserID":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET when POST DEFAULT, "AdminUserID":"admin"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//failsafe if reset fails....
        it('should return status 200 on post "AdminUserID":"admin"', function(done) {
            var payload = '{"AdminUserID":"admin"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AdminUserID":"admin"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
