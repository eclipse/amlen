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
define(['ism/tests/Utils','dojo/_base/lang'], function(Utils, lang) {

	doh.register("ism.tests.widgets.login", [
		//
		// Tests defined with the DOH test fixture
		//
		{
		    name: "login",
		    timeout: 5000,
		    runTest: function() {
		    	dojo.xhrPost({
		    		headers: { 
		    			"Content-Type": "application/json"
		    		},
		    		sync: true,
		    		url:Utils.getBaseUri() + "rest/login",
		    		handleAs: "json",
		    		putData: JSON.stringify({"id":"admin", "password":"admin"}),
		    		load: function(data) {
		    			doh.assertTrue(data.username == "admin");
		    			// verify we are actually authenticated
		    			dojo.xhrGet({
		    				url: this.getBaseUri() + "rest/config/appliance/status/ismserver",
		    				handleAs: "json",
		    				preventCache: true,
		    				sync: true,
		    				load: function(data){
		    				},
		    				error: function(error, ioargs) {
		    					// check the status
		    					if (ioargs && ioargs.status && ioargs.xhr.status == 403) {
		    						console.log("Cannot get status, request is forbidden");
		    						doh.assertTrue(false);
		    					}		    					
		    				}				
		    			});			
		    		},
		    		error: function(error, ioargs) {
		    			console.log("login failed: ");
		    			console.dir(ioargs);
		    			doh.assertTrue(false);
		    		}
		    	});
		    	return true;
		    }
		}	
	]);
});
