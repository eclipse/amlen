// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
var assert = require('assert')
var request = require('supertest')
var should = require('should')

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'PreSharedKey/';
var expectDefault = '{"PreSharedKey":"","Version":"'+ FVT.version +'"}' ;
// 134683 in the DIR it will be named, psk.csv, yet the REAL FILE NAME will be preserved for GET (unlike LDAP)
 var expectConfiged = '{"PreSharedKey":"PreSharedKey3","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// PreSharedKey test cases
describe('PreSharedKey', function() {
this.timeout( FVT.defaultTimeout );

    // PreSharedKey Get test cases
    describe('Get in Pristine', function() {
    
        it('should return status 200 on get, DEFAULT of "PreSharedKey"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    describe('Upload Keys', function() {
    
        it('should return status 200 on PUT "PreSharedKey1"', function(done) {
            sourceFile = '../common/psk_1024rows.txt';
            targetFile = 'PreSharedKey1';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on PUT "PreSharedKey2"', function(done) {
            sourceFile = '../common/psk_1024rows.txt';
            targetFile = 'PreSharedKey2';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on PUT "PreSharedKey3"', function(done) {
            sourceFile = '../common/psk_1024rows.txt';
            targetFile = 'PreSharedKey3';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on PUT "NOTaPreSharedKey"', function(done) {
//            sourceFile = 'test/security/restapi-PreSharedKey.js';
            sourceFile = 'mar400.wasltpa.keyfile';
            targetFile = 'NOTaPreSharedKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
        
    });

    // PreSharedKey Update test cases
    describe('PSK[""}', function() {
    
        it('should return status 200 on post "PreSharedKey":PreSharedKey1', function(done) {
            var payload = '{"PreSharedKey":"PreSharedKey1"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PreSharedKey":"PreSharedKey1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// This is a POST Delete actually
        it('should return status 200 on post "PreSharedKey":null (Default)', function(done) {
            var payload = '{"PreSharedKey":null}';
            verifyConfig = {"PreSharedKey":""} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PreSharedKey":null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on post "PreSharedKey":PreSharedKey2', function(done) {
            var payload = '{"PreSharedKey":"PreSharedKey2"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PreSharedKey":PreSharedKey2', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on RESTART Server and not loose config', function(done) {
            var payload = ( '{"Service":"Server"}' );
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart/", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RESTART Server and not loose PSK config ', function(done) {
            this.timeout( FVT.REBOOT + 3000 );
            FVT.sleep( FVT.REBOOT );
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey2"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
// can not be set to empty string if value exists
        it('should return status 400 on post "PreSharedKey":""', function(done) {
            var payload = '{"PreSharedKey":""}';
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey2"}' );
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: PreSharedKey Value: \"\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "PreSharedKey":"" fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 134808 - this really should fail, but there is NO OVERWRITE on Singletons 
        it('should return status 200 on post "PreSharedKey":PreSharedKey3 no overwrite', function(done) {
            var payload = '{"PreSharedKey":"PreSharedKey3"}';
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey2"}'  ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA8280","Message":"The certificate already exists. Set Overwrite to true to replace the existing certificate." };
//            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey3"}' ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PreSharedKey":PreSharedKey3, no overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 134808 - this really should pass, yet there is NO OVERWRITE on Singletons 
        it('should return status 404 on post "PreSharedKey":PreSharedKey3 with Overwrite', function(done) {
            var payload = '{"PreSharedKey":"PreSharedKey3", "Overwrite":true}';
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey3"}' ) ;
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
           verifyMessage = {"status":404, "Code":"CWLNA0111","Message":"The property name is not valid: Property: Overwrite." };
           FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on get, PreSharedKey3 in uri - wrong', function(done) {
            verifyConfig = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PreSharedKey/PreSharedKey3 is not valid." };
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'PreSharedKey3', FVT.getFailureCallback, verifyConfig, done)
        });        
        it('should return status 400 on get, psk.csv in uri - wrong', function(done) {
            verifyConfig = {"status":400, "Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/PreSharedKey/psk.csv is not valid." };
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'psk.csv', FVT.getFailureCallback, verifyConfig, done)
        });        
        it('should return status 200 on get, "PreSharedKey":PreSharedKey3  and Overwrite', function(done) {
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey3"}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });        
        
    });


    // PreSharedKey Delete test cases
    describe('Delete', function() {
// 134813 - Delete is NO LONGER valid, Set to NULL to remove PSK LIST
        it('should return status 403 when deleting PreSharedKey', function(done) {
            verifyConfig = JSON.parse( '{"PreSharedKey":"PreSharedKey3"}' ) ;
//            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."};
            verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for PreSharedKey object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status on get, "PreSharedKey" after first delete', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 134814 - Delete is NO LONGER valid
//        it('should return status 404 when deleting PreSharedKey twice', function(done) {
//            verifyConfig = JSON.parse( expectDefault ) ;
//            verifyMessage = {"status":404,"Code":"CWLNA0113","Message":"The requested item is not found."};
//            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
//        });
//        it('should return status 200 on get, "PreSharedKey":"" after second delete', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//        });
        
    });

    // PreSharedKey Error test cases
    describe('Error', function() {
// 134684
        it('should return status 404 "PreSharedKey":"LostInTheEther"', function(done) {
            var payload = '{"PreSharedKey":"LostInTheEther"}';
            verifyConfig = JSON.parse( expectConfiged ) ;
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: PreSharedKey Name: LostInTheEther" };
//            verifyMessage = {"status":404,"Code":"CWLNA6169","Message":"Cannot find the PreShared key file LostInTheEther. Make sure it has been uploaded." };
            verifyMessage = {"status":400,"Code":"CWLNA6169","Message":"Cannot find the PreShared key file LostInTheEther. Make sure it has been uploaded." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after POST "PreSharedKey":"LostInTheEther"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to NOTaPreSharedKey', function(done) {
            var payload = '{"PreSharedKey":"NOTaPreSharedKey"}';
            verifyConfig = JSON.parse( expectConfiged ) ;
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PreSharedKey Property: PreSharedKey Type: InvalidType:JSON_STRING"};
//            verifyMessage = {"status":400, "Code":"CWLNA0453","Message":"Configuration error."} ;
            verifyMessage = {"status":400, "Code":"CWLNA0465", "Message":"The PreSharedKey file is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, failed "PreSharedKey":NOTaPreSharedKey', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to false', function(done) {
            var payload = '{"PreSharedKey":false}';
            verifyConfig = JSON.parse( expectConfiged ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PreSharedKey Property: PreSharedKey Type: InvalidType:JSON_FALSE"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, failed "PreSharedKey":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 1', function(done) {
            var payload = '{"PreSharedKey":1}';
            verifyConfig = JSON.parse( expectConfiged ) ;
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: PreSharedKey Property: PreSharedKey Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, failed "PreSharedKey":1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // PreSharedKey Reset to Default
    describe('Reset to Default', function() {
    
// This will DELETE anything that might be there.
        it('should return status 200 on post "PreSharedKey":null (Default)', function(done) {
            var payload = '{"PreSharedKey":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PreSharedKey":null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
