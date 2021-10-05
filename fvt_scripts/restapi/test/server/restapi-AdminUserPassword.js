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

var uriObject = 'AdminUserPassword/';
var expectDefault = '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/AdminUserPassword/ is not valid.","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// AdminUserPassword test cases
describe('AdminUserPassword:', function() {
this.timeout( FVT.defaultTimeout );

    // AdminUserPassword Get test cases
    describe('Pristine Get:', function() {
    
        it('should return status 400 on get, DEFAULT of "AdminUserPassword"(admin)', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });
        
    });

    // AdminUserPassword Update test cases
    describe('Update:', function() {
    
        it('should return status 200 on post "AdminUserPassword":"ADMIN12345"', function(done) {
            var payload = '{"AdminUserPassword":"ADMIN12345"}';
//            verifyConfig = JSON.parse( payload ) ;
            verifyConfig = JSON.parse( expectDefault ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on GET "AdminUserPassword"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });
        
    });


    // AdminUserPassword Delete test cases
    describe('Delete', function() {
    
        it('should return status 403 when deleting AdminUserPassword', function(done) {
            var payload = '{"AdminUserPassword":"ADMIN12345"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: AdminUserPassword"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on GET "AdminUserPassword" after failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });
        
        it('should return status 400 on post "AdminUserPassword":null', function(done) {
            var payload = '{"AdminUserPassword":null}';
//            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: AdminUserPassword."} ;
//            verifyConfig = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyConfig = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"AdminUserPassword\":null is not valid."} ;
            verifyMessage = verifyConfig ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on GET "AdminUserPassword":null', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });
        
    });

    // AdminUserPassword Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to Name too long', function(done) {
            var payload = '{"AdminUserPassword":"'+ FVT.long16Name + 7 +'"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"A property value is not valid: Property: AdminUserPassword Value: name too long."};
//            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminUserPassword Value: \\\"'+ FVT.long16Name + 7 +'\\\"."}' );
//            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object:  Property: AdminUserPassword Value: '+ FVT.long16Name + 7 +'."}' );
            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0146","Message":"The value that is specified for the property is too long. Property: AdminUserPassword Value: '+ FVT.long16Name + 7 +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on GET "AdminUserPassword" after POST', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });


    });
    
    // AdminUserPassword Reset to Default
    describe('Is NOT A Reset to Default', function() {
// 109525
        it('should return status 400 on post "AdminUserPassword":""', function(done) {
            var payload = '{"AdminUserPassword":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = { "status":400,"Code":"CWLNA0116","Message":"A null argument is not allowed." } ;
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: AdminUserPassword Value: \"NULL\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return status 200 on post "AdminUserPassword":"admin"', function(done) {
            var payload = '{"AdminUserPassword":"admin"}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 400 on get, "AdminUserPassword":"admin"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getFailureCallback, verifyConfig, done)
        });
        
    });

});
