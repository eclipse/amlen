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

var uriObject = 'TraceLevel/';
var objectMin = '1';
var objectDefault = '5';
var objectMax = '9';
// value can also be : '7,mqtt:5,admin:9'  -- just to make it interesting see DEFECT 93958 for explanation....
// trace components are @ server_utils/include/tracecomponent.h
var expectDefault = '{"TraceLevel":"'+ objectDefault +'","Version":"'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// TraceLevel test cases
describe('TraceLevel:', function() {
this.timeout( FVT.defaultTimeout );

    // TraceLevel Get test cases
    describe('Pristine Get:', function() {
    
        it('should return status 200 on get, DEFAULT of "TraceLevel":'+objectDefault, function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceLevel Update test cases
    describe('Simple Range:', function() {
    
        it('should return status 200 on post "TraceLevel":"'+objectMax+'"', function(done) {
            var payload = '{"TraceLevel":"'+ objectMax +'"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+objectMax+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// Defect 91335 (having Version  passed)    
        it('should return status 200 on post "TraceLevel":"'+objectDefault+'"', function(done) {
            var payload = expectDefault;
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+objectDefault+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceLevel":"2"', function(done) {
            var payload = '{"TraceLevel":"2"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceLevel":null(DEFAULT)', function(done) {
            var payload = '{"TraceLevel":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"5" (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceLevel":"8"', function(done) {
            var payload = '{"TraceLevel":"8"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"8"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceLevel Update test cases
    describe('Complex Range:', function() {
    
        it('should return status 200 on post "TraceLevel":"3,mqtt:9"', function(done) {
            var payload = '{"TraceLevel":"3,mqtt:9"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"3,mqtt:9"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceLevel":"6,mqtt=2,Admin:9"', function(done) {
            var payload = '{"TraceLevel":"6,mqtt:2,Admin:9"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"6,mqtt:2,Admin:9"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "TraceLevel":"1,mqtt=3,Admin:5,Engine:7,JMS=9"', function(done) {
            var payload = '{"TraceLevel":"1,mqtt=3,Admin:5,Engine:7,JMS=9"}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"1,mqtt=3,Admin:5,Engine:7,JMS=9"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		

        it('should return status 200 on RESTART Server and not loose config', function(done) {
            var payload = ( '{"Service":"Server"}' );
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart/", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RESTART Server and not loose config "TraceLevel":"1,mqtt=3,Admin:5,Engine:7,JMS=9"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
		    FVT.sleep( FVT.REBOOT );
		    verifyConfig = {"TraceLevel":"1,mqtt=3,Admin:5,Engine:7,JMS=9"} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		

        it('should return status 200 on post "TraceLevel":"'+objectMin+'"', function(done) {
            var payload = ( '{"TraceLevel":"'+ objectMin +'"}' );
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // TraceLevel Delete test cases
    describe('Delete:', function() {
    
        it('should return status 403 when deleting TraceLevel', function(done) {
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: TraceLevel"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // TraceLevel Error test cases
    describe('Error:', function() {

        it('should return status 400 on post "TraceLevel":"" (Invalid)', function(done) {
            var payload = '{"TraceLevel":""}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
//?            verifyMessage = {"status":400, "Code":"CWLNA0118","Message":"The properties are not valid."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"\"\"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 400 on POST (semicolon bad) "TraceLevel":"3;mqtt:9"', function(done) {
            var payload = '{"TraceLevel":"3;mqtt:9"}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"3;mqtt:9\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+ objectMin +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 400 on POST (inverted component:level) "TraceLevel":"3,9:admin"', function(done) {
            var payload = '{"TraceLevel":"3,9:admin"}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"3,9:admin\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TraceLevel":"'+ objectMin +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to NORMAL', function(done) {
            var payload = '{"TraceLevel":"NORMAL"}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"NORMAL\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to "0", out of bounds', function(done) {
            var payload = '{"TraceLevel":"0"}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"0\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to -1, NUMBER out of bounds, not String', function(done) {
            var payload = '{"TraceLevel":-1}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: -1."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceLevel Property: InvalidType:JSON_INTEGER Value: -1."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceLevel Property: TraceLevel Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 10, out of bounds', function(done) {
            var payload = '{"TraceLevel":"10"}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: \"10\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to set to 10, Number not a String', function(done) {
            var payload = '{"TraceLevel":10}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: 10."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceLevel Property: InvalidType:JSON_INTEGER Value: 10."};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceLevel Property: TraceLevel Type: InvalidType:JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 400 when trying to set to 1.0, number yet not integer in range', function(done) {
            var payload = '{"TraceLevel":1.0}';
            verifyConfig = JSON.parse( '{"TraceLevel":"'+ objectMin +'"}' );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: TraceLevel Value: 1.0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: Singleton Name: TraceLevel Property: InvalidType:JSON_REAL Value: 1.0."};
//            verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property type is not valid. Object: Singleton Name: TraceLevel Property: TraceLevel Type: InvalidType:JSON_REAL"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Singleton Name: TraceLevel Property: TraceLevel Type: InvalidType:JSON_REAL"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "TraceLevel":"'+objectMin+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    // TraceLevel Reset to Default
    describe('Reset to Default', function() {
        it('should return status 200 on post "TraceLevel":null Default is"'+objectDefault+'"', function(done) {
            var payload = '{"TraceLevel":null}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TraceLevel":'+objectMin, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
