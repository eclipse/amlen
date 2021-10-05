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

	doh.register("ism.tests.widgets.test_MQConnectivity", [
		//
		// MQConnectivity tests, including both QueueManagerConnection and DestinationMappingRule,
		//      as queue manager connections are required in order to create destination mapping rules
		//
		// QueueManagerConnection test suite:
		//		test_clearAllRules_initial delete any existing rules so that QMC can be deleted
		//      test_clearAll_initial:  delete any existing QueueManagerConnections
		//      test_getNone:    		verify that a GET with no configured QueueManagerConnections returns the appropriate response (empty array)
		//		test_add_qmc1:       	adds a single QueueManagerConnection
		//      test_get_qmcById:    	verify that the newly created QueueManagerConnection can be GET'ed by specifying its resource location
		//      test_get_qmcWrongId: 	verify that a GET with an invalid resource returns the proper response
		//      test_add_qmc2:       	adds another QueueManagerConnection
		//      test_add_qmc3:       	adds another QueueManagerConnection
		//      test_get_qmcAll:     	verifies that both created QueueManagerConnections can be GET'ed
		//      test_rename1:    		renames QueueManagerConnection Unit Test 4 to "Unit Test 4"
		//      test_clearAllQMC:   	delete any existing QueueManagerConnections
		// DestinationMappingRule test suite:
		//      test_clearAllRules_initial:	delete any existing DestinationMappingRules
		//      test_getRuleNone:    		verify that a GET with no configured DestinationMappingRules returns the appropriate response (empty array)
		//		test_add_rule1:       		adds a single DestinationMappingRule
		//      test_getRuleById:    		verify that the newly created DestinationMappingRule can be GET'ed by specifying its resource location
		//      test_getRuleWrongId: 		verify that a GET with an invalid resource returns the proper response
		//      test_add_rule2:       		adds another DestinationMappingRule
		//      test_add_rule3:       		adds another DestinationMappingRule
		//      test_disableRule:       	disables a rule
		//      test_getAllRules:  			verifies that both created DestinationMappingRules can be GET'ed
		//      test_rename1:    			renames DestinationMappingRule Unit Test 4 to "Unit Test 4"
		//      test_clearAllRules:  		delete any existing DestinationMappingRules		

		new LoginFixture ({	name: "test_clearAllRules_initial",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							if (data[i].Enabled) {
								// we have to disable the rule before we can delete it
								data[i].Enabled = false;
								dojo.xhrPut({
									url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" +  encodeURI(data[i].Name),
									postData: JSON.stringify(data[i]),
									handleAs: "json",
									sync: true,
				                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
									load: function(data, ioArgs) {										
									},
									error: function(error) {
										d.errback(new Error("Unexpected Return: ", error));
									}	
								});
							}
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURI(data[i].Name),
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
		new LoginFixture ({name: "test_clearAllQMC_initial",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + encodeURI(data[i].Name),
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
		new LoginFixture ({name: "test_getNone", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.debug(obj.length);
					console.debug(this.expected);				
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_qmc1",
			resource: {
				Name: "Unit Test 1",
				QueueManagerName: "QM1",
				ConnectionName: "192.168.56.2",
				ChannelName: "SYSTEM.DEF.SVRCONN"			
			},
			expected: "config/mqconnectivity/queueManagerConnections/0/Unit+Test+1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					console.log(uri.indexOf(this.expected));
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_get_qmcById",
			resource: {
				Name: "Unit Test 1",
				QueueManagerName: "QM1",
				ConnectionName: "192.168.56.2",
				ChannelName: "SYSTEM.DEF.SVRCONN"	
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.QueueManagerName == expected.QueueManagerName);
					doh.assertTrue(obj.ConnectionName == expected.ConnectionName);
					doh.assertTrue(obj.ChannelName == expected.ChannelName);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + encodeURI(this.resource.Name),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						console.log(data);
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_get_qmcWrong",expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + "InvalidName",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						console.log(data);
						// it's an error if we get here
						doh.assertTrue(false);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_qmc2",
			resource: { // create one without a description
				Name: "Unit Test 2",
				QueueManagerName: "qm2",
				ConnectionName: "192.168.56.4",
				ChannelName: "SYSTEM.DEF.SVRCONN"	
			},
			expected: "config/mqconnectivity/queueManagerConnections/0/Unit+Test+2",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({	name: "test_add_qmc3",
			resource: {
				Name: "Unit Test 3",
				QueueManagerName: "qm_3",
				ConnectionName: "192.168.56.6",
				ChannelName: "SYSTEM.DEF.SVRCONN"	
			},
			expected: "config/mqconnectivity/queueManagerConnections/0/Unit+Test+3",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_get_qmcAll", expected: 3,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		
		/* Renaming is not currently supported
		{
			name: "test_rename1",
			timeout: 1000,
			setUp: function() {
				this.qmconn = new QueueManagerConnection({
					resource: {
						Name: "Unit Test 4",
					}
				});
				this.oldName = "Unit Test 1";
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.qmconn.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.enabled == expected.enabled);
					doh.assertTrue(obj.port == expected.port);
					doh.assertTrue(obj.ipAddr == expected.ipAddr);
					doh.assertTrue(obj.protocol == expected.protocol);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.qmconn.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}, */
		
		new LoginFixture ({name: "test_clearAllRules_initial",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURI(data[i].Name),
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
		new LoginFixture ({name: "test_getRuleNone", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
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
		new LoginFixture ({name: "test_add_rule1",
			resource: {
				Name: "Rule Unit Test 1",
				QueueManagerConnection: "Unit Test 1",
				RuleType: 1,
				Source: "/test/topic/1",	
				Destination: "MQQueue1",
				Enabled: false
			},
			expected: "config/mqconnectivity/destinationMappingRules/0/Rule+Unit+Test+1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						console.debug("rule created ok");
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.debug("rule creation error");
						console.error(error);
					}
				});

				return d;
			},
			timeout: 10000 // 10 second timeout to make sure server has caught up
		}),
		new LoginFixture ({name: "test_getRuleById",
			resource: {
				Name: "Rule Unit Test 1",
				QueueManagerConnection: "Unit Test 1",
				RuleType: 1,
				Source: "/test/topic/1",	
				Destination: "MQQueue1",
				Enabled: false
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.QueueManagerConnection == expected.QueueManagerConnection);
					doh.assertTrue(obj.RuleType == expected.RuleType);
					doh.assertTrue(obj.Source == expected.Source);
					doh.assertTrue(obj.Destination == expected.Destination);
					doh.assertTrue(obj.Enabled == expected.Enabled);					
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURI(this.resource.Name),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						console.log(data);
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getRuleWrong", expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + "InvalidName",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						console.log(data);
						// it's an error if we get here
						doh.assertTrue(false);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_rule2",
			resource: {
				Name: "Rule2",
				QueueManagerConnection: "Unit Test 1",
				RuleType: 2,
				Source: "/test/topic/1",	
				Destination: "/test/mqtopic",
				Enabled: false
			},
			expected: "config/mqconnectivity/destinationMappingRules/0/Rule2",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			},
			timeout: 10000 // 10 second timeout to make sure server has caught up
		}),
		new LoginFixture ({name: "test_add_rule3",
			resource: {
				Name: "Rule3",
				QueueManagerConnection: "Unit Test 1,Unit Test 2",
				RuleType: 14,
				Source: "/test/topic/3",	
				Destination: "Queue1",
				Enabled: true
			},
			expected: "config/mqconnectivity/destinationMappingRules/0/Rule3",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						d.callback(uri);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			},
			timeout: 10000 // 10 second timeout to make sure server has caught up
		}),
		new LoginFixture ({name: "test_disableRule",
			resource: {
				Enabled: false
			},
			expected: "config/mqconnectivity/destinationMappingRules/0/Rule3",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" +  encodeURIComponent("Rule3"),
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			},
			timeout: 10000 // 10 second timeout to make sure server has caught up
		}),
		new LoginFixture ({name: "test_addRuleInvalidQName",
			resource: {
				Name: "Unit Test Invalid Q name",
				QueueManagerConnection: "QM1,QM2",
				RuleType: 3,
				Source: "InvalidCharacters!!!!",	
				Destination: "/test/topic/3",
				Enabled: true
			},
			expectedCode: "CWLNA5023",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
							console.log(data);
							// it's an error if we get here
							doh.assertTrue(false);
						},
						error: lang.hitch(this, function(error, ioArgs) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_addRuleInvalidTopicChars",
			resource: {
				Name: "Unit Test Invalid Topic chars",
				QueueManagerConnection: "QM1,QM2",
				RuleType: 3,
				Source: "AGoodQName",	
				Destination: "/test/topic/#/wildcards/not/allowed",
				Enabled: true
			},	
			expectedCode: "CWLNA5094",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
							console.log(data);
							// it's an error if we get here
							doh.assertTrue(false);
						},
						error: lang.hitch(this, function(error, ioArgs) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_addRuleInvalidTopicChars2",
			resource: {
				Name: "Unit Test Invalid Topic chars",
				QueueManagerConnection: "QM1,QM2",
				RuleType: 3,
				Source: "AGoodQName",	
				Destination: "/test/topic/+/wildcards/not/allowed",
				Enabled: true
			},	
			expectedCode: "CWLNA5094",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
							console.log(data);
							// it's an error if we get here
							doh.assertTrue(false);
						},
						error: lang.hitch(this, function(error, ioArgs) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_addRuleQNameTooLong",
			resource: {
				Name: "Unit Test Q name too long",
				QueueManagerConnection: "QM1,QM2",
				RuleType: 3,
				Source: "QNameIsTooLongOnly48CharactersAreAllowed123456789",	
				Destination: "/test/topic/",
				Enabled: true
			},
			expectedCode: "CWLNA5100",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
							console.log(data);
							// it's an error if we get here
							doh.assertTrue(false);
						},
						error: lang.hitch(this, function(error, ioArgs) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getAllRules", expected: 3,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		
		/* Renaming is not currently supported
		{
			name: "test_rename1",
			timeout: 1000,
			setUp: function() {
				this.expected = new DestinationMappingRule({
					resource: {
						Name: "Unit Test 4",
						QueueManagerConnection: "QM1",
						RuleType: 1,
						Source: "/test/topic/1",	
						Destination: "MQQueue1",
						Enabled: true
					}
				});
				this.oldName = "Unit Test 1";
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.Name == this.resource.Name);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}, */
		new LoginFixture ({	name: "test_clearAllRules",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/mqconnectivity/destinationMappingRules/0/" + encodeURI(data[i].Name),
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
		new LoginFixture ({name: "test_clearAllQmc",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0",
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/mqconnectivity/queueManagerConnections/0/" + encodeURI(data[i].Name),
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
		})
	]);
});
