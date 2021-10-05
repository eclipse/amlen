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

	doh.register("ism.tests.widgets.test_Status", [

       new LoginFixture ({name: "test_getByName", expected: 1, processName: "ismserver",
			runTest: function() {
				var d = new doh.Deferred();

				 d.addCallback(lang.hitch(this,function(data) {
					 console.log(data);
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.ServerState == ""+this.expected);
		          }));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/status/" + this.processName,
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
	    new LoginFixture ({name: "test_getFirmware", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response.length > this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/firmware",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
	    new LoginFixture ({name: "test_getUpTime", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.Response.length > this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/uptime",
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
