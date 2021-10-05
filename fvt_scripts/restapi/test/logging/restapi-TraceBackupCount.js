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

var uriObject = 'TraceBackupCount/';
var objectMin = 1;
var objectDefault = 3;
var objectMax = 100;
// https://w3-connections.ibm.com/wikis/home?lang=en-us#!/wiki/W11ac9778a1d4_4f61_913c_a76a78c7bff7/page/Trace%20backup%20and%20offloading
var expectDefault = '{"TraceBackupCount":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// TraceBackupCount test cases
describe('TraceBackupCount', function() {
this.timeout( FVT.defaultTimeout );

    // TraceBackupCount Get test cases
    describe('Get in Pristine', function() {
	
        it('should return status 200 on get, DEFAULT of "TraceBackupCount":3', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // TraceBackupCount Update test cases
    describe('Range[1-3-100]', function() {
	
        it('should return status 200 on post "TraceBackupCount":50', function(done) {
            var payload = '{"TraceBackupCount":50}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupCount":50', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	
        it('should return status 200 on post "TraceBackupCount":1 (MIN)', function(done) {
            var payload = '{"TraceBackupCount":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupCount":"1 (MIN)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TraceBackupCount":3 (Default)', function(done) {
            var payload = '{"TraceBackupCount":'+ objectDefault +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupCount":3 (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceBackupCount":100 (Max)', function(done) {
            var payload = '{"TraceBackupCount":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupCount":100 (Max)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
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
        it('should return status 200 on get, "TraceBackupCount" persists', function(done) {
            verifyConfig = JSON.parse( '{"TraceBackupCount":'+ objectMax +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
				
	});


    // TraceBackupCount Delete test cases
    describe('Delete', function() {
	
        it('should return status 403 when deleting TraceBackupCount', function(done) {
            verifyConfig = {"TraceBackupCount":100} ;
//			verifyMessage = {"status":403,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceBackupCount"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // TraceBackupCount Error test cases
    describe('Error', function() {

    	it('should return status 400 when trying to set to JUNK', function(done) {
            var payload = '{"TraceBackupCount":"JUNK"}';
            verifyConfig = {"TraceBackupCount":100} ;
//			verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackupCount Value: JUNK."};
			verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackupCount Property: TraceBackupCount Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to "69", number but is a string', function(done) {
            var payload = '{"TraceBackupCount":"69"}';
            verifyConfig = {"TraceBackupCount":100} ;
//			verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackupCount Value: 69."};
			verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackupCount Property: TraceBackupCount Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 0, out of bounds below', function(done) {
            var payload = '{"TraceBackupCount":0}';
            verifyConfig = {"TraceBackupCount":100} ;
			verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupCount Value: \"0\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 301, out of bounds above', function(done) {
            var payload = '{"TraceBackupCount":301}';
            verifyConfig = {"TraceBackupCount":100} ;
			verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupCount Value: \"301\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceBackupCount":1.0}';
            verifyConfig = {"TraceBackupCount":100} ;
//			verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackupCount Value: 1.0."};
			verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackupCount Property: TraceBackupCount Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceBackupCount":100', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 400 on post "TraceBackupCount":"" (invalid)', function(done) {
            var payload = '{"TraceBackupCount":""}';
            verifyConfig = {"TraceBackupCount":100} ;
			verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackupCount Property: TraceBackupCount Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceBackupCount":"" (invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});
	
    // TraceBackupCount Reset to Default
    describe('Reset to Default:'+ objectDefault, function() {
	
        it('should return status 200 on post "TraceBackupCount":null (DEFAULT)', function(done) {
            var payload = '{"TraceBackupCount":null}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupCount":null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
