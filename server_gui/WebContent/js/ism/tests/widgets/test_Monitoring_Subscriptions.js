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

	doh.register("ism.tests.widgets.test_Monitoring_Subscriptions", [

  		// make a valid request for PublishedMsgsHighest 
        new LoginFixture ({name: "test_getSubscriptions_PublishedMsgsHighest", 
     	   stat: {
     		   Action:"Subscription",
     		   SubName:"*",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"All",
     		   StatType:"PublishedMsgsHighest",
     		   ResultCount:100
     	   },
     	   emptyRC: "113", // No subscriptions
     	   expectedStats: ["SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM",
     	                   "BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent",
     	                   "IsShared","Consumers","ExpiredMsgs","DiscardedMsgs"],
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					if (result.RC) {
 	 					doh.assertTrue(this.emptyRC === result.RC); 						
 					} else {
 						doh.assertTrue(result.length > 0);
 						var compareResult = result[0];
 						for (var i in this.expectedStats) {
 							var prop = this.expectedStats[i];
 							console.log("checking " + prop);
 							doh.assertTrue(compareResult[prop] !== undefined);
 						}
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
 						d.callback({});
 					}
 				});

 				return d;
 		   }
 		}),
  		// make a valid request for ExpiredDiscardedHighest 
        new LoginFixture ({name: "test_getSubscriptions_ExpiredMsgsHighest", 
     	   stat: {
     		   Action:"Subscription",
     		   SubName:"*",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"All",
     		   StatType:"ExpiredMsgsHighest",
     		   ResultCount:100
     	   },
     	   emptyRC: "113", // No subscriptions
     	   expectedStats: ["SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM",
     	                   "BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent",
     	                   "IsShared","Consumers","ExpiredMsgs","DiscardedMsgs"],
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					if (result.RC) {
 	 					doh.assertTrue(this.emptyRC === result.RC); 						
 					} else {
 						doh.assertTrue(result.length > 0);
 						var compareResult = result[0];
 						for (var i in this.expectedStats) {
 							var prop = this.expectedStats[i];
 							console.log("checking " + prop);
 							doh.assertTrue(compareResult[prop] !== undefined);
 						}
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
 						d.callback({});
 					}
 				});

 				return d;
 		   }
 		}), 		
  		// make a valid request for ExpiredDiscardedLowest 
        new LoginFixture ({name: "test_getSubscriptions_DiscardedMsgsLowest", 
     	   stat: {
     		   Action:"Subscription",
     		   SubName:"*",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"All",
     		   StatType:"DiscardedMsgsLowest",
     		   ResultCount:100
     	   },
     	   emptyRC: "113", // No subscriptions
     	   expectedStats: ["SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM",
     	                   "BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent",
     	                   "IsShared","Consumers","ExpiredMsgs","DiscardedMsgs"],
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					if (result.RC) {
 	 					doh.assertTrue(this.emptyRC === result.RC); 						
 					} else {
 						doh.assertTrue(result.length > 0);
 						var compareResult = result[0];
 						for (var i in this.expectedStats) {
 							var prop = this.expectedStats[i];
 							console.log("checking " + prop);
 							doh.assertTrue(compareResult[prop] !== undefined);
 						}
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
 						d.callback({});
 					}
 				});

 				return d;
 		   }
 		}), 		

  		// make an invalid request for subscription stats
        new LoginFixture ({name: "test_getSubscriptionStats_invalid_StatType", 
      	   stat: {
     		   Action:"Subscription",
     		   SubName:"*",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"All",
     		   StatType:"SubscriptionsHighest", // invalid
     		   ResultCount:100
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
  		// make an invalid request for subscription stats
        new LoginFixture ({name: "test_getSubscriptionStats_invalid_SubType", 
      	   stat: {
     		   Action:"Subscription",
     		   SubName:"*",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"Current",  // invalid
     		   StatType:"BufferedMsgsHighest",
     		   ResultCount:100
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
        new LoginFixture ({name: "test_getSubscriptions_bogusSubName", 
      	   stat: {
     		   Action:"Subscription",
     		   SubName:"Bogus",
     		   TopicString:"*",
     		   ClientID:"*",
     		   SubType:"Durable",
     		   StatType:"BufferedHWMPercentHighest",
     		   ResultCount:100
     	   },
       	   expectedRC: "113", // no data
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(result.RC === this.expectedRC);
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
 						d.callback({});
 					}
 				});

 				return d;
   		   }
   		}),
        new LoginFixture ({name: "test_getSubscriptions_bogusTopicString", 
       	   stat: {
      		   Action:"Subscription",
      		   SubName:"*",
      		   TopicString:"Bogus",
      		   ClientID:"*",
      		   SubType:"Nondurable",
      		   StatType:"RejectedMsgsHighest",
      		   ResultCount:100
      	   },
           expectedRC: "113", // no data
  		   runTest: function() {
  				var d = new doh.Deferred();

  				d.addCallback(lang.hitch(this, function(result) {
  					doh.assertTrue(result.RC === this.expectedRC);
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
  						d.callback({});
  					}
  				});

  				return d;
    		   }
    		}),
            new LoginFixture ({name: "test_deleteSubscription_bogus",
            	expectedCode: "CWLNA5005",
            	embeddedRC: "[RC=207]",
    			runTest: function() {
    				var d = new doh.Deferred();

    				d.addCallback(lang.hitch(this, function(response) {
    					doh.assertTrue(response.code == this.expectedCode);
    					doh.assertTrue(response.message.indexOf(this.embeddedRC) > -1);
    				}));

    				dojo.xhrDelete({
    					url: Utils.getBaseUri() + "rest/config/subscriptions/0/"+encodeURIComponent("bogus"),
    					handleAs: "json",
    					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    					load: function(data, ioArgs) {
    						d.callback({});
    					},
    					error: function(error) {
    						console.error(error);
    						d.callback(JSON.parse(error.responseText));
    					}
    				});

    				return d;
    			}
    		})		 		
	]);
});
