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

var uriObject = 'TraceConnection/';
var expectDefault = '{"TraceConnection":"","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here : valid options:  ip, port, client, user ; * is wildcard
// Marc:  "TraceConnection":"type[=:]value" where type is "endpoint", "clientaddr" or "clientid"  
// TraceConnection test cases
describe('TraceConnection:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceConnection Get test cases
    describe('Get in Pristine:', function() {
    
        it('should return status 200 on get, DEFAULT of "TraceConnection":""', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // Clientaddr:
    describe('clientaddr:', function() {
    
        it('should return status 200 on post "TraceConnection":"clientaddr=*:1883"', function(done) {
            var payload = '{"TraceConnection":"clientaddr=*:1883"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientaddr=*:1883"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceConnection":"clientaddr:9.3.*"', function(done) {
            var payload = '{"TraceConnection":"clientaddr:9.3.*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientaddr:9.3.*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceConnection":"clientaddr:9.3.*,clientaddr:*:1883"', function(done) {
            var payload = '{"TraceConnection":"clientaddr:9.3.*,clientaddr:*:1883"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientaddr:9.3.*,clientaddr:*:1883"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceConnection":"clientaddr:c0000*@9.3.*,clientaddr:fc00:0001:*"', function(done) {
            var payload = '{"TraceConnection":"clientaddr:c0000*@9.3.*,clientaddr:fc00:0001:*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientaddr:c0000*@9.3.*,clientaddr:fc00:0001:*"', function(done) {
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
        it('should return status 200 on get, {"TraceConnection":"clientaddr:c0000*@9.3.*,clientaddr:fc00:0001:*"} persists', function(done) {
            verifyConfig = {"TraceConnection":"clientaddr:c0000*@9.3.*,clientaddr:fc00:0001:*"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To Default:', function() {        
        it('should return status 200 on post "TraceConnection":null(RESET to DEFAULT)', function(done) {
            var payload = '{"TraceConnection":null}';
            verifyConfig = JSON.parse( '{"TraceConnection":""}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    // Clientid:
    describe('clientid:', function() {
    
        it('should return status 200 on post "TraceConnection":"clientid=u*"', function(done) {
            var payload = '{"TraceConnection":"clientid=u*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientid=u*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "TraceConnection":"clientid:a-*"', function(done) {
            var payload = '{"TraceConnection":"clientid:a-*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"clientid:a-*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 200 on post "TraceConnection":"" (SET to DEFAULT value)', function(done) {
            var payload = '{"TraceConnection":""}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    // Endpoint:
    describe('endpoint:', function() {
    
        it('should return status 200 on post "TraceConnection":"endpoint:Demo*"', function(done) {
            var payload = '{"TraceConnection":"endpoint:Demo*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"endpoint:Demo*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceConnection":"endpoint=fvt_*"', function(done) {
            var payload = '{"TraceConnection":"endpoint=fvt_*"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":"endpoint=fvt_*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    // TraceConnection Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting TraceConnection', function(done) {
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceConnection"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceConnection":"time"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceConnection Error test cases
    describe('Error:', function() {


        it('should return status 400 on post "TraceConnection":IPv4&IPv6', function(done) {
            var payload = '{"TraceConnection":["clientaddr":"9.3.*","clientaddr":"fc00:0001:*"]}';
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"Code":"CWLNA6001","Message":"Failed to parse administrative request {\"TraceConnection\":[\"clientaddr\":\"9.3.*\",\"clientaddr\":\"fc00:0001:*\"]}: RC=1."};
            verifyMessage = {"Code":"CWLNA6001","Message":"Failed to parse administrative request ']' expected near ':': RC=1."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceConnection":"ip":IPv4&IPv6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on post "TraceConnection":"userid@IP"', function(done) {
            var payload = '{"TraceConnection":"userid@9.3.179*"}';
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: userid@9.3.179*."};
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: \"userid@9.3.179*\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after "TraceConnection":"userid@IP"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on post "TraceConnection":mixed bag', function(done) {
            var payload = '{"TraceConnection":"userid@9.3.179*,clientid:a-*"}';
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: userid@9.3.179*,clientid:a-*."};
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: \"userid@9.3.179*,clientid:a-*\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after "TraceConnection":mixed bag', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 when trying to set to client=fvt*', function(done) {
            var payload = '{"TraceConnection":"client=fvt*"}';
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"status":404,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: client=fvt*."};
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: \"client=fvt*\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after "TraceConnection":"client=fvt*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 110622
        it('should return status 400 when trying to set to NotUnnamedComposite', function(done) {
            var payload = '{"TraceConnection":{"clientid":"NotUnnamedComposite"}}';
            verifyConfig = JSON.parse( '{"TraceConnection":"endpoint=fvt_*"}' ) ;
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceConnection Value: NULL."};
//            verifyMessage = {"Code":"CWLNA0132","Message":"The property value is not valid. Object: SingletonObject Name: TraceConnection Property: InvalidType:JSON_OBJECT Value: \"clientid\":\"NotUnnamedComposite\""};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: SingletonObject Name: TraceConnection Property: TraceConnection Type: JSON_OBJECT"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after "TraceConnection":NotUnnamedComposite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    // TraceConnection Reset to Default
    describe('Reset to Default', function() {
        it('should return status 200 on post "TraceConnection":""(Default)', function(done) {
            var payload = '{"TraceConnection":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceConnection":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

});
