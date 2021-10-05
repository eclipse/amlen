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

	doh.register("ism.tests.widgets.test_Locale", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({//getting current locale
		    name: "test_getLocale",
	    	expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    console.log(JSON.stringify(data.locales));
		                doh.assertTrue(data.RC == this.expected); 
		                //doh.assertTrue(data.locales[0] == ""); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/locale",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({	//getting all available locales
		    name: "test_getAvailableLocales",
		    expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    console.log(JSON.stringify(data.locales));
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.locales.length > this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/locale/available",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setLocale",
	    	value:  {"country":"US","language":"en"},
	    	expected: 0,
	    	expectedValue: ["en_US"],
	    	originalValue: "",
		    runTest: function() {
		    	  //the test should be read from the bottom up
		          var d = new doh.Deferred();
		          var d2 = new doh.Deferred();
		          
		          //assessing return from last set
		          var postAssert = lang.hitch(this,function(data) {
		  			console.log("postAssert: "+JSON.stringify(data.locales));
		  			doh.assertTrue(data.RC == this.expected);
		  			doh.assertTrue(data.locales[0].locale == this.expectedValue);  
		          });
		          
		          //setting new value
		          d2.addCallback(postAssert);
		          var doPost = lang.hitch(this, function() {
		        	  console.log("doPost: "+JSON.stringify(this.value));
		        	  dojo.xhrPost({
		                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		  					url: Utils.getBaseUri() + "rest/config/appliance/locale",
		  					handleAs: "json",
		  					putData: JSON.stringify(this.value),
		  					load: function(data) {
				  				  d2.callback(data);
				  			}
		        	  }); 	
		          });
		          
		          //recording current value, scheduling set
		          var getAssert = lang.hitch(this,function(data) {
		        	  console.log("getAssert: "+JSON.stringify(data.locales));
		        	  doh.assertTrue(data.RC == this.expected); 
		        	  this.originalValue = data.locales[0].locale;
		        	  doPost();
		          });
		          
		          //getting current value
		          d.addCallback(getAssert);
		          dojo.xhrGet({
		        	  url: Utils.getBaseUri() + "rest/config/appliance/locale",
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
		    	 var valueArray = this.originalValue.split("_");
		    	 var val =  {"country":valueArray[0],"language":valueArray[1]};
		    	 d.addCallback(lang.hitch(this,function(data) {
		        	    console.log(JSON.stringify(data.locales));
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.locales.length == this.val.length); 
		         }));
		         console.log(JSON.stringify(val));
		         dojo.xhrPost({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	 	url: Utils.getBaseUri() + "rest/config/appliance/locale",
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
