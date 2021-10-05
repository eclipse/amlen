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

var objectDefault = true;
var uriObject = 'Transaction/';
var expectDefault = '{"Transaction": [ ] ,"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here
// CMDS are SERVICE: [commit|rollback|forget]/transaction,  Monitor: Transaction   (yes case is different...)
// Successful XID testing is in the legacy JCS Heuristics test case run in automation

// XATransaction test cases
describe('XATransaction:', function() {
this.timeout( FVT.defaultTimeout );

    // XATransaction Get test cases
    describe('Pristine Get on Monitor:', function() {
	
        it('should return status 200 on get, DEFAULT of "Transaction"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // XATransaction action test cases
    describe('commit', function() {
	
        it('should return status 400 POST "T"ransaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /commit/Transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"commit/Transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":404, "Code":"CWLNA0113","Message":"The requested item is not found." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"commit/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"009:008:001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: XID Value: \"009:008:001\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"commit/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
// 136352
        it('should return status 400 POST "PID, not XID"', function(done) {
            var payload = '{"PID":"57415344"}';
            verifyConfig = JSON.parse( payload ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: Transaction Name: commit Property: PID" } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: transaction Name:  Property: PID" } ;
			verifyMessage = { "status":400, "Code":"CWLNA0115","Message":"An argument is not valid: Name: PID." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"commit/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Config Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/commit is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+"commit/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Monitor Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: POST is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriMonitorDomain+"commit/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
		
	});


    // XATransaction action test cases
    describe('rollback', function() {
	
        it('should return status 400 POST "T"ransaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /rollback/Transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"rollback/Transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":404, "Code":"CWLNA0113","Message":"The requested item is not found." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"rollback/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"009:008:001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: XID Value: \"009:008:001\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"rollback/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
// 136352	
        it('should return status 400 POST "PID, not XID"', function(done) {
            var payload = '{"PID":"57415344"}';
            verifyConfig = JSON.parse( payload ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: Transaction Name: rollback Property: PID" } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: transaction Name:  Property: PID" } ;
			verifyMessage = { "status":400, "Code":"CWLNA0115","Message":"An argument is not valid: Name: PID." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"rollback/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Config Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/rollback is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+"rollback/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Monitor Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: POST is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriMonitorDomain+"rollback/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
		
	});

    // XATransaction action test cases
    describe('forget', function() {
	
        it('should return status 400 POST "T"ransaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /forget/Transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"forget/Transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":404, "Code":"CWLNA0113","Message":"The requested item is not found." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"forget/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "transaction"', function(done) {
            var payload = '{"XID":"009:008:001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: XID Value: \"009:008:001\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"forget/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
// 136352
        it('should return status 400 POST "PID, not XID"', function(done) {
            var payload = '{"PID":"57415344"}';
            verifyConfig = JSON.parse( payload ) ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: Transaction Name: forget Property: PID" } ;
//			verifyMessage = { "status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: transaction Name:  Property: PID" } ;
			verifyMessage = { "status":400, "Code":"CWLNA0115","Message":"An argument is not valid: Name: PID." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"forget/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Config Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/forget is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+"forget/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "Monitor Domain"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: POST is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriMonitorDomain+"forget/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
		
	});

    // XATransaction Delete test cases
    describe('Delete', function() {
	
        it('should return status 400 when deleting Transaction', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/Transaction/57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001 is not valid."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 when deleting transaction', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/transaction/57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001 is not valid."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+"transaction/57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 when deleting transaction', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/forget/transaction/57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001 is not valid."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+"forget/transaction/57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
		
    });

    // XATransaction Error test cases
    describe('Error', function() {
	
        it('should return status 400 POST "start/tranaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/start/transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"start/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "start/Tranaction"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/start/Transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"start/Transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return status 400 POST "begin"', function(done) {
            var payload = '{"XID":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E2:00000001"}';
            verifyConfig = JSON.parse( payload ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: /begin/transaction is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"begin/transaction", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
		
    });

});
