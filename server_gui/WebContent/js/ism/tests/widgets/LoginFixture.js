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
define(['ism/tests/Utils','dojo/_base/lang', 'dojo/_base/declare', 'dojo/cookie'], function(Utils, lang, declare, cookie) {

	// All tests that require login should instantiate this fixture.
	var LoginFixture = declare("ism.tests.widgets.LoginFixture", null, {
		
		timeout: 5000,
		name: "unnamed",
		runTest: function() {
			console.log("No runTest provided");
			doh.assertTrue(false);
		},

		/**
		 * pass in an object that contains the following:
		 * String name
		 * function runTest
		 * 
		 * Any additional arguments that might normally be set during setUp may also be defined
		 * 
		 * e.g. {name: "myTestName, foo: "myData", runTest: function () {// the test to run }}
		 */
		constructor: function(args) {
			dojo.safeMixin(this, args);
		},		
		
	    setUp: function() {
			// verify we are actually authenticated
			var success = false;
			for (var i = 0; i < 3 && !success; i++) {	    // if server is just started this seems to fail occasionally	
		    	dojo.xhrPost({
		    		headers: { 
		    			"Content-Type": "application/json"
		    		},
		    		sync: true,
		    		url:Utils.getBaseUri() + "rest/login",
		    		handleAs: "json",
		    		putData: JSON.stringify({"id":"admin", "password":"admin"}),
		    		load: lang.hitch(this,function(data) {
		    			dojo.xhrGet({
		    				url: Utils.getBaseUri() + "rest/config/appliance/status/ismserver",
		    				handleAs: "json",
		    				preventCache: true,
		    				sync: true,
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},	    				
		    				load: function(data){
		    					success = true;
		    				},
		    				error: function(error, ioargs) {
		    					console.dir(error);
		    				}				
		    			});	
		    		}),
		    		error: function(error, ioargs) {
		    			console.log("login failed: ");
		    			console.dir(error);
		    		}
		    	})
			};
	    }
	});
	
	return LoginFixture;

});
