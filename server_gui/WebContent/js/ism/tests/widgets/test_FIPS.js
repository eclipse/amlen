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

	doh.register("ism.tests.widgets.test_FIPS", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_FIPS_true", runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {			        	  
						  console.log("data.FIPS (expecting true) = " + data.FIPS);
						  doh.assertTrue(data.FIPS === true || data.FIPS.toLowerCase() === "true" );
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/appliance/fips/0",
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
					    url:Utils.getBaseUri() + "rest/config/appliance/fips/0",
					    handleAs: "json",
					    putData: JSON.stringify({"FIPS":true}),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }}),
			new LoginFixture ({name: "test_FIPS_false", runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {			        	  
						  console.log("data.FIPS (expecting false) = " + data.FIPS);
						  doh.assertTrue(data.FIPS === false || data.FIPS.toLowerCase() === "false" );
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/appliance/fips/0",
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
					    url:Utils.getBaseUri() + "rest/config/appliance/fips/0",
					    handleAs: "json",
					    putData: JSON.stringify({"FIPS":false}),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }})
	]);
});
