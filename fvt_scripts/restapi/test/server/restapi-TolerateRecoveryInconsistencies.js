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

var uriObject = 'TolerateRecoveryInconsistencies/';
// https://knowledgecenters.hursley.ibm.com/mq-internal/SSWMAJ_2.0.0/com.ibm.ism.doc/Reference/SpecialCmd/cmd_get_tolerate.html
// Found this looking at InfoCenter... :-/

var expectDefault = '{"TolerateRecoveryInconsistencies":false,"Version":"'+ FVT.version +'"}' ;
var expectEnabled = '{"TolerateRecoveryInconsistencies":true,"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// TolerateRecoveryInconsistencies test cases
describe('TolerateRecoveryInconsistencies:', function() {

    // TolerateRecoveryInconsistencies Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "TolerateRecoveryInconsistencies"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // TolerateRecoveryInconsistencies Update test cases
    describe('[T|F]:', function() {
	
        it('should return status 200 on post "TolerateRecoveryInconsistencies":true', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TolerateRecoveryInconsistencies": true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TolerateRecoveryInconsistencies":false', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TolerateRecoveryInconsistencies":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TolerateRecoveryInconsistencies":true', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TolerateRecoveryInconsistencies": true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

	
    describe('SvcRestart', function() {
   
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
        it('should return status 200 on get, "TolerateRecoveryInconsistencies" persists', function(done) {
            verifyConfig = JSON.parse( expectEnabled ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });


    // TolerateRecoveryInconsistencies Delete test cases
    describe('Delete', function() {
	
        it('should return status 403 when deleting TolerateRecoveryInconsistencies', function(done) {
            verifyConfig = JSON.parse( expectEnabled ) ;
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TolerateRecoveryInconsistencies"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // TolerateRecoveryInconsistencies Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to 1', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":1}';
            verifyConfig = JSON.parse( expectEnabled ) ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TolerateRecoveryInconsistencies Property: TolerateRecoveryInconsistencies Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed set to 1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to string "true"', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":"true"}';
            verifyConfig = JSON.parse( expectEnabled ) ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TolerateRecoveryInconsistencies Property: TolerateRecoveryInconsistencies Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed set to string "true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to empty string ""', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":""}';
            verifyConfig = JSON.parse( expectEnabled ) ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TolerateRecoveryInconsistencies Property: TolerateRecoveryInconsistencies Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed set to empty string ""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 1.0', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":1.0}';
            verifyConfig = JSON.parse( expectEnabled ) ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TolerateRecoveryInconsistencies Property: TolerateRecoveryInconsistencies Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed set to 1.0', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });


	});
	
    // TolerateRecoveryInconsistencies Reset to Default
    describe('Reset to Default', function() {

        it('should return status 200 on post "TolerateRecoveryInconsistencies":null', function(done) {
            var payload = '{"TolerateRecoveryInconsistencies":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TolerateRecoveryInconsistencies": false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
