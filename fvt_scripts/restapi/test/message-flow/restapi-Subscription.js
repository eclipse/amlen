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

var url = 'http://' + process.env.IMA_AdminEndpoint;
FVT.trace( 0,   ' === IMA_AdminEndpoint: '+ url );

var index = url.lastIndexOf(':');
var msgServer = url.substring(7, index);
FVT.trace( 0,  'MQTTServer IP is :  '+ msgServer );


var uriObject = 'Subscription/';
var expectDefault = '{"Subscription":{},"Version": "'+ FVT.version +'"}';

function PubSub( port, clientID, cleanSession, store, msgSend, waitSeconds, topic, qosPub, qosSub) {
		var msgCount = 0;
		var client_options = JSON.parse( '{"clientId":"'+ clientID +'", "clean":'+ cleanSession +', "incomingStore":"'+ store +'"}' );
//??number is not a function??		this.timeout(3000);
		FVT.trace( 1,  'client:'+ clientID +' CleanSession:'+ cleanSession +' Store:'+ store +' Send:'+ msgSend +' msgs w/in '+ waitSeconds +' using Topic:topic/'+ topic +' at QoS(Pub, Sub):('+ qosPub +','+ qosSub +').' );

		try {
			it('should send and receive "'+ msgSend +'" messages on PORT:'+ port, function(done) {
				this.timeout( waitSeconds );
				var client = mqtt.connect('mqtt://'+ msgServer +':'+ port, client_options );

				client.on('connect', function() {
					FVT.trace( 0,  ' C O N N E C T : '+ clientID  );
					client.subscribe('topic/'+port, {"clean":false,"qos":qosSub}, FVT.subCallback );
				    // Don't want to publish to quickly - subscriber might not be ready, delay these commands for 1 sec
					    setTimeout( function() {
							FVT.trace( 0,  'S U B S C R I B E : Preparing Client '+ clientID );
						}, 1000 );
							
					for ( var i=1 ; i <= msgSend ; i++ ) {
						client.publish('topic/'+port, 'Hello mqtt:'+ port +' msg#'+ i, {"qos":qosPub,"retain":true} );
					}
				});

				client.on('message', function(topic, message) {
					msgCount++;
					FVT.trace( 0,  ' M E S S A G E #'+ msgCount +':: '+ message.toString() );
					if ( msgCount == msgSend ) {
					    // Don't want to close to quickly on last message, delay these commands for 1 sec
					    setTimeout( function() {
								client.unsubscribe('topic/'+port);
								client.end();
								FVT.trace( 0,  'C O M P L E T E : Client is done() called for '+ clientID );
							}, 1000 );
					}
				});

				client.on('error', function() {
					FVT.trace( 0,  ' E R R O R : '+ clientID  );
//					done();
				});

				client.on('subscribe', function() {
					FVT.trace( 0,  ' S U B S C R I B E : '+ clientID  );
//					done();
				});

				client.on('publish', function() {
					FVT.trace( 0,  ' P U B L I S H : '+ clientID  );
//					done();
				});

				client.on('disconnect', function() {
					FVT.trace( 0,  ' D I S C O N N E C T : '+ clientID  );
//					done();
				});

				client.on('close', function() {
					FVT.trace( 0,  ' C L O S E D : '+ clientID  );
					done();
				});

			});
		} catch (ex) {
		   FVT.trace( 0, 'Unexpected Exception in MQTT Client '+ clientID +' Msging'+ String(ex) );
		}
	};
	

//  ====================  Test Cases Start Here  ====================  //

describe('Subscription:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "Subscription":{}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done );
        });
	
	});

//  ====================   Create Subscription test cases  ====================  //
    describe('Enable the Endpoints for Messaging', function() {

        it('should return status 200 on post DemoEndpoint "Enabled:true"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done );
        });
        it('should return status 200 on get, "Enabled:true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post DemoMqttEndpoint "Enabled:true"', function(done) {
            var payload = '{"Endpoint":{"DemoMqttEndpoint":{"Enabled":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done );
        });
        it('should return status 200 on get, "Enabled:true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoMqttEndpoint', FVT.getSuccessCallback, verifyConfig, done );
        });


    });
	
    describe('Subroutine Subscription and Publish to Port 16102: ', function() {   
      //PubSub( port, "clientID", cleanSession, "store",     msgSend, waitSeconds, "topicSuffix", qosPub, qosSub)	
		PubSub( 16102, "client16102", false, "inbox_client16102", 50, 3000, "2/16102", 1, 1);
    });
