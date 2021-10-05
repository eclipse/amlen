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

describe('ClientSet:', function() {
this.timeout( FVT.clientSetTimeout + 3000 );

    // EXPORT
    describe('Export:', function() {
        // Must provide FileName
        describe('FileName[R]:', function() {

            it('should return 400 on Export ClientSet with misspelled FileName', function(done) {
                payload = '{"Filename":"ExportOrgMoveBadFileName", "Password":"OrgMovePswd23456", "Topic":"^.OrgMove" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: Filename.", "RequestID":"ExportOrgMoveBadFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet without FileName', function(done) {
                payload = '{"Password":"NoFileName", "Topic":"^.OrgMove" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: FileName Value: null.", "RequestID":"ExportOrgMoveNoFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet FileName too long', function(done) {
                payload = '{ "FileName":"'+ FVT.long256Name +'", "Password":"OrgMovePswd23456", "Topic":"^.OrgMove" }'
                verifyConfig = {} ;
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0221", "Message":"The file has been truncated or contains invalid data", "RequestID":"ExportFileNameTooLong" }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0133", "Message":"The name of the configuration object is too long. Object: FileName Property: '+ FVT.long256Name +' Value: 272.", "RequestID":"ExportFileNameTooLong" }' );
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet FileName contains FWDSLASH', function(done) {
                payload = '{ "FileName":"/~/ExportNoFWDSLASHFileName.OrgMove", "Password":"OrgMovePswd23456", "Topic":"^.OrgMove" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: FileName Value: \"\/~\/ExportNoFWDSLASHFileName.OrgMove\".", "RequestID":"ExportNoFWDSLASHFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet FileName contains .status', function(done) {
                payload = '{ "FileName":"ExportFilenameNotHaveExtension.status", "Password":"OrgMovePswd23456", "Topic":"^.OrgMove" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: FileName Value: \"ExportFilenameNotHaveExtension.status\".", "RequestID":"ExportFilenameNotHaveExtension.status" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

        });
        
        // *****  Success and Overwrite  *****   *****   *****   *****   *****   *****   *****   *****  
        describe('Overwrite[F]:', function() {
            // ***** Successful Export for Status and Import   ******
            it('should return 202 on Export ClientSet FileName maxlength', function(done) {
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"sixteen890123456", "Topic":"^", "ClientID":"^" }'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"ExportMaxFileName" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet FileName collision', function(done) {
                // sleep to allow time for the file to get created
                FVT.sleep( 1 );
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"boo!", "Topic":"^", "ClientID":"^" }'
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0223", "Message":"A file named '+ FVT.maxFileName255 +' already exists.", "RequestID":"ExportMaxFileNameCollision" }' );
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 202 on Export ClientSet FileName maxlength', function(done) {
                FVT.sleep( 1 );
                payload = '{ "FileName":"'+ FVT.maxFileName255 +'", "Password":"OrgMovePswd23456", "Topic":"^", "ClientID":"^", "Overwrite":true }'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"ExportMaxFileNameOverwrite" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 202 on Export ClientSet FileName to Delete', function(done) {
                FVT.sleep( 1 );
                payload = '{ "FileName":"ExportOrgMoveToDelete", "Password":"sixteen890123456", "Topic":"^", "ClientID":"^" }'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"ExportOrgMoveToDelete" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

        });
        
        // Must Provide Password
        describe('Password[R]:', function() {
        
            it('should return 400 when Password misspelled', function(done) {
                payload = '{"FileName":"ExportOrgMoveBadPassword", "PassWord":"OrgMovePswd23456", "ClientID":"^..OrgMove"}'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: PassWord.", "RequestID":"ExportOrgMoveBadPassword" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 when Password missing', function(done) {
                payload = '{ "FileName":"ExportOrgMoveNoPassword", "ClientID":"^..OrgMove" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: Password Value: null.", "RequestID":"ExportOrgMoveNoPassword" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
// MAX Length 16?  <TBD, currently passes>
            it('should return 400 when Password too long', function(done) {
                payload = '{ "FileName":"ExportOrgMovePasswordTooLong", "Password":"12345678901234567", "ClientID":"^" }'
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: Password Value: 12345678901234567.", "RequestID":"ExportOrgMovePasswordTooLong" };
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0144", "Message":"The value that is specified for the property on the configuration object is too long. Object: Password Property: ******** Value: 17.", "RequestID":"ExportFileNameTooLong" }' );
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

        });
        
        // Must Provide Topic OR ClientID
        describe('Topic or ClientID[R]:', function() {
        
            it('should return 400 when ClientID misspelled', function(done) {
                payload = '{"FileName":"ExportOrgMoveBadClientID", "Password":"OrgMovePswd23456", "ClientId":"^..OrgMove"}'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: ClientId.", "RequestID":"ExportOrgMoveBadClientID" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
        
            it('should return 400 when Topic misspelled', function(done) {
                payload = '{"FileName":"ExportOrgMoveBadTopic", "Password":"OrgMovePswd23456", "TopicName":"^/OrgMove"}'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0115", "Message":"An argument is not valid: Name: TopicName.", "RequestID":"ExportOrgMoveBadTopic" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 When given neither Topic and ClientID', function(done) {
                payload = '{"FileName":"ExportOrgMoveNoTopicClientID", "Password":"OrgMovePswd23456" }'
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: ClientID and Topic Value: null.", "RequestID":"ExportOrgMoveNoTopicClientID" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
// MIGHT NOT BE POSSIBLE TO TELL REGEXPR
            it('should return 400 When Topic not REGEXP', function(done) {
                payload = '{"FileName":"ExportOrgMoveTopicRegExpr", "Password":"OrgMovePswd23456", "Topic":"*" }'
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: ClientID and Topic Value: null.", "RequestID":"ExportOrgMoveTopicRegExpr" };
                verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: Topic Value: \"*\".", "RequestID":"ExportOrgMoveTopicRegExpr" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
// MIGHT NOT BE POSSIBLE TO TELL REGEXPR
            it('should return 400 When ClientID not REGEXP', function(done) {
                payload = '{"FileName":"ExportOrgMoveClientIDRegExpr", "Password":"OrgMovePswd23456", "ClientID":"*" }'
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: ClientID and Topic Value: null.", "RequestID":"ExportOrgMoveClientIDRegExpr" };
                verifyMessage = { "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: ClientID Value: \"*\".", "RequestID":"ExportOrgMoveTopicRegExpr" };
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });

        });
        
        // FORCE on Delete
        describe('Force Delete:', function() {

            it('should return 202 on Export ALL ClientSets by Topic and ClientID to Terminate', function(done) {
                payload = '{"FileName":"ExportTERMINATE", "Password":"OrgMovePswd", "Topic":"^", "ClientID":"^"}'
                verifyConfig = {} ;
                verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"RequestIDTERMINATE" };
                // exportSuccessCallback need to retrieve FVT.RequestID
                FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "export/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
            });
        
            it('should return 200 on DELETE RequestIDTERMINATE Export FORCE', function(done) {
                aRequestID = FVT.getRequestID( "RequestIDTERMINATE" );
                verifyConfig =  {} ;
                verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6173", "Message":"The export ClientSet with request ID '+ aRequestID +' has been deleted." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/ClientSet/"+ aRequestID +'?Force=true' , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });
    // something like terminated?
            it('should return 404 on GET RequestIDTERMINATE Export status in now gone', function(done) {
                aRequestID = FVT.getRequestID( "RequestIDTERMINATE" );
                verifyConfig =  {  } ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."} ;
    			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID '+ aRequestID +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID , FVT.getFailureCallback, verifyMessage, done);
            });

        
        });
        
        // URL Problem
        describe('WrongDomain:', function() {
                
            it('should return 400 when Domain not Service', function(done) {
                payload = '{"FileName":"ExportOrgMoveBadDomain", "Password":"OrgMovePswd23456", "ClientId":"^", "Topic":"^"}'
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0404", "Message":"The HTTP request is for an object which does not exist.", "RequestID":"ExportOrgMoveBadDomain" };
                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/configuration/export is not valid." };
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain + "export/ClientSet", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
        

        });
        
    });
    
    
    // EXPORT Status
    describe('Export Status:', function() {
        // Must provide RequestID
        describe('RequestID[R]:', function() {
//149167 149186
            it('should return 400 on Export ClientSet Status with missing RequestID', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/export/ClientSet is not valid." };
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/export/ClientSet is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet", FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 404 on Export ClientSet with bad RequestID URL1', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." };
    			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID 0 was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/0", FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 404 on Export ClientSet with bad RequestID URL2', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."};
    			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID '+ FVT.maxFileName255 +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ FVT.maxFileName255, FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet with bad URL1', function(done) {
                aRequestID = FVT.getRequestID( "ExportMaxFileName" );
                verifyConfig = {} ;
// or?                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/status/export/'+ aRequestID +' is not valid." }' );
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/export/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/export/'+ aRequestID +' is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

            it('should return 400 on Export ClientSet with bad URL2', function(done) {
                aRequestID = FVT.getRequestID( "ExportMaxFileName" );
                verifyConfig = {} ;
// or                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/status/ClientSet/'+ aRequestID +' is not valid." }' );
//                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /status/ClientSet/'+ aRequestID +' is not valid." }' );
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/status/ClientSet/'+ aRequestID +' is not valid." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/ClientSet/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

        });
    });
    
    // EXPORT Delete
    describe('Export Delete:', function() {
        // Must provide RequestID and proper URL path
        describe('RequestID[R]:', function() {

            it('should return 400 DELETE Export ClientSet Delete with missing RequestID', function(done) {
                verifyConfig = {} ;
                verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/export/ClientSet is not valid." };
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/ClientSet", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 DELETE Export ClientSet with bad RequestID URL1', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." };
    			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID 0 was not found." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/ClientSet/0", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 DELETE Export ClientSet with bad RequestID URL2', function(done) {
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." };
    			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID MaxFileName was not found." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/ClientSet/MaxFileName", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 400 DELETE Export ClientSet with bad path URL1', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/export/'+ aRequestID +' is not valid." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/"+ aRequestID, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 200 GET Export ClientSet ExportOrgMoveToDelete remains after Delete bad path URL1', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
                verifyMessage = { "Status":1, "RetCode":0 };
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
            });

            it('should return 400 DELETE Export ClientSet with bad path URL2', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
//nope            verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/ClientSet/'+ aRequestID +' is not valid." }' );
// 149026        verifyMessage = { "status":400, "Code":"CWLNA0118", "Message":"The properties are not valid." };
                verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/'+ FVT.version +'/service/ClientSet/'+ aRequestID +' is not valid." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "ClientSet/"+ aRequestID, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 200 GET Export ClientSet ExportOrgMoveToDelete remains after Delete bad path URL2', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
                verifyMessage = { "Status":1, "RetCode":0 };
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
            });

            it('should return 200 DELETE Export ClientSet with good URL', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
                verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6173", "Message":"The export ClientSet with request ID '+ aRequestID +' has been deleted." }' );
                FVT.makeDeleteRequest( FVT.uriServiceDomain + "export/ClientSet/"+ aRequestID, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });

            it('should return 404 GET Export ClientSet ExportOrgMoveToDelete Deleted', function(done) {
                aRequestID = FVT.getRequestID( "ExportOrgMoveToDelete" );
                verifyConfig = {} ;
//                verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." };
	    		verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The export request for ClientSet with request ID '+ aRequestID +' was not found." }' );
                FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID, FVT.getFailureCallback, verifyMessage, done);
            });

        });

    });


// There is not FILE GET of the EXPORT file, it must be done manually outside of JS.    
// restapi-IMPORTClientSet_ErrorPath will resume this test with a:  File Put of the EXPORT file

    describe('ExportStatusForImport:', function() {
// Adding GET status/export/ClientSet of the two that will be used in Import test case.  Defect 152379
//?What should the status be of the Export whose file was overwritten
		it('should return 200 GET Export ClientSet ExportMaxFileName - was overwritten', function(done) {
			aRequestID = FVT.getRequestID( "ExportMaxFileName" );
			verifyConfig = {} ;
//			verifyMessage = { "Status":1, "RetCode":0, "Code": "CWLNA6011",  "Message": "The requested configuration change has completed successfully." };
			verifyMessage = JSON.parse( '{ "Status":1, "RetCode":0, "Code": "CWLNA6177",  "Message": "The export request for ClientSet with request ID '+ aRequestID  +' has completed." }' );
			FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
		});

		it('should return 200 GET Export ClientSet ExportMaxFileNameOverwrite - this is final copy', function(done) {
			aRequestID = FVT.getRequestID( "ExportMaxFileNameOverwrite" );
			verifyConfig = {} ;
//			verifyMessage = { "Status":1, "RetCode":0, "Code": "CWLNA6011",  "Message": "The requested configuration change has completed successfully." };
			verifyMessage = JSON.parse( '{ "Status":1, "RetCode":0, "Code": "CWLNA6177",  "Message": "The export request for ClientSet with request ID '+ aRequestID  +' has completed." }' );
			FVT.makeGetRequest( FVT.uriServiceDomain + "status/export/ClientSet/"+ aRequestID, FVT.getSuccessCallback, verifyMessage, done);
		});
		
    });

});
