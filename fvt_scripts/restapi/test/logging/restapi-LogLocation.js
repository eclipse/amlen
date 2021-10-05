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

var uriObject = 'LogLocation/';
var expectDefault = '{"LogLocation": { \
  "AdminLog": { "Destination": "/var/messagesight/diag/logs/imaserver-admin.log", "LocationType": "file" }, \
  "ConnectionLog": { "Destination": "/var/messagesight/diag/logs/imaserver-connection.log", "LocationType": "file" }, \
  "DefaultLog": { "Destination": "/var/messagesight/diag/logs/imaserver-default.log", "LocationType": "file" }, \
  "MQConnectivityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-mqconnectivity.log", "LocationType": "file" }, \
  "SecurityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-security.log", "LocationType": "file" } \
}, "Version": "'+ FVT.version +'"}' ;

var deletedAdminLog = '{"LogLocation": { \
  "ConnectionLog": { "Destination": "/var/messagesight/diag/logs/imaserver-connection.log", "LocationType": "file" }, \
  "DefaultLog": { "Destination": "/var/messagesight/diag/logs/imaserver-default.log", "LocationType": "file" }, \
  "MQConnectivityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-mqconnectivity.log", "LocationType": "file" }, \
  "SecurityLog": { "Destination": "/var/messagesight/diag/logs/imaserver-security.log", "LocationType": "file" } \
}, "Version": "'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// LogLocation test cases
describe('LogLocation', function() {
this.timeout( FVT.defaultTimeout );

    // LogLocation Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "LogLocation"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // Name test cases
    describe('Name:', function() {
	
        // it('should return status 200 on post "LogLocation":Max Length Name', function(done) {
            // var payload = '{"LogLocation":"'+ FVT.long1024String +'"}';
            // verifyConfig = JSON.parse( payload ) ;
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "LogLocation":"Max Length Name"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        // });
		
	});

    // Destinatione test cases
    describe('Destination:', function() {
	
        // it('should return status 200 on post "LogLocation":Max Length Name', function(done) {
            // var payload = '{"LogLocation":"'+ FVT.long1024String +'"}';
            // verifyConfig = JSON.parse( payload ) ;
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "LogLocation":"Max Length Name"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        // });
		
	});

    // LocationType test cases
    describe('LocationType:', function() {
	
        // it('should return status 200 on post "LogLocation":Max Length Name', function(done) {
            // var payload = '{"LogLocation":"'+ FVT.long1024String +'"}';
            // verifyConfig = JSON.parse( payload ) ;
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "LogLocation":"Max Length Name"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        // });
		
	});

    // LogLocation Delete test cases
    describe('Delete:', function() {
	
        it('should return status 400 DELETE ALL LogLocation', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed DELETE ALL LogLocation', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 116319
        it('should return status 403 DELETE specific log', function(done) {
            verifyConfig = JSON.parse( deletedAdminLog ) ;
			verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for LogLocation object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'AdminLog', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, "LogLocation" after Fail DELETE AdminLog', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 POST AdminLog back', function(done) {
            var payload = '{"LogLocation":{ "AdminLog": { "Destination": "/var/messagesight/diag/logs/imaserver-admin.log", "LocationType": "file" }}}';
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":200,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Name Value: \"UpdatePluginsLog\"."} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST AdminLog back', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // LogLocation Error test cases
    describe('Error', function() {

        it('should return status 400 POST Name not in ENUM', function(done) {
            var payload = '{"LogLocation":{ "UpdatePluginsLog": {"Destination": "/var/messagesight/diag/logs/updatePlugins.log","LocationType":"file"}}}';
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Name Value: \"UpdatePluginsLog\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on Get after POST fails Name not in ENUM', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 POST bad Destination', function(done) {
            var payload = '{"LogLocation":{ "AdminLog": {"Destination": "/mnt/messagesight/diag/logs/updateAdmin.log","LocationType":"file"}}}';
            verifyConfig = JSON.parse( expectDefault ) ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: \"/mnt/messagesight/diag/logs/updateAdmin.log\"."} ;
//			verifyMessage = {"status":400,"Code":"CWLNA7295","Message":"The property value is not valid: Property: Destination Value: \"/mnt/messagesight/diag/logs/updateAdmin.log\"."} ;
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Destination Value: \"/mnt/messagesight/diag/logs/updateAdmin.log\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on Get after POST fails bad Destination', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

	});
	

});
