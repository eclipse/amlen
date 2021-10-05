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

	doh.register("ism.tests.widgets.test_Monitoring_MQConnectivity", [

  		// make a valid request for CommitCount 
        new LoginFixture ({name: "test_getMQConnectivity_PublishedMsgsHighest", 
     	   stat: {
     		  Action: "DestinationMappingRule",
     		  MonitoringType: "mqconn_monitoring",
     		  Name: "*",
     		  RuleType: "Any",
     		  Association: "*",
     		  StatType: "CommitCount",
     		  ResultCount: 100
     		},
     	   emptyRC: "113", // No subscriptions
     	   expectedStats: ["RuleName","QueueManagerConnection","CommitCount","RollbackCount","CommittedMessageCount","PersistentCount","NonpersistentCount","Status","ExpandedMessage"],
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
  		// make an invalid request 
        new LoginFixture ({name: "test_getSubscriptionStats_invalid_StatType", 
      	   stat: {
      		  Action: "DestinationMappingRule",
      		  MonitoringType: "mqconn_monitoring",
      		  Name: "*",
      		  RuleType: "Any",
      		  Association: "*",
      		  StatType: "BadConnections", //invalid
      		  ResultCount: 100
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
        new LoginFixture ({name: "test_getMQConnectivity_bogusRuleName", 
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
        new LoginFixture ({name: "test_getMQConnectivity_bogusRuleName", 
       	   stat: {
       		  Action: "DestinationMappingRule",
       		  MonitoringType: "mqconn_monitoring",
       		  Name: "Bogus",
       		  RuleType: "Any",
       		  Association: "*",
       		  StatType: "RollbackCount", 
       		  ResultCount: 100
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
    		}),
            new LoginFixture ({name: "test_getMQConnectivity_bogusRuleType", 
            	   stat: {
            		  Action: "DestinationMappingRule",
            		  MonitoringType: "mqconn_monitoring",
            		  Name: "*",
            		  RuleType: "Bogus",
            		  Association: "*",
            		  StatType: "RollbackCount", 
            		  ResultCount: 100
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
         		}),
         		// add a rule to be sure we get something back
        		new LoginFixture ({name: "test_getMQConnectivity_withRule",
        			qmc: {
        				Name: "Unit Test 1",
        				QueueManagerName: "QM1",
        				ConnectionName: "192.168.56.2",
        				ChannelName: "SYSTEM.DEF.SVRCONN"			
        			},
        			rule: {
        				Name: "Rule Unit Test 1",
        				QueueManagerConnection: "Unit Test 1",
        				RuleType: 1,
        				Source: "/test/topic/1",	
        				Destination: "MQQueue1",
        				Enabled: false
        			},
    	     	    stat: {
    	     	    	Action: "DestinationMappingRule",
    	     	    	MonitoringType: "mqconn_monitoring",
    	     	    	Name: "Rule Unit Test 1",
    	     	    	RuleType: "IMATopicToMQQueue",
    	     	    	Association: "Unit Test 1",
    	     	    	StatType: "CommitCount",
    	     	    	ResultCount: 100
    	      		},        			
        			expectedQmc: "config/mqconnectivity/queueManagerConnections/0/Unit+Test+1",
        			expectedRule: "config/mqconnectivity/destinationMappingRules/0/Rule+Unit+Test+1",
        	     	expectedStats: ["RuleName","QueueManagerConnection","CommitCount","RollbackCount","CommittedMessageCount","PersistentCount","NonpersistentCount","Status","ExpandedMessage"],
        	     	expectedLength: 1,
        			runTest: function() {
        				var d = new doh.Deferred();

        				d.addCallback(lang.hitch(this, function(pass) {
        					doh.assertTrue(pass);
        				}));

        				var _this = this;
        				dojo.xhrPost({
        					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
        					postData: JSON.stringify(_this.qmc),
        					handleAs: "json",
                            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
        					load: function(data, ioArgs) {
        						var uri = ioArgs.xhr.getResponseHeader("location");
            					doh.assertTrue(uri.indexOf(_this.expectedQmc) != -1);        						
            					dojo.xhrPost({
            						url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
            						postData: JSON.stringify(_this.rule),
            						handleAs: "json",
            	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
            						load: function(data, ioArgs) {
            							console.debug("rule created ok");
            							var uri = ioArgs.xhr.getResponseHeader("location");
            							doh.assertTrue(uri.indexOf(_this.expectedRule) != -1);
            			 				dojo.xhrPost({
            			 					url: Utils.getBaseUri() + "rest/monitoring",
            			 					postData: JSON.stringify(_this.stat),
            			 					handleAs: "json",
            			 					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
            			 					load: function(data) {
            			 						doh.assertTrue(data.length === _this.expectedLength);
            			 						var compareResult = data[0];
            			 						for (var i in _this.expectedStats) {
            			 							var prop = _this.expectedStats[i];
            			 							console.log("checking " + prop);
            			 							doh.assertTrue(compareResult[prop] !== undefined);
            			 						}
            			 						doh.assertTrue(compareResult.RuleName === _this.stat.Name);
            			 						doh.assertTrue(compareResult.QueueManagerConnection === _this.stat.Association);
            			 						
            			 						d.callback(true);
            			 					},
            			 					error: function(error) {
            			 						console.error(error);
            			 						d.callback(false);
            			 					}
            			 				});

            						},
            						error: function(error) {
            							console.debug("rule creation error");
            							console.error(error);
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
        			},
        			tearDown: function() {
						dojo.xhrDelete({
							url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURIComponent(this.rule.Name),
							handleAs: "json",
                            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                            sync: true,
                            load: lang.hitch(this, function() {
    							dojo.xhrDelete({
    								url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + encodeURIComponent(this.qmc.Name),
    								handleAs: "json",
    								sync: true,
                                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                    error: function(error){
                                    	console.log(error);
                                    	doh.assertTrue(false);
                                    }
    							});
                            }),
                            error: function(error) {
                            	console.log(error);
                            	doh.assertTrue(false);
                            }
						});
       				
        			}
        		}),
         	
        ]);
});
