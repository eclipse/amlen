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

	doh.register("ism.tests.widgets.test_Monitoring_Endpoints", [

  		// make a valid request for endpoint history
        new LoginFixture ({name: "test_getEndpointHistory_all_duration300", 
     	   stat: {
     		   Action: "Endpoint",
     		   Duration: "300",
     		   StatType: "ActiveConnections",
     		   SubType: "History"
     	   },
    	   expectedResults: { "Endpoint":"All Endpoints", "Type":"ActiveConnections", "Duration":"300", "Interval":"5"},
     	   expectedDataPoints: 300 / 5,
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
					for (var prop in this.expectedResults) {
						console.log("checking " + prop);
						doh.assertTrue(result[prop] === this.expectedResults[prop]);
					}
					// verify the correct number of data points
					doh.assertTrue(result.Data !== undefined);
					var dataPoints = result.Data.split(",").length-1;
					console.log("found " + dataPoints + " dataPoints");
					doh.assertTrue(dataPoints === this.expectedDataPoints);
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
        new LoginFixture ({name: "test_getEndpointHistory_all_duration86400", 
      	   stat: {
      		   Action: "Endpoint",
      		   Duration: "86400",
      		   StatType: "ActiveConnections",
      		   SubType: "History"
      	   },
     	   expectedResults: { "Endpoint":"All Endpoints", "Type":"ActiveConnections", "Duration":"86400", "Interval":"60"},
      	   expectedDataPoints: 86400 / 60,
  		   runTest: function() {
  				var d = new doh.Deferred();

  				d.addCallback(lang.hitch(this, function(result) {
 					for (var prop in this.expectedResults) {
 						console.log("checking " + prop);
 						doh.assertTrue(result[prop] === this.expectedResults[prop]);
 					}
 					// verify the correct number of data points
 					doh.assertTrue(result.Data !== undefined);
 					var dataPoints = result.Data.split(",").length-1;
 					console.log("found " + dataPoints + " dataPoints");
 					doh.assertTrue(dataPoints === this.expectedDataPoints);
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
  		// make an invalid request for endpoint stats
        new LoginFixture ({name: "test_getEndpointStats_invalid_statType", 
      	   stat: {
     		   Action: "Endpoint",
     		   Duration: "300",
     		   StatType: "RejectedMsgsHighest", // invalid for Endpoint
     		   SubType: "History"
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
  		// make an invalid request for endpoint stats
        new LoginFixture ({name: "test_getEndpointStats_invalid_subType", 
      	   stat: {
     		   Action: "Endpoint",
     		   Duration: "300",
     		   StatType: "Connections",
     		   SubType: "Bogus"
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
 		
 		new LoginFixture ({name: "test_clearEndpoints", 
			runTest: function() {
				var d = new doh.Deferred();
	
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
	
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURIComponent(data[i].Name),
								sync: true,
								handleAs: "json",
	                            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
	                            load: function(data) {},
	                            error: function(error) {
	                            	console.log("Could not delete MessageHub");
	        						var response = JSON.parse(error.responseText);
	                            	d.callback(false);
	                            }
							});
						}
						d.callback(true);
					}),
					error: function(error) {
						d.callback(false);
					}
				});
	
				return d;
			}
		}), 
		
        new LoginFixture ({name: "test_getEndpointCurrent_all", 
       	   stat: {
       		   Action: "Endpoint",
       		   SubType: "Current"
       	   },
     	   emptyRC: "351", // no endpoint configured
   		   runTest: function() {
   				var d = new doh.Deferred();

   				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.emptyRC === result.RC); 						
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
        new LoginFixture ({name: "test_getEndpointCurrent_BadConnections", 
        	   stat: {
        		   Action: "Endpoint",
        		   StatType: "BadConnections",
        		   SubType: "Current"
        	   },
     	   emptyRC: "351", // no endpoint configured
		   runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.emptyRC === result.RC); 						
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
    		
     		// create an endpoint and test knowing we have one
    		new LoginFixture ({name: "test_createEndpoint", 
    			messagehub: {Name: "Endpoint Unit Test Hub", Description: "Message hub for endpoint unit testing"},
    			connectionPolicy: {	Name: "Endpoint Unit Test Connection Policy", Protocol: "MQTT"},
    			messagingPolicy: {	Name: "Endpoint Unit Test Messaging Policy", Destination: "/Test/Topic/*", 
    				DestinationType: "Topic", ActionList: "Publish,Subscribe", MaxMessages: "5000", Protocol: "MQTT"},
    			endpoint: {
    				Name: "Unit Test 1",
    				Enabled: "true",
    				Port: "16105",
    				Interface: "All",
    				Protocol: "All",
    				SecurityProfile: "",
    				MessageHub: "Endpoint Unit Test Hub",
    				ConnectionPolicies: "Endpoint Unit Test Connection Policy",
    				MessagingPolicies: "Endpoint Unit Test Messaging Policy"
    			},
    			runTest: function() {
    				var d = new doh.Deferred();

    				d.addCallback(lang.hitch(this, function(pass) {
    					doh.assertTrue(pass);
    				}));

    				var _this = this;
    				dojo.xhrPost({
    					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
    					postData: JSON.stringify(_this.messagehub),
    					handleAs: "json",
    					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    					load: function(data) {
    						dojo.xhrPost({
    							url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
    							postData: JSON.stringify(_this.connectionPolicy),
    							handleAs: "json",
    							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    							load: function(data) {
    								dojo.xhrPost({
    									url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
    									postData: JSON.stringify(_this.messagingPolicy),
    									handleAs: "json",
    									headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    									load: function(data) {
    										dojo.xhrPost({
    											url: Utils.getBaseUri() + "rest/config/endpoints/0",
    											postData: JSON.stringify(_this.endpoint),
    											handleAs: "json",
    											headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    											load: function(data) {
    												d.callback(true);
    											},
    											error: function(error) {
    												d.callback(false);
    											}
    										});
    									},
    									error: function(error) {
    										d.callback(false);
    									}
    								});								
    							},
    							error: function(error) {
    								d.callback(false);
    							}
    						});
    					},
    					error: function(error) {
    						console.error(error);
    						d.callback(false);
    					}
    				});

    				return d;
    			}
    		}),
            new LoginFixture ({name: "test_getEndpointCurrent_all_withEndpoint", 
            	   stat: {
            		   Action: "Endpoint",
            		   SubType: "Current"
            	   },
           	   expectedStats: ["Name", "IPAddr", "Enabled", "Total", "Active", "Messages", "Bytes", "LastErrorCode", "ConfigTime", "ResetTime", "BadConnections"],
    		   runTest: function() {
    				var d = new doh.Deferred();

    				d.addCallback(lang.hitch(this, function(result) {
						doh.assertTrue(result.length > 0);
	 					var compareResult = result[0];
						for (var i in this.expectedStats) {
							var prop = this.expectedStats[i];
							console.log("checking " + prop);
							doh.assertTrue(compareResult[prop] !== undefined && compareResult[prop] !== "");
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
            new LoginFixture ({name: "test_getEndpointCurrent_BadConnections_withEndpoint", 
             	   stat: {
             		   Action: "Endpoint",
             		   StatType: "BadConnections",
             		   SubType: "Current"
             	   },
            	   expectedStats: ["Name", "IPAddr", "Enabled", "Total", "Active", "Messages", "Bytes", "LastErrorCode", "ConfigTime", "ResetTime", "BadConnections"],
         		   runTest: function() {
         				var d = new doh.Deferred();

         				d.addCallback(lang.hitch(this, function(result) {
	   						doh.assertTrue(result.length > 0);
	   	 					var compareResult = result[0];
	   						for (var i in this.expectedStats) {
	   							var prop = this.expectedStats[i];
	   							console.log("checking " + prop);
	   							doh.assertTrue(compareResult[prop] !== undefined && compareResult[prop] !== "");
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
    		new LoginFixture ({name: "test_reset", 
    			messagehub: {Name: "Endpoint Unit Test Hub"},
    			connectionPolicy: {Name: "Endpoint Unit Test Connection Policy"},
    			messagingPolicy: {Name: "Endpoint Unit Test Messaging Policy"},
    			runTest: function() {
    				var d = new doh.Deferred();

    				d.addCallback(lang.hitch(this, function(pass) {
    					doh.assertTrue(pass);
    				}));
    				
    				var _this = this;

    				dojo.xhrGet({
    					url: Utils.getBaseUri() + "rest/config/endpoints/0",
    					handleAs: "json",
    					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    					load: lang.hitch(this, function(data, ioArgs) {
    						for (var i = 0; i < data.length; i++) {
    							console.log("Deleting " + data[i].Name);
    							dojo.xhrDelete({
    								url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURIComponent(data[i].Name),
    								sync: true,
    								handleAs: "json",
                                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                    load: function(data) {},
                                    error: function(error) {
                                    	console.log("Could not delete MessageHub");
                						var response = JSON.parse(error.responseText);
                                    	d.callback(false);
                                    }
    							});
    						}
    						dojo.xhrDelete({
    							url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURIComponent(_this.messagehub.Name),
    							handleAs: "json",
    							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    							load: function(data) {
    								dojo.xhrDelete({
    									url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURIComponent(_this.connectionPolicy.Name),
    									handleAs: "json",
    									headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    									load: function(data) {
    										dojo.xhrDelete({
    											url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURIComponent(_this.messagingPolicy.Name),
    											handleAs: "json",
    											headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
    											load: function(data) {
    												d.callback(true);
    											},
    											error: function(error) {
    												d.callback(false);
    											}
    										});								
    									},
    									error: function(error) {
    										d.callback(false);
    									}
    								});
    							},
    							error: function(error) {
    								console.error(error);
    								d.callback(false);
    							}
    						});												
    					}),
    					error: function(error) {
    						d.callback(false);
    					}
    				});

    				return d;
    			}
    		})   		
	]);
});
