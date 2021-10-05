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

var uriObject = 'PluginDebugServer/' ;
var objectMin = ""  ;
var objectDefault = "";
var objectMax = ""  ;
var expectDefault = '{"PluginDebugServer":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// PluginDebugServer test cases
describe('PluginDebugServer:', function() {
this.timeout( FVT.defaultTimeout );

    // PluginDebugServer Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "PluginDebugServer":"'+ objectDefault +'"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // PluginDebugServer Update test cases
    describe('Range[locsl IPs]:', function() {
// 114833, 140919
        it('should return status 200 on post local ip "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get local ip "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on post remote ip "PluginDebugServer":"'+ FVT.M1_HOST +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.M1_HOST +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get remote ip "PluginDebugServer":"'+ FVT.M1_HOST +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on post hostname "PluginDebugServer":"'+ FVT.A1_SERVERNAME +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get hostname "PluginDebugServer":"'+ FVT.A1_SERVERNAME +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113852
        it('should return status 200 on post "PluginDebugServer":null(DEFAULT)', function(done) {
            var payload = '{"PluginDebugServer":null}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ objectDefault +'"}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginDebugServer":""(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginDebugServer":"'+ objectDefault +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ objectDefault +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ objectDefault +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "PluginDebugServer":"'+ FVT.msgServer +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.msgServer +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ FVT.msgServer +'"', function(done) {
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
        it('should return status 200 on get, "PluginDebugServer" persists', function(done) {
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		
       
    });


    // PluginDebugServer Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting PluginDebugServer', function(done) {
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: PluginDebugServer"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "PluginDebugServer":"'+ FVT.msgServer +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // PluginDebugServer Error test cases
    describe('Error:', function() {

        it('should return status 400 on post where "PluginDebugServer" not local IP', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A2_msgServer +'"}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugServer Value: \\\"'+ FVT.A2_msgServer +'\\\"."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ FVT.msgServer +'" after PluginDebugServer not local IP', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post  with "PluginDebugServer" in URI', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A1_msgServer_2 +'"}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginDebugServer is not valid."};
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginDebugServer is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ FVT.msgServer +'" after PluginDebugServer in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "PluginDebugServer/Status" in URI', function(done) {
//            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginDebugServer/Status is not valid."};
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PluginDebugServer/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "PluginDebugServer?Status" in URI', function(done) {
//   		verifyConfig = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: PluginDebugServer?Status."} ;
// 97055            verifyConfig = {"status":404,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/PluginDebugServer?Status is not valid."};
// 97055            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginDebugServer?Status', FVT.getFailureCallback, verifyConfig, done);
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'PluginDebugServer?Status', FVT.getSuccessCallback, verifyConfig, done);
        });
// 114340
       it('should return status 400 on POST "PluginDebugServer":"list of ips"', function(done) {
           var payload = '{"PluginDebugServer":"'+ FVT.msgServer +','+ FVT.A1_msgServer_2 +'"}';
           verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
           verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugServer Value: \\\"'+ FVT.msgServer +','+ FVT.A1_msgServer_2 +'\\\"."}' ) ;
           FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
       });
       it('should return status 200 on GET "PluginDebugServer":"'+ FVT.msgServer +'" after "list of ips" failed', function(done) {
           FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
       });

// 114338
        it('should return status 400 on POST "PluginDebugServer":9', function(done) {
            var payload = '{"PluginDebugServer":9}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: PluginDebugServer Value: 9."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugServer Value: 9."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: PluginDebugServer Name: InvalidType:JSON_INTEGER Property: *UNKNOWN* Value: \"9\"."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugServer Property: PluginDebugServer Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 114338
        it('should return status 400 on POST "PluginDebugServer":false', function(done) {
            var payload = '{"PluginDebugServer":false}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: PluginDebugServer Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugServer Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugServer Property: PluginDebugServer Type: InvalidType:JSON_BOOLEAN"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PluginDebugServer Property: PluginDebugServer Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginDebugServer":"'+ FVT.A1_msgServer_2 +'" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginDebugServer":hostname no quotes', function(done) {
            var payload = '{"PluginDebugServer":'+ FVT.A1_HOSTNAME +'}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
//??			FVT.trace( 0, "HOSTNAME "+ FVT.A1_HOSTNAME.search( /[0-9]/)  );
            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA6001","Message":"Failed to parse administrative request invalid token near \''+ FVT.A1_HOSTNAME.substring(0,FVT.A1_HOSTNAME.search(/[0-9.]/) ) +'\': RC=1."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginDebugServer":"'+ FVT.A1_HOSTNAME +'" after hostname no quotes failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "PluginDebugServer":"hostname"', function(done) {
            var payload = '{"PluginDebugServer":"'+ FVT.A1_HOSTNAME +'"}';
            verifyConfig = JSON.parse( '{"PluginDebugServer":"'+ FVT.msgServer +'"}' ) ;
            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PluginDebugServer Value: \\\"'+ FVT.A1_HOSTNAME +'\\\"."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PluginDebugServer":"'+ FVT.A1_HOSTNAME +'" after hostname failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // PluginDebugServer Reset to Default
    describe('Reset to Default:', function() {
//98121
        it('should return status 200 on post "PluginDebugServer":"'+ objectDefault +'"', function(done) {
            var payload = '{"PluginDebugServer":"'+ objectDefault +'"}';
            verifyConfig = JSON.parse( payload  ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "PluginDebugServer":"'+ objectDefault +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
