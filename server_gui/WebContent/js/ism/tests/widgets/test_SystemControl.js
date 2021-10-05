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

	doh.register("ism.tests.widgets.test_SystemControl", [

	   new LoginFixture ({name: "test_setHostName", expected: 0, hostname: "ismAppliance",
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response == this.hostname);
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/hostname/" + this.hostname,
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
	    new LoginFixture ({name: "test_getHostName", expected: 0, hostname: "ismAppliance",
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response == this.hostname);
		          }));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/hostname",
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
	    new LoginFixture ({name: "test_setLocateLEDOn", expected: 0, onOff: "on",
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response == this.onOff);
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/locateLED/" + this.onOff,
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
	    new LoginFixture ({name: "test_setLocateLEDOff", expected: 0, onOff: "off",
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response == this.onOff);
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/locateLED/" + this.onOff,
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
	    new LoginFixture ({name: "test_stopIsmServer", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/stop/ismserver",
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
	    new LoginFixture ({name: "test_startIsmServer",
			timeout: 10000,
		    expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/start/ismserver",
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
	    new LoginFixture ({name: "test_forceStopIsmServer",
			timeout: 10000,
		    expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/forceStop",
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
	    new LoginFixture ({name: "test_startIsmServer2",
			timeout: 10000,
	    	expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/start/ismserver",
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
		})	
		/*
	    new LoginFixture ({name: "test_restartOrShutdownAppliance",	expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		          }));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + /*"rest/config/appliance/reboot"*//* "rest/config/appliance/shutdown",
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
		})*/
		
	]);
});
