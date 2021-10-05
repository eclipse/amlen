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

var verifyConfig = {} ;
var verifyMessage = {};
var uriObject = 'DestinationMappingRule/' ;
var expectDefault = '{"DestinationMappingRule":{},"Version": "'+ FVT.version +'"}' ;
// Rule type 	From: 						To:
//   1 		IBM MessageSight topic 		WebSphere MQ queue
//   2 		IBM MessageSight topic 		WebSphere MQ topic
//   3 		WebSphere MQ queue 			IBM MessageSight topic
//   4 		WebSphere MQ topic 			IBM MessageSight topic
//   5 		IBM MessageSight topic subtree 	WebSphere MQ queue
//   6 		IBM MessageSight topic subtree 	WebSphere MQ topic
//   7 		IBM MessageSight topic subtree 	WebSphere MQ topic subtree
//   8 		WebSphere MQ topic subtree 	IBM MessageSight topic
//   9 		WebSphere MQ topic subtree 	IBM MessageSight topic subtree
//   10 	IBM MessageSight queue 		WebSphere MQ queue
//   11 	IBM MessageSight queue 		WebSphere MQ topic
//   12 	WebSphere MQ queue 			IBM MessageSight queue
//   13 	WebSphere MQ topic 			IBM MessageSight queue
//   14 	WebSphere MQ topic subtree 	IBM MessageSight queue

var expectPopulated = '{"DestinationMappingRule":{ \
  "TestDestinationMappingRule": {"QueueManagerConnection":"TestQMConn","RuleType":2,"Source":"/topic","Destination":"MQ_TOPIC","MaxMessages": 5000,"Enabled": true,"RetainedMessages": "None"},  \
  "a256_CharName111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445123456": {"QueueManagerConnection":"TestQMConn","RuleType":1,"Source":"/topic","Destination":"AQUEUE","MaxMessages": 5000,"Enabled": true,"RetainedMessages": "None" }  \
},"Version": "'+ FVT.version +'"}' ;

var expectTestDMRule = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Destination":"MQ_TOPIC","Enabled":false,"MaxMessages":5000,"QueueManagerConnection":"TestQMConn","RetainedMessages":"None","RuleType":2,"Source":"/topic"}}}' ;
var expectTestDMRuleEnabled = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Destination":"MQ_TOPIC","Enabled":true,"MaxMessages":5000,"QueueManagerConnection":"TestQMConn","RetainedMessages":"None","RuleType":2,"Source":"/topic"}}}' ;

var expectMaxNameDMRule = '{"DestinationMappingRule":{"'+ FVT.long256Name+'":{"Destination":"'+ FVT.long48MQQueueName +'","Enabled":false,"MaxMessages":20000000,"QueueManagerConnection":"TestQMConn","RetainedMessages":"None","RuleType":1,"Source":"'+ FVT.topicLevel31 +'"}}}' ;
var expectMaxNameDMRuleEnabled = '{"DestinationMappingRule":{"'+ FVT.long256Name+'":{"Destination":"'+ FVT.long48MQQueueName +'","Enabled":true,"MaxMessages":20000000,"QueueManagerConnection":"TestQMConn","RetainedMessages":"None","RuleType":1,"Source":"'+ FVT.topicLevel31 +'"}}}' ;

var DMRuleList = [ "DMRule1_MSt_MQq", "DMRule2_MSt_MQt", "DMRule3_MQq_MSt", "DMRule4_MQt_MSt", "DMRule5_MStt_MQq", "DMRule6_MStt_MQt", "DMRule7_MStt_MQtt", "DMRule8_MQtt_MSt", "DMRule9_MQtt_MStt", "DMRule10_MSq_MQq", "DMRule11_MSq_MQt", "DMRule12_MQq_MSq", "DMRule13_MQt_MSq", "DMRule14_MQtt_MSq" ] ;

