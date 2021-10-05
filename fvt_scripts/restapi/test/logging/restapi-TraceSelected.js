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

var uriObject = 'TraceSelected/';
var expectDefault = '{"TraceSelected":"","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here
//  FOR Valid Filters, see:  server_utils/include/tracecomponents.h
// TraceSelected test cases
describe('TraceSelected:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceSelected Get test cases
    describe('Pristine Get:', function() {
// 109524
        it('should return status 200 on get, DEFAULT of "TraceSelected":""', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    // TraceSelected Update test cases
    describe('Update Valid Range:', function() {
    
        it('should return status 200 on post "TraceSelected":"6,Mqtt:9"', function(done) {
            var payload = '{"TraceSelected":"6,Mqtt:9"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":"6,Mqtt:9"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on post "TraceSelected":"1,Mqtt:9,Jms:8,Engine:7"', function(done) {
            var payload = '{"TraceSelected":"1,Mqtt:9,Jms:8,Engine:7"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":"1,Mqtt:9,Jms:8,Engine:7"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when trying to set alternate delimiters', function(done) {
            var payload = '{"TraceSelected":"2,Mqtt=1,Jms=2,Engine=3"}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":"2,Mqtt=1,Jms=2,Engine=3"', function(done) {
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
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, {"TraceSelected":"2,Mqtt=1,Jms=2,Engine=3"} persists', function(done) {
            verifyConfig = {"TraceSelected":"2,Mqtt=1,Jms=2,Engine=3"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To EmptyString:', function() {
        it('should return status 200 on post "TraceSelected":""(ok now, not DELETE)', function(done) {
            var payload = '{"TraceSelected":""}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceSelected":"2,Transport:1"', function(done) {
            var payload = '{"TraceSelected":"2,Transport:1"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });


    // TraceSelected Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting TraceSelected', function(done) {
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceSelected"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceSelected":"Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    // TraceSelected Error test cases
    describe('Error:', function() {

        it('should return status 400 when trying to set to ABNORMAL', function(done) {
            var payload = '{"TraceSelected":"ABNORMAL"}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: \"ABNORMAL\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set a SEMI-COLON Delimited string', function(done) {
            var payload = '{"TraceSelected":"Mqtt:9;Jms:8;Engine:7"}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: \"Mqtt:9;Jms:8;Engine:7\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set without COLON #s', function(done) {
            var payload = '{"TraceSelected":"Mqtt,Jms,Engine"}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: \"Mqtt,Jms,Engine\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to lowercase names', function(done) {
            var payload = '{"TraceSelected":"mqtt:1,jms:2,engine:3"}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: \"mqtt:1,jms:2,engine:3\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set invalid syntax, no overall Level', function(done) {
            var payload = '{"TraceSelected":"Mqtt=1,Jms=2,Engine=3"}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: \"Mqtt=1,Jms=2,Engine=3\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set as NUMBER', function(done) {
            var payload = '{"TraceSelected":5}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: JSON_INTEGER."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceSelected Property: TraceSelected Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set as BOOLEAN', function(done) {
            var payload = '{"TraceSelected":false}';
            verifyConfig = {"TraceSelected":"2,Transport:1"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceSelected Value: JSON_BOOLEAN"};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceSelected Property: TraceSelected Type: InvalidType:JSON_BOOLEAN"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceSelected Property: TraceSelected Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceSelected":"2,Transport:1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    // TraceSelected Reset to Default
    describe('Reset to Default:', function() {
        it('should return status 200 on post "TraceSelected":null(Default)', function(done) {
            var payload = '{"TraceSelected":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceSelected":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
