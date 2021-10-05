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

var uriObject = 'TraceBackup/';
var objectMin = 0 ;
var objectDefault = 1;   // 115572
var objectMax = 2 ;
// https://w3-connections.ibm.com/wikis/home?lang=en-us#!/wiki/W11ac9778a1d4_4f61_913c_a76a78c7bff7/page/Trace%20backup%20and%20offloading
var expectDefault = '{"TraceBackup":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// TraceBackup test cases
describe('TraceBackup:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceBackup Get test cases
    describe('Pristine Get:', function() {
// 115572
        it('should return status 200 on get, DEFAULT of "TraceBackup":'+ objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceBackup Update test cases
    describe('Range[0-0-2]:', function() {
    
        it('should return status 200 on post "TraceBackup":1', function(done) {
            var payload = '{"TraceBackup":1}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackup":"1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceBackup":'+objectMin, function(done) {
            var payload = '{"TraceBackup":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackup":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceBackup":'+objectMax, function(done) {
            var payload = '{"TraceBackup":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackup":'+objectMax, function(done) {
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
        it('should return status 200 on get, "TraceBackup" persists', function(done) {
            verifyConfig = JSON.parse( '{"TraceBackup":'+ objectMax +'}' )  ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });


    // TraceBackup Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting TraceBackup', function(done) {
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceBackup"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 on post "TraceBackup":"" (invalid)', function(done) {
            var payload = '{"TraceBackup":""}';
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackup Property: TraceBackup Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceBackup":"" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
                
    });

    // TraceBackup Error test cases
    describe('Error:', function() {

        it('should return status 400 when trying to set to NORMAL', function(done) {
            var payload = '{"TraceBackup":"NORMAL"}';
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackup Value: NORMAL."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackup Property: TraceBackup Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to true', function(done) {
            var payload = '{"TraceBackup":true}';
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackup Value: JSON_BOOLEAN."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackup Property: TraceBackup Type: InvalidType:JSON_TRUE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to "0", but as a string', function(done) {
            var payload = '{"TraceBackup":"0"}';
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackup Property: TraceBackup Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to -1, out of bounds', function(done) {
            var payload = '{"TraceBackup":-1}';
            verifyConfig = {"TraceBackup":2} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackup Value: \"-1\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 3, out of bounds', function(done) {
            var payload = '{"TraceBackup":3}';
            verifyConfig = {"TraceBackup":2} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackup Value: \"3\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceBackup":1.0}';
            verifyConfig = {"TraceBackup":2} ;
//            verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: TraceBackup Value: \"1.0\"."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceBackup Property: TraceBackup Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackup":2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    // TraceBackup Reset to Default
    describe('Reset to Default:', function() {
// 115572
        it('should return status 200 on post "TraceBackup":null (Default='+ objectDefault +')', function(done) {
            var payload = '{"TraceBackup":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackup":null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
