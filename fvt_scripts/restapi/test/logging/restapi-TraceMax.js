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

var uriObject = 'TraceMax/';
var objectMin = '250KB';
var objectDefault = '200MB';
var objectMax = '500MB';
var expectDefault = '{"TraceMax":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// TraceMax test cases
describe('TraceMax', function() {
this.timeout( FVT.defaultTimeout );

    // TraceMax Get test cases
    describe('Get in Pristine', function() {
    
        it('should return status 200 on get, DEFAULT of "TraceMax":'+objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    // TraceMax Update test cases - MB,KB, .vs. K,M (should be OK but if rejected 97066)  .vs. k kb m mb  (just wrong) 96504, 95691
	// long string of defects on "Type":"BufferSize" and KB, MB  permutation 109518
    describe('Update Valid Range', function() {
    
        it('should return status 200 on post "TraceMax":"'+objectMin+'"', function(done) {
            var payload = '{"TraceMax":"'+ objectMin +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on post "TraceMax":"'+objectMax+'"', function(done) {
            var payload = '{"TraceMax":"'+ objectMax +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"'+objectMax+'"', function(done) {
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
        it('should return status 200 on get, {"TraceMax":"'+ objectMax +'"} persists', function(done) {
            verifyConfig = JSON.parse( '{"TraceMax":"'+ objectMax +'"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Range cont.:', function() {
        it('should return status 200 on post "TraceMax":"499 MB"', function(done) {
            var payload = '{"TraceMax":"499 MB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"499 MB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    
        it('should return status 200 on post "TraceMax":"400MB"', function(done) {
            var payload = '{"TraceMax":"400MB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"400MB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMax":"469 MB"', function(done) {
            var payload = '{"TraceMax":"469 MB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"469 MB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMax":"251 KB"', function(done) {
            var payload = '{"TraceMax":"251 KB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"251 KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMax":"269KB"', function(done) {
            var payload = '{"TraceMax":"269KB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"269KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMax":"300 KB"', function(done) {
            var payload = '{"TraceMax":"300 KB"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":"300 KB"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TraceMax":"'+objectMin+'"', function(done) {
            var payload = '{"TraceMax":"'+objectMin+'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });


    // TraceMax Delete test cases
    describe('Delete', function() {
    
        it('should return status 403 when deleting TraceMax', function(done) {
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceMax"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed DELETE, "TraceMax":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 109491
       it('should return status 400 on post "TraceMax":"" (invalid) ', function(done) {
           var payload = '{"TraceMax":""}';
           verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
           verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"\"\"\"."} ;
           FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
       });
       it('should return status 200 on GET after Failed POST "TraceMax":""', function(done) {
           FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    // TraceMax Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to NORMAL', function(done) {
            var payload = '{"TraceMax":"NORMAL"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"NORMAL\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "NORMAL" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 1024, but not as a string', function(done) {
            var payload = '{"TraceMax":1024}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: 1024."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceMax Property: InvalidType:JSON_INTEGER Value: 1024."};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMax Property: TraceMax Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST 1024 fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 249K, out of bounds', function(done) {
            var payload = '{"TraceMax":"249K"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"249K\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "249K" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 249KB, out of bounds', function(done) {
            var payload = '{"TraceMax":"249KB"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"249KB\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "249KB" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 501M, out of bounds', function(done) {
            var payload = '{"TraceMax":"501M"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"501M\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "501M" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 501MB, out of bounds', function(done) {
            var payload = '{"TraceMax":"501MB"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"501MB\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "501MB" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when trying to set to 500M, Number not a String', function(done) {
            var payload = '{"TraceMax":500M}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA6001","Message":"Failed to parse administrative request {\"TraceMax\":500M}: RC=1."};
            verifyMessage = {"status":400,"Code":"CWLNA6001","Message":"Failed to parse administrative request '}' expected near 'M': RC=1."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST 500M fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 109518
        it('should return status 400 when trying to set to 500m, string but lowercase', function(done) {
            var payload = '{"TraceMax":"500m"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"500m\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "500m" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 109518
        it('should return status 400 when trying to set to 275k, String but lower case', function(done) {
            var payload = '{"TraceMax":"275k"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"275k\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "275k" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 269, String but no KB/MB', function(done) {
            var payload = '{"TraceMax":"269"}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: \"269\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "269" fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to set to 250K, Number not a String', function(done) {
            var payload = '{"TraceMax":250K}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA6001","Message":"Failed to parse administrative request {\"TraceMax\":250K}: RC=1."};
            verifyMessage = {"status":400,"Code":"CWLNA6001","Message":"Failed to parse administrative request '}' expected near 'K': RC=1."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST 250K fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceMax":1.0}';
            verifyConfig = JSON.parse( '{"TraceMax":"'+objectMin+'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceMax Value: 1.0."};
//            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMax Property: TraceMax Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceMax Property: TraceMax Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST 1.0 fails, still "TraceMax":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
    // TraceMax Reset to Default
    describe('Reset to Default', function() {
// 109502
        it('should return status 200 on post "TraceMax":null(Default)', function(done) {
            var payload = '{"TraceMax":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceMax":'+objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

});
