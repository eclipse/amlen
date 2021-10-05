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

	doh.register("ism.tests.widgets.test_Monitoring_MQTTClients", [

        new LoginFixture ({name: "test_getMQTTClients_all", 
     	   stat: {
     		   Action: "MQTTClient",
     		   ClientID: "*",
     		   ResultCount: 100,
     		   StatType: "LastConnectedTimeOldest"
     	   },
    	   expectedStats: ["ClientId","IsConnected","LastConnectedTime"],
     	   emptyRC: "113", // no data
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
 							doh.assertTrue(compareResult[prop] !== undefined && compareResult[prop] !== "");
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
        new LoginFixture ({name: "test_getMQTTClients_bogusClientID", 
      	   stat: {
      		   Action: "MQTTClient",
      		   ClientID: "Bogus",
      		   ResultCount: 100,
      		   StatType: "LastConnectedTimeOldest"
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
 		
  		// make an invalid request for stats
        new LoginFixture ({name: "test_getMQTTClients_invalid_statType", 
      	   stat: {
     		   Action: "MQTTClient",
     		   ClientID: "*",
     		   ResultCount: 100,
     		   StatType: "LastConnectedTimeNewest", // invalid 
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
        new LoginFixture ({name: "test_deleteMQTTClient_bogus",
        	expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertTrue(response.code == this.expectedCode);
				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/mqttClients/0/"+encodeURIComponent("bogus"),
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
