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
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var verifyConfig = {} ;
var verifyMessage = {};

var uriObject = 'OAuthProfile/' ;
var expectDefault = '{"OAuthProfile": {},"Version": "'+ FVT.version +'"}' ;
//99548
var expectTestOAuthProfile = '{"OAuthProfile":{"TestOAuthProf":{"ResourceURL":"http://t.co","AuthKey":"access_token","UserInfoKey":"","KeyFileName":"","UserInfoURL":null,"GroupInfoKey":""}}}' ;
var expectMaxNameOAuthProfile = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"ResourceURL":"'+ FVT.long2048HTTPS +'","AuthKey":"'+ FVT.long256Name +'","UserInfoKey":"U'+ FVT.long254Name +'","KeyFileName":"'+ FVT.maxFileName255 +'","UserInfoURL":"'+ FVT.long2048HTTPS +'","GroupInfoKey":"G'+ FVT.long254Name +'"}}}' ;

var expectAllOAuthProfiles = '{ "Version": "'+ FVT.version +'", "OAuthProfile": { \
"TestOAuthProf":{"ResourceURL":"'+ FVT.long2048HTTP +'","AuthKey":"access_token","UserInfoKey":"","KeyFileName":"","UserInfoURL":null,"GroupInfoKey":""}, \
"'+ FVT.long256Name +'":{"ResourceURL":"'+ FVT.long2048HTTP +'","AuthKey":"OAuthKey","UserInfoKey":"","KeyFileName":"","UserInfoURL":null,"GroupInfoKey":""}}}' ;

//  ====================  Test Cases Start Here  ====================  //

