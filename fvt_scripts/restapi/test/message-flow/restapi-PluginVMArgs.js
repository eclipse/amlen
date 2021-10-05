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

var uriObject = 'PluginVMArgs/' ;
var objectMin = '""' ;
var objectDefault = '';
var objectMax = '""' ;
var expectDefault = '{"PluginVMArgs":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginVMArgs test cases
describe('PluginVMArgs:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginVMArgs Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "PluginVMArgs":"'+ objectDefault +'"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginVMArgs Update test cases
    describe('Update Valid Range:', function() {

        it('should return status 200 on post "PluginVMArgs":""', function(done) {
            var payload = '{"PluginVMArgs":""}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginVMArgs":"-XX:+ScavengeBeforeFullGC"', function(done) {
            var payload = '{"PluginVMArgs":"-XX:+ScavengeBeforeFullGC"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":"-XX:+ScavengeBeforeFullGC"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginVMArgs":"-Xss1024m"', function(done) {
            var payload = '{"PluginVMArgs":"-Xss1024m"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":"-Xss1024m"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginVMArgs":""', function(done) {
            var payload = '{"PluginVMArgs":""}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginVMArgs":"-Xms2G -Xmx4G"', function(done) {
            var payload = '{"PluginVMArgs":"-Xms2G -Xmx4G"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":"-Xms256m -Xmx4G"', function(done) {
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
        it('should return status 200 on get, "PluginVMArgs" persists', function(done) {
            verifyConfig = {"PluginVMArgs":"-Xms2G -Xmx4G"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		
        it('should return status 200 on post "PluginVMArgs":null (DEFAULT: "")', function(done) {
            var payload = '{"PluginVMArgs":null}';
            verifyConfig = {"PluginVMArgs":""} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginVMArgs":null (DEFAULT: "")', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });



    // PluginVMArgs Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting PluginVMArgs', function(done) {
            verifyConfig = {"PluginVMArgs":""} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginVMArgs"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "PluginVMArgs":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginVMArgs Error test cases
    describe('Error:', function() {
// 98121, 109491
        it('should return 400 on POST "PluginVMArgs":"MIN"', function(done) {
            var payload = '{"PluginVMArgs":"MIN"}';
            verifyConfig = {"PluginVMArgs":""} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginVMArgs Value: \"MIN\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET still "PluginVMArgs":"" afater MIN fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post  with "PluginVMArgs" in URI', function(done) {
            var payload = '{"PluginVMArgs":"-Xss1024m"}';
            verifyConfig = {"PluginVMArgs":""} ;
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginVMArgs is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "PluginVMArgs":"" after PluginVMArgs in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on GET  with "PluginVMArgs/Status" in URI', function(done) {
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginVMArgs/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});

        it('should return status 200 on GET  with "PluginVMArgs?Status" in URI', function(done) {
            verifyConfig = {"PluginVMArgs":""} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginVMArgs?Status', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "PluginVMArgs":"JUNK"', function(done) {
            var payload = '{"PluginVMArgs":"JUNK"}';
            verifyConfig = {"PluginVMArgs":""} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginVMArgs Value: \"JUNK\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "PluginVMArgs":"" after "Junk" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginVMArgs":9', function(done) {
            var payload = '{"PluginVMArgs":9}';
            verifyConfig = {"PluginVMArgs":""} ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginVMArgs Property: PluginVMArgs Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginVMArgs":"" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginVMArgs":false', function(done) {
            var payload = '{"PluginVMArgs":false}';
            verifyConfig = {"PluginVMArgs":""} ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginVMArgs Property: PluginVMArgs Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginVMArgs":"MAX" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
