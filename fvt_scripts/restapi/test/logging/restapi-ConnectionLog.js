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

var uriObject = 'ConnectionLog/';
var objectMin = 'MIN' ;
var objectDefault = 'NORMAL';
var objectMax = 'MAX' ;
var expectDefault = '{"ConnectionLog":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// ConnectionLog test cases
describe('ConnectionLog:', function() {
this.timeout( FVT.defaultTimeout );

    // ConnectionLog Get test cases
    describe('Pristine Get:', function() {
    
        it('should return status 200 on get, DEFAULT of "ConnectionLog":"NORMAL"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ConnectionLog Update test cases
    describe('[MIN-NORMAL-MAX]:', function() {
    
        it('should return status 200 on post "ConnectionLog":"MAX"', function(done) {
            var payload = '{"ConnectionLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//98121
        it('should return status 200 on post "ConnectionLog":null(DEFAULT)', function(done) {
            var payload = '{"ConnectionLog":null}';
            verifyConfig = {"ConnectionLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "ConnectionLog":null(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ConnectionLog":"MAX"', function(done) {
            var payload = '{"ConnectionLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MAX"', function(done) {
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
        it('should return status 200 on get, "ConnectionLog":"MAX" persists', function(done) {
            verifyConfig = {"ConnectionLog":"MAX"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To Default:', function() {
    
        it('should return status 200 on post "ConnectionLog":"NORMAL"', function(done) {
            var payload = '{"ConnectionLog":"NORMAL"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConnectionLog":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "ConnectionLog":"MIN"', function(done) {
            var payload = '{"ConnectionLog":"MIN"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    // ConnectionLog Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting ConnectionLog', function(done) {
            verifyConfig = {"ConnectionLog":"MIN"} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: ConnectionLog"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//98121
        it('should return status 400 on post "ConnectionLog":"" (No DELETE)', function(done) {
            var payload = '{"ConnectionLog":""}';
            verifyConfig = {"ConnectionLog":"MIN"} ;
// not 0337?            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: ConnectionLog Property: InvalidType:JSON_NULL Value: \"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ConnectionLog Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "ConnectionLog":"" (No DELETE)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // ConnectionLog Error test cases
    describe('Error:', function() {
// defect 96932, 97055, 101954
        it('should return status 400 on post  with "ConnectionLog" in URI', function(done) {
            var payload = '{"ConnectionLog":"MAX"}';
            verifyConfig = {"ConnectionLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/ConnectionLog is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "ConnectionLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "ConnectionLog":"JUNK"', function(done) {
            var payload = '{"ConnectionLog":"JUNK"}';
            verifyConfig = {"ConnectionLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: ConnectionLog Value: \"JUNK\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "ConnectionLog":"min"', function(done) {
            var payload = '{"ConnectionLog":"normal"}';
            verifyConfig = {"ConnectionLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: ConnectionLog Value: \"normal\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ConnectionLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // ConnectionLog Reset to Default
    describe('Reset to Default:', function() {
    
        it('should return status 200 on post "ConnectionLog":"NORMAL" ', function(done) {
            var payload = '{"ConnectionLog":"NORMAL"}';
            verifyConfig = {"ConnectionLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "ConnectionLog":"NORMAL"(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
