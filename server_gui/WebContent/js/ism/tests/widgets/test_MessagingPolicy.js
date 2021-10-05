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

	doh.register("ism.tests.widgets.test_MessagingPolicy", [
		//
		// Messaging Policy test suite:
		//      test_clearAll:           		delete any existing policies
		//      test_getNone:            		verify that a GET with no configured policies returns the appropriate response (empty array)
		//		test_add1:               		adds a single policy
		//		test_add_unknown_protocol       verify error when using unknown protocol
		//      test_add_incompatible_protocol  verify error when using a protocol that is not valid for a destination type
		//      test_getById:            		verify that the newly created policy can be GET'ed by specifying its resource location
		//      test_getWrongId:         		verify that a GET with an invalid resource returns the proper response
		//      test_add2:               		adds another policy
		//      test_add3:               		adds another policy
		//      test_getAll:             		verifies that both created policies can be GET'ed
		//      test_rename1:           		 renames policy 1 
		//      test_getByIdRename:     		 verify that the renamed policy can be GET'ed by specifying its resource location
		//      test_delete1:            		verifies that a delete succeeds
		//      test_getAllAfterDelete:  		verify that the deleted policy is gone
		//      test_clearAll:           		delete any existing policies
		new LoginFixture ({name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(data[i].Name),
								handleAs: "json",
								headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
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

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.length == this.expected) {
							d.callback(true);
						} else {
							d.errback(new Error("Actual number of returned records (" + data.length + ") doesn't match expected (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add1",
			policy: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessageTimeToLive: "uNlimiTed"
			},
			expected: "config/messagingPolicies/0/Messaging+Policy+1",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					postData: JSON.stringify(this.policy),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						if (uri.indexOf(this.expected) != -1) {
							d.callback(true);
						} else {
							d.errback(new Error("Returned location (" + uri + ") does not contain expected path (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_unknown_protocol",  expectedCode: "CWLNA5028",
			policy: {
				Name: "Messaging Policy Unknown Protocol",
				Description: "A description for this policy.",
				Protocol: "bogus",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000"
			},

			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					postData: JSON.stringify(this.policy),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						console.log(data);
						// it's an error if we get here
						doh.assertTrue(false);
					}),
					error: function(error) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_incompatible_protocol", expectedCode: "CWLNA5141",
			policy: {
				Name: "Messaging Policy Bad Protocol",
				Description: "A description for this policy.",
				Protocol: "MQTT",
				Destination: "TheQueueName",
				DestinationType: "Queue",
				ActionList: "Send",
				MaxMessages: "10000"
			},

			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					postData: JSON.stringify(this.policy),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						console.log(data);
						// it's an error if we get here
						doh.assertTrue(false);
					}),
					error: function(error) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getById",
			expected: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessageTimeToLive: "unlimited"
			},
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.expected.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.GroupID == this.expected.GroupID &&
							data.Destination == this.expected.Destination &&
							data.ActionList == this.expected.ActionList &&
							data.MaxMessages == this.expected.MaxMessages &&
							data.MaxMessageTimeToLive == this.expected.MaxMessageTimeToLive) {
							d.callback(true);
						} else {
							console.debug(data, this.expected);
							d.errback(new Error("Mismatch between records."));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getWrong", expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + "InvalidName",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.errback(new Error("Unexpected Return: ", data));
					},
					error: lang.hitch(this, function(error, ioArgs) {
						if (error.responseText != null) {
							var response = JSON.parse(error.responseText);
							if (response.code == this.expectedCode) {
								d.callback(true);
							} else {
								d.errback(new Error("Actual code (" + response.code + ") doesn't match expected (" + this.expectedCode + ")"));
							}
						} else {
							d.errback(new Error("Unexpected Return: ", error));
						}
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add2",
			policy: {
				Name: "Messaging Policy 2",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_2",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessageTimeToLive: "1"
			},
			expected: "config/messagingPolicies/0/Messaging+Policy+2",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					postData: JSON.stringify(this.policy),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						if (uri.indexOf(this.expected) != -1) {
							d.callback(true);
						} else {
							d.errback(new Error("Returned location (" + uri + ") does not contain expected path (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add3",
			policy: {
				Name: "Messaging Policy 3",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_3",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessageTimeToLive: "2147483647"					
			},
			expected: "config/messagingPolicies/0/Messaging+Policy+3",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					postData: JSON.stringify(this.policy),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						var uri = ioArgs.xhr.getResponseHeader("location");
						if (uri.indexOf(this.expected) != -1) {
							d.callback(true);
						} else {
							d.errback(new Error("Returned location (" + uri + ") does not contain expected path (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getAll", expected: 3,
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.length == this.expected) {
							d.callback(true);
						} else {
							d.errback(new Error("Actual number of returned records (" + data.length + ") doesn't match expected (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		/** rename not supported yet
		new LoginFixture ({name: "test_rename1",
			expected: {
				Name: "Messaging Policy 1 rename",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000"
			},
			oldName: "Messaging Policy 1",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.expected),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.GroupID == this.expected.GroupID &&
							data.Destination == this.expected.Destination &&
							data.ActionList == this.expected.ActionList &&
							data.MaxMessages == this.expected.MaxMessages) {
							d.callback(true);
						} else {
							console.debug(data, this.expected);
							d.errback(new Error("Mismatch between records."));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getByIdRename",
			expected: {
				Name: "Messaging Policy 1 rename",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000"
			},
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.expected.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.GroupID == this.expected.GroupID &&
							data.Destination == this.expected.Destination &&
							data.ActionList == this.expected.ActionList &&
							data.MaxMessages == this.expected.MaxMessages) {
							d.callback(true);
						} else {
							console.debug(data, this.expected);
							d.errback(new Error("Mismatch between records."));
						}
					}),
					error: function(error) {
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		*/
		new LoginFixture ({name: "test_delete1", policyName: "Messaging Policy 2",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.policyName),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getAllAfterDelete", expected: 2,
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.length == this.expected) {
							d.callback(true);
						} else {
							d.errback(new Error("Actual number of returned records (" + data.length + ") doesn't match expected (" + this.expected + ")"));
						}
					}),
					error: function(error) {
						console.error(error);
						d.errback(new Error("Unexpected Return: ", error));
					}
				});

				return d;
			}
		}),
		
		/** TEST VALIDATION **/
		
		new LoginFixture ({name: "test_MissingDestination",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5012",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_MissingDestinationType",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),	
		new LoginFixture ({name: "test_InvalidDestinationType",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Bogus",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),	
		new LoginFixture ({name: "test_MissingActionList",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5012",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),	
		new LoginFixture ({name: "test_MissingFilters",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe"
			},
			expectedRC: "CWLNA5129",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),						
		new LoginFixture ({name: "test_Topic_invalidMaxMessages",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "20000001",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Topic_invalidMaxMessages_NaN",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "none",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5054",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Topic_invalidMaxMessagesBehavior",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "none",
				MaxMessagesBehavior: "FallDownAndDie",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5054",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),					
		new LoginFixture ({name: "test_Topic_invalidActionList",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Control",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Topic_invalidProtocol",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT,TCP"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Topic_invalidMaxMessageTimeToLive_0",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT,JMS",
				MaxMessageTimeToLive: "0"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Topic_invalidMaxMessageTimeToLive_Nan",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT,JMS",
				MaxMessageTimeToLive: "Bob"
			},
			expectedRC: "CWLNA5054",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),		
		new LoginFixture ({name: "test_Topic_invalidMaxMessageTimeToLive_tooBig",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Topic",
				ActionList: "Publish,Subscribe",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT,JMS",
				MaxMessageTimeToLive: "2147483648"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Subscription_invalidMaxMessages",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				GroupID: "UserName",
				Destination: "/Test/Topic/*",
				DestinationType: "Subscription",
				ActionList: "Control",
				MaxMessages: "20000001",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Subscription_invalidClientID",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Subscription/*",
				DestinationType: "Subscription",
				ActionList: "Control",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5128",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Subscription_invalidMaxMessagesBehavior",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				GroupID: "UserName",
				Destination: "/Test/Subscription/*",
				DestinationType: "Subscription",
				ActionList: "Control",
				MaxMessages: "",
				MaxMessagesBehavior: "FallDownAndDie",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),					
		new LoginFixture ({name: "test_Subscription_invalidActionList",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				GroupID: "UserName",
				Destination: "/Test/Subscription/*",
				DestinationType: "Subscription",
				ActionList: "Publish",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Subscription_invalidProtocol",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				GroupID: "UserName",
				Destination: "/Test/Subscription/*",
				DestinationType: "Subscription",
				ActionList: "Control",
				MaxMessages: "10000",
				MaxMessagesBehavior: "RejectNewMessages",
				DisconnectedClientNotification: false,
				Protocol: "MQTT,TCP"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				

		new LoginFixture ({name: "test_Queue_invalidMaxMessages",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				MaxMessages: "2000",
				MaxMessagesBehavior: "",
				DisconnectedClientNotification: false,
				Protocol: "JMS"
			},
			expectedRC: "CWLNA5128",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Queue_invalidMaxMessagesBehavior",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				MaxMessagesBehavior: "DiscardOldMessages",
				Protocol: "JMS"
			},
			expectedRC: "CWLNA5128",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),					
		new LoginFixture ({name: "test_Queue_invalidActionList",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Control",
				Protocol: "JMS"
			},
			expectedRC: "CWLNA5028",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Queue_invalidProtocol",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				Protocol: "MQTT"
			},
			expectedRC: "CWLNA5141",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Queue_invalidMaxMessageTimeToLive_0",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				Protocol: "JMS",
				MaxMessageTimeToLive: "0"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_Queue_invalidMaxMessageTimeToLive_Nan",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				Protocol: "JMS",
				MaxMessageTimeToLive: "Bob"
			},
			expectedRC: "CWLNA5054",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_Queue_invalidMaxMessageTimeToLive_NegativeNumber",
			resource: {
				Name: "Messaging Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				GroupID: "UserName",
				Destination: "/Test/Queue/*",
				DestinationType: "Queue",
				ActionList: "Send,Browse,Receive",
				Protocol: "JMS",
				MaxMessageTimeToLive: "-1"
			},
			expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
		         d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		         }));

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(this.resource.Name),
					putData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data) {
						d.callback(data);
					}),
					error: function(error) {
						d.callback(JSON.parse(error.responseText));
					}
				});

				return d;
			}
		}),				
		new LoginFixture ({name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagingPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(data[i].Name),
								handleAs: "json",
								headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
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
