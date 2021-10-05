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

var uriObject = 'PluginDebugPort/' ;
var objectMin = 0 ;
var objectDefault = 0;   //Not 9102 like defect 113850 suggests
var objectMax = 65535 ;
var expectDefault = '{"PluginDebugPort":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginDebugPort test cases
describe('PluginDebugPort:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginDebugPort Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on GET, DEFAULT of "PluginDebugPort":'+ objectDefault , function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginDebugPort Update test cases
    describe('Range['+ objectMin +','+ objectDefault +','+ objectMax +']', function() {

        it('should return status 200 on POST "PluginDebugPort":'+ objectMax , function(done) {
            var payload = '{"PluginDebugPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginDebugPort":'+ objectMax , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113850
        it('should return status 200 on POST "PluginDebugPort":null(DEFAULT)', function(done) {
            var payload = '{"PluginDebugPort":null}';
            verifyConfig = JSON.parse( '{"PluginDebugPort":'+ objectDefault +'}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginDebugPort":null(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginDebugPort":'+ objectMax , function(done) {
            var payload = '{"PluginDebugPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginDebugPort":'+ objectMax , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginDebugPort":'+ objectMin , function(done) {
            var payload = '{"PluginDebugPort":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginDebugPort":'+ objectMin , function(done) {
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
        it('should return status 200 on get, "PluginDebugPort" persists', function(done) {
            verifyConfig = JSON.parse( '{"PluginDebugPort":'+ objectMin +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		
        it('should return status 200 on POST "PluginDebugPort":'+ objectDefault , function(done) {
            var payload = '{"PluginDebugPort":'+ objectDefault +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginDebugPort":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // PluginDebugPort Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 on DELETE PluginDebugPort', function(done) {
            verifyConfig = JSON.parse( '{"PluginDebugPort":'+ objectDefault +'}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginDebugPort"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, unchanged "PluginDebugPort":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginDebugPort Error test cases
    describe('Error:', function() {

        it('should return status 400 on POST "PluginDebugPort":"" (invalid)', function(done) {
            var payload = '{"PluginDebugPort":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugPort Value: \"\"\"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugPort Property: PluginDebugPort Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginDebugPort":"" (invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST with PluginDebugPort in URI', function(done) {
            var payload = '{"PluginDebugPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginDebugPort is not valid."};
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginDebugPort is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after PluginDebugPort in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "PluginDebugPort/Status" in URI', function(done) {
//            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginDebugPort/Status is not valid."};
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginDebugPort/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "PluginDebugPort?Status" in URI', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginDebugPort?Status', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "PluginDebugPort":"JUNK"', function(done) {
            var payload = '{"PluginDebugPort":"JUNK"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugPort Value: \"JUNK\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugPort Property: PluginDebugPort Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginDebugPort":"JUNK" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginDebugPort":65536', function(done) {
            var payload = '{"PluginDebugPort":65536}';
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugPort Value: \"65536\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginDebugPort":65536 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113850
        it('should return status 400 on POST "PluginDebugPort":-1', function(done) {
            var payload = '{"PluginDebugPort":-1}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugPort Property: PluginDebugPort Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugPort Value: \"-1\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginDebugPort":0 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginDebugPort":false', function(done) {
            var payload = '{"PluginDebugPort":false}';
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugPort Property: PluginDebugPort Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginDebugPort":false failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
