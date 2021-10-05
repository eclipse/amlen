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
var fs = require('fs')

var FVT = require('../restapi-CommonFunctions.js')

var verifyConfig = {} ;
var verifyMessage = {};

var uriObject = 'LTPAProfile/' ;
var expectDefault = '{"LTPAProfile": {},"Version": "'+ FVT.version +'"}' ;
// password is imasvtest - for reals!
var expectTestLTPAProfile = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPANewKey", "Password":"XXXXXX"}}}' ;
var expectMaxLenLTPAProfile =  '{"LTPAProfile":{ "'+ FVT.long256Name +'": {"KeyFileName": "'+ FVT.maxFileName255 +'","Password": "XXXXXX"}}}' ;

var expectAllLTPAProfiles = '{ "Version": "'+ FVT.version +'", "LTPAProfile": { \
 "TestLTPAProf": {  "KeyFileName": "LTPANewKey",  "Password": "XXXXXX"  }, \
 "'+ FVT.long256Name +'": {  "KeyFileName": "'+ FVT.maxFileName255 +'",  "Password": "XXXXXX" } }}' ;


// from SL testEnv.sh
var LDAP_URI = '10.47.65.22';
var LDAP_SERVER_1 = '10.47.65.22';
var LDAP_SERVER_1_PORT = 389 ;
var LDAP_SERVER_1_SSLPORT = 6636 ;
var LTPAWAS_IP = '10.47.65.22' ;
var MQSERVER1_IP = '10.47.65.37' ;
var MQSERVER2_IP = '10.47.65.22' ;
var MQKEY = 'AFfvt05' ;


//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,KeyFileName",
//        password, verify, overwrite

