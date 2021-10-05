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

	doh.register("ism.tests.widgets.test_MessageHub", [
		//
		// MessageHub test suite:
		//      test_clearAll:   try to delete any existing MessageHubs
		//      test_clearEndpoints:  Delete any endpoints
		//      test_clearAll:   Delete any remaining MessageHubs
		//      test_getNone:    verify that a GET with no configured MessageHubs returns the appropriate response (empty array)
		//		test_add1:       adds a single MessageHub
		//      test_getById:    verify that the newly created MessageHub can be GET'ed by specifying its resource location
		//      test_getWrongId: verify that a GET with an invalid resource returns the proper response
		//      test_add2:       adds another MessageHub
		//      test_add3:       adds another MessageHub
		//      test_getAll:     verifies that both created MessageHubs can be GET'ed
		//      test_clearAll:   delete any existing MessageHubs 
		
		new LoginFixture ({name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(data[i].Name),
								sync: true,
								handleAs: "json",
                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                load: function(data) {},
                                error: function(error) {
                                	console.log("Could not delete MessageHub");
            						var response = JSON.parse(error.responseText);
                                	doh.assertTrue(response.message.indexOf("[RC=438]") > 0);
                                }
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
		new LoginFixture ({name: "test_clearEndpoints",
			runTest: function() {
				var d = new doh.Deferred();

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
                                	doh.assertTrue(false);
                                }
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
		new LoginFixture ({name: "test_clearAll2",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(data[i].Name),
								sync: true,
								handleAs: "json",
                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                load: function(data) {},
                                error: function(error) {
                                	console.log("Could not delete MessageHub");
            						var response = JSON.parse(error.responseText);
                                	doh.assertTrue(false);
                                }
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
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
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
		new LoginFixture ({name: "test_add1", messagehub: {Name: "Unit Test 1", Description: "Message hub for unit test 1"}, expected: "config/messagehubs/0/Unit+Test+1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					postData: JSON.stringify(this.messagehub),
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
        new LoginFixture ({name: "test_getById", expected: {Name: "Unit Test 1", Description: "Message hub for unit test 1"}, 
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.expected;
					doh.assertTrue(obj.Name == expected.Name);
                    doh.assertTrue(obj.Description == expected.Description);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(this.expected.Name),
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
					url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + "InvalidName",
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
        new LoginFixture ({name: "test_add2", messagehub: {Name: "Unit Test 2", Description: "Message hub for unit test 2"}, expected: "config/messagehubs/0/Unit+Test+2",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					console.log(this.expected);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					postData: JSON.stringify(this.messagehub),
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
        new LoginFixture ({name: "test_add3", messagehub: {Name: "Unit Test 3", Description: "Message hub for unit test 3"}, expected: "config/messagehubs/0/Unit+Test+3",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					postData: JSON.stringify(this.messagehub),
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
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
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
		/* rename not supported yet {
			name: "test_rename1",
			timeout: 1000,
			setUp: function() {
				this.messagehub = new MessageHub({
					resource: {
						name: "Unit Test 4",
						description: "Message hub for unit test 4"
					}
				});
				this.oldName = "Unit Test 1";
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.messagehub.resource;
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.enabled == expected.enabled);
					doh.assertTrue(obj.port == expected.port);
					doh.assertTrue(obj.ipAddr == expected.ipAddr);
					doh.assertTrue(obj.protocol == expected.protocol);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.messagehub.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					headers: {
						"Content-Type": "application/json"
					},
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
       new LoginFixture ({name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/messagehubs/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/messagehubs/0/" + encodeURI(data[i].Name),
								handleAs: "json",
								headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                load: function(data) {},
                                error: function(error) {
                                	console.log("Could not delete MessageHub");
            						var response = JSON.parse(error.responseText);
                                	doh.assertTrue(false);
                                }
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
