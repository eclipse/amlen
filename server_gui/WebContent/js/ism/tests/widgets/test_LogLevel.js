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

	doh.register("ism.tests.widgets.test_LogLevel", [
		// LogLevel tests:
	    new LoginFixture ({	//getting current log level
		    name: "test_setLogLevel",
		    newValue: { LogLevel: "MIN" },
		    logType: "LogLevel",
		    runTest: function() {
		        var d = new doh.Deferred();
		        
		        d.addCallback(lang.hitch(this,function(data) {
		        	doh.assertTrue(true);
		        	return true;
	        	}));

		        dojo.xhrPut({
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					url: Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
  					handleAs: "json",
  					putData: JSON.stringify(this.newValue),
  					load: function(data, ioargs) {
  						console.log("SET returns: ", data, ioargs);
		  				d.callback();
		  			},
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    }
		}),
	    new LoginFixture ({//getting current log level
		    name: "test_getLogLevel",
	    	expected: "MIN",
	    	logType: "LogLevel",
		    runTest: function() {
		        var d = new doh.Deferred();
		        var d2 = new doh.Deferred();

		        d.addCallback(lang.hitch(this,function(data) {
		        	console.log(data.LogLevel);
		            doh.assertTrue(data.LogLevel == this.expected); 
	        	}));
		        dojo.xhrGet({
					url:Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		            load: function(data) {
		                d.callback(data);
		            },
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    },
		}),
	    new LoginFixture ({	//getting current log level
		    name: "test_setConnectionLog",
	    	newValue: { LogLevel: "MIN" },
		    logType: "ConnectionLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        
		        d.addCallback(lang.hitch(this,function(data) {
		        	doh.assertTrue(true);
		        	return true;
	        	}));

		        dojo.xhrPut({
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					url: Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
  					handleAs: "json",
  					putData: JSON.stringify(this.newValue),
  					load: function(data, ioargs) {
  						console.log("SET returns: ", data, ioargs);
		  				d.callback();
		  			},
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    }
		}),
	    new LoginFixture ({//getting current log level
		    name: "test_getConnectionLog",
	    	expected: "MIN",
		    logType: "ConnectionLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        var d2 = new doh.Deferred();

		        d.addCallback(lang.hitch(this,function(data) {
		        	console.log(data.LogLevel);
		            doh.assertTrue(data.LogLevel == this.expected); 
	        	}));
		        dojo.xhrGet({
					url:Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		            load: function(data) {
		                d.callback(data);
		            },
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    },
		}),
	    new LoginFixture ({	//getting current log level
		    name: "test_setSecurityLog",
	    	newValue: { LogLevel: "MAX" },
	    	logType: "SecurityLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        
		        d.addCallback(lang.hitch(this,function(data) {
		        	doh.assertTrue(true);
		        	return true;
	        	}));

		        dojo.xhrPut({
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					url: Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
  					handleAs: "json",
  					putData: JSON.stringify(this.newValue),
  					load: function(data, ioargs) {
  						console.log("SET returns: ", data, ioargs);
		  				d.callback();
		  			},
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    }
		}),
	    new LoginFixture ({	//getting current log level
		    name: "test_getSecurityLog",
	    	expected: "MAX",
	    	logType: "SecurityLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        var d2 = new doh.Deferred();

		        d.addCallback(lang.hitch(this,function(data) {
		        	console.log(data.LogLevel);
		            doh.assertTrue(data.LogLevel == this.expected); 
	        	}));
		        dojo.xhrGet({
					url:Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		            load: function(data) {
		                d.callback(data);
		            },
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    },
		}),
	    new LoginFixture ({	//getting current log level
		    name: "test_setAdminLog",
	    	newValue: { LogLevel: "MIN" },
	    	logType: "AdminLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        
		        d.addCallback(lang.hitch(this,function(data) {
		        	doh.assertTrue(true);
		        	return true;
	        	}));

		        dojo.xhrPut({
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					url: Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
  					handleAs: "json",
  					putData: JSON.stringify(this.newValue),
  					load: function(data, ioargs) {
  						console.log("SET returns: ", data, ioargs);
		  				d.callback();
		  			},
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    }
		}),
	    new LoginFixture ({	//getting current log level
		    name: "test_getAdminLog",
	    	expected: "MIN",
	    	logType: "AdminLog",
		    runTest: function() {
		        var d = new doh.Deferred();
		        var d2 = new doh.Deferred();

		        d.addCallback(lang.hitch(this,function(data) {
		        	console.log(data.LogLevel);
		            doh.assertTrue(data.LogLevel == this.expected); 
	        	}));
		        dojo.xhrGet({
					url:Utils.getBaseUri() + "rest/config/loglevel/" + this.logType,
					handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		            load: function(data) {
		                d.callback(data);
		            },
					error: function(error) {
						console.error(error);
					}
				}); 
		        return d;
		    },
		})
	]);
});
