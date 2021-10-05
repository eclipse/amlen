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

	doh.register("ism.tests.widgets.test_LibertyProperties", [
		//
		// Tests defined with the DOH test fixture
		//
       new LoginFixture ({ name: "test_getLibertyProperty",
		    timeout: 10000,
      	    property: "default_https_port",
            expectedValue: "9087", 
		    runTest: function() {
		    	 var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(response) {
		              doh.assertTrue(response[this.property] == this.expectedValue);
		          }));
		          dojo.xhrGet({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
					    handleAs: "json",
		                load: function(data) {
		                	d.callback(data);
		                },
		                error: function(error) {
		                	console.log(error);
						}
				 }); 
		         return d;
		    }
		}),
	    new LoginFixture ({   name: "test_getLibertyProperty_propertyNotFound",
		    timeout: 10000,
       	    expectedRC: "CWLNA5069",
    	    property: "default_https_port2",
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(response) {
		                doh.assertTrue(response.code == this.expectedRC);
		          }));
		          dojo.xhrGet({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
					    handleAs: "json",
		                load: function(data) {
		                	// it's an error if we get here
							doh.assertTrue(false);
		                },
		                error: function(error, args) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						}
				 }); 
		         return d;
		    }
		}),
	    new LoginFixture ({name: "test_setLibertyProperty_reserverdPort",
	    	value:  {"value":"9088"},
	    	expectedRC: "CWLNA5086", // reserved port
	    	property: "default_https_port",
		    runTest: function() {
		          var d = new doh.Deferred();
		          //setting new value
		          d.addCallback(lang.hitch(this,function(response) {
		                doh.assertTrue(response && response.code == this.expectedRC);
			      }));
		          dojo.xhrPut({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		  				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
		  				handleAs: "json",
		  				putData: JSON.stringify(this.value),
		  				load: function(data) {
				  			  d.callback(data); // error
				  		},
		                error: function(error, args) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						}
		          }); 	
		        return d;
		    }
		}),
	    new LoginFixture ({	name: "test_getLibertyProperty2",
		    timeout: 10000,
      	    property: "default_https_port",
            expectedValue: "9087", 
		    runTest: function() {
		    	 var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(response) {
		              doh.assertTrue(response[this.property] == this.expectedValue);
		          }));
		          dojo.xhrGet({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
					    handleAs: "json",
		                load: function(data) {
		                	d.callback(data);
		                },
		                error: function(error) {
		                	console.log(error);
						}
				 }); 
		         return d;
		    }
		}),
	    new LoginFixture ({name: "test_setLibertyProperty_ltpa_expiration",
		    value:  {"value":"121"},
		    expectedValue: "121",
		    property: "ltpa_expiration",
		    runTest: function() {
		          var d = new doh.Deferred();
		          //setting new value
		          d.addCallback(lang.hitch(this,function(data) {
			  			console.log("putAssert: "+JSON.stringify(data[this.property]));
			  			doh.assertTrue(data[this.property] == this.expectedValue);
			      }));
		          dojo.xhrPut({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		  				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
		  				handleAs: "json",
		  				putData: JSON.stringify(this.value),
		  				load: function(data) {
				  			  d.callback(data);
				  		}
		          }); 	
		        return d;
		    }
		}),
	    new LoginFixture ({	name: "test_getLibertyProperty_ltpa_expiration",
		    timeout: 10000,
      	    property: "ltpa_expiration",
            expectedValue: "121", 
		    runTest: function() {
		    	 var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(response) {
		              doh.assertTrue(response[this.property] == this.expectedValue);
		          }));
		          dojo.xhrGet({
	                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + this.property,
					    handleAs: "json",
		                load: function(data) {
		                	d.callback(data);
		                },
		                error: function(error) {
		                	console.log(error);
						}
				 }); 
		         return d;
		    }
		}),		
	]);
});
