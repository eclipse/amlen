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
var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLxxx","Message":"blah blah"}

var uriObject = 'MessageHub/';
var expectDefault = '{"MessageHub":{"DemoHub":{"Description": "Demo Message Hub."}},"Version": "' + FVT.version + '"}';

var expectAllMessageHubs = '{ "Version": "' + FVT.version + '", "MessageHub": { \
 "DemoHub": {  "Description": "Demo Message Hub."  }, \
 "TestMsgHub": {  "Description": ""  }, \
 "'+ FVT.long256Name +'": {  "Description": ""}}}' ;
 
//  ====================  Test Cases Start Here  ====================  //

describe('MessageHub', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "MessageHub":"DemoHub":{..}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//91335
		it('should return status 200 on POST roundtrip', function(done) {
            var payload = expectDefault;
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET roundtrip', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Add&Update Description:', function() {

        it('should return status 200 on post "No description"', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{"Description": "The TEST Message Hub."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{"Description": "The NEW TEST Message Hub."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST RESET "Description":"" ', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":{"Description": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET POST RESET "Description":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Valid Range test cases  ====================  //
    describe('Name Valid Range:', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"MessageHub":{"' + FVT.long256Name + '":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePOST "Max Length Name" (NOOP)', function(done) {
            var payload = '{"MessageHub":{"' + FVT.long256Name + '":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after REPOST "Max Length Name" (NOOP)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"MessageHub":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//98284
        it('should return status 200 on POST to reset "Description":null ', function(done) {
            var payload = '{"MessageHub":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse( '{"MessageHub":{"'+ FVT.long256Name +'":{"Description":""}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get to reset "Description":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

      describe('Config Persists:', function() {
	
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
        it('should return status 200 on get, "MessageHubs" persists', function(done) {
            verifyConfig = JSON.parse( expectAllMessageHubs) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

      });


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"MessageHub":{"'+ FVT.long256Name +'":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"MessageHub":{"'+ FVT.long256Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: '+ FVT.long256Name +'","MessageHub":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

// 96657
        it('should return status 400 when POST DELETE of "TestMsgHub"', function(done) {
            var payload = {"MessageHub":{"TestMsgHub":null}} ;
            verifyConfig = {"MessageHub":{"TestMsgHub":{"Description": ""}}};
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestMsgHub\":null is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: \"TestMsgHub\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST DELETE, "TestMsgHub" is NOT gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        // it('should return status 404 on GET after POST DELETE, "TestMsgHub" not found', function(done) {
            // verifyConfig = {"status":404,"MessageHub":{"TestMsgHub":{}}} ;
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestMsgHub', FVT.verify404NotFound, verifyConfig, done);
        // });
// Until Post Delete Works, then change this to REDELETE
        it('should return status 200 when deleting "TestMsgHub"', function(done) {
            var payload = '{"MessageHub":{"TestMsgHub":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestMsgHub', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestMsgHub" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestMsgHub" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"MessageHub":{"TestMsgHub":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: TestMsgHub","MessageHub":{"TestMsgHub":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestMsgHub', FVT.verify404NotFound, verifyConfig, done);
        });
		
    });

//  ====================  Error test cases  ====================  //
    describe('Error:', function() {

        it('should return status 400 when trying to Delete All Message Hubs, just bad form', function(done) {
            var payload = '{"MessageHub":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"MessageHub\":null} is not valid." };
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"MessageHub\":null is not valid." };
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"MessageHub\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Message Hubs, DemoHub should be in use', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 97256
        it('should return status 400 when trying to POST Message Hub with Name Too Long', function(done) {
            var payload = '{"MessageHub":{"'+ FVT.long257Name +'":{"Description":"Name too long, DO NOT Create"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: MessageHub Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Message Hub with Name Too Long was not created', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"MessageHub":{"'+ FVT.long257Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: '+ FVT.long257Name +'","MessageHub":{"'+ FVT.long257Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });
//97256, 99808
        it('should return status 400 when trying to POST Message Hub with Description Too Long', function(done) {
            var payload = '{"MessageHub":{"DescriptionTooLong":{"Description":"'+ FVT.long1025String +'"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0132","Message":"The property value is not valid: Property: Description Value: '+ FVT.long1025String +'." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: MessageHub Property: Description Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Message Hub with Description Too Long was not created', function(done) {
//           verifyConfig = JSON.parse( '{"status":404,"MessageHub":{"DescriptionTooLong":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: DescriptionTooLong","MessageHub":{"DescriptionTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'DescriptionTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
//97706 - getting 113, too generic.  138 would be best possibly, just don't like 113.
// 100367, 120951
        it('should return status 400 when trying to POST Message Hub with Invalid parameter Update', function(done) {
            var payload = '{"MessageHub":{"BadParamUpdate":{"Description":"Update is not valid EXPOSED key","Update":true}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0113","Message":"The requested item is not found." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0132","Message":"The property value is not valid: Property: Update Value: true." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessageHub Name: BadParamUpdate Property: Update" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessageHub Name: BadParamUpdate Property: Update" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Message Hub BadParamUpdate was not created', function(done) {
//            verifyConfig = {"status":404,"MessageHub":{"BadParamUpdate":{}}};
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: BadParamUpdate","MessageHub":{"BadParamUpdate":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamUpdate", FVT.verify404NotFound, verifyConfig, done);
        });
// 100367, 120951
        it('should return status 40o when trying to POST Message Hub with Invalid parameter Updated', function(done) {
            var payload = '{"MessageHub":{"BadParamUpdateD":{"Description":"Updated is not valid key","Updated":true}}}';
		    verifyConfig = JSON.parse( payload );
//	98136		verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0132","Message":"The property value is not valid: Property: Updated Value: true." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Updated." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessageHub Name: BadParamUpdateD Property: Updated" }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: MessageHub Name: BadParamUpdateD Property: Updated" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Message Hub BadParamUpdateD was not created', function(done) {
//            verifyConfig = {"status":404,"MessageHub":{"BadParamUpdateD":{}}};
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: MessageHub Name: BadParamUpdateD","MessageHub":{"BadParamUpdateD":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamUpdateD", FVT.verify404NotFound, verifyConfig, done);
        });
		
    });
    
});
