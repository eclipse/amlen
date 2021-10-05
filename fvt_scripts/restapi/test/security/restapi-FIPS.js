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

var uriObject = 'FIPS/';
var expectDefault = '{"FIPS":false,"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// FIPS test cases
describe('FIPS', function() {
this.timeout( FVT.defaultTimeout );

    // FIPS Get test cases
    describe('Get in Pristine', function() {
	
        it('should return status 200 on get, DEFAULT of "FIPS"(F)', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // FIPS Update test cases
    describe('Update Valid Range', function() {
	
        it('should return status 200 on post "FIPS":true', function(done) {
            var payload = '{"FIPS":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":"true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "FIPS":false', function(done) {
            var payload = '{"FIPS":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "FIPS":true', function(done) {
            var payload = '{"FIPS":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":"true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "FIPS":false', function(done) {
            var payload = '{"FIPS":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // FIPS Delete test cases
    describe('Delete', function() {
	
        it('should return status 403 when deleting FIPS', function(done) {
            verifyConfig = {"FIPS":false} ;
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: FIPS"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // FIPS Error test cases
    describe('Error', function() {

        it('should return status 400 "FIPS":""', function(done) {
            var payload = '{"FIPS":""}';
            verifyConfig = {"FIPS":false} ;
//			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: \"\"."};
//			verifyMessage = {"status":400, "Code":"CWLNA0128","Message":"The name of a named configuration object is not valid: (null)."};
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: \"\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: FIPS Property: FIPS Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after POST "FIPS":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to NORMAL', function(done) {
            var payload = '{"FIPS":"NORMAL"}';
            verifyConfig = {"FIPS":false} ;
//			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: NORMAL."};
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: \"NORMAL\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: FIPS Property: FIPS Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to "0"', function(done) {
            var payload = '{"FIPS":"0"}';
            verifyConfig = {"FIPS":false} ;
//			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: 0."};
//			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: \"0\"."};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: FIPS Property: FIPS Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: FIPS Property: FIPS Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 1', function(done) {
            var payload = '{"FIPS":1}';
            verifyConfig = {"FIPS":false} ;
//			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: 1."};
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: FIPS Value: \"1\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: FIPS Property: FIPS Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "FIPS":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});
	
    // FIPS Reset to Default
    describe('Reset to Default', function() {
	
        it('should return status 200 on post "FIPS":true', function(done) {
            var payload = '{"FIPS":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":"true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    // Config persists Check
	
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
        it('should return status 200 on get, "FIPS" persists', function(done) {
            verifyConfig = {"FIPS":true} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		

// 107769
        it('should return status 200 on post "FIPS":null(Default=F)', function(done) {
            var payload = '{"FIPS":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "FIPS":null (false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
