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
	
	doh.register("ism.tests.widgets.test_NtpServers", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({	name: "test_getNtpServers", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.servers.length == this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/ntp-servers",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setNtpServers", expected: 0, value:  ["9.9.9.9", "10.1.2.3", "5.8.6.7"],
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.value.length); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ntp-servers",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getNtpServers_expecting3", servers: 3, expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.servers); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/ntp-servers",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setNtpServers_to_0", value: [], expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.value.length); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ntp-servers",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getNtpServers_expecting_0", expectedLength: 0, expectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expectedRC); 
		                doh.assertTrue(data.servers.length == this.expectedLength); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/ntp-servers",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		})
	]);
});
