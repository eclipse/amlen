/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
define(['ism/tests/Utils','dojo/_base/lang','ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {

	doh.register("ism.tests.widgets.test_Monitoring_Queues", [

	      // remove all queues to start this test run                                                    	
          new LoginFixture ({name: "test_deleteAllQueues",
  			runTest: function() {
  				var d = new doh.Deferred();
  				dojo.xhrGet({
  					url: Utils.getBaseUri() + "rest/config/queues/0",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					load: lang.hitch(this, function(data, ioArgs) {
  						for (var i = 0; i < data.length; i++) {
  							var record = data[i];
  							console.log("Deleting " + data[i].Name);
  							dojo.xhrDelete({
  								url: Utils.getBaseUri() + "rest/config/queues/0/" + encodeURI(data[i].Name),
  								handleAs: "json",
                                  headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")}
  							});
  						}
  						setTimeout(d.getTestCallback(function(){
  							doh.assertTrue(true);
  						}), 500);
  					}),
  					error: function(error) {
  						d.errback(new Error("Unexpected Return: ", error));
  					}
  				});

  				return d;
  			}
  		}),
  		// make a valid request when there are no queues
        new LoginFixture ({name: "test_getQueueData_none", 
     	   stat: {
     		   Action: "Queue",
     		   Name: "*",
     		   ResultCount: 100,
     		   StatType: "BufferedMsgsHighest"
     	   },
     	   expectedRC: "113", // no data
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.expectedRC === result.RC);
  				}));

 				dojo.xhrPost({
 					url: Utils.getBaseUri() + "rest/monitoring",
 					postData: JSON.stringify(this.stat),
 					handleAs: "json",
 					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
 					load: function(data) {
 						d.callback(data);
 					},
 					error: function(error) {
 						console.error(error);
 						doh.assertTrue(false);
 					}
 				});

 				return d;
 		   }
 		}),
 		// create a queue so we get some stats
        new LoginFixture ({name: "test_create_queue1",
            resource: {
                Name: "queue1",
                MaxMessages: "5000",
                AllowSend: true,
                ConcurrentConsumers: true
            },
			expected: "config/queues/0/queue1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/queues/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
						doh.assertTrue(false);
					}
				});

				return d;
			}
		}),
	   // make a valid request to get the stats                                                       
       new LoginFixture ({name: "test_getQueueData_BufferedMsgsHighest", 
    	   stat: {
    		   Action: "Queue",
    		   Name: "*",
    		   ResultCount: 100,
    		   StatType: "BufferedMsgsHighest"
    	   },
    	   expectedResultLength: 1,
    	   expectedResult: {"Name":"queue1","Producers":0,"Consumers":0,"BufferedMsgs":0,"BufferedMsgsHWM":0,
    		   "BufferedPercent":0.0,"MaxMessages":5000,"ProducedMsgs":0,"ConsumedMsgs":0,"RejectedMsgs":0,"BufferedHWMPercent":0.0,
    		   "ExpiredMsgs":0},
		   runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(result) {
					doh.assertTrue(this.expectedResultLength === result.length);
					var compareResult = result[0];
					for (var prop in this.expectedResult) {
						console.log("checking " + prop);
						doh.assertTrue(this.expectedResult[prop] === compareResult[prop]);
					}											
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/monitoring",
					postData: JSON.stringify(this.stat),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
						doh.assertTrue(false);
					}
				});

				return d;
		   }
		}),
	   // make a valid request to get the stats                                                       
       new LoginFixture ({name: "test_getQueueData_ExpiredMsgsHighest", 
    	   stat: {
    		   Action: "Queue",
    		   Name: "*",
    		   ResultCount: 100,
    		   StatType: "BufferedMsgsHighest"
    	   },
    	   expectedResultLength: 1,
    	   expectedResult: {"Name":"queue1","Producers":0,"Consumers":0,"BufferedMsgs":0,"BufferedMsgsHWM":0,
    		   "BufferedPercent":0.0,"MaxMessages":5000,"ProducedMsgs":0,"ConsumedMsgs":0,"RejectedMsgs":0,"BufferedHWMPercent":0.0,
    		   "ExpiredMsgs":0},
		   runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(result) {
					doh.assertTrue(this.expectedResultLength === result.length);
					var compareResult = result[0];
					for (var prop in this.expectedResult) {
						console.log("checking " + prop);
						doh.assertTrue(this.expectedResult[prop] === compareResult[prop]);
					}											
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/monitoring",
					postData: JSON.stringify(this.stat),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
						doh.assertTrue(false);
					}
				});

				return d;
		   }
		}),
	   // make a valid request to get the stats                                                       
       new LoginFixture ({name: "test_getQueueData_ExpiredMsgsLowest", 
    	   stat: {
    		   Action: "Queue",
    		   Name: "*",
    		   ResultCount: 100,
    		   StatType: "BufferedMsgsHighest"
    	   },
    	   expectedResultLength: 1,
    	   expectedResult: {"Name":"queue1","Producers":0,"Consumers":0,"BufferedMsgs":0,"BufferedMsgsHWM":0,
    		   "BufferedPercent":0.0,"MaxMessages":5000,"ProducedMsgs":0,"ConsumedMsgs":0,"RejectedMsgs":0,"BufferedHWMPercent":0.0,
    		   "ExpiredMsgs":0},
		   runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(result) {
					doh.assertTrue(this.expectedResultLength === result.length);
					var compareResult = result[0];
					for (var prop in this.expectedResult) {
						console.log("checking " + prop);
						doh.assertTrue(this.expectedResult[prop] === compareResult[prop]);
					}											
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/monitoring",
					postData: JSON.stringify(this.stat),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
						doh.assertTrue(false);
					}
				});

				return d;
		   }
		}),		
		// make nvalid requests
       new LoginFixture ({name: "test_getQueueData_invalidStatType", 
    	   stat: {
    		   Action: "Queue",
    		   Name: "*",
    		   ResultCount: 100,
    		   StatType: "SubscriptionsHighest" // invalid for Queue
    	   },
    	   expectedCode: "CWLNA5003",
		   runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(result) {
					doh.assertTrue(this.expectedCode === result.code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/monitoring",
					postData: JSON.stringify(this.stat),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
		   }
		}),		
		// delete the queue we created earlier
        new LoginFixture ({name: "test_delete_queue1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/queues/0/queue1",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.callback(false);
					}
				});

				return d;
			}
		})		
	]);
});
