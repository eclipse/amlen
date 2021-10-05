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

var uriObject = 'TraceBackupDestination/';
var expectDefault = '{"TraceBackupDestination":"","Version":"'+ FVT.version +'"}' ;
var objectDefault = "\"\"";
// https://w3-connections.ibm.com/wikis/home?lang=en-us#!/wiki/W11ac9778a1d4_4f61_913c_a76a78c7bff7/page/Trace%20backup%20and%20offloading

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// TraceBackupDestination test cases
describe('TraceBackupDestination', function() {
this.timeout( FVT.defaultTimeout );

    // TraceBackupDestination Get test cases
    describe('Get in Pristine', function() {
// 115603
        it('should return status 200 on get, DEFAULT of "TraceBackupDestination":'+objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // TraceBackupDestination Update test cases
    describe('Range[ftp://, scp://]2048', function() {
	
        it('should return status 200 on post "TraceBackupDestination":"ftp://"', function(done) {
            var payload = '{"TraceBackupDestination":"ftp://127.0.0.1:9090/some/path/"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"ftp://127.0.0.1:9090/some/path/"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done) ;
        });
	
        it('should return status 200 on post "TraceBackupDestination":"A LONG ftp://URL"', function(done) {
            var payload = '{"TraceBackupDestination":"'+ FVT.long2048FTP +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"A LONG ftp://URL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TraceBackupDestination":"A LONG scp://URL"', function(done) {
            var payload = '{"TraceBackupDestination":"'+ FVT.long2048SCP +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"A LONG scp://URL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"', function(done) {
            var payload = '{"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"', function(done) {
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
        it('should return status 200 on get, "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/" persists', function(done) {
            verifyConfig = {"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // TraceBackupDestination Delete test cases
    describe('Delete', function() {
        it('should return status 403 when deleting TraceBackupDestination', function(done) {
            verifyConfig = {"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"} ;
//			verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceBackupDestination"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // TraceBackupDestination Error test cases
    describe('Error', function() {
	
        it('should return status 400 on post "TraceBackupDestination":"TOO LONG ftp://URL"', function(done) {
            var payload = '{"TraceBackupDestination":"'+ FVT.long2048FTP +'/x"}';
            verifyConfig = {"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"} ;
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupDestination Value: \\\"' + FVT.long2048FTP + '/x\\\"."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"A LONG ftp://URL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 115593
        it('should return status 400 when trying to set to JUNK', function(done) {
            var payload = '{"TraceBackupDestination":"JUNK"}';
            verifyConfig = {"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"} ;
//			verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupDestination Value: \"JUNK\"."};
//			verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property value is not valid: Property: TraceBackupDestination Value: \"JUNK\"."};
			verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupDestination Value: \"JUNK\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 115593
        it('should return status 400 when trying to set to HTTP', function(done) {
            var payload = '{"TraceBackupDestination":"'+ FVT.long2048HTTP +'"}' ;
            verifyConfig = {"TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"} ;
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceBackupDestination Value: \\\"' + FVT.long2048HTTP + '\\\"."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceBackupDestination":"scp://127.0.0.1:8080/some/path/"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });
	
    // TraceBackupDestination Reset to Default
    describe('Reset to Default', function() {
	
        it('should return status 200 on post "TraceBackupDestination":"" (DEFAULT)', function(done) {
            var payload = '{"TraceBackupDestination":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":'+objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TraceBackupDestination":"ftp://"', function(done) {
            var payload = '{"TraceBackupDestination":"ftp://127.0.0.1:9090/some/path/"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":"ftp://127.0.0.1:9090/some/path/"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "TraceBackupDestination":null (DEFAULT)', function(done) {
            var payload = '{"TraceBackupDestination":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceBackupDestination":'+objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
