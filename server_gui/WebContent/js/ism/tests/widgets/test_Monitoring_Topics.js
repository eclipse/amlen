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

	doh.register("ism.tests.widgets.test_Monitoring_Topics", [

	      // remove all topic monitors to start this test run  (get all followed by delete each one)                                                  	
          new LoginFixture ({name: "test_deleteAllTopicMonitors",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrGet({
  					url: Utils.getBaseUri() + "rest/config/topicMonitors/0",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					load: lang.hitch(this, function(data, ioArgs) {
  						for (var i = 0; i < data.length; i++) {
  							var record = data[i];
  							console.log("Deleting " + data[i].Name);
  							dojo.xhrDelete({
  								url: Utils.getBaseUri() + "rest/config/topicMonitors/0/" + encodeURIComponent(data[i].TopicString),
  								sync: true,
  								handleAs: "json",
                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                error: function(error) {
              						console.dir(error);
              						d.callback(false);                                }
  							});
  						}
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}),
  		// make a valid request when there are no topic monitors
        new LoginFixture ({name: "test_getTopicMonitorData_none", 
     	   stat: {
     		   Action: "Topic",
     		   TopicString: "*",
     		   ResultCount: "All",
     		   StatType: "PublishedMsgsHighest"
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
 		// create a valid topic monitor so we get some stats
        new LoginFixture ({name: "test_create_topicMonitor_1",
            TopicString: "test1/#",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/topicMonitors/0",
					postData: JSON.stringify({TopicString: this.TopicString}),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						console.dir(error);
						d.callback(false);
					}
				});

				return d;
			}
		}),
		
 		// post invalid topic monitors 
        new LoginFixture ({name: "test_create_topicMonitor_missingWildcard",
            TopicString: "test1/foo",
            expectedCode: "CWLNA5109",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(code) {
					doh.assertTrue(this.expectedCode === code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/topicMonitors/0",
					postData: JSON.stringify({TopicString: this.TopicString}),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(null);
					},
					error: function(error) {
						console.dir(error);
						d.callback(JSON.parse(error.responseText).code);
					}
				});

				return d;
			}
		}),
        new LoginFixture ({name: "test_create_topicMonitor_$SYS",
            TopicString: "$SYS/test1/foo",
            expectedCode: "CWLNA5109",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(code) {
					doh.assertTrue(this.expectedCode === code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/topicMonitors/0",
					postData: JSON.stringify({TopicString: this.TopicString}),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(null);
					},
					error: function(error) {
						console.dir(error);
						d.callback(JSON.parse(error.responseText).code);
					}
				});

				return d;
			}
		}),		
  		
  		// get all and verify there is just one
        new LoginFixture ({name: "test_get_all",
  			runTest: function() {
  				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(data) {
					doh.assertTrue(data && data.length == 1);
				}));  				
  				dojo.xhrGet({
  					url: Utils.getBaseUri() + "rest/config/topicMonitors/0",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.dir(error);
						doh.assertTrue(false);
					}
  				});

  				return d;
  			}
  		}),		
		
	   // make a valid request to get the stats                                                       
       new LoginFixture ({name: "test_getTopicMonitorData_valid", 
    	   stat: {
     		   Action: "Topic",
     		   TopicString: "*",
     		   ResultCount: "All",
     		   StatType: "PublishedMsgsHighest"
    	   },
    	   expectedResultLength: 1,
    	   expectedResult: {"TopicString":"test1/#"},
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
		
	   // make invalid requests
       new LoginFixture ({name: "test_getTopicData_invalidStatType", 
    	   stat: {
     		   Action: "Topic",
     		   TopicString: "*",
     		   ResultCount: "All",
     		   StatType: "BufferedMsgsHighest" // invalid for Topic
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
		new LoginFixture ({name: "test_getTopicData_invalidStatType", 
    	   stat: {
     		   Action: "Topic",
     		   TopicString: "*",
     		   ResultCount: "All",
     		   StatType: "BufferedMsgsHighest" // invalid for Topic
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
			
		// delete the topicMonitor we created earlier
        new LoginFixture ({name: "test_delete_topicMonitor",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/topicMonitors/0/"+encodeURIComponent("test1/#"),
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
