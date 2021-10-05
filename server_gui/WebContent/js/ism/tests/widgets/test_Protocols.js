/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

	doh.register("ism.tests.widgets.test_Protocols", [
		//
		// Protocols test suite:
		//      test_getJMS:    verify jms protocol can be obtained by name
		//      test_getMQTT:    verify mqtt protocol can be obtained by name
		//      test_getWrongName: verify that a GET with an invalid resource returns the proper response
		//      test_getAll:     verifies that the two build in protocols - jms and mqtt are returned
		
        new LoginFixture ({name: "test_getJMS", expected: {"Name":"jms","Topic":true,"Shared":true,"Queue":true,"Browse":true,"Capabilities":15}, 
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.expected;
					doh.assertTrue(obj.Name == expected.Name);
                    doh.assertTrue(obj.Topic == expected.Topic);
                    doh.assertTrue(obj.Shared == expected.Shared);
                    doh.assertTrue(obj.Queue == expected.Queue);
                    doh.assertTrue(obj.Browse == expected.Browse);
                    doh.assertTrue(obj.Capabilities == expected.Capabilities);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/protocols/0/" + encodeURI(this.expected.Name),
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
        new LoginFixture ({name: "test_getMQTT", expected: {"Name":"mqtt","Topic":true,"Shared":true,"Queue":false,"Browse":false,"Capabilities":3}, 
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var expected = this.expected;
					doh.assertTrue(obj.Name == expected.Name);
                    doh.assertTrue(obj.Topic == expected.Topic);
                    doh.assertTrue(obj.Shared == expected.Shared);
                    doh.assertTrue(obj.Queue == expected.Queue);
                    doh.assertTrue(obj.Browse == expected.Browse);
                    doh.assertTrue(obj.Capabilities == expected.Capabilities);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/protocols/0/" + encodeURI(this.expected.Name),
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
        new LoginFixture ({name: "test_getWrongName", expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					console.log(response.code, this.expectedCode);
					doh.assertTrue(response.code == this.expectedCode);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/protocols/0/" + "bogus",
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
  
        new LoginFixture ({name: "test_getAll", expected: 2,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.length == this.expected);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/protocols/0",
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
	]);
});
