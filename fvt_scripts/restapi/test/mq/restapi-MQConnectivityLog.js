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

var uriObject = 'MQConnectivityLog/';
var objectMin = 'MIN' ;
var objectDefault = 'NORMAL';
var objectMax = 'MAX' ;
var expectDefault = '{"MQConnectivityLog":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = '';
var verifyMessage = {};


// Test Cases Start Here

// MQConnectivityLog test cases
describe('MQConnectivityLog', function() {
this.timeout( FVT.mqTimeout );

    // MQConnectivityLog Get test cases
    describe('Pristine Get:', function() {
    
        it('should return status 200 on GET DEFAULT of "MQConnectivityLog":"'+ objectDefault+'"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 124623
		it('should SAVE the currect state of MQConnectivityEnabled to reset at end', function(done){
		    verifyConfig = { "MQConnectivityEnabled":false } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MQConnectivityEnabled", FVT.getMQConnStateCallback, verifyConfig, done)
		});
    
        it('should return status 200 on POST "MQConnectivityEnabled":false', function(done) {
            var payload = '{"MQConnectivityEnabled":false}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MQ status after Enabled:false', function(done) {
		    this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
		    verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET "MQConnectivityEnabled":false', function(done) {
            verifyConfig = {"MQConnectivityEnabled":false} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MQConnectivityEnabled", FVT.getSuccessCallback, verifyConfig, done)
        });

	});

    // MQConnectivityLog Update test cases
    describe('Value['+objectMin+'-'+objectDefault+'-'+objectMax+']:', function() {

        it('should return status 200 on POST "MQConnectivityLog":"'+objectMax+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectMax+'"}';
            verifyConfig = JSON.parse( payload ) ;
			if ( FVT.A1_TYPE == "DOCKER" ) {
// 124623
//				verifyMessage = { "status":400, "Code":"CWLNA0316","Message":"The IBM IoT MessageSight server could not connect to MQ Connectivity service."} ;
    			verifyMessage = { "status":200, "Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
			} else { 
    			verifyMessage = { "status":200, "Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
			}
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":"'+objectMax+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityEnabled":true', function(done) {
            var payload = '{"MQConnectivityEnabled":true}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MQ status after Enabled:true', function(done) {
		    this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
		    verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET "MQConnectivityEnabled":true', function(done) {
            verifyConfig = {"MQConnectivityEnabled":true};
            FVT.makeGetRequest( FVT.uriConfigDomain+"MQConnectivityEnabled", FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "MQConnectivityLog":"'+objectMin+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectMin+'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":"'+objectMax+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 200 on POST "MQConnectivityLog":"'+objectDefault+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectDefault+'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":"'+objectDefault+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityLog":"'+objectMin+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectMin+'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityLog":null (DEFAULT)', function(done) {
            var payload = '{"MQConnectivityLog":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "MQConnectivityLog":"'+objectMin+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectMin+'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MQConnectivityLog":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST RESTART IMAserver', function(done) {
            var payload = '{"Service":"Server"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MQ status after RESTART', function(done) {
		    this.timeout( FVT.REBOOT + FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.REBOOT + FVT.START_MQ );
			verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on GET after RESTART IMAserver "MQConnectivityLog":"'+objectMin+'"', function(done) {
			verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    // MQConnectivityLog Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 on DELETE MQConnectivityLog', function(done) {
            verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: MQConnectivityLog"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed POST "MQConnectivityLog"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// This no longer resset to default?
        it('should return status 400 on POST "MQConnectivityLog":""', function(done) {
            var payload = '{"MQConnectivityLog":""}';
            verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
//            verifyMessage = {"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MQConnectivityLog Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after failed POST Delete "MQConnectivityLog"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // MQConnectivityLog Error test cases
    describe('Error:', function() {
// defect 96932
        it('should return status 400 on POST  with "MQConnectivityLog" in URI', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectMax+'"}';
            verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/MQConnectivityLog is not valid."};
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/MQConnectivityLog is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after fail POST URI "MQConnectivityLog"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "MQConnectivityLog":"JUNK"', function(done) {
            var payload = '{"MQConnectivityLog":"JUNK"}';
            verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MQConnectivityLog Value: \"JUNK\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed POST "MQConnectivityLog":"JUNK"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "MQConnectivityLog":"normal"', function(done) {
            var payload = '{"MQConnectivityLog":"normal"}';
            verifyConfig = JSON.parse( '{"MQConnectivityLog":"'+objectMin+'"}' );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MQConnectivityLog Value: \"normal\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed POST "MQConnectivityLog":"normal"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
    // MQConnectivityLog Reset to Default
    describe('Reset to Default:', function() {
// THIS IS Taking a REALLY long time in RPM automation - increase mqTimeout to 55secs    
        it('should return status 200 on FINAL POST "MQConnectivityLog":"'+objectDefault+'"', function(done) {
            var payload = '{"MQConnectivityLog":"'+objectDefault+'"}';
            verifyConfig = JSON.parse( expectDefault ); 
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on final GET "MQConnectivityLog":"'+objectDefault+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on FINAL POST "MQConnectivityEnabled":'+ FVT.MQConnState, function(done) {
            var payload = '{"MQConnectivityEnabled":'+ FVT.MQConnState +'}';
            verifyConfig = JSON.parse( payload ); 
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MQ status after Final Reset Enabled', function(done) {
		    this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
			if ( FVT.MQConnState == true ) {
			    verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning );
			} else {
			    verifyConfig = JSON.parse( FVT.expectMQConnectivityDefault );
			}
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/MQConnectivity", FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on final GET "MQConnectivityEnabled":"'+ FVT.MQConnState, function(done) {
            verifyConfig = JSON.parse( '{"MQConnectivityEnabled":'+ FVT.MQConnState +'}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'MQConnectivityEnabled', FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