/* 	
    describe('Create the Subscription and Publish to Port 16102: ', function() {
//	    var CID1_16102_options = {"clientId":"CID1_16102", "clean":false,"connectTimeout":60,"incomingStore":"inbox_CID1_16102","outgoingStore":"outbox_CID1_16102" } ;
//	    var CID1_16102_options = {"clientId":"CID1_16102", "clean":false,"connectTimeout":60,"incomingStore":"inbox_CID1_16102" } ;
	    var CID1_16102_options = {"clientId":"CID1_16102", "clean":false,"incomingStore":"inbox_CID1_16102" } ;
        var msgCount = 0;
        var msgSend = 10;
	    this.timeout(3000);
		try {
			it('should send and receive "'+ msgSend +'" messages on Port 16102', function(done) {
				this.timeout(3000);
	//			var client = mqtt.connect('mqtt://10.10.10.10:16102', CID1_16102_options );
				var client = mqtt.connect('mqtt://'+ FVT.msgServer +':16102', CID1_16102_options );
				client.on('connect', function() {
					client.subscribe('topic/2/16102', {"clean":false,"qos":2}, FVT.subCallback );
//QoS					client.subscribe('topic/2/16102', {"clean":false,"qos":1}, FVT.subCallback );
					for ( var i=1 ; i <= msgSend ; i++ ) {
						client.publish('topic/2/16102', 'Hello mqtt:16102 #'+ i, {"qos":2,"retain":true} );
//QoS						client.publish('topic/2/16102', 'Hello mqtt:16102 #'+ i, {"qos":1,"retain":true} );
					}
				});

				client.on('message', function(topic, message) {
					msgCount++;
					FVT.trace( 0,  ' M E S S A G E #'+ msgCount +':: '+ message.toString() );
					if ( msgCount == msgSend ) {
					    client.unsubscribe('topic/2/16102');
						client.end();
						FVT.trace( 0,  'C O M P L E T E : Client.end-ed and done() called for CID1_16102.' );
//						done();
					}
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
	 */
	
    describe('Subroutine Subscription and Publish to Port 1883:', function() {
      //PubSub( port, "clientID", cleanSession, "store",     msgSend, waitSeconds, "topicSuffix", qosPub, qosSub)	
		PubSub( 1883, "client1883", false, "inbox_client1883", 50, 3000, "1883", 1, 1);
    });
	/* 
    describe('Create the Subscription and Publish to Port 1883:', function() {
		var msgCount = 0;
		var msgSend = 50;
		var CID1_1883_options = {"clientId":"CID1_1883", "clean":false, "incomingStore":"inbox_CID1_1883"} ;
		this.timeout(3000);
		try {
			it('should send and receive "'+ msgSend +'" messages on POST 1883', function(done) {
				this.timeout(3000);
	//			var client = mqtt.connect('mqtt://10.10.10.10:1883', CID1_1883_options );
				var client = mqtt.connect('mqtt://'+ msgServer +':1883', CID1_1883_options );
				client.on('connect', function() {
					client.subscribe('topic/1883', {"clean":false,"qos":1}, FVT.subCallback );
					for ( var i=1 ; i <= msgSend ; i++ ) {
						client.publish('topic/1883', 'Hello mqtt:1883 #'+ i, {"qos":1,"retain":true} );
					}
				});

				 client.on('message', function(topic, message) {
					msgCount++;
					FVT.trace( 0,  ' M E S S A G E #'+ msgCount +':: '+ message.toString() );
					if ( msgCount == msgSend ) {
						client.unsubscribe('topic/1883');
						client.end();
						FVT.trace( 0,  'C O M P L E T E : Client.end-ed and done() called for CID1_1883.' );
//						done();
					}
				});

				client.on('close', function() {
					FVT.trace( 0,  ' C L O S E D : CID1_1883'  );
					done();
				});

			});
		} catch (ex) {
		   FVT.trace( 0, 'Unexpected Exception in MQTT:1883 Msging'+ String(ex) );
		}
	});
	 */
	
    describe('Get the Subscriptions:', function() {
	
        // it('should return status 200 on "Get Subscription for CID1_16102"', function(done) {
	    	// var payload = '{"Subscription":{"/topic/2/16102":{}}}';
            // verifyConfig = JSON.parse(payload);
            // FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'CID1_16102', FVT.getSuccessCallback, verifyConfig, done );
        // });

        it('should return status 200 on "Get Subscription for client16102"', function(done) {
	    	var payload = '{"Subscription":[{"SubName": "topic/2/16102","TopicString":"topic/2/16102","ClientID": "client16102","IsDurable": "True","MaxMessages": 5000,"RejectedMsgs": 0,"IsShared": "False","Consumers": 0,"DiscardedMsgs": 0,"ExpiredMsgs": 0,"MessagingPolicy": "DemoTopicPolicy"}]}';
            // Ignoring Fields that are too random:  "BufferedMsgsHWM": 51,"PublishedMsgs": 357,"BufferedHWMPercent": 1,"BufferedMsgs": 0,"BufferedPercent": 0,
            verifyConfig = JSON.parse(payload);
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?ClientID=client16102', FVT.getSuccessCallback, verifyConfig, done );
        });
	
        // it('should return status 200 on post "Get Subscription for CID1_1883"', function(done) {
	    	// var payload = '{"Subscription":{"/topic/1883":{}}}';
            // verifyConfig = JSON.parse(payload);
            // FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'CID1_1883', FVT.getSuccessCallback, verifyConfig, done);
        // });
	
        it('should return status 200 on post "Get Subscription for client1883"', function(done) {
	    	var payload = '{"Subscription":[{"SubName": "topic/1883","TopicString":"topic/1883","ClientID": "client1883","IsDurable": "True","MaxMessages": 5000,"RejectedMsgs": 0,"IsShared": "False","Consumers": 0,"DiscardedMsgs": 0,"ExpiredMsgs": 0,"MessagingPolicy": "DemoTopicPolicy"}]}';
            // Ignoring Fields that are too random:  "BufferedMsgsHWM": 51,"PublishedMsgs": 357,"BufferedHWMPercent": 1,"BufferedMsgs": 0,"BufferedPercent": 0,
            verifyConfig = JSON.parse(payload);
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?ClientID=client1883', FVT.getSuccessCallback, verifyConfig, done);
        });

	});
	
