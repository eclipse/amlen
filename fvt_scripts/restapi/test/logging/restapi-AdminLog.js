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

var uriObject = 'AdminLog/' ;
var objectMin = 'MIN' ;
var objectDefault = 'NORMAL';
var objectMax = 'MAX' ;
var expectDefault = '{"AdminLog":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// AdminLog test cases
describe('AdminLog:', function() {
this.timeout( FVT.defaultTimeout );

    // AdminLog Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "AdminLog":"NORMAL"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    // AdminLog Update test cases
    describe('Update Valid Range:', function() {

        it('should return status 200 on post "AdminLog":"MAX"', function(done) {
            var payload = '{"AdminLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//98121
        it('should return status 200 on post "AdminLog":null(DEFAULT)', function(done) {
            var payload = '{"AdminLog":null}';
            verifyConfig = {"AdminLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"NORMAL"(DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "AdminLog":"MAX"', function(done) {
            var payload = '{"AdminLog":"MAX"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"MAX"', function(done) {
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
        it('should return status 200 on get, "AdminLog":"MAX" persists', function(done) {
            verifyConfig = {"AdminLog":"MAX"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			
    describe('Reset To Default:', function() {

        it('should return status 200 on post "AdminLog":"NORMAL"', function(done) {
            var payload = '{"AdminLog":"NORMAL"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "AdminLog":"MIN"', function(done) {
            var payload = '{"AdminLog":"MIN"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    // AdminLog Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting AdminLog', function(done) {
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: AdminLog"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, unchanged "AdminLog":"MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // AdminLog Error test cases
    describe('Error:', function() {
// 98121, 109491
        it('should return status 400 on post "AdminLog":""(invalid)', function(done) {
            var payload = '{"AdminLog":""}';
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminLog Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET still "AdminLog":""(invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//defect 96932 101954 (per 97055, should be 200?)
        it('should return status 400 on post  with "AdminLog" in URI', function(done) {
            var payload = '{"AdminLog":"MAX"}';
            verifyConfig = {"AdminLog":"MIN"} ;
            verifyMessage = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/AdminLog is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get "AdminLog":"MIN" after AdminLog in URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 97055 - ONLY FLAG SLASHES, ignore QueryParams
        it('should return status 400 on GET  with "AdminLog/Status" in URI', function(done) {
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/AdminLog/Status is not valid."};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'Status', FVT.getFailureCallback, verifyConfig, done) ;
		});
// defect 97055
        it('should return status 200 on GET  with "AdminLog?Status" in URI', function(done) {
//   		verifyConfig = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: AdminLog?Status."} ;
// 97055            verifyConfig = {"status":404,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/AdminLog?Status is not valid."};
// 97055            FVT.makeGetRequest( FVT.uriConfigDomain+'AdminLog?Status', FVT.getFailureCallback, verifyConfig, done);
            verifyConfig = {"AdminLog":"MIN"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'AdminLog?Status', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "AdminLog":"JUNK"', function(done) {
            var payload = '{"AdminLog":"JUNK"}';
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: AdminLog Value: JUNK."} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminLog Value: \"JUNK\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "AdminLog":"MAX" after "Junk" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "AdminLog":"min"', function(done) {
            var payload = '{"AdminLog":"min"}';
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: AdminLog Value: min."};
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminLog Value: \"min\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "AdminLog":"MAX" after "min" failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "AdminLog":9', function(done) {
            var payload = '{"AdminLog":9}';
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: AdminLog Value: 9."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminLog Value: 9."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminLog Property: AdminLog Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "AdminLog":"MAX" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "AdminLog":false', function(done) {
            var payload = '{"AdminLog":false}';
            verifyConfig = {"AdminLog":"MIN"} ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: AdminLog Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminLog Value: false."};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminLog Property: AdminLog Type: InvalidType:JSON_BOOLEAN"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: AdminLog Property: AdminLog Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "AdminLog":"MAX" after 9 failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // AdminLog Reset to Default
    describe('Reset to Default:', function() {
//98121
        it('should return status 200 on post "AdminLog":"NORMAL"', function(done) {
            var payload = '{"AdminLog":"NORMAL"}';
            verifyConfig = {"AdminLog":"NORMAL"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get "AdminLog":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
