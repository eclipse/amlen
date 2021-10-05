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

	doh.register("ism.tests.widgets.test_TimeZones", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getTimeZones", expectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expectedRC); 
		                doh.assertTrue(data.timezones.length > this.expectedRC); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/timezones",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({ name: "test_getCurrentTimeZone", expectedLength: 1, expectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    console.log(JSON.stringify(data.timezones.length));
		                doh.assertTrue(data.RC == this.expectedRC);
		                doh.assertTrue(data.timezones.length == this.expectedLength); 
		          }));
		          dojo.xhrGet({ 	  
					    url: Utils.getBaseUri() + "rest/config/appliance/timezones/current",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({ name: "test_setCurrentTimeZone", 
			value:  {"timezone":"EET"}, 
	    	expected: 0,
	    	expectedValue: "EET",
	    	originalValue: "",
		    runTest: function() {
		    	 //the test should be read from the bottom up
		          var d = new doh.Deferred();
		          var d2 = new doh.Deferred();
		          
		          //assessing return from last set
		          var postAssert = lang.hitch(this,function(data) {
		  			console.log("postAssert: "+JSON.stringify(data.timezones[0].timezone));
		  			doh.assertTrue(data.RC == this.expected);
		  			doh.assertTrue(data.timezones[0].timezone == this.expectedValue);  
		          });
		          
		          //setting new value
		          d2.addCallback(postAssert);
		          var doPost = lang.hitch(this, function() {
		        	  console.log("doPost: "+JSON.stringify(this.value));
		        	  dojo.xhrPost({
		                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		  					url: Utils.getBaseUri() + "rest/config/appliance/timezones",
		  					handleAs: "json",
		  					putData: JSON.stringify(this.value),
		  					load: function(data) {
				  				  d2.callback(data);
				  			}
		        	  }); 	
		          });
		          
		          //recording current value, scheduling set
		          var getAssert = lang.hitch(this,function(data) {
		        	  console.log("getAssert: "+JSON.stringify(data.timezones[0].timezone));
		        	  doh.assertTrue(data.RC == this.expected); 
		        	  this.originalValue = data.timezones[0].timezone;
		        	  doPost();
		          });
		          
		          //getting current value
		          d.addCallback(getAssert);
		          dojo.xhrGet({
		        	  url: Utils.getBaseUri() + "rest/config/appliance/timezones/current",
		        	  handleAs: "json",
                      headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	  load: function(data) {
		  				  d.callback(data);
		  			  }
		          }); 
		          
		        return d;
		    },
		    tearDown: function() {
		    	 //return back to original value
		    	 var d = new doh.Deferred();
		    	 var val =  {"timezone":"UTC"};
		    	 d.addCallback(lang.hitch(this,function(data) {
		        	    console.log(JSON.stringify(data.locale));
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.timezones.length == this.val.length); 
		         }));
		         console.log(JSON.stringify(val));
		         dojo.xhrPost({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	 	url: Utils.getBaseUri() + "rest/config/appliance/timezones",
		        	 	handleAs: "json",
		        	 	putData: JSON.stringify(val),
		                load: function(data) {
		                     d.callback(data);
		                }
				 }); 
		         return d;
		    }
		})
	]);
});
