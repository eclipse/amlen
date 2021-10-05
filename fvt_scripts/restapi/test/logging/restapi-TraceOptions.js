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

var uriObject = 'TraceOptions/';
var expectDefault = '{"TraceOptions":"time,thread,where","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here : valid options:  where, thread, append, time

// TraceOptions test cases
describe('TraceOptions:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceOptions Get test cases
    describe('Pristine Get:', function() {
// 109524
        it('should return status 200 on get, DEFAULT of "TraceOptions":""', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceOptions Update test cases
    describe('Update Valid Range:', function() {
    
        it('should return status 200 on post "TraceOptions":"where"', function(done) {
            var payload = '{"TraceOptions":"where"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"where"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceOptions":"thread"', function(done) {
            var payload = '{"TraceOptions":"thread"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"thread"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceOptions":"append"', function(done) {
            var payload = '{"TraceOptions":"append"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"append"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceOptions":"time"', function(done) {
            var payload = '{"TraceOptions":"time"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"time""time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceOptions":"where,time"', function(done) {
            var payload = '{"TraceOptions":"where,time"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"where,time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceOptions":"thread,append,where"', function(done) {
            var payload = '{"TraceOptions":"thread,append,where"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"thread,append,where"', function(done) {
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
        it('should return status 200 on get, {"TraceOptions":"thread,append,where"} persists', function(done) {
            verifyConfig = {"TraceOptions":"thread,append,where"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To EmptyString:', function() {
        it('should return status 200 on post "TraceOptions":""(ok now, just not DELETE anymore)', function(done) {
            var payload = '{"TraceOptions":""}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceOptions":"time"', function(done) {
            var payload = '{"TraceOptions":"time"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // TraceOptions Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting TraceOptions', function(done) {
            verifyConfig = {"TraceOptions":"time"} ;
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceOptions"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceOptions Error test cases
    describe('Error:', function() {

        it('should return status 400 when trying to set to NOTNORMAL', function(done) {
            var payload = '{"TraceOptions":"NOTNORMAL"}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceOptions Value: NOTNORMAL."};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: \"NOTNORMAL\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to timeless', function(done) {
            var payload = '{"TraceOptions":"timeless"}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: timeless."};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: \"timeless\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 0, but not as a string', function(done) {
            var payload = '{"TraceOptions":0}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceOptions Value: 0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: 0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceOptions Property: InvalidType:JSON_INTEGER Value: 0"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceOptions Property: TraceOptions Type: InvalidType:JSON_INTEGER"};		
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to -1, out of bounds', function(done) {
            var payload = '{"TraceOptions":-1}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceOptions Value: -1."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: -1."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceOptions Property: InvalidType:JSON_INTEGER Value: -1"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceOptions Property: TraceOptions Type: InvalidType:JSON_INTEGER"};		
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 3, Number out of bounds', function(done) {
            var payload = '{"TraceOptions":3}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceOptions Value: 3."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: 3."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceOptions Property: InvalidType:JSON_INTEGER Value: 3"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceOptions Property: TraceOptions Type: InvalidType:JSON_INTEGER"};		
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceOptions":1.0}';
            verifyConfig = {"TraceOptions":"time"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceOptions Value: 1.0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceOptions Value: 1.0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceOptions Property: InvalidType:JSON_INTEGER Value: 1.0"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceOptions Property: TraceOptions Type: InvalidType:JSON_REAL"};		
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceOptions":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    // TraceOptions Reset to Default
    describe('Reset to Default', function() {
        it('should return status 200 on post "TraceOptions":null(Default)', function(done) {
            var payload = '{"TraceOptions":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceOptions":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
