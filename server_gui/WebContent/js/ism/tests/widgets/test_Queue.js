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

	doh.register("ism.tests.widgets.test_Queue", [
		//
		// Queue test suite:
		//      test_clearAll:   delete any existing Queues
		//      test_getNone:    verify that a GET with no configured Queues returns the appropriate response (empty array)
		//		test_add1:       adds a single Queue
		//      test_getById:    verify that the newly created Queue can be GET'ed by specifying its resource location
		//      test_getWrongId: verify that a GET with an invalid resource returns the proper response
		//      test_add2:       adds another Queue
		//      test_add3:       adds another Queue
		//      test_getAll:     verifies that both created Queues can be GET'ed
		//      test_rename1:    renames Queue Unit Test 1 to "Unit Test 4"
		//      test_clearAll:   delete any existing Queues
		
        new LoginFixture ({name: "test_clearAll",
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
        new LoginFixture ({name: "test_getNone", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.debug(obj.length);
					console.debug(this.expected);				
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/queues/0",
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
                Description: "Queue for unit test 1",
                MaxMessages: "100000",
                AllowSend: true,
                ConcurrentConsumers: true,
                DiscardMessages: true
            },
			expected: "config/queues/0/Unit+Test+1",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					console.log(uri.indexOf(this.expected));
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
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
					}
				});

				return d;
			}
		}),
        new LoginFixture ({name: "test_getById", 
            resource: {
                Name: "Unit Test 1",
                Description: "Queue for unit test 1",
                MaxMessages: "100000",
                AllowSend: true,
                ConcurrentConsumers: true,
                DiscardMessages: true
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.resource;
					console.debug("obj is "+ obj.Name +" and expected "+ expected.Name);
					console.debug("obj is "+ obj.Description +" and expected "+ expected.Description);
					console.debug("obj is "+ obj.MaxMessages +" and expected "+ expected.MaxMessages);
					console.debug("obj is "+ obj.AllowSend +" and expected "+ expected.AllowSend);
					console.debug("obj is "+ obj.ConcurrentConsumers +" and expected "+ expected.ConcurrentConsumers);
					console.debug("obj is "+ obj.DiscardMessages +" and expected "+ expected.DiscardMessages);
					doh.assertTrue(obj.Name == expected.Name);
					doh.assertTrue(obj.Description == expected.Description);
					doh.assertTrue(obj.MaxMessages == expected.MaxMessages);
					doh.assertTrue(obj.AllowSend == expected.AllowSend);
					doh.assertTrue(obj.ConcurrentConsumers == expected.ConcurrentConsumers);
					doh.assertTrue(obj.DiscardMessages == expected.DiscardMessages);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/queues/0/" + encodeURI(this.resource.Name),
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
					url: Utils.getBaseUri() + "rest/config/queues/0/" + "InvalidName",
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
                Description: "Queue for unit test 2",
                MaxMessages: "200000",
                AllowSend: false,
                ConcurrentConsumers: true,
                DiscardMessages: false
            },
            expected: "config/queues/0/Unit+Test+2",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
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
					}
				});

				return d;
			}
		}),
        new LoginFixture ({name: "test_add3",
			resource: {
                Name: "Unit Test 3",
                Description: "Queue for unit test 3",
                MaxMessages: 250000,
                AllowSend: false,
                ConcurrentConsumers: false,
                DiscardMessages: false
            },
			expected: "config/queues/0/Unit+Test+3",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(uri) {
					console.log(uri);
					doh.assertTrue(uri.indexOf(this.expected) != -1);
				}));

				var deferred = dojo.xhrPost({
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
					url: Utils.getBaseUri() + "rest/config/queues/0",
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
			},
			timeout: 10000 // 10 second timeout to make sure ismserver has caught up
		}),
		// TODO: not clear if we keep the rename yet, so this is a to do..
		/*{
			name: "test_rename1",
			timeout: 1000,
			setUp: function() {
				this.qmconn = new QueueManagerConnection({
					resource: {
						Name: "Unit Test 4",
						Description: "Queue manager for unit test 4"
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
					url: Utils.getBaseUri() + "rest/config/queuesManagerConnections/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.qmconn.resource),
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
		},*/
        new LoginFixture ({	name: "test_clearAll", 
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
		})
	]);
});
