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

var uriObject = 'MQConnectivityEnabled/';

var expectDefault = '{"MQConnectivityEnabled":false,"Version":"'+ FVT.version +'"}' ;
var expectEnabled = '{"MQConnectivityEnabled":true,"Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};

// Test Cases Start Here

// MQConnectivityEnabled test cases
describe('MQConnectivityEnabled:', function() {
this.timeout( FVT.mqTimeout );

    // MQConnectivityEnabled Get test cases
    describe('Pristine Get:', function() {
    
        it('should return status 200 on get, DEFAULT of "MQConnectivityEnabled":false', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on get, DEFAULT of /status/MQConnectivity', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MQConnectivityEnabled Update test cases
    describe('Valid Range:', function() {
    
        it('should return status 200 on POST "MQConnectivityEnabled":true', function(done) {
            var payload = '{"MQConnectivityEnabled":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityEnabled":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 GET, ENABLED /status/MQConnectivity', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
// Now try to stop the service and check status
        it('should return status 200 on POST Server to Stop  "MQConnectivity"', function(done) {
            var payload = '{"Service":"MQConnectivity"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"stop", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityEnabled":true remains same', function(done) {
		    verifyConfig = JSON.parse( expectEnabled );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 GET, ENABLED, but stopped /status/MQConnectivity', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityStopped ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 200 on post "MQConnectivityEnabled":null (DEFAULT)', function(done) {
            var payload = '{"MQConnectivityEnabled":null}';
            verifyConfig = {"MQConnectivityEnabled":false} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MQConnectivityEnabled":null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on get, DEFAULT of /status/MQConnectivity', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityEnabled":true', function(done) {
            var payload = '{"MQConnectivityEnabled":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET reset "MQConnectivityEnabled":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 GET, reset ENABLED /status/MQConnectivity', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST RESTART IMAserver', function(done) {
            var payload = '{"Service":"Server"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after  RESTART IMAserver "MQConnectivityEnabled":true', function(done) {
		    this.timeout( FVT.REBOOT + FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.REBOOT + FVT.START_MQ );
			verifyConfig = JSON.parse( '{"MQConnectivityEnabled":true}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 400 GET, after restart ENABLED /status/mqconnectivity', function(done) {
            verifyConfig = { "status":400, "Code":"CWLNA0137","Message":"The REST API call: mqconnectivity is not valid."} ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/mqconnectivity', FVT.getFailureCallback, verifyConfig, done)
        });
        it('should return status 200 GET, after restart ENABLED /status/MQConnectivity', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityEnabled":false', function(done) {
            var payload = '{"MQConnectivityEnabled":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "MQConnectivityEnabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET, FALSE of /status/MQConnectivity', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST RESTART IMAserver', function(done) {
            var payload = '{"Service":"Server"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RESTART "MQConnectivityEnabled":false', function(done) {
		    this.timeout( FVT.REBOOT + FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.REBOOT + FVT.START_MQ  );
			verifyConfig = JSON.parse( '{"MQConnectivityEnabled":false}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET after RESTART FALSE /status/MQConnectivity', function(done) {
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    // MQConnectivityEnabled Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 on DELETE MQConnectivityEnabled', function(done) {
            verifyConfig = JSON.parse( expectDefault ); 
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: MQConnectivityEnabled"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed DELETE "MQConnectivityEnabled"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "MQConnectivityEnabled":"" ', function(done) {
            var payload = '{"MQConnectivityEnabled":""}';
            verifyConfig = JSON.parse( expectDefault ); 
//            verifyMessage = {"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: MQConnectivityEnabled Property: MQConnectivityEnabled Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed POST "MQConnectivityEnabled":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MQConnectivityEnabled Error test cases
    describe('Error:', function() {

        it('should return status 400 on POST  with "MQConnectivityEnabled" in URI', function(done) {
            var payload = '{"MQConnectivityEnabled":true}';
            verifyConfig = JSON.parse( expectDefault ); 
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/MQConnectivityEnabled is not valid."};
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/MQConnectivityEnabled is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed POST with URI "MQConnectivityEnabled" ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "MQConnectivityEnabled":"JUNK"', function(done) {
            var payload = '{"MQConnectivityEnabled":"JUNK"}';
            verifyConfig = JSON.parse( expectDefault ); 
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MQConnectivityEnabled Value: JUNK."} ;
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: MQConnectivityEnabled Property: MQConnectivityEnabled Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed POST "MQConnectivityEnabled":"JUNK"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "MQConnectivityEnabled":"true" (STRING)', function(done) {
            var payload = '{"MQConnectivityEnabled":"true"}';
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MQConnectivityEnabled Value: normal."};
            verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: MQConnectivityEnabled Property: MQConnectivityEnabled Type: InvalidType:JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on Get after failed POST "MQConnectivityEnabled":"true" (STRING)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // MQConnectivityEnabled Reset to Default
    describe('Reset to Default:', function() {
    
        it('should return status 200 on final POST "MQConnectivityEnabled":false', function(done) {
            var payload = '{"MQConnectivityEnabled":false}';
            verifyConfig = JSON.parse( expectDefault ); 
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on final GET "MQConnectivityEnabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 GET, FINAL ENABLED:false', function(done) {
			this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done)
        });

    });

});