//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {
// Delete does not use QueryParameters anymore
        it('should return status 400 when deleting "Subscription /topic/2/16102":', function(done) {
            verifyConfig = {"Subscription":[{"SubName":"topic/2/16102"}]};
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: /ima/v1/service/Subscription/ is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: /ima/v1/service/Subscription/ is not valid."};
//            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'?ClientID=client16102', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'?SubName=topic/2/16102', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Subscription /topic/2/16102" still there', function(done) {
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after delete, "Subscription /topic/2/16102" still found', function(done) {
		    verifyConfig = { "Status":200,"Subscription":[{"SubName":"topic/2/16102"}] } ;
//??            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?ClientID=client16102', FVT.verify404NotFound, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?ClientID=client16102', FVT.getSuccessCallback, verifyConfig, done);
        });
// This is how to DELETE
        it('should return status 200 when deleting "Subscription ClientId/SubName /topic/2/16102"', function(done) {
            verifyConfig = {"Subscription":[{"SubName":"topic/2/16102"}]};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'?ClientID=CID1_16102', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
//            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'?ClientID=CID1_16102&SubName=/topic/2/16102', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'client16102/topic/2/16102', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Subscription ClientId/SubName /topic/2/16102" gone', function(done) {
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Subscription ClientId/SubName /topic/2/16102" not found', function(done) {
//		    verifyConfig = { "Status":404,"Subscription":[{"SubName":"topic/2/16102"}] } ;
		    verifyConfig = { "Status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Subscription Name: topic/2/16102","Subscription":[{"SubName":"topic/2/16102"}] } ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?clientID=client16102', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "Subscription /topic/1883":', function(done) {
            verifyConfig = {"Subscription":[{"SubName":"topic/1883"}]};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject+'client1883/topic/1883', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Subscription /topic/1883" gone', function(done) {
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Subscription /topic/1883" not found', function(done) {
//		    verifyConfig = { "Status":404,"Subscription":[] } ;
		    verifyConfig = { "Status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Subscription Name: topic/1883","Subscription":[{"SubName":"topic/1883"}] } ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject+'?ClientID=client1883', FVT.verify404NotFound, verifyConfig, done);
        });

    });



