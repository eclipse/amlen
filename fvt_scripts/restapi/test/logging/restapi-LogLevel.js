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

var uriObject = 'LogLevel/';
var objectMin = 'MIN' ;
var objectDefault = 'NORMAL';
var objectMax = 'MAX' ;
var expectDefault = '{"LogLevel":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// LogLevel test cases
describe('LogLevel:', function() {
this.timeout( FVT.defaultTimeout );

    // LogLevel Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "LogLevel":"NORMAL"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // LogLevel Update test cases
    describe('Update Valid Range:', function() {
    
        it('should return status 200 on post "LogLevel":"MAX"', function(done) {
            var payload = '{"LogLevel":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "LogLevel":"MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 98121
        it('should return status 400 on post "LogLevel":NULL (DEFAULT)', function(done) {
            var payload = '{"LogLevel":null}';
            verifyConfig = {"LogLevel":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "LogLevel":NULL (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "LogLevel":"MAX"', function(done) {
            var payload = '{"LogLevel":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "LogLevel":"MAX"', function(done) {
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
        it('should return status 200 on get, "LogLevel":"MAX" persists', function(done) {
            verifyConfig = {"LogLevel":"MAX"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To Default:', function() {

        it('should return status 200 on post "LogLevel":"NORMAL"', function(done) {
            var payload = '{"LogLevel":"NORMAL"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "LogLevel":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "LogLevel":"MIN"', function(done) {
            var payload = '{"LogLevel":"MIN"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "LogLevel":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // LogLevel Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting LogLevel', function(done) {
            verifyConfig = {"LogLevel":"MIN"} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: LogLevel"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "LogLevel":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 98121
        it('should return status 400 on post "LogLevel":"" (No DELETE)', function(done) {
            var payload = '{"LogLevel":""}';
            verifyConfig = {"LogLevel":"MIN"} ;
//            verifyMessage = {"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: LogLevel Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "LogLevel":"" (No DELETE)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // LogLevel Error test cases
    describe('Error', function() {
// 101954
        it('should return status 400 on post  with "LogLevel" in URI', function(done) {
            var payload = '{"LogLevel":"MAX"}';
            verifyConfig = {"LogLevel":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/LogLevel is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after "LogLevel" in URI fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on post when trying to set "LogLevel":9', function(done) {
            var payload = '{"LogLevel":9}';
            verifyConfig = {"LogLevel":"MIN"} ;
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: LogLevel Value: 9."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: LogLevel Property: LogLevel Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "LogLevel":9 fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });


        it('should return status 400 when trying to set to MAXIMUM', function(done) {
            var payload = '{"LogLevel":"MAXIMUM"}';
            verifyConfig = {"LogLevel":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: LogLevel Value: \"MAXIMUM\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after "LogLevel":"MAXIMUM" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });
    
    // LogLevel Reset to Default
    describe('Reset to Default', function() {
    
        it('should return status 200 on post "LogLevel":"NORMAL"', function(done) {
            var payload = '{"LogLevel":"NORMAL"}';
            verifyConfig = {"LogLevel":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "LogLevel":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
