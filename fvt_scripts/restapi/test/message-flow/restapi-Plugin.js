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

var uriObject = 'Plugin/';
var expectDefault = '{"Plugin":{},"Version": "' + FVT.version + '"}';


//  ====================  Test Cases Start Here  ====================  //
//"ListObjects":"Name"
// also; 
//    File[a jar/zip file that contains file plugin.json with config basics],
//    PropertiesFile[pluginProps.json  local config params for plugin]

describe('Plugin', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "Plugin":{..}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

		it('should return status 200 on POST roundtrip', function(done) {
            var payload = expectDefault;
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET roundtrip', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
/*   done by CURL command in framework
//  ====================   Create - Add - Update test cases  ====================  //
    describe('PreReq Files:', function() {

        it('should return status 200 on PUT "jsonplugin.zip"', function(done) {
            sourceFile = FVT.M1_IMA_SDK +'/ImaTools/ImaPlugin/lib/jsonplugin.zip';
            targetFile = 'jsonplugin.zip';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUTBINARY "jsonplugin.zip"', function(done) {
            sourceFile = FVT.M1_IMA_SDK +'/ImaTools/ImaPlugin/lib/jsonplugin.zip';
            targetFile = 'jsonplugin.BINARY.zip';
            FVT.makePutBinary( FVT.uriFileDomain+targetFile, sourceFile ,targetFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "jsonplugin.json"', function(done) {
            sourceFile = '../plugin_tests/jsonplugin.json';
            targetFile = 'jsonplugin.json';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "restplugin.zip"', function(done) {
            sourceFile = FVT.M1_IMA_SDK +'/ImaTools/ImaPlugin/lib/restplugin.zip';
            targetFile = 'restplugin.zip';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "package.json"', function(done) {
            sourceFile = '../plugin_tests/restmsg/package.json';
            targetFile = 'package.json';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });


    });
*/
//  ====================   Create - Add - Update test cases  ====================  //
    describe('File[zip filename]:', function() {

        it('should return status 200 on POST "Plugin" zip only', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin.zip"}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after 1st POST Plugin', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET "Protocol"  after 1st POST Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.defaultProtocol );
            FVT.makeGetRequest( FVT.uriConfigDomain+"Protocol", FVT.getSuccessCallback, verifyConfig, done);
        });

// Restart - 135483
        it('should return status 200 on GET "Status" before restart with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginStopped  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( expectDefault ) ;
	    verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET "Plugin" with jsonplugin.zip', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET "Protocol"', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgProtocol );
            FVT.makeGetRequest( FVT.uriConfigDomain+"Protocol", FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 129946
        it('should return status 400 on Service GET PropertiesFile bad-form query', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  //?
			verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/Properties is not valid." } ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"Properties?Plugin=json_msg", FVT.getFailureCallback, verifyMessage, done);
        });
        it('should return status 404 on GET Properties "Plugin" when bad URI 1', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  //?
			verifyMessage = { "status":404, "Code":"CWLNA0111", "Message":"The property name is not valid: Property: json_msg." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });
        it('should return status 404 on GET Properties "Plugin" when bad URI 2', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  //?
			verifyMessage = { "status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: Plugin Name: PropertiesFile" } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });
        it('should return status 400 on GET Properties "Plugin" when there are none', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  // would work if there were a PropertiesFile!
//			verifyMessage = { "status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: Plugin Name: json_msg" } ;
			verifyMessage = { "status":400, "Code":"CWLNA6198", "Message":"The requested item \"Plugin/json_msg/PropertiesFile\" is not found." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });

		
/*  Moved to CURL and Plugin_2
        it('should return status 200 on rePUT "jsonplugin.zip"', function(done) {
            sourceFile = FVT.M1_IMA_SDK +'/ImaTools/ImaPlugin/lib/jsonplugin.zip';
            targetFile = 'jsonplugin.zip';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on rePOST "Plugin" zip only', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin2.zip"}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	THIS RESUMES IN restapi-Plugin_2.js since not able to PUT a ZIP and not have mocha corrupt it.
*/		

    });

//  ====================   off beat Protocol tests  ====================  //
    describe('Protocol:', function() {
// 136991
        it('should return status 400 on POST to disable "Protocol":json_msg', function(done) {
            var payload = '{"Protocol":{"json_msg":{"UseTopic":false}}}';
            verifyConfig = JSON.parse( FVT.json_msgProtocol );
//			verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Protocol."};
			verifyMessage = {"status":400,"Code":"CWLNA0140","Message":"The object Protocol is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST Protocol fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"Protocol", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 403 on DELETE "Protocol"', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgProtocol );
			verifyMessage = { "status":403, "Code":"CWLNA0372", "Message":"Delete is not allowed for Protocol object." }
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"Protocol/json_msg", FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE Protocol fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"Protocol", FVT.getSuccessCallback, verifyConfig, done);
        });

	});
    
});