//  ====================  Cleanup after the test cases  ====================  //
    describe('Cleanup After Test Cases:', function() {

        it('should return status 200 on post DemoEndpoint "Enabled:false"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled:false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'/Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post DemoMqttEndpoint "Enabled: false"', function(done) {
            var payload = '{"Endpoint":{"DemoMqttEndpoint":{"Enabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled:false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'/Endpoint/DemoMqttEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on DELETE  "MQTT ClientID client1883"', function(done) {
            verifyConfig = {"MQTTClient":[{"ClientID":"client1883"}]};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+'MQTTClient/client1883', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET all after delete "MQTT ClientID client1883" gone', function(done) {
            FVT.makeGetRequest( FVT.uriMonitorDomain+'MQTTClient', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after delete, "MQTT ClientID client1883" not found', function(done) {
		    verifyConfig = { "Status":404,"Subscription":[] } ;
		    verifyConfig = { "Status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Subscription Name: topic/1883","Subscription":[{"SubName":"topic/1883"}] } ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+'MQTTClient?ClientID=client1883', FVT.verify404NotFound, verifyConfig, done);
        });
	
        it('should return status 200 on DELETE  "MQTT ClientID client16102"', function(done) {
            verifyConfig = {"MQTTClient":[{"ClientID":"client16102"}]};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+'MQTTClient/client16102', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET all after delete "MQTT ClientID client16102" gone', function(done) {
            FVT.makeGetRequest( FVT.uriMonitorDomain+'MQTTClient', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after delete, "MQTT ClientID client16102" not found', function(done) {
		    verifyConfig = { "Status":404,"Subscription":[] } ;
		    verifyConfig = { "Status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Subscription Name: topic/1883","Subscription":[{"SubName":"topic/1883"}] } ;
            FVT.makeGetRequest( FVT.uriMonitorDomain+'MQTTClient?ClientID=client16102', FVT.verify404NotFound, verifyConfig, done);
        });
		
    });


//  ====================  Error test cases  ====================  //
    describe('Error Test Cases: General:', function() {

        it('should return status 400 when trying to Delete All Subscriptions', function(done) {
            verifyConfig = '"Subscription":{}';
//			verifyMessage = {"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ObjectName Value: null."};
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/Subscription/ is not valid."};
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/Subscription/ is not valid."};
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return status 400 when trying to use POST MonitorDomain to Create Subscriptions', function(done) {
            var payload = '{"Subscription":{"IllegalWay":{"SubscriptionName":"ShouldNotWork","ClientID":"*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/monitor/ is not valid."};
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: POST is not valid."};
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: POST is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriMonitorDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return status 400 when trying to use POST ServiceDomain to Create Subscriptions', function(done) {
            var payload = '{"Subscription":{"IllegalWay2":{"SubscriptionName2":"ShouldNotWorkEither","ClientID":"*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service is not valid."};
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/ is not valid."};
			verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
		
        it('should return status 400 when trying to use POST ConfigDomain to Create Subscriptions', function(done) {
            var payload = '{"Subscription":{"IllegalWay2":{"SubscriptionName2":"ShouldNotWorkEither","ClientID":"*"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/configuration is not valid."};
			verifyMessage = {"status":400,"Code":"CWLNA0207","Message":"An invalid parameter was supplied."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });

        it('should return status 200 on get, Get All Subscriptions, yet no Subscriptions should be left', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriMonitorDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });
    

});
