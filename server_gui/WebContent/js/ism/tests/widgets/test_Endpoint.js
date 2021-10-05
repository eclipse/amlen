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

	doh.register("ism.tests.widgets.test_Endpoint", [
		//
		// Endpoint test suite:
		//      test_getNone:    			verify that a GET with no configured Endpoints returns the appropriate response (empty array)
		//		test_add1:       			adds a single Endpoint
		//		test_add_unknown_protocol   verify error when using unknown protocol
		//      test_getById:    			verify that the newly created Endpoint can be GET'ed by specifying its resource location
		//      test_getWrongId: 			verify that a GET with an invalid resource returns the proper response
		//      test_add2:       			adds another Endpoint
		//      test_add3:       			adds another Endpoint
		//      test_getAll:     			verifies that both created Endpoints can be GET'ed
		//      test_rename1:    			reanmes Endpoint 1 to "Unit Test 4"
		//      test_delete1:    			verifies that a delete succeeds
		
		new LoginFixture ({name: "test_prep", 
			messagehub: {Name: "Endpoint Unit Test Hub", Description: "Message hub for endpoint unit testing"},
			connectionPolicy: {	Name: "Endpoint Unit Test Connection Policy", Protocol: "MQTT"},
			messagingPolicy: {	Name: "Endpoint Unit Test Messaging Policy", Destination: "/Test/Topic/*", 
				DestinationType: "Topic", ActionList: "Publish,Subscribe", MaxMessages: "5000", Protocol: "MQTT"},
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

				return d;
			}
		}),

		new LoginFixture ({name: "test_getNone", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
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
		new LoginFixture ({name: "test_add1",
			resource: {
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
			expected: "config/endpoints/0/Unit+Test+1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
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
		new LoginFixture ({name: "test_add_unknown_protocol",  expectedCode: "CWLNA5028",
			resource: {
				Name: "Unit Test 1",
				Enabled: "true",
				Port: "16105",
				Interface: "All",
				Protocol: "bogus",
				SecurityProfile: "",
				MessageHub: "Endpoint Unit Test Hub",
				ConnectionPolicies: "Endpoint Unit Test Connection Policy",
				MessagingPolicies: "Endpoint Unit Test Messaging Policy"
			},

			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
					postData: JSON.stringify(this.resource),
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
			resource: {
				Name: "Unit Test 1",
				Enabled: "true",
				Port: "16105",
				Interface: "All",
				Protocol: "All",
				MessageHub: "Endpoint Unit Test Hub",
				ConnectionPolicies: "Endpoint Unit Test Connection Policy",
				MessagingPolicies: "Endpoint Unit Test Messaging Policy"					
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.Enabled == expected.Enabled);
					doh.assertTrue(obj.Port == expected.Port);
					doh.assertTrue(obj.Interface == expected.Interface);
					doh.assertTrue(obj.Protocol == expected.Protocol);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURI(this.resource.Name),
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
		new LoginFixture ({name: "test_getWrong", expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/endpoints/0/" + "InvalidName",
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
		new LoginFixture ({name: "test_add2",
			resource: {
				Name: "Unit Test 2",
				Enabled: "true",
				Port: "16106",
				Interface: "All",
				Protocol: "JMS,MQTT",
				SecurityProfile: "",
				MessageHub: "Endpoint Unit Test Hub",
				ConnectionPolicies: "Endpoint Unit Test Connection Policy",
				MessagingPolicies: "Endpoint Unit Test Messaging Policy"				
			},
			expected: "config/endpoints/0/Unit+Test+2",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
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
		new LoginFixture ({name: "test_add3",
			resource: {
					Name: "Unit Test 3",
					Enabled: "false",
					Port: "16107",
					Interface: "All",
					Protocol: "All",
					SecurityProfile: "",
					MessageHub: "Endpoint Unit Test Hub",
					ConnectionPolicies: "Endpoint Unit Test Connection Policy",
					MessagingPolicies: "Endpoint Unit Test Messaging Policy"
				},
			expected: "config/endpoints/0/Unit+Test+3",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
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
		new LoginFixture ({name: "test_getAll", expected: 3,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/endpoints/0",
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
		/* rename not supported yet
		new LoginFixture ({name: "test_rename1",
			resource: {
				Name: "Unit Test 4",
				Enabled: "true",
				Port: "16105",
				Interface: "All",
				Protocol: "All",
				SecurityProfile: ""
			},
			oldName: "Unit Test 1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.Enabled == expected.Enabled);
					doh.assertTrue(obj.Port == expected.Port);
					doh.assertTrue(obj.Interface == expected.Interface);
					doh.assertTrue(obj.Protocol == expected.Protocol);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURI(this.oldName),
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
		}),
		*/
		new LoginFixture ({name: "test_delete1", Name: "Unit Test 1",
			runTest: function() {
				var d = new doh.Deferred();
				
				d.addCallback(function(pass) {
					doh.assertTrue(pass);
				});

				var deferred = dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURI(this.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						console.log("delete returned");
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.callback(false);
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
								url: Utils.getBaseUri() + "rest/config/endpoints/0/" + encodeURI(data[i].Name),
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
							url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(_this.messagehub.Name),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data) {
								dojo.xhrDelete({
									url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(_this.connectionPolicy.Name),
									handleAs: "json",
									headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
									load: function(data) {
										dojo.xhrDelete({
											url: Utils.getBaseUri() + "rest/config/messagingPolicies/0/" + encodeURI(_this.messagingPolicy.Name),
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
		}),		
	]);
});
