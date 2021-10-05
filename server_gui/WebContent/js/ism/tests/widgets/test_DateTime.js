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

	doh.register("ism.tests.widgets.test_DateTime", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getDateTime", expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.dateTime.length > this.expected); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/datetime",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_setDateTime", expected: 0, value: {"value": new Date().getTime() + 60000},
		    runTest: function() {
		          var d = new doh.Deferred();
		          var d2 = new doh.Deferred();
		          
		          var getAssert = lang.hitch(this,function(data) {
			  			console.log("getAssert: "+JSON.stringify(data.dateTime[0]));
			  		    doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.dateTime.length > 0); 
			      });
		          
		          d2.addCallback(getAssert)
		          var doGet = lang.hitch(this, function() {
		        	  dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/appliance/datetime",
						    handleAs: "json",
		                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
			                load: function(data) {
			                      d2.callback(data);
			                }
					 }); 
		          });
		          
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		                doh.assertTrue(data.dateTime.length > this.expected); 
		                doGet();
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
					    url: Utils.getBaseUri() + "rest/config/appliance/datetime",
					    handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    },
		    tearDown: function() {
		    	 //return back to current time and restart the webui
		    	 var d = new doh.Deferred();
		    	 var date = new Date();
			     this.datetime = date.getTime();
			     this.value =  {"value":this.datetime};
			     this.expected = 0;
			     
		    	 d.addCallback(lang.hitch(this,function(data) {
		    		    console.log(JSON.stringify(data.dateTime[0]));
		                doh.assertTrue(data.RC == this.expected); 
		                doh.assertTrue(data.dateTime.length > this.expected); 
		         }));
		    	
		         console.log(JSON.stringify(this.value));
		         dojo.xhrPost({
		        	 	url: Utils.getBaseUri() + "rest/config/appliance/datetime",
		        	 	handleAs: "json",
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	 	putData: JSON.stringify(this.value),
		                load: function(data) {
  	                        d.callback(data);
							dojo.xhrPut({
			                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
								url: Utils.getBaseUri() + "rest/config/appliance/restart/webui",
								handleAs: "json"
							});		                   
		                }
				 }); 
		         return d;
		    }
		})
	]);
});