describe('OAuthProfile:', function() {
this.timeout( FVT.defaultTimeout );

    //  ====================   GET test cases  ====================  //
    describe('GET - verify in Pristine State:', function() {
        it('should return status 200 on GET, DEFAULT of "OAuthProf":{}}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	
	});

    //  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq keyfiles:', function() {

        it('should return status 200 on PUT "OAuthKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "OAuthNewKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthNewKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "OAuthMaxLenNameKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

	});

/* PASSWORD?  DO NOT HAVE ONE? */
/*
 THINGS TO ADD:   Overwrite:false,true 		
*/

    describe('Create:', function() {

        it('should return status 200 on POST "TestOAuthProf"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{ "ResourceURL":"'+ FVT.long2048HTTP +'" }}}';
            verifyConfig = JSON.parse( '{"OAuthProfile":{"TestOAuthProf":{"ResourceURL":"'+ FVT.long2048HTTP +'","AuthKey":"access_token"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116863
        it('should return status 200 on POST "MaxLength Name"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"ResourceURL":"'+ FVT.long2048HTTP +'","AuthKey":"OAuthKey"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "MaxLength Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
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
        it('should return status 200 on get, "OAuthProfile" persists', function(done) {
            verifyConfig = JSON.parse( expectAllOAuthProfiles ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

			

    describe('ResourceURL[http,https]R:', function() {
// 105591, 116863
        it('should return status 200 on repost "TestOAuthProf"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"ResourceURL":"http://t.co","AuthKey":"authority_token"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET updated "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on repost "MaxLenName OAuthProfile"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"ResourceURL":"'+ FVT.long2048HTTPS +'","KeyFileName":"'+ FVT.maxFileName255 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET updated "MaxLenName OAuthProfile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

    describe('KeyFileName[""]:', function() {
//99513
        it('should return status 200 on POST add KeyFileName to "TestOAuthProf"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf" add KeyFile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99513, 117702
        it('should return status 200 on POST new Keyfile "TestOAuthProf"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthNewKey"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf" new KeyFile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePUT "OAuthKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on rePOST "TestOAuthProf" since it was replaced', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on reGET, "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePUT2 "OAuthKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 400 on rePOST "TestOAuthProf" without overwrite', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile"}}}';
            verifyConfig = JSON.parse(payload);
			verifyMessage = {"status":400,"Code":"CWLNA6201","Message":"The \"OAuthKeyfile\" already exists. Set \"Overwrite\":true to update the existing configuration."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on reGET, "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePOST "TestOAuthProf" with overwrite', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile","Overwrite":true}}}';
            verifyConfig = JSON.parse( '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on reGET, "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116862

        it('should return status 200 on POST "TestOAuthProf" KeyFile:""', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":""}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf" KeyFile:""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePUT3 "OAuthKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on POST "TestOAuthProf" KeyFile:"OAuthKeyfile"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"OAuthKeyfile"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf" KeyFile:"OAuthKeyfile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "TestOAuthProf" KeyFile RESET to Default', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":null}}}';
            verifyConfig = JSON.parse( '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":""}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestOAuthProf" KeyFile RESET to Default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
	
	});

    describe('AuthKey[access_token]256:', function() {
//99513 116711
        it('should return status 200 on POST update "256 AuthKey"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"AuthKey":"'+ FVT.long256Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "256 AuthKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST AuthKey RESET to Default', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"AuthKey":null}}}';
            verifyConfig = JSON.parse( '{"OAuthProfile":{"TestOAuthProf":{"AuthKey":"access_token"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET AuthKey RESET to Default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('UserInfoURL[http,https]:', function() {
//99513, 116711
        it('should return status 200 on POST update "UserInfoURL":http', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"UserInfoURL":"'+ FVT.long2048HTTP +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL":http', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116863
        it('should return status 200 on POST update "UserInfoURL":https', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"UserInfoURL":"'+ FVT.long2048HTTPS +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL":https', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePUT3 "OAuthKeyfile"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = 'OAuthKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on POST TestOAuthProf "UserInfoURL":https with Keyfile', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":"https://needKeyFile.net", "KeyFileName":"OAuthKeyfile"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL":https', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116865 - Empty String now invalid...
//x 113388 NOW THIS IS VALID..I can have a KeyFileName and not have an HTTPS URL to use it.
// FUTURE: R. said should be relationship between (if) UserInfoURL and (then must be) UserInfoKey (? or GroupInfoKey)
        it('should return status 200 on POST TestOAuthProf "UserInfoURL":"" leave keyfile', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":""}}}';
//x            verifyConfig = {"OAuthProfile":{"TestOAuthProf":{ "ResourceURL": "http://t.co", "AuthKey": "access_token", "KeyFileName": "OAuthKeyfile", "UserInfoURL": "https://needKeyFile.net", "UserInfoKey": "", "GroupInfoKey": ""}}} ;
            verifyConfig = {"OAuthProfile":{"TestOAuthProf":{ "ResourceURL": "http://t.co", "AuthKey": "access_token", "KeyFileName": "OAuthKeyfile", "UserInfoURL": "", "UserInfoKey": "", "GroupInfoKey": ""}}} ;
//x            verifyMessage = {"status": 400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserInfoURL Value: \"NULL\"."};
//x            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig,  done);
        });
        it('should return status 200 on GET, "UserInfoURL":"" leave keyfile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 400 on POST TestOAuthProf "UserInfoURL":null leave keyfile', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":null}}}';
            verifyConfig = {"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":null, "ResourceURL": "http://t.co", "AuthKey": "access_token", "KeyFileName": "OAuthKeyfile", "UserInfoKey": "", "GroupInfoKey": ""}}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL":null leave keyfile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116862
        it('should return status 200 on POST TestOAuthProf "UserInfoURL":http', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":"http://oauth.org","KeyFileName":null}}}';
            verifyConfig = {"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":"http://oauth.org","KeyFileName":""}}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL":http', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 116711
        it('should return status 200 on POST TestOAuthProf "UserInfoURL" RESET to Default', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoURL":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "UserInfoURL" RESET to Default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('UserInfoKey[null]256:', function() {
//99513
        it('should return status 200 on POST add "UserInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"UserInfoKey":"U'+ FVT.long254Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET MaxLen "UserInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on POST update "UserInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoKey":"OAuthUserInfoKey"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET updated "UserInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on POST RESET to Default "UserInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoKey":null}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET "UserInfoKey" RESET to Default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('GroupInfoKey[null]256:', function() {
//99513  105592
        it('should return status 200 on POST add MaxLen "GroupInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"G'+ FVT.long254Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MaxLen "GroupInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST update "GroupInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"GroupInfoKey":"OAuthGroupInfoKey"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "GroupInfoKey":"groupkey"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"GroupInfoKey":"groupkey"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupInfoKey":"groupkey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "GroupInfoKey":null RESET to Default', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"GroupInfoKey":null}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupInfoKey":null RESET to Default', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});



//  ====================  Error test cases  ====================  //
  describe('Error:', function() {
    describe('General:', function() {

        it('should return status 400 when trying to Delete All OAuthProfiles with POST (Bad Form)', function(done) {
            var payload = '{"OAuthProfile":null}';
            verifyConfig = JSON.parse( expectMaxNameOAuthProfile );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"OAuthProfile\":null} is not valid."} ;
			verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"OAuthProfile\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET #1 after failed Delete All OAuthProfiles with POST (Bad Form)', function(done) {
            verifyConfig = JSON.parse( expectMaxNameOAuthProfile );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET #2 after failed Delete All OAuthProfiles with POST (Bad Form)', function(done) {
            verifyConfig = JSON.parse( expectTestOAuthProfile );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestOAuthProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to Update Invalid Keyword', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UsePost":"True"}}}';
		    verifyConfig = JSON.parse( expectTestOAuthProfile );
//			verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: OAuthProfile Name: TestOAuthProf Property: UsePost Type: JSON_STRING"} ;
//			verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: OAuthProfile Name: TestOAuthProf Property: UsePost"} ;
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: OAuthProfile Name: TestOAuthProf Property: UsePost"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, after POST of Invalid Keyword', function(done) {
		    verifyConfig = JSON.parse( expectTestOAuthProfile );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });
  

    describe('ResourceURL:', function() {

        it('should return status 400 on POST "2049 ResourceURL"', function(done) {
            var payload = '{"OAuthProfile":{"TooLongURL":{"ResourceURL":"'+ FVT.long2049HTTPS +'"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: ResourceURL Name: '+ FVT.long2049HTTPS +'."}' );
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0112","Message":"The property value is not valid: Property: ResourceURL Value: \\\"'+ FVT.long2049HTTPS +'\\\"."}' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: OAuthProfile Property: ResourceURL Value: '+ FVT.long2049HTTPS +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "2049 ResourceURL"', function(done) {
//            verifyConfig = {"status":404,"OAuthProfile":{"TooLongURL":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: TooLongURL","OAuthProfile":{"TooLongURL":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TooLongURL', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "ResourceURL":"ftp"', function(done) {
            var payload = '{"OAuthProfile":{"NoFTPOAuthProf":{"ResourceURL":"ftp://t.co"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: ResourceURL Value: \"ftp://t.co\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST fails "ResourceURL":"ftp"', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: NoFTPOAuthProf","OAuthProfile":{"NoFTPOAuthProf":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoFTPOAuthProf', FVT.verify404NotFound, verifyConfig, done);
        });

	});

    describe('KeyFileName:', function() {

        it('should return status 400 on POST "KeyFileName":"LostKeyFile"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"KeyFileName":"LostKeyFile"}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
//            verifyMessage = JSON.parse( '{"Code":"CWLNA8279","Message":"Cannot find the key file specified. Make sure it has been imported."}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8279","Message":"Cannot find the key file specified. Make sure it has been uploaded."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA6185","Message":"Cannot find the key file LostKeyFile. Make sure it has been uploaded."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST Fails "KeyFileName":"LostKeyFile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on PUT "OAuthMaxLenNameKey"', function(done) {
		    sourceFile = 'mar400.wasltpa.keyfile';
			targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 400 on POST "KeyFileName" overwrite:0', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"KeyFileName":"'+ FVT.maxFileName255 +'", "Overwrite":0}}}';
            verifyConfig = JSON.parse( expectMaxNameOAuthProfile );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8485","Message":"The \\\"'+ FVT.maxFileName255 +'\\\" already exists. Set Overwrite:true to update the existing configuration."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: OAuthProfile Name: '+ FVT.long256Name +' Property: Overwrite Type: JSON_INTEGER"}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST Fails "KeyFileName" overwrite:0', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('AuthKey:', function() {
// 116878
        it('should return status 400 on POST "257 AuthKey"', function(done) {
            var payload = '{"OAuthProfile":{"TooLongAuthKey":{"ResourceURL":"http://AuthKeyTooLong.edu", "AuthKey":"'+ FVT.long257Name +'"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: AuthKey Name: '+ FVT.long257Name +'."}' );
            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: OAuthProfile Property: AuthKey Value: '+ FVT.long257Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "257 AuthKey"', function(done) {
//            verifyConfig = {"status":404,"OAuthProfile":{"TooLongAuthKey":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: TooLongAuthKey","OAuthProfile":{"TooLongAuthKey":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TooLongAuthKey', FVT.verify404NotFound, verifyConfig, done);
        });
// Control Chars are below x'20, comma or Lead?Trail Space
        it('should return status 400 on POST "No,AuthKey"', function(done) {
            var payload = '{"OAuthProfile":{"NoCommaAuthKey":{"AuthKey":"No,AuthKey"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "No,AuthKey"', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: NoCommaAuthKey","OAuthProfile":{"NoCommaAuthKey":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoCommaAuthKey', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "NoTrailSpaceAuthKey"', function(done) {
            var payload = '{"OAuthProfile":{"NoTrailSpaceAuthKey":{"AuthKey":"NoTrailSpaceAuthKey "}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: NoTrailSpaceAuthKey ." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "NoTrailSpaceAuthKey"', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: NoTrailSpaceAuthKey","OAuthProfile":{"NoTrailSpaceAuthKey":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoTrailSpaceAuthKey', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "NoLeadSpaceAuthKey"', function(done) {
            var payload = '{"OAuthProfile":{"NoLeadSpaceAuthKey":{"AuthKey":" NoLeadSpaceAuthKey"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "NoLeadSpaceAuthKey"', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: NoLeadSpaceAuthKey","OAuthProfile":{"NoLeadSpaceAuthKey":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoLeadSpaceAuthKey', FVT.verify404NotFound, verifyConfig, done);
        });
		
        it('should return status 400 on POST remove "AuthKey":""', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"AuthKey":""}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: AuthKey Value: \"\"\"\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET "AuthKey":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('UserInfoURL:', function() {
// 116900
        it('should return status 400 on POST "2049 UserInfoURL"', function(done) {
            var payload = '{"OAuthProfile":{"TooLongURL":{"ResourceURL":"http://UserUrlTooLong.jsp","UserInfoURL":"'+ FVT.long2049HTTPS +'"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: UserInfoURL Name: '+ FVT.long2049HTTPS +'."}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserInfoURL Value: \\\"'+ FVT.long2049HTTPS +'\\\"."}' );
//            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: OAuthProfile Name: TooLongURL Property: UserInfoURL"} ;
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserInfoURL Value: \\\"' + FVT.long2049HTTPS +'\\\"."}' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: OAuthProfile Property: UserInfoURL Value: '+ FVT.long2049HTTPS +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "2049 UserInfoURL"', function(done) {
//            verifyConfig = {"status":404,"OAuthProfile":{"TooLongURL":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: TooLongURL","OAuthProfile":{"TooLongURL":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TooLongURL', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST "ftp UserInfoURL"', function(done) {
            var payload = '{"OAuthProfile":{"NoFTPURL":{"UserInfoURL":"ftp://No.ftp.url.edu"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0138","Message":"The property name is invalid. Object: OAuthProfile Name: NoFTPURL Property: UserInfoURL"}' );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: UserInfoURL Value: \"ftp://No.ftp.url.edu\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST Fails "ftp UserInfoURL"', function(done) {
//            verifyConfig = {"status":404,"OAuthProfile":{"NoFTPURL":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: NoFTPURL","OAuthProfile":{"NoFTPURL":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoFTPURL', FVT.verify404NotFound, verifyConfig, done);
        });
//x  113388 made this valid for GUI
        it('should return status 400 on POST remove "UserInfoKey":""', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoKey":""}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
//x			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: UserInfoKey Value: \"\"\"\"."};
//x            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET "UserInfoKey":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('UserInfoKey:', function() {
// 116878
        it('should return status 400 on POST "257 UserInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoKey":"'+ FVT.long257Name +'"}}}';
		    verifyConfig = JSON.parse( expectTestOAuthProfile ) ;
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: UserInfoKey Name: '+ FVT.long257Name +'."}' );
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: OAuthProfile Property: UserInfoKey Value: '+ FVT.long257Name +'."}' );
            verifyMessage = JSON.parse( '{"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: OAuthProfile Property: UserInfoKey Value: '+ FVT.long257Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "257 UserInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "UserInfoKey":9', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"UserInfoKey":9}}}';
		    verifyConfig = JSON.parse( expectTestOAuthProfile ) ;
            verifyMessage = JSON.parse( '{"Code":"CWLNA0127","Message":"The property type is not valid. Object: OAuthProfile Name: TestOAuthProf Property: UserInfoKey Type: JSON_INTEGER"}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UserInfoKey":9', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


    describe('GroupInfoKey:', function() {

        it('should return status 400 on POST "257 GroupInfoKey"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"'+ FVT.long257Name +'"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: GroupInfoKey Name: '+ FVT.long257Name +'."}' );
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: OAuthProfile Property: GroupInfoKey Value: '+ FVT.long257Name +'."}' );
            verifyMessage = JSON.parse( '{"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: OAuthProfile Property: GroupInfoKey Value: '+ FVT.long257Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "257 GroupInfoKey"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 139919
        it('should return status 400 on POST "GroupInfoKey":"no,comma"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"no,comma"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
//?            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8485","Message":"The '+ FVT.maxFileName255 +' already exists. Set Overwrite:true to update the existing configuration." }' );
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"no,comma"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 139919
        it('should return status 400 on POST "GroupInfoKey":"9noLeadNumber"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"9noLeadNumber"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"9noLeadNumber"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "GroupInfoKey":" noLeadSpace"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":" noLeadSpace"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupInfoKey Value: \" noLeadSpace\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":" noLeadSpace"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 139919, 140261
        it('should return status 400 on POST "GroupInfoKey":"noTrailSpace "', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"noTrailSpace "}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
//???            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: noTrailSpace ." } ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupInfoKey Value: \"noTrailSpace \"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"noTrailSpace "', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 139919
        it('should return status 400 on POST "GroupInfoKey":"\\nobackslash"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"\\nobackslash"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"\\nobackslash"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST "GroupInfoKey":"no\"quote"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"no\"quote"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            verifyMessage = {"status":400,"Code":"CWLNA6001","Message":"Failed to parse administrative request '}' expected near 'quote': RC=1." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"no\"quote"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 139919
        it('should return status 400 on POST "GroupInfoKey":"no=equal"', function(done) {
            var payload = '{"OAuthProfile":{"'+ FVT.long256Name +'":{"GroupInfoKey":"no=equal"}}}';
		    verifyConfig = JSON.parse( expectMaxNameOAuthProfile ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "GroupInfoKey":"no=equal"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//x 113388 NOW "" is valid, FUTURE should be a relationship between UserInfoURL and UserInfoKey/GroupInfoKey
        it('should return status 200 on POST remove "GroupInfoKey":""', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"GroupInfoKey":""}}}';
            verifyConfig = JSON.parse( expectTestOAuthProfile );
//x			verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupInfoKey Value: \"\"\"\"."};
//x            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupInfoKey":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

	
// End of Error:
  }); 

	
	//  ====================  Delete test cases  ====================  //
    describe('Delete Test Cases:', function() {

        it('should return status 200 when deleting OAuthProfile "Max Length Name"', function(done) {
            verifyConfig = '{"OAuthProfile":{"'+ FVT.long256Name +'":null}}';
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on GET all after delete, "Max Length Name" gone', function(done) {
		    verifyConfig = JSON.parse( '{"OAuthProfile":{"'+ FVT.long256Name +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after delete, "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"OAuthProfile":{"'+ FVT.long256Name +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: '+ FVT.long256Name +'","OAuthProfile":{"'+ FVT.long256Name +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done );
        });


        var objName = 'TestOAuthProf'
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"OAuthProfile":{"'+ objName +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET all after delete, "'+ objName +'" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after delete, "'+ objName +'" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"OAuthProfile":{"'+ objName +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: '+ objName +'","OAuthProfile":{"'+ objName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done );
        });
		
    });

});