describe('LTPAProfile:', function() {
this.timeout( FVT.defaultTimeout );

    //  ====================   Get test cases  ====================  //
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "LTPAProf":{}}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	
	});

    //  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq keyfiles:', function() {

        it('should return status 200 on PUT "LTPAKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'LTPAKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LTPANewKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'LTPANewKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LTPAMaxLenNameKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

	});


    describe('Create:', function() {
// 113632
        it('should return status 200 on post "TestLTPAProf"', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPAKeyfile","Password":"imasvtest"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPAKeyfile","Password":"XXXXXX"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestLTPAProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 112982
        it('should return status 200 on re-PUT "LTPAMaxLenNameKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
        it('should return status 200 on post "MaxLength Name"', function(done) {
            var payload = '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"imasvtest"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"XXXXXX"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxLength Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('KeyFileName:', function() {

        it('should return status 200 on repost "TestLTPAProf"', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPANewKey","Password":"imasvtest"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPANewKey","Password":"XXXXXX"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, repost "TestLTPAProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


  describe('Error:', function() {

//  ====================  Error test cases  ====================  //
    describe('General:', function() {
//105422
        it('should return status 400 when trying to Delete All LTPAProfiles with POST', function(done) {
            var payload = '{"LTPAProfile":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"LTPAProfile\":null} is not valid."} ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"LTPAProfile\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All LTPAProfiles,  should be in use', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99554, 120951
        it('should return status 400 when trying to Update Invalid Keyword', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"Verify":"True"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0116","Message":"A null argument is not allowed."} ;
//??    	verifyMessage = {"Code":"CWLNA0132","Message":"The property value is not valid. Object: LTPAProfile Name: Verify Property: InvalidType Value: True or JSON_STRING."} ;
//			verifyMessage = {"Code":"CWLNA0132","Message":"The property value is not valid. Object: LTPAProfile Name: Verify Property: InvalidType Value: JSON_STRING."} ;
//			verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: LTPAProfile Name: TestLTPAProf Property: Verify Type: JSON_STRING"} ;
//			verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: LTPAProfile Name: TestLTPAProf Property: Verify"} ;
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: LTPAProfile Name: TestLTPAProf Property: Verify"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, after POST of Invalid Keyword', function(done) {
            verifyConfig = {"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPANewKey","Password":"XXXXXX"}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });

    describe('KeyFileName:', function() {

        it('should return status 400 on POST "KeyFileName":tooLong', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"'+ FVT.long256Name +'","Password":"itDoesNotMatter"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"'+ FVT.long256Name +'","Password":"XXXXXX"}}}' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: LTPAProfile Property: KeyFileName Value: '+ FVT.long256Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "KeyFileName":tooLong', function(done) {
		    verifyConfig = JSON.parse( expectTestLTPAProfile );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestLTPAProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "KeyFileName":"MissingFile"', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"MissingFile","Password":"itDoesNotMatter"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"MissingFile","Password":"XXXXXX"}}}' );
//			verifyMessage = {"Code":"CWLNA8279","Message":"Cannot find the key file specified. Make sure it has been imported."} ;
//			verifyMessage = {"Code":"CWLNA8279","Message":"Cannot find the key file specified. Make sure it has been uploaded."} ;
//			verifyMessage = {"Code":"CWLNA0190","Message":"LTPA key file is not valid, or the key file and password do not match."} ;
			verifyMessage = {"Code":"CWLNA0190","Message":"LTPA key file is not valid or missing, or the key file and password do not match."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "KeyFileName":"MissingFile"', function(done) {
		    verifyConfig = JSON.parse( expectTestLTPAProfile );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestLTPAProf', FVT.getSuccessCallback, verifyConfig, done);
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 114320
        it('should return status 400 on post update "256 Char Name LTPAProf", need to reupload', function(done) {
            var payload = '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"imasvtest", "Overwrite":true}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"XXXXXX" }}}' );
//			verifyMessage = {"status":400,"Code":"CWLNA8279","Message":"Cannot find the key file specified. Make sure it has been uploaded."}  ;
			verifyMessage = {"status":400,"Code":"CWLNA0190","Message":"LTPA key file is not valid or missing, or the key file and password do not match."}  ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "256 Char Name LTPAProf" need to reupload', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on re-PUT "LTPAMaxLenNameKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
//112980, 113507
        it('should return status 400 on RE-POST "256 Char Name LTPAProf" without Overwrite', function(done) {
            var payload = '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"imasvtest"}}}';
            verifyConfig = JSON.parse( expectMaxLenLTPAProfile );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8485","Message":"The \\"'+ FVT.maxFileName255 +'\\" already exists. Set Overwrite=True to update the existing configuration.\\n"}' ) ;
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8485","Message":"The \\"'+ FVT.maxFileName255 +'\\" already exists. Set Overwrite=True to update the existing configuration."}' ) ;
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8485","Message":"The \\"'+ FVT.maxFileName255 +'\\" already exists. Set Overwrite to true to update the existing configuration."}' ) ;
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA6201","Message":"The \\"'+ FVT.maxFileName255 +'\\" already exists. Set \\"Overwrite\\":true to update the existing configuration."}' ) ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST "256 Char Name LTPAProf" without Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post update "256 Char Name LTPAProf" with OVERWRITE', function(done) {
            var payload = '{"LTPAProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'","Password":"imasvtest", "Overwrite":true}}}';
            verifyConfig = JSON.parse( expectMaxLenLTPAProfile );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "256 Char Name LTPAProf" with OVERWRITE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

    describe('Password:', function() {

        it('should return status 200 on PUT "LTPANewKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'LTPANewKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 400 on post "TestLTPAProf" bad password', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPANewKey","Password":"WRONG!123_Password"}}}';
		    verifyConfig = JSON.parse( expectTestLTPAProfile );
//			verifyMessage = {"status":400, "Code":"CWLNA8484", "Message":"LTPA KeyFile is not valid, or the KeyFile and Password do not match.\n" } ;
//			verifyMessage = {"status":400, "Code":"CWLNA8484", "Message":"LTPA KeyFile is not valid, or the KeyFile and Password do not match." } ;
			verifyMessage = {"status":400, "Code":"CWLNA0190", "Message":"LTPA key file is not valid, or the key file and password do not match." } ;
			verifyMessage = {"status":400, "Code":"CWLNA0190", "Message":"LTPA key file is not valid or missing, or the key file and password do not match." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "TestLTPAProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

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
        it('should return status 200 on get, "LTPAProfile" persists', function(done) {
            verifyConfig = JSON.parse( expectAllLTPAProfiles ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

	
	//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting LTPAProfile "Max Length Name"', function(done) {
            verifyConfig = JSON.parse( '{"LTPAProfile":{"'+ FVT.long256Name +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"LTPAProfile":{"'+ FVT.long256Name +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code\":\"CWLNA0136\",\"Message\":\"The item or object cannot be found. Type: LTPAProfile Name: '+ FVT.long256Name +'","LTPAProfile":{"'+ FVT.long256Name +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done );
        });


        var objName = 'TestLTPAProf'
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"LTPAProfile":{"'+ objName +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "'+ objName +'" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "'+ objName +'" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"LTPAProfile":{"'+ objName +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code\":\"CWLNA0136\",\"Message\":\"The item or object cannot be found. Type: LTPAProfile Name: '+ objName +'","LTPAProfile":{"'+ objName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done );
        });
		
    });
	
});
