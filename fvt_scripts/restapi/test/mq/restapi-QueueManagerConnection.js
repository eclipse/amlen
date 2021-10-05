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

var verifyConfig = {} ;
var verifyMessage = {};
var uriObject = 'QueueManagerConnection/' ;
var expectDefault = '{"QueueManagerConnection":{},"Version": "'+ FVT.version +'"}' ;

var expectTestQMConnect = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Force":false, "ChannelUserName":"", "ChannelUserPassword":""}}}';

var expectAllQMConnect = '{  "Version": "v1",  "QueueManagerConnection": { \
  "TestQMConnect": {  "QueueManagerName": "QM_MQJMS",  "ChannelUserName": "",  "ConnectionName": "'+ FVT.MQSERVER1_IP +'(1415)",  "Force": false,  "SSLCipherSpec": "",  "ChannelName": "CHNLJMS", "Force":false, "ChannelUserName":"", "ChannelUserPassword":"" }, \
  "'+ FVT.long256Name +'": {  "QueueManagerName": "'+ FVT.long48MQName +'",  "ChannelUserName": "",  "ConnectionName": "'+ FVT.MQSERVER1_IP +'",  "Force": false,  "SSLCipherSpec": "'+ FVT.long32Name +'",  "ChannelName": "CHNLJMS", "Force":false, "ChannelUserName":"", "ChannelUserPassword":""}}}' ;
//  ====================  Test Cases Start Here  ====================  //
//  "ListObjects":"Name,QueueManagerName,ConnectionName,SSLCipherSpec,ChannelName"  also Force, ChannelUserPassword, ChannelUserName, Verify

/* TODO:  MQSSLKey, MQStashPassword, Overwrite!   These are not shown in schema for some reason, added late 
*/

describe('QueueManagerConnection:', function() {
this.timeout( FVT.mqTimeout );

    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "QueueManagerConnection":{}', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should SAVE the currect state of MQConnectivityEnabled to reset at end', function(done){
            verifyConfig = { "MQConnectivityEnabled":false } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MQConnectivityEnabled", FVT.getMQConnStateCallback, verifyConfig, done)
        });
        
    });
// Like any ENDPOINT, it must be ENABLED to work
    describe('MUST Enable the MQConnectivity Endpoint:', function() {
        it('should return 200 on POST, MQConnectivityEnabled:true', function(done) {
            var payload = '{"MQConnectivityEnabled":true}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET MQConn Status', function(done) {
            this.timeout( FVT.START_MQ + 2000 ); 
            FVT.sleep( FVT.START_MQ );
            verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning );
            FVT.makeGetRequest( FVT.uriServiceDomain + 'status/MQConnectivity' , FVT.getSuccessCallback , verifyConfig , done );
        });
    
    });    

    
//  ====================   Name - Name of the Queue Manager Connection object (R)  ====================  //
//          "Restrictions":"No leading or trailing space. No control characters or comma.",
 
    describe('Name[256},Verify', function() {
// 136559
//  change is not committed on Verify:true and parameters are GOOOD
        it('should return status 200 on POST "TestQMConnect" verify:true GOOD', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Verify":true }}}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TestQMConnect" verify:true GOOD', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 136559
//  change is not committed on Verify:true and parameters are NOT GOOOD        
        it('should return status 400 on POST "TestQMConnect" verify:true BAD', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1414)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Verify":true }}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0316","Message":"The IBM IoT MessageSight server could not connect to MQ Connectivity service."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0310","Message":"Host error while connecting to queue manager"}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "TestQMConnect" verify:true BAD', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

//  change IS committed (unlike LDAP) on Verify:false and parameters are NOT GOOOD        
        it('should return status 200 on POST "TestQMConnect" verify:false BAD', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1515)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Verify":false }}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TestQMConnect" verify:false BAD', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 136559
        it('should return status 400 on POST "TestQMConnect" verify:true tests BAD', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Verify":true }}}';
            verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1515)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Verify":false }}}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0316","Message":"The IBM IoT MessageSight server could not connect to MQ Connectivity service."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0310","Message":"Host error while connecting to queue manager"}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "TestQMConnect" verify:true tests BAD', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return status 200 on POST "TestQMConnect"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS", "Verify":false }}}';
            verifyConfig = JSON.parse( expectTestQMConnect );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "TestQMConnect"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return status 200 on POST "MaxLength QMConnect Name"', function(done) {
            var payload = '{"QueueManagerConnection":{"'+ FVT.long256Name +'":{"QueueManagerName":"'+ FVT.long48MQName +'","ConnectionName":"'+ FVT.MQSERVER1_IP +'","SSLCipherSpec":"'+ FVT.long32Name +'","ChannelName":"CHNLJMS"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MaxLength QMConnect Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Force - Force delete or update of the Queue Manager Connection. This might cause the XA IDs to be orphaned. (R)  ====================  //

    describe('Force[F]', function() {

        it('should return status 200 on POST "FORCE":true', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Force":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "FORCE":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "FORCE":null (DEFAULT:false)', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Force":null}}}';
            verifyConfig = JSON.parse( expectTestQMConnect );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "FORCE":null (DEFAULT:false)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "FORCE":true', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Force":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "FORCE":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "FORCE":false', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Force":false}}}';
            verifyConfig = JSON.parse( expectTestQMConnect );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "FORCE":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    

