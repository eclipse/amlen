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
define(["ism/tests/Utils", "dojo/_base/lang",	'ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {

	doh.register("ism.tests.widgets.test_License", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getLicenseContent", lang: "en",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(function(data) {
					doh.assertTrue(data && data.status === 0);
				});

				dojo.xhrGet({
				    url: Utils.getBaseUri() + "rest/config/license/"+this.lang,
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
				    preventCache: true,
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	console.error(error);
				    	doh.assertTrue(false);
				    }
				  });				
				return d;
			}
		}),
		new LoginFixture ({	name: "test_acceptLicense", lang: "en",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(function(data) {
					doh.assertTrue(data && data.status === 5);
				});

				dojo.xhrPost({
				    url: Utils.getBaseUri() + "rest/config/license/"+this.lang,
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	console.error(error);
				    	doh.assertTrue(false);
				    }
				  });				
				return d;
			}
		}),
		new LoginFixture ({	name: "confirm_LicenseAccepted",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(function(data) {
					doh.assertTrue(data && data.status === 5);
				});

				dojo.xhrGet({
				    url: Utils.getBaseUri() + "rest/config/license",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	console.error(error);
				    	doh.assertTrue(false);
				    }
				  });				
				return d;
			}
		})
	]);
});
