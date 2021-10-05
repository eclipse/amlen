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

	doh.register("ism.tests.widgets.test_NetworkInterfaces", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getEthernet_eth0",
		    timeout: 10000,
            expectedName: "eth0", 
            expectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expectedRC); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/network-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_not_defined", expectedName: "eth9", notExpectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC != this.notExpectedRC);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/network-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_malformed", expectedName: "eth99", notExpectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC != this.notExpectedRC);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/network-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_all", value: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		              doh.assertTrue(data.interfaces);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/network-interfaces/",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
	]);
});
