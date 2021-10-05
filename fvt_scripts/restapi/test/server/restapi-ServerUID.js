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

var uriObject = 'ServerUID/';
var expectDefault = '{"ServerUID":"","Version":"'+ FVT.version +'"}' ;
var thisUID = "" ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here
function getUIDCallback(err, res, done ){
    FVT.trace( 0, "RESPONSE:"+ JSON.stringify( res, null, 2 ) );
	jsonResponse = JSON.parse( res.text );
    thisUID = jsonResponse.ServerUID ;
	FVT.trace( 1, 'The ServerUID: '+ thisUID );
	expectDefault = '{"ServerUID":"'+ thisUID +'","Version":"'+ FVT.version +'"}' ;
	done();
}
// ServerUID test cases
describe('ServerUID:', function() {
this.timeout( FVT.defaultTimeout );

    // ServerUID Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "ServerUID"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, getUIDCallback, verifyConfig, done)
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
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "ServerUID" persists', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

    // ServerUID Update test cases
    describe('Not Settable:', function() {
	
        it('should return status 404 on post "ServerUID":Max Length Name', function(done) {
            var payload = '{"ServerUID":"'+ FVT.long1024Name +'"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: ServerUID Value: a16_CharName3456."};
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: ServerUID Value: '+ FVT.long1024Name +'."}' );
			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: ServerUID."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ServerUID": is NOT settable:', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 404 on post "ServerUID":Greater than Max Length Name', function(done) {
            var payload = '{"ServerUID":"'+ FVT.long1025Name +'"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: ServerUID Value: '+ FVT.long1025Name +'."}' );
			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: ServerUID."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ServerUID": is NOT settable:', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // ServerUID Delete test cases
    describe('Delete:', function() {
	
        it('should return status 403 when deleting ServerUID', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: ServerUID"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 404 on post "ServerUID":""(not DELETE anymore, invalid)', function(done) {
            var payload = '{"ServerUID":""}';
            verifyConfig = JSON.parse( expectDefault );
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"ServerUID\":null} is not valid."};
//			verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: ServerUID."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ServerUID":"0"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // ServerUID Error test cases
    describe('Error:', function() {

        it('should return status 400 when trying to set to a valid number', function(done) {
            var payload = '{"ServerUID":1}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"ServerUID\":1} is not valid."};
			verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: ServerUID."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "Default" after failed POST', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });


	});
	
    // ServerUID Reset to Default
    describe('Is NO Reset to Default:', function() {
	
        it('should return status 400 on post "ServerUID":null(Default)', function(done) {
            var payload = '{"ServerUID":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"ServerUID\":\"\"} is not valid."};
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: ServerUID Value: (BAD)."} ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"ServerUID\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "ServerUID":"0"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
