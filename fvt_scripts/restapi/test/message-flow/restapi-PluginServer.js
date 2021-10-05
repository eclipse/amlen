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

var uriObject = 'PluginServer/' ;
var objectMin = FVT.msgServer ;
var objectDefault = '127.0.0.1';
var objectMax = FVT.A1_msgServer_2 ;
var expectDefault = '{"PluginServer":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginServer test cases
describe('PluginServer:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginServer Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "PluginServer":"'+ objectDefault +'"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginServer Update test cases
    describe('Update Valid Range:', function() {

        it('should return status 200 on post "PluginServer":"'+ objectMax +'"', function(done) {
            var payload = '{"PluginServer":"'+ objectMax +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectMax +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113852
        it('should return status 200 on post "PluginServer":null(DEFAULT)', function(done) {
            var payload = '{"PluginServer":null}';
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectDefault +'"}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":""(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginServer":"'+ objectMax +'"', function(done) {
            var payload = '{"PluginServer":"'+ objectMax +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectMax +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginServer":"'+ objectDefault +'"', function(done) {
            var payload = '{"PluginServer":"'+ objectDefault +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectDefault +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginServer":"'+ objectMin +'"', function(done) {
            var payload = '{"PluginServer":"'+ objectMin +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectMin +'"', function(done) {
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
        it('should return status 200 on get, "PluginServer" persists', function(done) {
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
		
    });


    // PluginServer Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting PluginServer', function(done) {
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginServer"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "PluginServer":"'+ objectMin +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginServer Error test cases
    describe('Error:', function() {

        it('should return status 400 on post  with "PluginServer" in URI', function(done) {
            var payload = '{"PluginServer":"'+ objectMax +'"}';
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginServer is not valid."};
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginServer is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectMin +'" after PluginServer in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "PluginServer/Status" in URI', function(done) {
//            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginServer/Status is not valid."};
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginServer/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "PluginServer?Status" in URI', function(done) {
//   		verifyConfig = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: PluginServer?Status."} ;
// 97055            verifyConfig = {"status":404,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginServer?Status is not valid."};
// 97055            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginServer?Status', FVT.getFailureCallback, verifyConfig, done);
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginServer?Status', FVT.getSuccessCallback, verifyConfig, done);
        });
		
       it('should return status 400 on POST "PluginServer":"list of ips"', function(done) {
           var payload = '{"PluginServer":"'+ objectMin +','+ objectMax +'"}';
           verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
           verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginServer Value: \\\"'+ objectMin +','+ objectMax +'\\\"."}' ) ;
           FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
       });
       it('should return status 200 on GET "PluginServer":"'+ objectMin +'" after "list of ips" failed', function(done) {
           FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
       });

// 114338
        it('should return status 400 on POST "PluginServer":9', function(done) {
            var payload = '{"PluginServer":9}';
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: PluginServer Value: 9."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginServer Value: 9."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: PluginServer Name: InvalidType:JSON_INTEGER Property: *UNKNOWN* Value: \"9\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginServer Property: PluginServer Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginServer":"'+ objectMax +'" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 114338
        it('should return status 400 on POST "PluginServer":false', function(done) {
            var payload = '{"PluginServer":false}';
            verifyConfig = JSON.parse( '{"PluginServer":"'+ objectMin +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: PluginServer Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginServer Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginServer Property: PluginServer Type: InvalidType:JSON_BOOLEAN"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginServer Property: PluginServer Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginServer":"'+ objectMax +'" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // PluginServer Reset to Default
    describe('Reset to Default:', function() {
//98121
        it('should return status 200 on post "PluginServer":"'+ objectDefault +'"', function(done) {
            var payload = '{"PluginServer":"'+ objectDefault +'"}';
            verifyConfig = JSON.parse( payload  ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginServer":"'+ objectDefault +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
