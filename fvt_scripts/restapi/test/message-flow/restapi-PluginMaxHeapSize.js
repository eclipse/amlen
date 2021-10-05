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

var uriObject = 'PluginMaxHeapSize/' ;
var objectMin = 64 ;
var objectDefault = 512;
var objectMax = 65536 ;
var expectDefault = '{"PluginMaxHeapSize":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginMaxHeapSize test cases
describe('PluginMaxHeapSize:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginMaxHeapSize Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on GET, DEFAULT of "PluginMaxHeapSize":'+ objectDefault , function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginMaxHeapSize Update test cases
    describe('Range['+ objectMin +','+ objectDefault +','+ objectMax +']', function() {

        it('should return status 200 on POST "PluginMaxHeapSize":'+ objectMax , function(done) {
            var payload = '{"PluginMaxHeapSize":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginMaxHeapSize":'+ objectMax , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113852
        it('should return status 200 on POST "PluginMaxHeapSize":null(DEFAULT)', function(done) {
            var payload = '{"PluginMaxHeapSize":null}';
            verifyConfig = JSON.parse( '{"PluginMaxHeapSize":'+ objectDefault +'}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginMaxHeapSize":null(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginMaxHeapSize":'+ objectMax , function(done) {
            var payload = '{"PluginMaxHeapSize":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginMaxHeapSize":'+ objectMax , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginMaxHeapSize":'+ objectMin , function(done) {
            var payload = '{"PluginMaxHeapSize":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginMaxHeapSize":'+ objectMin , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        
// Verify Config Changes Persist
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
        it('should return status 200 on get, "PluginMaxHeapSize" persists', function(done) {
            verifyConfig = JSON.parse( '{"PluginMaxHeapSize":'+ objectMin +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		

        it('should return status 200 on POST "PluginMaxHeapSize":'+ objectDefault , function(done) {
            var payload = '{"PluginMaxHeapSize":'+ objectDefault +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginMaxHeapSize":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // PluginMaxHeapSize Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 on DELETE PluginMaxHeapSize', function(done) {
            verifyConfig = JSON.parse( '{"PluginMaxHeapSize":'+ objectDefault +'}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginMaxHeapSize"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, unchanged "PluginMaxHeapSize":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginMaxHeapSize Error test cases
    describe('Error:', function() {

        it('should return status 400 on POST "PluginMaxHeapSize":"" (invalid)', function(done) {
            var payload = '{"PluginMaxHeapSize":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"\"\"\"."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_STRING"} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"InvalidType:JSON_STRING\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginMaxHeapSize":"" (invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST with PluginMaxHeapSize in URI', function(done) {
            var payload = '{"PluginMaxHeapSize":'+ objectMax +'}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginMaxHeapSize is not valid."};
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginMaxHeapSize is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after PluginMaxHeapSize in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "PluginMaxHeapSize/Status" in URI', function(done) {
//            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginMaxHeapSize/Status is not valid."};
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginMaxHeapSize/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "PluginMaxHeapSize?Status" in URI', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginMaxHeapSize?Status', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "PluginMaxHeapSize":"JUNK"', function(done) {
            var payload = '{"PluginMaxHeapSize":"JUNK"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"JUNK\"."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_STRING"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"InvalidType:JSON_STRING\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginMaxHeapSize":"JUNK" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113852
        it('should return status 400 on POST "PluginMaxHeapSize":65537', function(done) {
            var payload = '{"PluginMaxHeapSize":65537}';
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"65537\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginMaxHeapSize":65537 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113852
        it('should return status 400 on POST "PluginMaxHeapSize":0', function(done) {
            var payload = '{"PluginMaxHeapSize":0}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"0\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginMaxHeapSize":0 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginMaxHeapSize":63', function(done) {
            var payload = '{"PluginMaxHeapSize":63}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"63\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginMaxHeapSize":63 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginMaxHeapSize":false', function(done) {
            var payload = '{"PluginMaxHeapSize":false}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_FALSE"};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"63\"."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginMaxHeapSize Value: \"InvalidType:JSON_FALSE\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginMaxHeapSize Property: PluginMaxHeapSize Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginMaxHeapSize":false failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
