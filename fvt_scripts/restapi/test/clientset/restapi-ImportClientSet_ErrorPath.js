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

var verifyConfig = {};
var verifyMessage = {};



//  ====================  Test Cases Start Here  ====================  //
//  OrgMove: Export and Import : POST, GET (status) and DELETE with PUT IMPORT file  (is no GET EXPORT file) Test Case 1 of 2.
// Overwrite is tested in the BasicClientSetSetup test

describe('ClientSet:', function() {
this.timeout( FVT.clientSetTimeout + 3000 );

// Assumes:  restapi-ExportClientSet has be run and manually move the EXPORT File to local directory.    
    
// IMPORT File Put -- MOCHA CORRUPTS BINARY FILES, MUST USE scp_file.sh in automation
/*    
    describe('PUT Import:', function() {
        // Must have gotten the file from EXPORT directory indenpenantly of RESTAPI , or can I depend on M1 to mount?
        
        it('should return status 200 on PUT "ExportClietnSet File"', function(done) {
            sourceFile = FVT.maxFileName255;
            targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

    });
*/
    
    
    // IMPORT
    describe('Import:', function() {
        // Must provide FileName
        describe('FileName[R]:', function() {

            it('should return 400 on Import ClientSet with misspelled FileName', function(done) {
                payload = '{"Filename":"ExportOrgMoveTopic", "Password":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: Filename.", "RequestID":"ImportBadFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Import ClientSet with missing FileName', function(done) {
                payload = '{"FileName":"MissingExport.file", "Password":"*$@dang_it!#" }'
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated IBM IoT MessageSight message number (221).", "RequestID":"ImportMissingFileName" };
                verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated ${IMA_PRODUCTNAME_SHORT} message number (221).", "RequestID":"ImportMissingFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Import ClientSet without FileName', function(done) {
                payload = '{"Password":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: FileName Value: null.", "RequestID":"ImportNoFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
            
        });

        // Must Provide Password
        describe('Password[R]:', function() {

            it('should return 400 when Password misspelled', function(done) {
                payload = '{"FileName":"ExportOrgMoveClientID", "PassWord":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: PassWord.", "RequestID":"ImportBadPassWord" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
        
            it('should return 400 when Password missing', function(done) {
                payload = '{ "FileName":"ExportOrgMoveClientID" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: Password Value: null.", "RequestID":"ImportNoPassword" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 when Password is wrong', function(done) {
                payload = '{"FileName":"ExportOrgMoveClientID", "Password":"NotAPswd" }'
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated IBM IoT MessageSight message number (221).", "RequestID":"ImportWrongPassword" };
                verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated ${IMA_PRODUCTNAME_SHORT} message number (221).", "RequestID":"ImportWrongPassword" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

        });
        
        // URL Problem
        describe('WrongDomain:', function() {
        
            it('should return 400 when Domain not Service', function(done) {
                payload = '{"FileName":"ImportOrgMoveBadDomain", "Password":"OrgMovePswd23456", "ClientId":"^", "Topic":"^"}'
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0404", "Message":"The HTTP request is for an object which does not exist.", "RequestID":"ImportOrgMoveBadDomain" };
                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/configuration/import is not valid." };
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain + "import/ClientSet", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
        

        });

        // *****  Successful Import  *****
        describe('ImportNoOverwrite:', function() {

            it('should return 202 on IMPORT ClientSet', function(done) {
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"ImportMaxFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on IMPORT ClientSet - No Overwrite', function(done) {
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"OrgMovePswd23456", "Overwrite":true }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: Overwrite.", "RequestID":"ImportMaxFileNameOverwrite" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });


        });

        // *****  Successful Import  *****
        describe('Force Delete:', function() {
            it('should return 202 on IMPORT ClientSet ImportTERMINATE', function(done) {
            // Allow for the above Import to get past the current ClientID, to avoid "CWLNA6174":"The ClientSet request with request id 22298 failed with reason ISMRC_ClientIDInUse (121)."
                this.timeout( FVT.clientSetTimeout + 3000 );
                FVT.sleep( FVT.clientSetTimeout );            
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"ImportTERMINATE" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
    
            it('should return 200 on DELETE ImportTERMINATE Import FORCE', function(done) {
                aRequestID = FVT.getRequestID( "ImportTERMINATE" );
                verifyConfig =  {} ;
                verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6173", "Message":"The import ClientSet with request ID '+ aRequestID +' has been deleted." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/ClientSet/"+ aRequestID +'?Force=true' , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });
    // something like terminated?
            it('should return 404 on GET ImportTERMINATE Import status in now gone', function(done) {
                aRequestID = FVT.getRequestID( "ImportTERMINATE" );
                verifyConfig =  {  } ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."} ;
				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID '+ aRequestID +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getFailureCallback, verifyMessage, done);
            });

        });
        
    });

    
    
    // IMPORT Status
    describe('Import Status:', function() {
        // Must provide RequestID
        describe('RequestID[R]:', function() {

            it('should return 400 GET Import ClientSet Status with missing RequestID', function(done) {
                verifyConfig = {} ;
//149167 149186
//                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/import/ClientSet is not valid." };
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/import/ClientSet is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet", FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 404 GET Import ClientSet with bad RequestID URL1', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."  };
				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID 0 was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/0", FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 404 GET Import ClientSet with bad RequestID URL2', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."  };
				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID '+ FVT.maxFileName255 +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/"+ FVT.maxFileName255, FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 400 GET Import ClientSet with bad URL1', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/import/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/import/'+ aRequestID +' is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 400 GET Import ClientSet with bad URL2', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/ClientSet/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/ClientSet/'+ aRequestID +' is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/ClientSet/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

        });

    });
    
    
    // IMPORT Delete
    describe('IMPORT Delete:', function() {
        // Must provide RequestID and proper URL path
        describe('RequestID[R]:', function() {

            it('should return 400 DELETE Import ClientSet Delete with missing RequestID', function(done) {
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/import/ClientSet is not valid."  };
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/ClientSet", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 DELETE Import ClientSet with bad RequestID number', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."  };
 				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID 0 was not found." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/ClientSet/0", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 DELETE Import ClientSet with bad RequestID URL', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: FileName Value: null."  };
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."  };
 				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID MaxFileName was not found." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/ClientSet/MaxFileName", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 DELETE Import ClientSet with bad path URL1', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /import/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/import/'+ aRequestID +' is not valid." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/"+ aRequestID, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 200 GET Import ClientSet ImportMaxFileName remains after Delete bad path URL1', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "Status":1, "RetCode":0, "Code": "CWLNA6177", "Message": "The import request for ClientSet with request ID '+ aRequestID +' has completed." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
            });
//149026
            it('should return 400 DELETE Import ClientSet with bad path URL2', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ClientSet/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/ClientSet/'+ aRequestID +' is not valid." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "ClientSet/"+ aRequestID, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 200 GET Import ClientSet ImportMaxFileName remains after Delete bad path URL2', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "Status":1, "RetCode":0, "Code": "CWLNA6177", "Message": "The import request for ClientSet with request ID '+ aRequestID +' has completed." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
            });
//149140
            it('should return 200 DELETE Import ClientSet with good URL', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6173", "Message":"The import ClientSet with request ID '+ aRequestID +' has been deleted." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "import/ClientSet/"+ aRequestID, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 GET Import ClientSet ImportMaxFileName Deleted', function(done) {
                aRequestID = FVT.getRequestID( "ImportMaxFileName" );
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." };
 				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID '+ aRequestID +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

        });

    });

});