var expectAllDMRule = '{  "Version": "v1",  "DestinationMappingRule": { \
 "TestDestinationMappingRule": {  "Destination": "MQ_TOPIC",  "RuleType": 2,  "RetainedMessages": "None",  "Enabled": false,  "MaxMessages": 5000,  "QueueManagerConnection": "TestQMConn",  "Source": "/topic"  }, \
 "'+ FVT.long256Name +'": {  "Destination": "'+ FVT.long48MQQueueName +'",  "RuleType": 1,  "RetainedMessages": "None",  "Enabled": false,  "MaxMessages": 20000000,  "QueueManagerConnection": "TestQMConn",  "Source": "'+ FVT.topicLevel31 +'"  }, \
 "DMRule1_MSt_MQq": {  "Enabled": false,  "RuleType": 1,  "RetainedMessages": "None",  "Source": "undefined",  "Destination": "'+ FVT.long48MQQueueName +'",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule2_MSt_MQt": {  "Enabled": false,  "RuleType": 2,  "RetainedMessages": "None",  "Source": "/mstopic",  "Destination": "'+ FVT.long48MQTopicName +'",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule3_MQq_MSt": {  "Enabled": false,  "RuleType": 3,  "RetainedMessages": "None",  "Source": "'+ FVT.long48MQQueueName +'",  "Destination": "'+ FVT.long1024TopicName +'",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule4_MQt_MSt": {  "Enabled": false,  "RuleType": 4,  "RetainedMessages": "None",  "Source": "'+ FVT.long48MQTopicName +'",  "Destination": "/mstopic",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule5_MStt_MQq": {  "Enabled": false,  "RuleType": 5,  "RetainedMessages": "None",  "Source": "/ms/topic/subtree/",  "Destination": "MQ_QUEUE",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule6_MStt_MQt": {  "Enabled": false,  "RuleType": 6,  "RetainedMessages": "None",  "Source": "/ms/topic/subtree",  "Destination": "/MQ_TOPIC",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule7_MStt_MQtt": { "Enabled": false,  "RuleType": 7,  "RetainedMessages": "None",  "Source": "/ms/topic/subtree",  "Destination": "/MQ/TOPIC/SUBTREE",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule8_MQtt_MSt": {  "Enabled": false,  "RuleType": 8,  "RetainedMessages": "None",  "Source": "/MQ/TOPIC/SUBTREE",  "Destination": "/mstopic",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule9_MQtt_MStt": { "Enabled": true,   "RuleType": 9,  "RetainedMessages": "None",  "Source": "/MQ/TOPIC/SUBTREE",  "Destination": "/ms/topic/subtree",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule10_MSq_MQq": {  "Enabled": false,  "RuleType": 10,  "RetainedMessages": "None",  "Source": "'+ FVT.long256Name +'",  "Destination": "MQ_QUEUE",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule11_MSq_MQt": {  "Enabled": false,  "RuleType": 11,  "RetainedMessages": "None",  "Source": "ms_queue",  "Destination": "/MQTOPIC",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule12_MQq_MSq": {  "Enabled": false,  "RuleType": 12,  "RetainedMessages": "None",  "Source": "MQ_QUEUE",  "Destination": "'+ FVT.long256Name +'",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule13_MQt_MSq": {  "Enabled": false,  "RuleType": 13,  "RetainedMessages": "None",  "Source": "/MQ_TOPIC",  "Destination": "ms_queue",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000  }, \
 "DMRule14_MQtt_MSq": { "Enabled": false,  "RuleType": 14,  "RetainedMessages": "None",  "Source": "/MQ/TOPIC/TREE",  "Destination": "ms_queue",  "QueueManagerConnection": "TestQMConn",  "MaxMessages": 5000 }  }}' ;

var QMConnList = "";
var max256ObjectName = '.11112222222222333333';
//var max256ObjectName = '.1111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445123456';
var maxList = 100;