//  ====================   QueueManagerName - The Queue Manager to connect to. (R)  ====================  //
//          "Restrictions":"Up to 48 single byte characters. Upper and lower case A-Z, 0-9, and . / _ %",
 
    describe('QueueManagerName[48]', function() {

        it('should return status 200 on POST "QueueManagerName":"qm_mqjms"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"qm_mqjms"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "QueueManagerName":"qm_mqjms"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "QueueManagerName":"123svtbridge/4567%890_managerz"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"123svtbridge/4567%890_managerz"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "QueueManagerName":"123svtbridge/4567%890_managerz"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "QueueManagerName":"QM_MQJMS"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "QueueManagerName":"QM_MQJMS"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    
    describe('RESTART Config Persist Check:', function() {
            
    // Config persists Check
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
		    this.timeout( FVT.REBOOT + FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.REBOOT + FVT.START_MQ );
            var verifyStatus = JSON.parse( FVT.expectMQRunningStatus ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "QMConn cfg" persists', function(done) {
            verifyConfig = JSON.parse( expectAllQMConnect ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


//  ====================   ConnectionName - Comma delimited list of Connections (R)  ====================  //
//           "Restrictions":"Must be in the format of an IP Address or IP Address(Port). Examples: 243.0.77.01, 243.0.77.2(1111)",
 
    describe('ConnectionName["",1024]', function() {

        it('should return status 200 on POST "ConnectionName":'+ FVT.MQSERVER1_IP, function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":'+ FVT.MQSERVER1_IP, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ConnectionName":'+ FVT.MQSERVER1_IP +'(PORT)', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":'+ FVT.MQSERVER1_IP +'(PORT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on POST "ConnectionName":'+ FVT.MQSERVER1_IPv6, function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IPv6 +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":'+ FVT.MQSERVER1_IPv6, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ConnectionName":'+ FVT.MQSERVER1_IPv6 +'(PORT)', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IPv6 +'(1416)"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":'+ FVT.MQSERVER1_IPv6 +'(PORT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ConnectionName":ip_list', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +','+ FVT.MQSERVER1_IP +'(1415),'+ FVT.MQSERVER1_IPv6 +','+ FVT.MQSERVER1_IPv6 +'(1416),'+ FVT.MQSERVER2_IP +'(1415)"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":ip_list', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ConnectionName":null (DEFAULT)', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":null}}}';
            verifyConfig = {"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":""}}};
            verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ConnectionName Value: \"(null)\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "ConnectionName":null (DEFAULT)', function(done) {
            verifyConfig = JSON.parse( '{"QueueManagerConnection": { "TestQMConnect": { "QueueManagerName": "QM_MQJMS", "ConnectionName": "'+ FVT.MQSERVER1_IP +','+ FVT.MQSERVER1_IP +'(1415),'+ FVT.MQSERVER1_IPv6 +','+ FVT.MQSERVER1_IPv6 +'(1416),'+ FVT.MQSERVER2_IP +'(1415)", "ChannelName": "CHNLJMS", "SSLCipherSpec": "", "ChannelUserPassword": "", "Force": false, "ChannelUserName": "" }}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ConnectionName":'+ FVT.MQSERVER1_IP, function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ConnectionName":'+ FVT.MQSERVER1_IP, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


//  ====================   ChannelName - The Channel to use while connecting to the MQ Queue Manager (R)  ====================  //
//          "Restrictions":"Up to 20 single byte characters. Upper and lower case A-Z, 0-9, and . / _ % ",
 
    describe('ChannelName[20]', function() {

        it('should return status 200 on rePOST "ChannelName":"CHNLJMS"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ChannelName":"CHNLJMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on reGET "ChannelName":"CHNLJMS"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ChannelName":"123/4567%890_manager"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ChannelName":"123/4567%890_manager"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ChannelName":"123/4567%890_manager"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "ChannelName":"CHNLJMS"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ChannelName":"CHNLJMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "ChannelName":"CHNLJMS"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });



//  ====================   SSLCipherSpec - SSL Cipher specification.  ====================  //
//           "Restrictions":"The value is a string with a maximum length of 32 characters",

    describe('SSLCipherSpec["",32]', function() {

        it('should return status 200 on POST "SSLCipherSpec":ABCabc1234567890ABCXYZ', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":"ABCabc1234567890ABCXYZ"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "SSLCipherSpec":ABCabc1234567890ABCXYZ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "SSLCipherSpec":null', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":null}}}';
            verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":""}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "SSLCipherSpec":null (Reset to Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "SSLCipherSpec":ABCabc1234567890ABCXYZ3456789012', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":"ABCabc1234567890ABCXYZ3456789012"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "SSLCipherSpec":ABCabc1234567890ABCXYZ3456789012', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST "SSLCipherSpec":""', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":""}}}';
            verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":""}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "SSLCipherSpec":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


//  ====================++++++++++++++++++++++++++++  Error test cases  ++++++++++++++++++++++++++++====================  //
    describe('Error:', function() {
          
    //  ====================   Name - Name of the Queue Manager Connection object (R)  ====================  //
    //          "Restrictions":"No leading or trailing space. No control characters or comma.",
     
        describe('Name[256}', function() {

            it('should return status 400 on POST Name too long', function(done) {
                var payload = '{"QueueManagerConnection":{"'+ FVT.long257Name +'":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"'+ FVT.long257Name +'":null}}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The length of the value of the configuration object is too long. Object: QueueManagerConnection Property: Name Value: '+ FVT.long257Name +'."}' );
// NOT A 121108                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The name of the configuration object is too long. Object: QueueManagerConnection Property: Name Value: '+ FVT.long257Name +'."}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: QueueManagerConnection Property: Name Value: '+ FVT.long257Name +'."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 on GET after POST fails Name too long', function(done) {
                verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"'+ FVT.long257Name +'":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: '+ FVT.long257Name +'"}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
            });

            it('should return status 400 on POST NameNo,Comma', function(done) {
                var payload = '{"QueueManagerConnection":{"NameNo,Comma":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"NameNo,Comma":null}}' );
                verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 on GET after POST fails NameNo,Comma', function(done) {
                verifyConfig = {"status":404,"QueueManagerConnection":{"NameNo,Comma":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: NameNo,Comma"} ;
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NameNo,Comma", FVT.verify404NotFound, verifyConfig, done);
            });

            it('should return status 400 on POST NameNoLeadingSpace', function(done) {
                var payload = '{"QueueManagerConnection":{" NameNoLeadingSpace":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"NameNoLeadingSpace":null}}' );
                verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 on GET after POST fails NameNoLeadingSpace', function(done) {
                verifyConfig = {"status":404,"QueueManagerConnection":{"NameNoLeadingSpace":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: NameNoLeadingSpace"} ;
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NameNoLeadingSpace", FVT.verify404NotFound, verifyConfig, done);
            });

            it('should return status 400 on POST NameNoTrailingSpace', function(done) {
                var payload = '{"QueueManagerConnection":{"NameNoTrailingSpace ":{"QueueManagerName":"QM_MQJMS","ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","SSLCipherSpec":"","ChannelName":"CHNLJMS"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"NameNoTrailingSpace":null}}' );
                verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: NameNoTrailingSpace ."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 on GET after POST fails NameNoTrailingSpace', function(done) {
                verifyConfig = {"status":404,"QueueManagerConnection":{"NameNoTrailingSpace":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: NameNoTrailingSpace"} ;
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NameNoTrailingSpace", FVT.verify404NotFound, verifyConfig, done);
            });

        });

    //  ====================   Force - Force delete or update of the Queue Manager Connection. This might cause the XA IDs to be orphaned. (R)  ====================  //
// This is not really exposed...
        // describe('Force[F]', function() {

            // it('should return status 200 on POST "FORCE":"true" (STRING)', function(done) {
                // var payload = '{"QueueManagerConnection":{"TestQMConnect":{"Force":"true"}}}';
                // verifyConfig = JSON.parse( '{"QueueManagerConnection":{"NameNoTrailingSpace":null}}' );
                // verifyMessage = {"status":400,"Code":"CWLNA0000","Message":""} ;
                // FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            // });
            // it('should return status 200 on GET "FORCE":"true" (STRING)', function(done) {
                // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            // });
        // });

        

    //  ====================   QueueManagerName - The Queue Manager to connect to. (R)  ====================  //
    //          "Restrictions":"Up to 48 single byte characters. Upper and lower case A-Z, 0-9, and . / _ %",
     
        describe('QueueManagerName[48]', function() {
//121108
            it('should return status 200 on POST "QueueManagerName":too long', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"'+ FVT.long48MQName +'9"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS"}}}' );
//                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: QueueManagerConnection Property: QueueManagerName Value: '+ FVT.long48MQName +'9."}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueueManagerConnection Property: QueueManagerName Value: '+ FVT.long48MQName +'9."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "QueueManagerName":too long not changed', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

            it('should return status 200 on POST "QueueManagerName":"No!Exclamation"', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"No!Exclamation"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS"}}}' );
                verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: QueueManagerName Value: \"No!Exclamation\"."};
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "QueueManagerName":"No!Exclamation" not changed', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

        });


    //  ====================   ConnectionName - Comma delimited list of Connections (R)  ====================  //
    //           "Restrictions":"Must be in the format of an IP Address or IP Address(Port). Examples: 243.0.77.01, 243.0.77.2(1111)",
     
        describe('ConnectionName["",1024]', function() {

            it('should return status 400 on POST "ConnectionName":true', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":true}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","ChannelName":"CHNLJMS"}}}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: QueueManagerConnection Name: TestQMConnect Property: ConnectionName Type: JSON_TRUE"}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ConnectionName":true not set', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

            it('should return status 400 on POST "ConnectionName":too long', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.long1025String +'"}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","ChannelName":"CHNLJMS"}}}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: QueueManagerConnection Property: ConnectionName Value: '+ FVT.long1025String +'."}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ConnectionName":too long not set', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });

        });

        
    //  ====================   ChannelName - The Channel to use while connecting to the MQ Queue Manager (R)  ====================  //
    //          "Restrictions":"Up to 20 single byte characters. Upper and lower case A-Z, 0-9, and . / _ % ",
     
        describe('ChannelName[20]', function() {

            it('should return status 200 on POST "ChannelName":true', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"ChannelName":true}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","ChannelName":"CHNLJMS"}}}');
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: QueueManagerConnection Name: TestQMConnect Property: ChannelName Type: JSON_TRUE"}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "ChannelName":true', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });
        });



    //  ====================   SSLCipherSpec - SSL Cipher specification.  ====================  //
    //           "Restrictions":"The value is a string with a maximum length of 32 characters",

        describe('SSLCipherSpec["",32]', function() {

            it('should return status 400 on POST "SSLCipherSpec":true', function(done) {
                var payload = '{"QueueManagerConnection":{"TestQMConnect":{"SSLCipherSpec":true}}}';
                verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConnect":{"ConnectionName":"'+ FVT.MQSERVER1_IP +'(1415)","ChannelName":"CHNLJMS","SSLCipherSpec":""}}}');
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: QueueManagerConnection Name: TestQMConnect Property: SSLCipherSpec Type: JSON_TRUE"}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on GET "SSLCipherSpec":true', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
            });
        });

    });


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"QueueManagerConnection":{"'+ FVT.long256Name +'":{}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"'+ FVT.long256Name +'":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: '+ FVT.long256Name +'"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 when POST DELETE "TestQMConnect" bad form', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":null}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: /ima/v1/configuration/QueueManagerConnection is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: /ima/v1/configuration/QueueManagerConnection is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, "TestQMConnect" is not gone yet', function(done) {
            verifyConfig = {"QueueManagerConnection":{"TestQMConnect":{"QueueManagerName":"QM_MQJMS"}}}
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 when DELETE "TestQMConnect"', function(done) {
            var payload = '{"QueueManagerConnection":{"TestQMConnect":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestQMConnect', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestQMConnect" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestQMConnect" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"TestQMConnect":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: TestQMConnect"}' );
            // or "Code":"CWLNA0113","Message":"The requested item is not found." 
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestQMConnect', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when deleting "MissingQueueManagerConnection"', function(done) {
            var payload = '{"QueueManagerConnection":{"MissingQueueManagerConnection":{}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'MissingQueueManagerConnection', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "MissingQueueManagerConnection" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "MissingQueueManagerConnection" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"MissingQueueManagerConnection":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: MissingQueueManagerConnection"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'MissingQueueManagerConnection', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

//  ====================  Error test cases  ====================  //
    describe('Error Test Cases: General:', function() {

        it('should return status 400 when trying to Delete All QueueManagerConnections, Syntax Error', function(done) {
            var payload = '{"QueueManagerConnection":null}';
            verifyConfig = {"QueueManagerConnection":{}};
//            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: {\"QueueManagerConnection\":null} is not valid."};
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"QueueManagerConnection\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All QueueManagerConnections, Syntax Error', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    

describe('Error Test Cases: AllowSend:', function() {

        it('should return status 400 when AllowSend not Boolean ', function(done) {
            var payload = '{"QueueManagerConnection":{"AllowSendBoolean":{"AllowSend":"YES"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0127","Message":"The REST API call: /ima/v1/configuration/QueueManagerConnection is not valid." };
            verifyMessage = {"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueueManagerConnection Name: AllowSendBoolean Property: AllowSend" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, AllowSendBoolean was not created', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"AllowSendBoolean":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: AllowSendBoolean"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"AllowSendBoolean", FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

describe('Error Test Cases: ConcurrentConsumers:', function() {

        it('should return status 400 on POST when ConcurrentConsumers not Boolean ', function(done) {
            var payload = '{"QueueManagerConnection":{"ConcurrentConsumersBoolean":{"ConcurrentConsumers":"NO"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueueManagerConnection Name: ConcurrentConsumersBoolean Property: ConcurrentConsumers"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConcurrentConsumersBoolean was not created', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"ConcurrentConsumersBoolean":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: ConcurrentConsumersBoolean"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ConcurrentConsumersBoolean", FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

describe('Error Test Cases: MaxMessages:', function() {

        it('should return status 400 when MaxMessages exceeds MAX', function(done) {
            var payload = '{"QueueManagerConnection":{"ExceedMaxMessages":{"MaxMessages":20000001}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code":"CWLNA0138","Message":"The property name is invalid. Object: QueueManagerConnection Name: ExceedMaxMessages Property: MaxMessages"  };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ExceedMaxMessages was not created', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"ExceedMaxMessages":{}},"Code":"CWLNA0136","Message": "The item or object cannot be found. Type: QueueManagerConnection Name: ExceedMaxMessages"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"ExceedMaxMessages", FVT.verify404NotFound, verifyConfig, done);
        });

    });


    describe('Reset to Default:', function() {
    
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
