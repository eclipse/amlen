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

var uriObject = 'SecurityLog/';
var objectMin = 'MIN' ;
var objectDefault = 'NORMAL';
var objectMax = 'MAX' ;
var expectDefault = '{"SecurityLog":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};



// Test Cases Start Here

// SecurityLog test cases
describe('SecurityLog:', function() {
this.timeout( FVT.defaultTimeout );

    // SecurityLog Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "SecurityLog":"NORMAL"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // SecurityLog Update test cases
    describe('[MIN-NORMAL-MAX]:', function() {

        it('should return status 200 on post "SecurityLog":"MAX"', function(done) {
            var payload = '{"SecurityLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "SecurityLog":"MAX"', function(done) {
             FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 98121
        it('should return status 200 on post "SecurityLog":null (DEFAULT)', function(done) {
            var payload = '{"SecurityLog":null}';
            verifyConfig = {"SecurityLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "SecurityLog":null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "SecurityLog":"MAX"', function(done) {
            var payload = '{"SecurityLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "SecurityLog":"MAX"', function(done) {
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
        it('should return status 200 on get, "SecurityLog":"MAX" persists', function(done) {
            verifyConfig = {"SecurityLog":"MAX"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To Default:', function() {
        it('should return status 200 on post "SecurityLog":"NORMAL"', function(done) {
            var payload = '{"SecurityLog":"NORMAL"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "SecurityLog":"NORMAL"', function(done) {
             FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "SecurityLog":"MIN"', function(done) {
            var payload = '{"SecurityLog":"MIN"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "SecurityLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // SecurityLog Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting SecurityLog', function(done) {
            verifyConfig = {"SecurityLog":"MIN"} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: SecurityLog"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "SecurityLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 98121
        it('should return status 400 on post "SecurityLog":"" (No DELETE)', function(done) {
            var payload = '{"SecurityLog":""}';
            verifyConfig = {"SecurityLog":"MIN"} ;
//            verifyMessage = {"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: SecurityLog Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET still "SecurityLog":"" (No DELETE)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // SecurityLog Error test cases
    describe('Error:', function() {
// 101954
        it('should return status 400 on post  with "SecurityLog" in URI', function(done) {
            var payload = '{"SecurityLog":"MAX"}';
            verifyConfig = {"SecurityLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/SecurityLog is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "SecurityLog"  in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "SecurityLog":"Maximum"', function(done) {
            var payload = '{"SecurityLog":"Maximum"}';
            verifyConfig = {"SecurityLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: SecurityLog Value: \"Maximum\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after "SecurityLog":"Maximum" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "SecurityLog":"min"', function(done) {
            var payload = '{"SecurityLog":"max"}';
            verifyConfig = {"SecurityLog":"MIN"} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: SecurityLog Value: \"max\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after "SecurityLog":"min" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // SecurityLog Reset to Default
    describe('Reset to Default:', function() {

        it('should return status 200 on post "SecurityLog":"NORMAL"', function(done) {
            var payload = '{"SecurityLog":"NORMAL"}';
            verifyConfig = {"SecurityLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "SecurityLog":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
