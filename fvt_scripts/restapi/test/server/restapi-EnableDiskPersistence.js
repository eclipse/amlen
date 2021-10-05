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

var objectDefault = true;
var uriObject = 'EnableDiskPersistence/';
var expectDefault = '{"EnableDiskPersistence":'+ objectDefault +',"Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// EnableDiskPersistence test cases
describe('EnableDiskPersistence:', function() {
this.timeout( FVT.defaultTimeout );

    // EnableDiskPersistence Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "EnableDiskPersistence"'+ objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // EnableDiskPersistence Update test cases
    describe('Update Valid Range{true]', function() {
	
        it('should return status 200 on post "EnableDiskPersistence":false', function(done) {
            var payload = '{"EnableDiskPersistence":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "EnableDiskPersistence":null (DEFAULT)', function(done) {
            var payload = '{"EnableDiskPersistence":null}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "EnableDiskPersistence":false', function(done) {
            var payload = '{"EnableDiskPersistence":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on post "EnableDiskPersistence":true', function(done) {
            var payload = '{"EnableDiskPersistence":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // EnableDiskPersistence Delete test cases
    describe('Delete', function() {
	
        it('should return status 403 when deleting EnableDiskPersistence', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
			verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: EnableDiskPersistence"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after DELETE EnableDiskPersistence fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on post "EnableDiskPersistence":""(Not DELETE anymore, invalid)', function(done) {
            var payload = '{"EnableDiskPersistence":""}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"EnableDiskPersistence\":null} is not valid."};
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"EnableDiskPersistence\":null is not valid."};
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_STRING"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":""(Not DELETE anymore, invalid)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // EnableDiskPersistence Error test cases
    describe('Error', function() {

        it('should return status 400 when trying to set to NORMAL', function(done) {
            var payload = '{"EnableDiskPersistence":"NORMAL"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: EnableDiskPersistence Value: NORMAL."};
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: EnableDiskPersistence Value: NORMAL."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "EnableDiskPersistence":"NORMAL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to "0"', function(done) {
            var payload = '{"EnableDiskPersistence":"0"}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: EnableDiskPersistence Value: 0." } ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "EnableDiskPersistence":"0"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 1', function(done) {
            var payload = '{"EnableDiskPersistence":1}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: EnableDiskPersistence Value: 1."};
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: EnableDiskPersistence Value: 1."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "EnableDiskPersistence":1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to -1', function(done) {
            var payload = '{"EnableDiskPersistence":-1}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: EnableDiskPersistence Value: -1."};
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: EnableDiskPersistence Value: -1."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: EnableDiskPersistence Property: EnableDiskPersistence Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "EnableDiskPersistence":-1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});
	
    // EnableDiskPersistence Reset to Default
    describe('Reset to Default', function() {

        it('should return status 200 on post "EnableDiskPersistence":null(Default='+ objectDefault +')', function(done) {
            var payload = '{"EnableDiskPersistence":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "EnableDiskPersistence":'+ objectDefault, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

});
