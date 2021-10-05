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

	doh.register("ism.tests.widgets.test_DnsServers", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_setDnsServers",
			value: ["9.9.9.9","10.1.2.3","5.8.6.7"],
		    expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.value.length); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/servers",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setDnsSearch",
			expected: 0,
		    value: ["10.10.10.10","100.12.28.39","54.83.64.77"],
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.value.length); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/search",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getDnsServers",
	    	expected: 0,
		    expectedLength: 3, 
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.servers.length == this.expectedLength); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/dns/servers",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		
		new LoginFixture ({ name: "test_getDnsSearch", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/search",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setDnsServers_null", value: [], expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.expected); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/servers",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setDnsSearch_null", value: [], expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.expected); 
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/search",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getDnsServers_null", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.servers.length ==  this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/dns/servers",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		
		new LoginFixture ({name: "test_getDnsSearch_null",expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.servers.length == this.expected); 
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/dns/search",
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
