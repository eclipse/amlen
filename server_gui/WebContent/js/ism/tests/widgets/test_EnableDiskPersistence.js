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

	doh.register("ism.tests.widgets.test_EnableDiskPersistence", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_EnableDiskPersistence_true", runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {			        	  
						  console.log("data.EnableDiskPersistence (expecting true) = " + data.EnableDiskPersistence);
						  doh.assertTrue(data.EnableDiskPersistence === true || data.EnableDiskPersistence.toLowerCase() === "true" );
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/appliance/enableDiskPersistence",
						    handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
			                load: function(data) {
			                      d2.callback(data);
			                }
					 }); 
			         return d2;
	        		  
		          }));
		          dojo.xhrPut({
		        	 	headers: { 
		        	 		"Content-Type": "application/json",
		        	 		"X-Session-Verify": dojo.cookie("xsrfToken")
		        	 	},	  	  
					    url:Utils.getBaseUri() + "rest/config/appliance/enableDiskPersistence",
					    handleAs: "json",
					    putData: JSON.stringify({"EnableDiskPersistence":true}),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		          return d;
		}}),
		new LoginFixture ({name: "test_EnableDiskPersistence_false", runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {			        	  
						  console.log("data.EnableDiskPersistence (expecting false) = " + data.EnableDiskPersistence);
						  doh.assertTrue(data.EnableDiskPersistence === false || data.EnableDiskPersistence.toLowerCase() === "false" );
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/appliance/enableDiskPersistence",
						    handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
						    load: function(data) {
			                      d2.callback(data);
			                }
					 }); 
			         return d2;
	        		  
		          }));
		          dojo.xhrPut({
		        	 	headers: { 
		        	 		"Content-Type": "application/json",
		        	 		"X-Session-Verify": dojo.cookie("xsrfToken")
		        	 	},	  	  
					    url:Utils.getBaseUri() + "rest/config/appliance/enableDiskPersistence",
					    handleAs: "json",
					    putData: JSON.stringify({"EnableDiskPersistence":false}),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }})
	]);
});
