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

	doh.register("ism.tests.widgets.test_ConnectionPolicy", [
		//
		// Connection Policy test suite:
		//      test_clearAll:          	delete any existing policies
		//      test_getNone:            	verify that a GET with no configured policies returns the appropriate response (empty array)
		//		test_add1:               	adds a single policy
		//		test_add_unknown_protocol   verify error when using unknown protocol
		//      test_getById:            	verify that the newly created policy can be GET'ed by specifying its resource location
		//      test_getWrongId:         	verify that a GET with an invalid resource returns the proper response
		//      test_add2:               	adds another policy
		//      test_add3:               	adds another policy
		//      test_getAll:             	verifies that both created policies can be GET'ed
		//      test_rename1:            	renames policy 1 
		//      test_getByIdRename:      	verify that the renamed policy can be GET'ed by specifying its resource location
		//      test_delete1:            	verifies that a delete succeeds
		//      test_getAllAfterDelete:  	verify that the deleted policy is gone
		//      test_clearAll:           	delete any existing policies
		new LoginFixture ({	name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(data[i].Name),
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
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
				Name: "Connection Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName",
				CommonNames: "CertificateName.cert",
				Protocol: "MQTT"
			},
			expected: "config/connectionPolicies/0/Connection+Policy+1",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
		new LoginFixture ({name: "test_add_unknown_protocol", expectedCode: "CWLNA5028",
			policy: {
				Name: "Connection Policy Bad Protocol",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName",
				Protocol: "bogus"
			},

			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
				Name: "Connection Policy 1",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName",
				CommonNames: "CertificateName.cert",
				Protocol: "MQTT"
			},
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(this.expected.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.ClientAddress == this.expected.ClientAddress &&
							data.UserID == this.expected.UserID &&
							data.CommonNames == this.expected.CommonNames &&
							data.Protocol == this.expected.Protocol) {
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

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + "InvalidName",
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
				Name: "Connection Policy 2",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_2",
				ClientAddress: "169.254.102.183",
				UserID: "UserName2",
				CommonNames: "CertificateName.cert",
				Protocol: "JMS"
			},
			expected: "config/connectionPolicies/0/Connection+Policy+2",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
				Name: "Connection Policy 3",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName3",
				CommonNames: "CertificateName.cert",
				Protocol: "MQTT"
			},
			expected: "config/connectionPolicies/0/Connection+Policy+3",
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
		/* rename is not supported yet
		new LoginFixture ({name: "test_rename1",
			expected: {
				Name: "Connection Policy 1 rename",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName",
				CommonNames: "CertificateName.cert",
				Protocol: "MQTT"
			},
			oldName: "Connection Policy 1",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(this.oldName),
					putData: JSON.stringify(this.expected),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.ClientAddress == this.expected.ClientAddress &&
							data.UserID == this.expected.UserID &&
							data.CommonNames == this.expected.CommonNames &&
							data.Protocol == this.expected.Protocol) {
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
				Name: "Connection Policy 1 rename",
				Description: "A description for this policy.",
				ClientID: "unit_test_client_1",
				ClientAddress: "169.254.102.183",
				UserID: "UserName",
				CommonNames: "CertificateName.cert",
				Protocol: "MQTT"
			},
			runTest: function() {
				var d = new doh.Deferred();

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(this.expected.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						if (data.Name == this.expected.Name &&
							data.Description == this.expected.Description &&
							data.ClientID == this.expected.ClientID &&
							data.ClientAddress == this.expected.ClientAddress &&
							data.UserID == this.expected.UserID &&
							data.CommonNames == this.expected.CommonNames &&
							data.Protocol == this.expected.Protocol) {
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
		new LoginFixture ({	name: "test_delete1", policyName: "Connection Policy 2",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(this.policyName),
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
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
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
		new LoginFixture ({name: "test_clearAll",
			runTest: function() {
				var d = new doh.Deferred();

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/connectionPolicies/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							var record = data[i];
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/connectionPolicies/0/" + encodeURI(data[i].Name),
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