// --------------------------------------------------------
//  makeQueueManagerConnection( objectName, uniqueName )
// --------------------------------------------------------
function makeQueueManagerConnection( objectName, uniqueName ){
    describe( 'Make QueueManagerConnection for: '+uniqueName, function () {
        
        it('should return 200 on POST "QueueManagerConnection for '+ uniqueName +'"', function(done) {
console.log( "user:" + uniqueName );
            var payload = '{"QueueManagerConnection":{"'+ objectName +'":{"ConnectionName":"'+ uniqueName +'","QueueManagerName":"QM_MQJMS","ChannelName":"CHNLJMS", "Verify":false }}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET "QueueManagerConnection for Channel'+ uniqueName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"Status":200,"QueueManagerConnection":{"'+ objectName +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"QueueManagerConnection/"+objectName, FVT.getSuccessCallback, verifyConfig, done);        });
        
    });
};    

// --------------------------------------------------------
//  deleteQueueManagerConnection( objectName, uniqueName )
// --------------------------------------------------------
function deleteQueueManagerConnection( objectName, uniqueName ){
    describe( 'Delete QueueManagerConnection for: '+uniqueName, function () {
        
        it('should return status 200 on DELETE "QueueManagerConnection '+ objectName +' for '+ uniqueName +'"', function(done) {
console.log( "user:" + uniqueName );
            verifyConfig = JSON.parse( '{"QueueManagerConnection":{"'+ objectName +'":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"QueueManagerConnection/"+objectName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return status 404 on GET "QueueManagerConnection '+ objectName +' for '+ uniqueName +'"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"'+ objectName +'":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: QMConn4'+ uniqueName + max256ObjectName +'" }' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueueManagerConnection/'+objectName, FVT.verify404NotFound, verifyConfig, done);
        });
            
    });
};    



// --------------------------------------------------------
//  deleteDestinationMappingRule( objectName )
// --------------------------------------------------------
function deleteDestinationMappingRule( objectName ){
    describe( 'Delete DestinationMappingRule for: '+objectName, function () {

    	it('should return 200 on POST to disable "DestinationMappingRule '+ objectName +'"', function(done) {
console.log( "DMRule:" + objectName );
		    this.Timeout = ( FVT.mqTimeout );
            payload = '{"DestinationMappingRule":{"'+ objectName +'":{"Enabled":false}}}' ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 GET "DestinationMappingRule":{'+ objectName +'}', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objectName, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on DELETE "DestinationMappingRule for '+ objectName +'"', function(done) {
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"'+ objectName +'":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"DestinationMappingRule/"+objectName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return status 404 on GET "DestinationMappingRule for '+ objectName +'"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"'+ objectName +'":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: '+ objectName +'" }' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'DestinationMappingRule/'+objectName, FVT.verify404NotFound, verifyConfig, done);
        });
            
    });
};    

//  ====================  Test Cases Start Here  ====================  //
//       "ListObjects":"Name,QueueManagerConnection,RuleType,Source,Destination,MaxMessages,Enabled,RetainedMessages",

describe('DestinationMappingRule:', function() {
this.timeout( FVT.mqTimeout*2 );

    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return 200 on GET, DEFAULT of "DestinationMappingRule":{}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

		it('should SAVE the currect state of MQConnectivityEnabled to reset at end', function(done){
		    verifyConfig = { "MQConnectivityEnabled":false } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"MQConnectivityEnabled", FVT.getMQConnStateCallback, verifyConfig, done)
		});
// 142901
        it('should return 200 on POST TraceLevel for debug 142901', function(done) {
            var payload = '{"TraceLevel":"5,admin:9,mqconn:9,tcp:9"}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
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
		    this.timeout( FVT.START_MQ + FVT.mqTimeout );
			FVT.sleep( FVT.START_MQ );
			verifyConfig = JSON.parse( FVT.expectMQConnectivityRunning );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/MQConnectivity', FVT.getSuccessCallback, verifyConfig, done);
        });
	
	});	
    
    // Create PRE_REQ QueueManagerConnection Objects
    describe('PreReq-QMConn:', function() {
        for( i = 0 ; i < maxList ; i++ ) {
            var paddedUniqueName = 'QM_'+( 1400 + i ).toString().slice(-4);
            var uniqueConnectionName = FVT.MQSERVER1_IP +  "(" + ( 1400  + i).toString().slice(-4) +  ")" ;
            paddedObjectName = 'QMConn4' + paddedUniqueName + max256ObjectName ;
            if ( i == 0 ) {
                QMConnList = paddedObjectName ;        
            } else {
                QMConnList = QMConnList+","+paddedObjectName ;
            }
            makeQueueManagerConnection( paddedObjectName, uniqueConnectionName );
        }
        makeQueueManagerConnection( "TestQMConn",  FVT.MQSERVER1_IP  );

        
    });
	
//  ====================   Name: Name of the Destination Mapping Rule object (R)  ====================  //
//          "Restrictions":"No leading or trailing space. No control characters or comma.",
    describe('Name[256]:', function() {

// 1 	IBM MessageSight topic 	WebSphere MQ queue
        it('should return 200 on POST "TestDestinationMappingRule"', function(done) {
            var payload =  expectTestDMRule ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "TestDestinationMappingRule"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Max Length Name DestinationMappingRule"', function(done) {
            var payload = expectMaxNameDMRule ;
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET,  "Max Length Name DestinationMappingRule"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

//  ====================   QueueManagerConnection: Comma delimited list of Queue Manager Connection objects (R)  ====================  //
//          "Restrictions":"Name of a QueueManagerConnection",

      describe('QueueManagerConnection["",256+?]:', function() {

        it('should return 200 on POST MaxLen DMRule Enabled:Active', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"Enabled":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, MaxLen DMRule Enabled:Active', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//120907
        it('should return 400 on POST "QueueManagerConnection":"long list" when rule is Active', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"'+  QMConnList +'"}}}';
            verifyConfig = JSON.parse( expectMaxNameDMRuleEnabled );
			verifyMessage = { "status":400,"Code":"CWLNA0300","Message":"Cannot modify destination mapping rule in enabled state."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"long list", rule is Active,post failed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST to Disable DMRule to update QueueManagerConnection', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"Enabled":false}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET to Disable DMRule to update QueueManagerConnection', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "QueueManagerConnection":"long list", while DMRule Disabled', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"'+  QMConnList +'"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"long list"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST MaxLen DMRule with ENABLED:true', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{ "Enabled":true}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"'+  QMConnList +'", "Enabled":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET MaxLen DMRule with ENABLED:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "QueueManagerConnection":"TestQMConn" remove long list, fails WHEN ENABLED:true', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"TestQMConn"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"'+  QMConnList +'","Enabled":true}}}' );
			verifyMessage = {"status":400,"Code":"CWLNA0300","Message":"Cannot modify destination mapping rule in enabled state."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"TestQMConn" remove long list, fails WHEN ENABLED:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "QueueManagerConnection":"TestQMConn" remove long list, fails with ENABLED:false NOT merged', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"'+  QMConnList +'", "Enabled":true}}}' );
//			verifyMessage = {"status":400,"Code":"CWLNA0300","Message":"Cannot modify destination mapping rule in enabled state."} ;
			verifyMessage = {"status":400,"Code":"CWLNA0301","Message":"Destination mapping rule state must not change when other updates to the rule are made."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"TestQMConn" remove long list, fails with ENABLED:false NOT merged', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// have to do each o fthe commands above as seperate commands.
        it('should return 200 on POST to Disable MaxLenDMRule to update QueueManagerConnection', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"Enabled":false}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET to Disable MaxLenDMRule to update QueueManagerConnection', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "QueueManagerConnection":"TestQMConn" remove long list', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"TestQMConn"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"TestQMConn", "Enabled":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"TestQMConn" remove long list', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "QueueManagerConnection":"TestQMConn" Enable after remove long list', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"Enabled":true}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"QueueManagerConnection":"TestQMConn", "Enabled":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "QueueManagerConnection":"TestQMConn" Enable after remove long list', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });


//  ====================   Enabled: Specify state of the rule (R)  ====================  //
    describe('Enabled[T]:', function() {

        it('should return 200 on POST "Enabled true"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 120406 - Set TO true when true gets error...
        it('should return 400 on POST "Enabled true" when true', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":true}}}';
            verifyConfig = JSON.parse( payload );
			verifyMessage = { "status":400, "Code":"CWLNA0300", "Message":"Cannot modify destination mapping rule in enabled state."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "Enabled true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Enabled false"', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 120908
        it('should return 200 on POST "Enabled:null (Default)"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":null}}}';
            verifyConfig =  JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled:null(Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// DestmapRule Enabled false for tests to come
        it('should return 200 on POST TestDMRule "Enabled false"', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST MaxLenName DMRule "Enabled false"', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name +'":{"Enabled":false}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


	});


//  ====================   RuleType: The mapping rule type (R)  ====================  //

      describe('RuleType[1-""-14]:', function() {

// 1 	IBM MessageSight topic 	WebSphere MQ queue
    	  it('should return 200 on POST "RuleType 1"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule1_MSt_MQq":{"RuleType":1,"Source":"/mstopic","Destination":"MQ_QUEUE","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 2 	IBM MessageSight topic 	WebSphere MQ topic
    	  it('should return 200 on POST "RuleType 2"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule2_MSt_MQt":{"RuleType":2,"Source":"/mstopic","Destination":"/MQ_TOPIC","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 3 	WebSphere MQ queue 	    IBM MessageSight topic
    	  it('should return 200 on POST "RuleType 3"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule3_MQq_MSt":{"RuleType":3,"Source":"MQ_QUEUE","Destination":"/mstopic","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 3"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 4 	WebSphere MQ topic 	    IBM MessageSight topic
    	  it('should return 200 on POST "RuleType 4"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule4_MQt_MSt":{"RuleType":4,"Source":"/MQ_TOPIC","Destination":"/mstopic","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 4"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 5 	IBM MessageSight topic subtree 	WebSphere MQ queue
    	  it('should return 200 on POST "RuleType 5"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule5_MStt_MQq":{"RuleType":5,"Source":"/ms/topic/subtree/","Destination":"MQ_QUEUE","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 5"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 6 	IBM MessageSight topic subtree 	WebSphere MQ topic
    	  it('should return 200 on POST "RuleType 6"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule6_MStt_MQt":{"RuleType":6,"Source":"/ms/topic/subtree","Destination":"/MQ_TOPIC","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 6"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 7 	IBM MessageSight topic subtree 	WebSphere MQ topic subtree
    	  it('should return 200 on POST "RuleType 7"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule7_MStt_MQtt":{"RuleType":7,"Source":"/ms/topic/subtree","Destination":"/MQ/TOPIC/SUBTREE","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 7"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 8 	WebSphere MQ topic subtree   	IBM MessageSight topic
    	  it('should return 200 on POST "RuleType 8"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule8_MQtt_MSt":{"RuleType":8,"Source":"/MQ/TOPIC/SUBTREE","Destination":"/mstopic","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 8"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 9 	WebSphere MQ topic subtree 	    IBM MessageSight topic subtree
    	  it('should return 200 on POST "RuleType 9"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule9_MQtt_MStt":{"RuleType":9,"Source":"/MQ/TOPIC/SUBTREE","Destination":"/ms/topic/subtree","QueueManagerConnection":"TestQMConn"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 9"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 10 	IBM MessageSight queue 	WebSphere MQ queue
    	  it('should return 200 on POST "RuleType 10"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule10_MSq_MQq":{"RuleType":10,"Source":"ms_queue","Destination":"MQ_QUEUE","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 10"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 11 	IBM MessageSight queue 	WebSphere MQ topic
    	  it('should return 200 on POST "RuleType 11"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule11_MSq_MQt":{"RuleType":11,"Source":"ms_queue","Destination":"/MQTOPIC","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 11"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 12 	WebSphere MQ queue 	        IBM MessageSight queue
    	  it('should return 200 on POST "RuleType 12"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule12_MQq_MSq":{"RuleType":12,"Source":"MQ_QUEUE","Destination":"ms_queue","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 12"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 13 	WebSphere MQ topic 	        IBM MessageSight queue
    	  it('should return 200 on POST "RuleType 13"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule13_MQt_MSq":{"RuleType":13,"Source":"/MQ_TOPIC","Destination":"ms_queue","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 13"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// 14 	WebSphere MQ topic subtree 	IBM MessageSight queue
    	  it('should return 200 on POST "RuleType 14"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule14_MQtt_MSq":{"RuleType":14,"Source":"/MQ/TOPIC/TREE","Destination":"ms_queue","QueueManagerConnection":"TestQMConn", "Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RuleType 14"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Source: The source of the message. Max 65535 characters. (R)  ====================  //
//          "Restrictions":"Valid MQ Queue name, or valid topic string with no # or + wildcards",
// No DEFAULT: required
      describe('Source[]65535:', function() {
	  
        it('should return 200 on POST "Source (48)MAX MQ /topic"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule4_MQt_MSt":{"Source":"'+ FVT.long48MQTopicName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Source (48)MAX MQ /topic"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Source (48)MAX MQ queuename"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule3_MQq_MSt":{"Source":"'+ FVT.long48MQQueueName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Source (48)MAX MQ queuename"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	  
        it('should return 200 on POST "Source (1025)MAX MS /topic"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule1_MSt_MQq":{"Source":"'+ FVT.long1024Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Source (1025)MAX MS /topic"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Source (256)MAX MS queuename"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule10_MSq_MQq":{"Source":"'+ FVT.long256Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Source (256)MAX MS queuename"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });

	  
//  ====================   Destination: The Destination of the message. Max 65535 characters. (R)  ====================  //
//          "Restrictions":"Valid MQ Queue name, or valid topic string with no # or + wildcards",
// No DEFAULT: required

      describe('Destination[]65535:', function() {

        it('should return 200 on POST "Destination (48)MAX MQ /topic"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule2_MSt_MQt":{"Destination":"'+ FVT.long48MQTopicName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Destination (48)MAX MQ /topic"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Destination (48)MAX MQ queuename"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule1_MSt_MQq":{"Destination":"'+ FVT.long48MQQueueName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Destination (48)MAX queuename"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Destination (1024)MAX MS /topic"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule3_MQq_MSt":{"Destination":"'+ FVT.long1024TopicName +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Destination (1024)MAX /topic"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Source (256)MAX MS queuename"', function(done) {
            var payload = '{"DestinationMappingRule":{"DMRule12_MQq_MSq":{"Destination":"'+ FVT.long256Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Source (256)MAX queuename"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });


//  ====================   MaxMessages: Maximum message count that can be buffered on the destination. (R)  ====================  //

      describe('MaxMessages[1-5000-20000000]:', function() {

        it('should return 200 on POST "MaxMessages MIN 1"', function(done) {
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name +'":{"MaxMessages":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages MIN 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessages MAX 20000000"', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages MAX 20000000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessages:null (Default)"', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":null}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages:null (Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessage:2"', function(done) {
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name +'":{"MaxMessages":2}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages:2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessages:19999999"', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":19999999}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages MAX 19999999"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessages:5000 Default"', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages:5000 Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "MaxMessages MAX 20000000"', function(done) {
            var payload = '{"DestinationMappingRule":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "MaxMessages MAX 20000000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });


//  ====================   RetainedMessages: Specify which messages are forwarded to a topic as retained messages. (R)  ====================  //
    describe('RetainedMessages[None, All]:', function() {

        it('should return 400 on POST "RetainedMessages":"ALL" not all UCASE', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"ALL"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"None"}}}' );
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"ALL\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"ALL" not all UCASE', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "RetainedMessages":"All" when Dest Topic', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"All"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"All"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RetainedMessages":"All" when Dest Topic', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "RetainedMessages":"All" when Dest Queue', function(done) {
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name+'":{"RetainedMessages":"All"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"'+ FVT.long256Name+'":{"RetainedMessages":"None"}}}' );
			verifyMessage = { "status":400, "Code":"CWLNA6211", "Message":"The \"RetainedMessages\" cannot be set to \"All\" when the \"Destination\" is a queue." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"All" when Dest Queue', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "RetainedMessages":null (DEFAULT)', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":null}}}';
            verifyConfig =  JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"None"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RetainedMessages":null (DEFAULT)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "RetainedMessages":"all"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"all"}}}';
            verifyConfig =  JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"None"}}}' );
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"all\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"all"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "RetainedMessages":"none"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"none"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"None"}}}' );
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"none\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"none"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "RetainedMessages":"All" when dest is topic', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"All"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RetainedMessages":"All" when dest is topic', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "RetainedMessages":"NONE"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"NONE"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"All"}}}' );
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"NONE\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"NONE" fails, still ALL', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 on POST "RetainedMessages":"AlL"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"AlL"}}}';
            verifyConfig = JSON.parse( '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"All"}}}' );
			verifyMessage = { "status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"AlL\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "RetainedMessages":"AlL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "RetainedMessages":"None"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"None"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "RetainedMessages":"None"', function(done) {
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
        it('should return status 200 on get, "DMRule cfg" persists', function(done) {
            verifyConfig = JSON.parse( expectAllDMRule ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


	
//  ====================++++++++++++++++++++++++++++  Error test cases  ++++++++++++++++++++++++++++====================  //
//    describe('Error:', function() {

	    describe('Error General:', function() {

			it('should return status 400 when trying to POST Delete All DestinationMappingRules, Syntax Error', function(done) {
				var payload = '{"DestinationMappingRule":null}';
				verifyConfig = JSON.parse( payload );
//				verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"DestinationMappingRule\":null} is not valid."};
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"DestinationMappingRule\":null is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET after Delete All DestinationMappingRules, Syntax Error', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 400 when trying to POST Delete DestinationMappingRule via URI', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappngRule":null}}';
				verifyConfig = JSON.parse( payload );
////				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET after POST Delete DestinationMappingRule via URI', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 400 when trying to POST Delete DestinationMappingRule via FULL URI', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappngRule":null}}';
				verifyConfig = JSON.parse( payload );
////				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject+"TestDestinationMappngRule", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET after failed POST Delete DestinationMappingRule via FULL URI', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});
			
        });		


	//  ====================   Name: Name of the Destination Mapping Rule object (R)  ====================  //
	//          "Restrictions":"No leading or trailing space. No control characters or comma.",
		describe('Error Name[256]:', function() {

			it('should return status 400 on POST "Name Too Long"', function(done) {
				var payload = '{"DestinationMappingRule":{"'+ FVT.long257Name +'":{}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: DestinationMappingRule Property: Name Value: '+ FVT.long257Name +'."}' ) ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on GET "Name Too Long"', function(done) {
				verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"'+ FVT.long257Name +'":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: '+ FVT.long257Name +'"}' );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on POST "NoLeadingSpace"', function(done) {
				var payload = '{"DestinationMappingRule":{" NoLeadingSpace":{}}}';
				verifyConfig = JSON.parse(payload);
////				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on GET "NoLeadingSpace"', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoLeadingSpace":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoLeadingSpace"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoLeadingSpace", FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on POST "NoTrailingSpace"', function(done) {
				var payload = '{"DestinationMappingRule":{" NoTrailingSpace":{}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on GET "NoTrailingSpace"', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoTrailingSpace":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoTrailingSpace"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoTrailingSpace", FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on POST "No,Comma"', function(done) {
				var payload = '{"DestinationMappingRule":{" No,Comma":{}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on GET "No,Comma"', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"No,Comma":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: No,Comma"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"No,Comma", FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on POST "1NoNumberFirst"', function(done) {
				var payload = '{"DestinationMappingRule":{" 1NoNumberFirst":{}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on GET "1NoNumberFirst"', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"1NoNumberFirst":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: 1NoNumberFirst"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"1NoNumberFirst", FVT.verify404NotFound, verifyConfig, done);
			});

		});

	//  ====================   QueueManagerConnection: Comma delimited list of Queue Manager Connection objects (R)  ====================  //
	//          "Restrictions":"Name of a QueueManagerConnection",

		describe('Error QueueManagerConnection["",256+?]:', function() {

			it('should return status 200 on post "QueueManagerConnection":"list too long"', function(done) {
				var payload = '{"DestinationMappingRule":{"QMCListNEVERTooLong":{"QueueManagerConnection":"'+  QMConnList +','+  QMConnList +','+  QMConnList +','+  QMConnList +'", "RuleType":1, "Source":"/ms_topic", "Destination":"MQ_QUEUE"}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
//				verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The list too long."} ;
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			it('should return status 200 on get, "QueueManagerConnection":"list too long"', function(done) {
//				verifyConfig = {"status":404,"DestinationMappingRule":{"QMCListNEVERTooLong":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: QMCListNEVERTooLong"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"QMCListNEVERTooLong", FVT.getSuccessCallback, verifyConfig, done);
			});
// This is passing, need to see what MAX LEN LIST is....THERE APPARENLTY IS NOT A LIMIT.   MQ handles what ever you throw at it.

			it('should return 200 on POST "Enabled false" to delete QMCListNEVERTooLong Rule', function(done) {
			    var seemsToTakeALongTime = FVT.mqTimeout * 2 ;
				console.log ("Setting Enabled:false on QMCListNEVERTooLong times out, this.Timeout is:"+ seemsToTakeALongTime );
				this.Timeout = ( seemsToTakeALongTime );   // take a long time in automation for some reason
				var payload = '{"DestinationMappingRule":{"QMCListNEVERTooLong":{"Enabled":false}}}';
				verifyConfig = JSON.parse( payload );
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			
// ??  GETTING ALL at this point cause a failure in the response chunking is truncated prematurely, to resolve I will ONLY get the ONE DMRule  (sad)			
			it('should return 200 on GET, "Enabled False" to delete QMCListNEVERTooLong Rule', function(done) {
//				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"QMCListNEVERTooLong", FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 when deleting "QMCListNEVERTooLong"', function(done) {
				var payload = '{"DestinationMappingRule":{"QMCListNEVERTooLong":{}}}';
				verifyConfig = JSON.parse( payload );
				verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
				FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'QMCListNEVERTooLong', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET all after delete, "QMCListNEVERTooLong" gone', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
			});
			it('should return status 404 on get after delete, "QMCListNEVERTooLong" not found', function(done) {
				verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"QMCListNEVERTooLong":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: QMCListNEVERTooLong"}' );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'QMCListNEVERTooLong', FVT.verify404NotFound, verifyConfig, done);
			});

			
			
		});


	//  ====================   RuleType: The mapping rule type (R)  ====================  //

		describe('Error RuleType[1-""-14]:', function() {

			it('should return status 400 on post "RuleType":0', function(done) {
				var payload = '{"DestinationMappingRule":{"NoRule0":{"RuleType":0}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RuleType Value: \"0\"."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on get, "RuleType":0', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoRule0":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoRule0"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoRule0", FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on post "RuleType":15', function(done) {
				var payload = '{"DestinationMappingRule":{"NoRule15":{"RuleType":15}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RuleType Value: \"15\"."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on get, "RuleType":15', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoRule15":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoRule15"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoRule15", FVT.verify404NotFound, verifyConfig, done);
			});

			it('should return status 400 on post "RuleType":"14"', function(done) {
				var payload = '{"DestinationMappingRule":{"NoRuleString":{"RuleType":"14"}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: NoRuleString Property: RuleType Type: JSON_STRING"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on get, "RuleType":"14"', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoRuleString":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoRuleString"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoRuleString", FVT.verify404NotFound, verifyConfig, done);
			});
// Currently there is, but that may be an error
			it('should return status 400 on post "RuleType":""', function(done) {
				var payload = '{"DestinationMappingRule":{"NoRuleEmptyString":{"RuleType":""}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: NoRuleEmptyString Property: RuleType Type: JSON_STRING"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on get, "RuleType":""', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoRuleEmptyString":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoRuleEmptyString"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoRuleEmptyString", FVT.verify404NotFound, verifyConfig, done);
			});
// Currently there is, but that may be an error
			it('should return status 400 on post "RuleType":null', function(done) {
				var payload = '{"DestinationMappingRule":{"NoRuleDefault":{"RuleType":null}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: NoRuleDefault Property: RuleType Type: JSON_NULL"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 404 on get, "RuleType":null', function(done) {
				verifyConfig = {"status":404,"DestinationMappingRule":{"NoRuleDefault":null},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: NoRuleDefault"} ;
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NoRuleDefault", FVT.verify404NotFound, verifyConfig, done);
			});

		});

	//  ====================   Source: The source of the message. Max 65535 characters. (R)  ====================  //
	//          "Restrictions":"Valid MQ Queue name, or valid topic string with no # or + wildcards",
	// ?DEFAULT:""?
		describe('Error Source["",65535]:', function() {
		  
			it('should return 400 on POST "Source":"#"', function(done) {
				var payload = '{"DestinationMappingRule":{"DMRule2_MSt_MQt":{"Source":"#"}}}';
				verifyConfig = JSON.parse( '{"DestinationMappingRule":{"DMRule2_MSt_MQt":{"RuleType":2,"Source":"/mstopic","Destination":"'+ FVT.long48MQTopicName +'", "RetainedMessages":"None" }}}');
//nope				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: DMRule2_MSt_MQt Property: Source Type: #"} ;
				verifyMessage = {"status":400,"Code":"CWLNA6210","Message":"The \"Destination\" or \"Source\" for \"RuleType=2\" is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "Source":"#" fails', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});
// Rule says Topic but gave a queuename and visaversa

		});

		  
	//  ====================   Destination: The Destination of the message. Max 65535 characters. (R)  ====================  //
	//          "Restrictions":"Valid MQ Queue name, or valid topic string with no # or + wildcards",
	// ?DEFAULT:""?

		describe('Error Destination["",65535]:', function() {
		  
			it('should return 400 on POST "Destination":"#"', function(done) {
				var payload = '{"DestinationMappingRule":{"DMRule3_MQq_MSt":{"Destination":"#"}}}';
				verifyConfig = JSON.parse( '{"DestinationMappingRule":{"DMRule3_MQq_MSt":{"RuleType":3,"Source":"'+ FVT.long48MQQueueName +'","Destination":"'+ FVT.long1024TopicName +'"}}}' );
//nope				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: DMRule3_MQq_MSt Property: Destination Type: #"} ;
				verifyMessage = {"status":400,"Code":"CWLNA6210","Message":"The \"Destination\" or \"Source\" for \"RuleType=3\" is not valid."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "Destination":"#"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});
// Rule says Topic but gave a queuename and visaversa
			
		});


	//  ====================   MaxMessages: Maximum message count that can be buffered on the destination. (R)  ====================  //

		describe('Error MaxMessages[1-5000-20000000]:', function() {

			it('should return status 400 on post "MaxMessages 0"', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"MaxMessages":0}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"0\"."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "MaxMessages 0"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on POST "MaxMessages MAX 20000001"', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"MaxMessages":20000001}}}';
				verifyConfig = JSON.parse(payload);
//				verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"20000001\"."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "MaxMessages MAX 20000001"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on POST "MaxMessages":"5000"', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"MaxMessages":"5000"}}}';
				verifyConfig = JSON.parse(payload);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: TestDestinationMappingRule Property: MaxMessages Type: JSON_STRING"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "MaxMessages":"5000"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});

        });


	//  ====================   Enabled: Specify state of the rule (R)  ====================  //
		describe('Error Enabled[T]:', function() {

			it('should return 200 on POST "Enabled":"true"', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":"true"}}}';
				verifyConfig = JSON.parse(payload);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: TestDestinationMappingRule Property: Enabled Type: JSON_STRING"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "Enabled":"true"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});

		});

	//  ====================   RetainedMessages: Specify which messages are forwarded to a topic as retained messages. (R)  ====================  //
		describe('Error RetainedMessages[None, All]:', function() {

			it('should return 200 on POST "RetainedMessages":"FREE4ALL"', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":"FREE4ALL"}}}';
				verifyConfig = JSON.parse(payload);
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: RetainedMessages Value: \"FREE4ALL\"."} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "RetainedMessages":"FREE4ALL"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
    		});

			it('should return 200 on POST "RetainedMessages":1', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":1}}}';
				verifyConfig = JSON.parse(payload);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: TestDestinationMappingRule Property: RetainedMessages Type: JSON_INTEGER"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "RetainedMessages":1', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on POST "RetainedMessages":true', function(done) {
				var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"RetainedMessages":true}}}';
				verifyConfig = JSON.parse(payload);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: DestinationMappingRule Name: TestDestinationMappingRule Property: RetainedMessages Type: JSON_TRUE"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return 200 on GET, "RetainedMessages":"All"', function(done) {
				verifyConfig = JSON.parse( expectTestDMRule );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestDestinationMappingRule", FVT.getSuccessCallback, verifyConfig, done);
			});



	    });
    
//    });
	
//  ====================++++++++++++++++++++++++++++  Delete test cases  ++++++++++++++++++++++++++++====================  //
    describe('Delete:', function() {
// MUST Disable before you can change or delete
		
        it('should return 200 on POST "Enabled true" on TestDMRule', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled True" on TestDMRule', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return 400 on DELETE ENABLED "TestDestinationMappingRule"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":null}}';
			verifyConfig = JSON.parse( expectTestDMRuleEnabled );
            verifyMessage = {"status":400,"Code":"CWLNA0300","Message":"Cannot modify destination mapping rule in enabled state."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestDestinationMappingRule', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET "TestDestinationMappingRule" still there', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return 200 on POST "Enabled false" to delete TestDMRule', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":{"Enabled":false}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled False" to delete TestDMRule', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on POST "Enabled false" to delete MaxName Rule', function(done) {
		    this.Timeout = ( FVT.mqTimeout );
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name +'":{"Enabled":false}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 on GET, "Enabled False" to delete MaxName Rule', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"DestinationMappingRule":{"'+ FVT.long256Name +'":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"'+ FVT.long256Name +'":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: '+ FVT.long256Name +'"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 when POST DELETE "TestDestinationMappingRule" bad form', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":null}}';
			verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: /ima/v1/configuration/DestinationMappingRule is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: /ima/v1/configuration/DestinationMappingRule is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET, "TestDestinationMappingRule" is not gone yet', function(done) {
		    verifyConfig = JSON.parse( expectTestDMRule );;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return 200 on DELETE "TestDestinationMappingRule"', function(done) {
            var payload = '{"DestinationMappingRule":{"TestDestinationMappingRule":null}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestDestinationMappingRule', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET all after delete, "TestDestinationMappingRule" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestDestinationMappingRule" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"TestDestinationMappingRule":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: TestDestinationMappingRule"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestDestinationMappingRule', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when deleting "MissingDestinationMappingRule"', function(done) {
            var payload = '{"DestinationMappingRule":{"MissingDestinationMappingRule":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'MissingDestinationMappingRule', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 on GET all after delete, "MissingDestinationMappingRule" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "MissingDestinationMappingRule" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"DestinationMappingRule":{"MissingDestinationMappingRule":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: DestinationMappingRule Name: MissingDestinationMappingRule"}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'MissingDestinationMappingRule', FVT.verify404NotFound, verifyConfig, done);
        });
		
		for (var i = 0; i < DMRuleList.length; i++) {
		    deleteDestinationMappingRule( DMRuleList[i] );
		}
    });
	
//  ====================++++++++++++++++++++++++++++  CLEANUP  ++++++++++++++++++++++++++++====================  //
	describe('Cleanup PreReq-QMConn:', function() {
	//QueueManagerConnection..
	    
        for( i = 0 ; i < maxList ; i++ ) {
            var paddedUniqueName = 'QM_'+( 1400 + i).toString().slice(-4);
            paddedObjectName = 'QMConn4' + paddedUniqueName + max256ObjectName ;
            deleteQueueManagerConnection( paddedObjectName, paddedUniqueName );
        }
 //       deleteQueueManagerConnection( "TestQMConn",  FVT.MQSERVER1_IP  );
        
        it('should return status 200 on DELETE "QueueManagerConnection TestQMConn for '+ FVT.MQSERVER1_IP +'"', function(done) {
            verifyConfig = JSON.parse( '{"QueueManagerConnection":{"TestQMConn":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"QueueManagerConnection/TestQMConn" , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return status 404 on GET "QueueManagerConnection TestQMConn for '+ FVT.MQSERVER1_IP +'"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"QueueManagerConnection":{"TestQMConn":{}},"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: QueueManagerConnection Name: TestQMConn" }' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'QueueManagerConnection/TestQMConn' , FVT.verify404NotFound, verifyConfig, done);
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
        
        it('should return 200 on POST TraceLevel to default', function(done) {
            var payload = '{"TraceLevel":"5"}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
    });

});
