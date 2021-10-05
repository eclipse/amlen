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

var uriObject = 'PluginPort/' ;
var objectMin = 1 ;
var objectDefault = 9103;
var objectMax = 65535 ;
var expectDefault = '{"PluginPort":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginPort test cases
describe('PluginPort:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginPort Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on GET, DEFAULT of "PluginPort":'+ objectDefault , function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginPort Update test cases
    describe('Range['+ objectMin +','+ objectDefault +','+ objectMax +']', function() {

        it('should return status 200 on POST "PluginPort":'+ objectMax , function(done) {
            var payload = '{"PluginPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginPort":'+ objectMax , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginPort":null(DEFAULT)', function(done) {
            var payload = '{"PluginPort":null}';
            verifyConfig = JSON.parse( '{"PluginPort":'+ objectDefault +'}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginPort":null(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginPort":'+ objectMax , function(done) {
            var payload = '{"PluginPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginPort":'+ objectMax , function(done) {
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
        it('should return status 200 on get, "PluginPort" persists', function(done) {
            verifyConfig = JSON.parse( '{"PluginPort":'+ objectMax +'}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		
        it('should return status 200 on POST "PluginPort":'+ objectMin , function(done) {
            var payload = '{"PluginPort":'+ objectMin +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginPort":'+ objectMin , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "PluginPort":'+ objectDefault , function(done) {
            var payload = '{"PluginPort":'+ objectDefault +'}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "PluginPort":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // PluginPort Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 on DELETE PluginPort', function(done) {
            verifyConfig = JSON.parse( '{"PluginPort":'+ objectDefault +'}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginPort"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, unchanged "PluginPort":'+ objectDefault , function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginPort Error test cases
    describe('Error:', function() {

        it('should return status 400 on POST "PluginPort":"" (invalid)', function(done) {
            var payload = '{"PluginPort":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginPort Value: \"\"\"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginPort Property: PluginPort Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginPort":"" (invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST with PluginPort in URI', function(done) {
            var payload = '{"PluginPort":'+ objectMax +'}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginPort is not valid."};
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginPort is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after PluginPort in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "PluginPort/Status" in URI', function(done) {
//            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginPort/Status is not valid."};
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginPort/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "PluginPort?Status" in URI', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginPort?Status', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "PluginPort":"JUNK"', function(done) {
            var payload = '{"PluginPort":"JUNK"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginPort Value: \"JUNK\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginPort Property: PluginPort Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "PluginPort":"JUNK" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginPort":65536', function(done) {
            var payload = '{"PluginPort":65536}';
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginPort Value: \"65536\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginPort":65536 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginPort":0', function(done) {
            var payload = '{"PluginPort":0}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginPort Property: PluginPort Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginPort Value: \"0\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginPort":0 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginPort":false', function(done) {
            var payload = '{"PluginPort":false}';
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginPort Property: PluginPort Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "PluginPort":false failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
