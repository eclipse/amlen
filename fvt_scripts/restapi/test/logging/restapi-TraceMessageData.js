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

var uriObject = 'TraceMessageData/';
var objectMin = 0
var objectMax = 65535
var objectDefault = 0
var expectDefault = '{"TraceMessageData":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};


// Test Cases Start Here

// TraceMessageData test cases
describe('TraceMessageData:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceMessageData Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "TraceMessageData":'+objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });

    // TraceMessageData Update test cases
    describe('Update Valid Range:', function() {
    
        it('should return status 200 on post "TraceMessageData":'+objectMax, function(done) {
            var payload = '{"TraceMessageData":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectMax, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
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
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "TraceMessageData":'+ objectMax +' persists', function(done) {
            verifyConfig = JSON.parse( '{"TraceMessageData":'+ objectMax +'}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Range Cont.:', function() {

	    it('should return status 200 on post "TraceMessageData":'+objectDefault, function(done) {
        	var payload = '{"TraceMessageData":'+ objectDefault +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on post "TraceMessageData":69', function(done) {
            var payload = '{"TraceMessageData":69}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMessageData":69', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMessageData":'+objectMin, function(done) {
            var payload = '{"TraceMessageData":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });


    // TraceMessageData Delete test cases
    describe('Delete:', function() {
        it('should return status 403 when deleting TraceMessageData', function(done) {
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceMessageData"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 on post "TraceMessageData":"" (now invalid)', function(done) {
            var payload = '{"TraceMessageData":""}';
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceMessageData Value: NULL."} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0118","Message":"The properties are not valid."} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMessageData Value: \"\"."} ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMessageData Property: TraceMessageData Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
    });

    // TraceMessageData Error test cases
    describe('Error:', function() {
        
        it('should return status 400 when trying to set to JUNK', function(done) {
            var payload = '{"TraceMessageData":"JUNK"}';
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceMessageData Value: JUNK."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMessageData Value: JUNK."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMessageData Property: TraceMessageData Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to "29", number but is a string', function(done) {
            var payload = '{"TraceMessageData":"29"}';
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceMessageData Value: 29."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMessageData Value: 29."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMessageData Property: TraceMessageData Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 65536, out of bounds above', function(done) {
            var payload = '{"TraceMessageData":65536}';
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceMessageData Value: 65536."};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMessageData Value: \"65536\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceMessageData":1.0}';
            verifyConfig = JSON.parse( '{"TraceMessageData":'+objectMin+'}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMessageData Value: 1.0."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMessageData Property: TraceMessageData Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceMessageData":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
    // TraceMessageData Reset to Default
    describe('Reset to Default:', function() {
    
        it('should return status 200 on post "TraceMessageData":null (DEFAULT)', function(done) {
            var payload = '{"TraceMessageData":null}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, reset to "TraceMessageData":'+objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

});
