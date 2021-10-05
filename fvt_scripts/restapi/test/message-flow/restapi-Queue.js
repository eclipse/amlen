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
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var verifyConfig = {} ;
var verifyMessage = {};
var uriObject = 'Queue/' ;
var expectDefault = '{"Queue":{},"Version": "'+ FVT.version +'"}' ;

var expectAllQueues = '{ "Version": "' + FVT.version + '", "Queue": { \
 "TestQueue": {  "ConcurrentConsumers": true,  "Description": "",  "AllowSend": true,  "MaxMessages": 5000  }, \
 "'+ FVT.long256Name +'": {  "ConcurrentConsumers": true,  "Description": "",  "AllowSend": true,  "MaxMessages": 5000  }}}' ;  



//  ====================  Test Cases Start Here  ====================  //

describe('Queue:', function() {
this.timeout( FVT.defaultTimeout );

// 101163
    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "Queue":{}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	
	});

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Add Update Field: Description', function() {

        it('should return status 200 on post "No description"', function(done) {
            var payload = '{"Queue":{"TestQueue":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"Queue":{"TestQueue":{"Description": "The TEST Queue."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Remove Description":""', function(done) {
            var payload = '{"Queue":{"TestQueue":{"Description": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Remove Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Max Length Description"', function(done) {
            var payload = '{"Queue":{"TestQueue":{"Description": "'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post to reset "Description"', function(done) {
            var payload = '{"Queue":{"TestQueue":{"Description": null}}}';
            verifyConfig = {"Queue":{"TestQueue":{"Description": ""}}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get to reset "Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
//  ====================   Valid Range test cases  ====================  //
    describe('Valid Range:', function() {
      describe('Name:', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
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
        it('should return status 200 on get, "Queues" persists', function(done) {
            verifyConfig = JSON.parse( expectAllQueues ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

      });

      describe('DiscardMessages[F]notSettable:', function() {
// 109688 - DiscardMessages is for deleting a queue with messages on it, and is used with a query parameter:  /ima/v1/configuration/Queue/[MyQueue]?DiscardMessages=True
        it('should return status 200 on post "EmptyQueue"', function(done) {
            var payload = '{"Queue":{"EmptyQueue":{}}}';
//            verifyConfig = JSON.parse( '{"Queue":{"EmptyQueue":{"MaxMessages": 5000,"Description": "","DiscardMessages": false,"AllowSend": true,"ConcurrentConsumers": true}}}' );
            verifyConfig = JSON.parse( '{"Queue":{"EmptyQueue":{"MaxMessages": 5000,"Description": "","AllowSend": true,"ConcurrentConsumers": true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET EmptyQueue', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when deleting "EmptyQueue"', function(done) {
            var payload = '{"Queue":{"EmptyQueue":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'EmptyQueue', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "EmptyQueue" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "EmptyQueue" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"Queue":{"EmptyQueue":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: EmptyQueue","Queue":{"EmptyQueue":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'EmptyQueue', FVT.verify404NotFound, verifyConfig, done);
        });

// NOT Empty Queue and DiscardMessages  --  CAN NOT be done with MQTT Client, need a JMS Implemenation in Javascript to do in this test case.
/*
    describe('DiscardMessages Test: ', function() {

		describe('Create Queue,Enable Endpoint: ', function() {
			it('should return status 200 on POST "EmptyQueue"', function(done) {
				var payload = '{"Queue":{"EmptyQueue":{"MaxMessages": 5}}}';
				verifyConfig = JSON.parse( '{"Queue":{"EmptyQueue":{"MaxMessages": 5,"Description": "","DiscardMessages": false,"AllowSend": true,"ConcurrentConsumers": true}}}' );
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			it('should return status 200 on GET EmptyQueue', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 on post DemoEndpoint "Enabled:true"', function(done) {
				var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled":true}}}';
				verifyConfig = JSON.parse(payload);
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done );
			});
			it('should return status 200 on get, "Enabled:true"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
			});
			
        });
	
		describe('Create the Subscription and Publish to Port 16102: ', function() {
			var CID1_16102_options = {"clientId":"CID1_16102", "clean":false,"incomingStore":"inbox_CID1_16102" } ;
			var msgCount = 0;
			var msgSend = 10;
			this.timeout(3000);
			try {
				it('should send and receive "'+ msgSend +'" messages on Port 16102', function(done) {
					this.timeout(3000);
					var client = mqtt.connect('mqtt://'+ FVT.msgServer +':16102', CID1_16102_options );
					FVT.trace( 1,  'client connected: ' + client.toString() );
					client.on('connect', function() {
						client.subscribe('EmptyQueue', {"clean":false,"qos":2}, FVT.subCallback );
    					FVT.trace( 1,  'client subscribed' );
						for ( var i=1 ; i <= msgSend ; i++ ) {
							client.publish('EmptyQueue', 'Hello mqtt:16102 #'+ i, {"qos":2,"retain":true} );
        					FVT.trace( 1,  'client published' );
						}
					});

					client.on('message', function(topic, message) {
						msgCount++;
						FVT.trace( 0,  ' M E S S A G E #'+ msgCount +':: '+ message.toString() );
						if ( msgCount == msgSend ) {
							client.unsubscribe('EmptyQueue');
							client.end();
							FVT.trace( 0,  'C O M P L E T E : Client.end-ed and done() called for CID1_16102.' );
						}
					});

					client.on('read', function() {
						FVT.trace( 0,  ' R E A D : CID1_16102'  );
					});

					client.on('close', function() {
						FVT.trace( 0,  ' C L O S E D : CID1_16102'  );
						done();
					});

				});
			} catch (ex) {
			   FVT.trace( 0, 'Unexpected Exception in MQTT:16102 Msging'+ String(ex) );
			}
		
		});
	

		describe('Delete Queue,Disable Endpoint: ', function() {
			it('should return status 200 on post DemoEndpoint "Enabled:false"', function(done) {
				var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled":false}}}';
				verifyConfig = JSON.parse(payload);
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done );
			});
			it('should return status 200 on get, "Enabled:false"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
			});

			
			
			it('should return status 400 on DELETE "EmptyQueue" with messages queued', function(done) {
				var payload = '{"Queue":{"EmptyQueue":{}}}';
				verifyConfig = JSON.parse( payload );
				verifyMessage = {"status":400,"Code": "CWLNA6011","Message": "The requested configuration change has NOT completed successfully."};
				FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'EmptyQueue', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get all after delete, "EmptyQueue" failed with not EMPTY', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 when deleting "EmptyQueue"', function(done) {
				var payload = '{"Queue":{"EmptyQueue":{}}}';
				verifyConfig = JSON.parse( payload );
				verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
				FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'EmptyQueue?DiscardMessages=True', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get all after delete, "EmptyQueue" gone', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
			});
			it('should return status 404 on get after delete, "EmptyQueue" not found', function(done) {
				verifyConfig = JSON.parse( '{"status":404,"Queue":{"EmptyQueue":{}}}' );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'EmptyQueue', FVT.verify404NotFound, verifyConfig, done);
			});

		});
	});
*/

//  IT CAN NOT BE SET and now 400 (120951)
        it('should return status 404 on post "DiscardMessages true"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"DiscardMessages":true}}}';
//            verifyConfig = JSON.parse( '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages": 5000,"Description": "","DiscardMessages": false,"AllowSend": true,"ConcurrentConsumers": true}}}' );
            verifyConfig = JSON.parse( '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages": 5000,"Description": "","AllowSend": true,"ConcurrentConsumers": true}}}' );
//            verifyMessage = {"status":404,"Code": "CWLNA0111","Message": "The property name is not valid: Property: DiscardMessages."};
//            verifyMessage = JSON.parse( '{"status":404,"Code": "CWLNA0138","Message": "The property name is invalid. Object: Queue Name: '+ FVT.long256Name +' Property: DiscardMessages"}'  );
            verifyMessage = JSON.parse( '{"status":400,"Code": "CWLNA0138","Message": "The property name is invalid. Object: Queue Name: '+ FVT.long256Name +' Property: DiscardMessages"}'  );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "DiscardMessages false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 404 on post "DiscardMessages false"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"DiscardMessages":false}}}';
//            verifyConfig = JSON.parse( '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages": 5000,"Description": "","DiscardMessages": false,"AllowSend": true,"ConcurrentConsumers": true}}}' );
            verifyConfig = JSON.parse( '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages": 5000,"Description": "","AllowSend": true,"ConcurrentConsumers": true}}}' );
//            verifyMessage = {"status":404,"Code": "CWLNA0111","Message": "The property name is not valid: Property: DiscardMessages."};
//            verifyMessage = JSON.parse( '{"status":404,"Code": "CWLNA0138","Message": "The property name is invalid. Object: Queue Name: '+ FVT.long256Name +' Property: DiscardMessages"}'  );
            verifyMessage = JSON.parse( '{"status":400,"Code": "CWLNA0138","Message": "The property name is invalid. Object: Queue Name: '+ FVT.long256Name +' Property: DiscardMessages"}'  );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "DiscardMessages false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

      });


      describe('AllowSend[T]:', function() {
        it('should return status 200 on post "AllowSend false"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"AllowSend":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AllowSend false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 101164
        it('should return status 200 on post "AllowSend Default(true)"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"AllowSend":null}}}';
            verifyConfig = JSON.parse('{"Queue":{"' + FVT.long256Name + '":{"AllowSend":true}}}');
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AllowSend Default(true)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "AllowSend true"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"AllowSend":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AllowSend true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "AllowSend false"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"AllowSend":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AllowSend false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

      });
	  
      describe('ConcurrentConsumers[T]:', function() {
	  
        it('should return status 200 on post "ConcurrentConsumers false"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"ConcurrentConsumers":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConcurrentConsumers false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 101164
        it('should return status 200 on post "ConcurrentConsumers Default(true)"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"ConcurrentConsumers":null}}}';
            verifyConfig = JSON.parse('{"Queue":{"' + FVT.long256Name + '":{"ConcurrentConsumers":true}}}');
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConcurrentConsumers Default(true)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ConcurrentConsumers true"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"ConcurrentConsumers":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConcurrentConsumers true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ConcurrentConsumers false"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"ConcurrentConsumers":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConcurrentConsumers false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });

	  
      describe('MaxMessages[1-5000-20000000]:', function() {

        it('should return status 200 on post "MaxMessages MIN 1"', function(done) {
            var payload = '{"Queue":{"'+ FVT.long256Name +'":{"MaxMessages":1}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages MIN 1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessages MAX 20000000"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages":20000000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages MAX 20000000"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessages:null (Default)"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages":null}}}';
            verifyConfig = JSON.parse( '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages:null (Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessage:2"', function(done) {
            var payload = '{"Queue":{"'+ FVT.long256Name +'":{"MaxMessages":2}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages:2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessages:19999999"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages":19999999}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages MAX 19999999"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MaxMessages:5000 Default"', function(done) {
            var payload = '{"Queue":{"' + FVT.long256Name + '":{"MaxMessages":5000}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxMessages:5000 Default"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
      });

    });

	
//  ====================  Delete test cases  ====================  //
    describe('Delete Test Cases:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"Queue":{"'+ FVT.long256Name +'":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"Queue":{"'+ FVT.long256Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: '+ FVT.long256Name +'","Queue":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when POST DELETE "TestQueue" now bad form', function(done) {
            var payload = '{"Queue":{"TestQueue":null}}';
			verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: /ima/v1/configuration/Queue is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: /ima/v1/configuration/Queue is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, "TestQueue" is not gone yet', function(done) {
		    verifyConfig = {"Queue":{"TestQueue":{"Description":""}}}
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// No Post Delete, have to really delete

        it('should return status 400 when POST DELETE "TestQueue" now bad form', function(done) {
            var payload = '{"Queue":{"TestQueue":null}}';
			verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestQueue\":null is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: \"TestQueue\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, "TestQueue" is not gone yet', function(done) {
		    verifyConfig = {"Queue":{"TestQueue":{"Description":""}}}
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 when deleting "TestQueue"', function(done) {
            var payload = '{"Queue":{"TestQueue":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestQueue', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestQueue" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestQueue" not found', function(done) {
//		    verifyConfig = {"status":404,"Queue":{"TestQueue":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: TestQueue","Queue":{"TestQueue":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestQueue', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when deleting "MissingQueue"', function(done) {
            var payload = '{"Queue":{"MissingQueue":{}}}';
			verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'MissingQueue', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "MissingQueue" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "MissingQueue" not found', function(done) {
//		    verifyConfig = {"status":404,"Queue":{"MissingQueue":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: MissingQueue","Queue":{"MissingQueue":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'MissingQueue', FVT.verify404NotFound, verifyConfig, done);
        });
		
    });

//  ====================  Error test cases  ====================  //
    describe('Error: General:', function() {

        it('should return status 400 when trying to Delete All Queues, Syntax Error', function(done) {
            var payload = '{"Queue":null}';
		    verifyConfig = {"Queue":{}};
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"Queue\":null} is not valid."};
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid."} ;
			verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/configuration/Queue is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain+uriObject, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Queues, Syntax Error', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

		
    });
    

describe('Error: Name[256]:', function() {

        it('should return status 400 POST Queue Name TooLong.', function(done) {
            var payload = '{"Queue":{"'+ FVT.long257Name +'":{}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Queue Property: Name Value: '+ FVT.long257Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 Get Queue Name TooLong', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: '+ FVT.long257Name +'","Queue":{"'+ FVT.long257Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });
		
    });

describe('Error: Description:', function() {

        it('should return status 400 POST Queue DescriptionTooLong.', function(done) {
            var payload = '{"Queue":{"DescTooLong":{"Description":"'+ FVT.long1025String +'"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: Queue Property: Description Value: '+ FVT.long1025String +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET DescriptionTooLong not created', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: DescTooLong","Queue":{"DescTooLong":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"DescTooLong", FVT.verify404NotFound, verifyConfig, done);
        });
		
    });
    

describe('Error: AllowSend:', function() {
// 109698
        it('should return status 400 when AllowSend String not Boolean ', function(done) {
            var payload = '{"Queue":{"AllowSendString":{"AllowSend":"YES"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: AllowSendString Property: AllowSend Type: InvalidType:JSON_STRING"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: AllowSendString Property: AllowSend Type: JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, AllowSendString was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"AllowSendString":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: AllowSendString","Queue":{"AllowSendString":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'AllowSendString', FVT.verify404NotFound, verifyConfig, done);

        });

        it('should return status 400 when AllowSend Integer not Boolean ', function(done) {
            var payload = '{"Queue":{"AllowSendInteger":{"AllowSend":1}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: AllowSendInteger Property: AllowSend Type: InvalidType:JSON_INTEGER"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: AllowSendInteger Property: AllowSend Type: JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, AllowSendInteger was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"AllowSendInteger":{}}};
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: AllowSendInteger","Queue":{"AllowSendInteger":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'AllowSendInteger', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

describe('Error: ConcurrentConsumers:', function() {

        it('should return status 400 when ConcurrentConsumers not a String ', function(done) {
            var payload = '{"Queue":{"ConcurrentConsumersBoolean":{"ConcurrentConsumers":"NO"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: ConcurrentConsumersBoolean Property: ConcurrentConsumers Type: InvalidType:JSON_STRING"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: ConcurrentConsumersBoolean Property: ConcurrentConsumers Type: JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConcurrentConsumersBoolean was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"ConcurrentConsumersBoolean":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: ConcurrentConsumersBoolean","Queue":{"ConcurrentConsumersBoolean":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ConcurrentConsumersBoolean', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when ConcurrentConsumers not an Integer ', function(done) {
            var payload = '{"Queue":{"ConcurrentConsumersBoolean":{"ConcurrentConsumers":0}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: ConcurrentConsumersBoolean Property: ConcurrentConsumers Type: InvalidType:JSON_STRING"};
            verifyMessage = {"status":400, "Code":"CWLNA0127","Message":"The property type is not valid. Object: Queue Name: ConcurrentConsumersBoolean Property: ConcurrentConsumers Type: JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ConcurrentConsumersBoolean was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"ConcurrentConsumersBoolean":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: ConcurrentConsumersBoolean","Queue":{"ConcurrentConsumersBoolean":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ConcurrentConsumersBoolean', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

describe('Error: MaxMessages[1-5000-20000000]:', function() {

		it('should return status 400 when MaxMessages exceeds MAX', function(done) {
            var payload = '{"Queue":{"ExceedMaxMessages":{"MaxMessages":20000001}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: 20000001." };
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"20000001\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ExceedMaxMessages was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"ExceedMaxMessages":{}}};
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: ExceedMaxMessages","Queue":{"ExceedMaxMessages":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ExceedMaxMessages', FVT.verify404NotFound, verifyConfig, done);
        });

		it('should return status 400 when MaxMessages exceeds MIN', function(done) {
            var payload = '{"Queue":{"ExceedMaxMessages":{"MaxMessages":0}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration/Queue is not valid." };
//            verifyMessage = {"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessageSize Value: 0." };
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxMessages Value: \"0\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ExceedMaxMessages was not created', function(done) {
//            verifyConfig = {"status":404,"Queue":{"ExceedMaxMessages":{}}};
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Queue Name: ExceedMaxMessages","Queue":{"ExceedMaxMessages":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ExceedMaxMessages', FVT.verify404NotFound, verifyConfig, done);
        });

	});

});
