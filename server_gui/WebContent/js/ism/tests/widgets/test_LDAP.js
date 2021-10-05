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
define([
	"ism/widgets/User",
	'dojo/_base/lang',
	"ism/tests/Utils",
	"ism/config/Resources",
	'ism/tests/widgets/LoginFixture'
	], function(User, lang, Utils, Resources, LoginFixture) {
	
	doh.register("ism.tests.widgets.test_LDAP", [
		//
		// External LDAP
		//
		new LoginFixture ({name: "test_noLDAP",			
			runTest: function() {
				console.debug("Starting Test: test_noLDAP");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_noLDAP - return data");
	        	    console.dir(data);
	        	    doh.assertTrue(data.length == 0);
	        	    
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					sync: true,
					load: function(data){
						d.callback(data);
					},
					error: function(error, ioargs) {
				  	  	d.errback(new Error("Unexpected Return: ", error));
					}
				});
				return d;
			}
		}),
		new LoginFixture ({name: "test_createLDAP1", 
			ldapdata: '{"URL":"ldap://www.ldap.com","BaseDN":"ldapBaseDN","BindDN":"ldapBindDN","BindPassword":"password","IgnoreCase":"true","UserSuffix":"myUserFilter={12}","GroupSuffix":"myGroupFilter={12}","UserIdMap":"testUsers=awesomeUsers","GroupIdMap":"testGroups=boringGroups","GroupMemberIdMap":"testMemberGroups=boringGroups","Timeout":10,"enableCache":"true","CacheTimeout":10,"GroupCacheTimeout":10}',
		    runTest: function() {
		    	console.debug("Starting Test: test_createLDAP1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_createLDAP1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.ldapdata,
					sync: true,
					load: function(data){
					    d.callback(data);
					},
					error: function(error, ioargs) {
					    d.errback(new Error("Unexpected Return: ", error));
					}
				});
		        return d;
		    }
		}),
		new LoginFixture ({name: "test_verifyCreateLDAP1",			
			runTest: function() {
				console.debug("Starting Test: test_verifyCreateLDAP1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_verifyCreateLDAP1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.length; i++) {
	        	    	if (data[i].id == "MyLDAP") {
	        	    		found = true;
	        	    		doh.assertTrue(data[i].URL == "www.ldap.com");
	        	    		doh.assertTrue(data[i].BaseDN == "ldapBaseDN");
	        	    		doh.assertTrue(data[i].BindDN == "ldapBindDN");
	        	    		doh.assertTrue(data[i].BindPassword == "password");
	        	    		doh.assertTrue(data[i].IgnoreCase == "true");
	        	    		doh.assertTrue(data[i].userfilter == "myUserFilter={12}");
	        	    		doh.assertTrue(data[i].groupfilter == "myGroupFilter={12}");
	        	    		doh.assertTrue(data[i].UserIdMap == "true");
	        	    		doh.assertTrue(data[i].GroupIdMap == "testUsers=awesomeUsers");
	        	    		doh.assertTrue(data[i].groupfilter == "testGroups=boringGroups");
	        	    		doh.assertTrue(data[i].Timeout == 10);
	        	    		doh.assertTrue(data[i].enableCache == "true");
	        	    		doh.assertTrue(data[i].CacheTimeout == 10);
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					sync: true,
					load: function(data){
						d.callback(data);
					},
					error: function(error, ioargs) {
				  	  	d.errback(new Error("Unexpected Return: ", error));
					}
				});
				return d;
			}
		}),
		new LoginFixture ({ name: "test_modifyLDAP1",
			ldapdata: '{"URL":"ldap://www.moreldaps.com","BaseDN":"ldapBaseDN","BindDN":"ldapBindDN","BindPassword":"password","IgnoreCase":"true","UserSuffix":"myUserFilter={12}","GroupSuffix":"myGroupFilter={12}","UserIdMap":"testUsers=awesomeUsers","GroupIdMap":"testGroups=boringGroups","GroupMemberIdMap":"testMemberGroups=boringGroups","Timeout":10,"enableCache":"true","CacheTimeout":10,"GroupCacheTimeout":10}',
		    runTest: function() {
		    	console.debug("Starting Test: test_modifyLDAP1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_modifyLDAP1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.ldapdata,
				    sync: true,
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	d.errback(new Error("Unexpected Return: ", error));
				    }
				});
		        return d;
		    }
		}),
		new LoginFixture ({name: "test_verifyModifyLDAP1",			
			runTest: function() {
				console.debug("Starting Test: test_verifyModifyLDAP1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_verifyModifyLDAP1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.length; i++) {
	        	    	if (data[i].id == "MyLDAP") {
	        	    		found = true;
	        	    		doh.assertTrue(data[i].URL == "www.moreldaps.com");
	        	    		doh.assertTrue(data[i].BaseDN == "ldapBaseDN");
	        	    		doh.assertTrue(data[i].BindDN == "ldapBindDN");
	        	    		doh.assertTrue(data[i].BindPassword == "password");
	        	    		doh.assertTrue(data[i].IgnoreCase == "true");
	        	    		doh.assertTrue(data[i].userfilter == "myUserFilter={12}");
	        	    		doh.assertTrue(data[i].groupfilter == "myGroupFilter={12}");
	        	    		doh.assertTrue(data[i].UserIdMap == "true");
	        	    		doh.assertTrue(data[i].GroupIdMap == "testUsers=awesomeUsers");
	        	    		doh.assertTrue(data[i].groupfilter == "testGroups=boringGroups");
	        	    		doh.assertTrue(data[i].Timeout == 10);
	        	    		doh.assertTrue(data[i].enableCache == "true");
	        	    		doh.assertTrue(data[i].CacheTimeout == 10);
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					sync: true,
					load: function(data){
						d.callback(data);
					},
					error: function(error, ioargs) {
				  	  	d.errback(new Error("Unexpected Return: ", error));
					}
				});
				return d;
			}
		}),
		new LoginFixture ({name: "test_enable_LDAP1",
			ldapdata: '{"URL":"ldap://www.moreldaps.com","BaseDN":"ldapBaseDN","BindDN":"ldapBindDN","BindPassword":"password","IgnoreCase":"true","UserSuffix":"myUserFilter={12}","GroupSuffix":"myGroupFilter={12}","UserIdMap":"testUsers=awesomeUsers","GroupIdMap":"testGroups=boringGroups","GroupMemberIdMap":"testMemberGroups=boringGroups","Timeout":10,"enableCache":"true","CacheTimeout":10,"GroupCacheTimeout":10,"Enabled":"true"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_enable_LDAP1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_enable_LDAP1 - return data");
		        	console.dir(data);
		            if (data.RC !== 0) {
		            	console.log("cannot enable ldap because some of the configuration is incorrect or the configuration of the appliance this is running on is incorrect.");		            	
		            }
		            // TODO if you expect this to pass, then change the line below
		            doh.assertTrue(data.RC !== 0);
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.ldapdata,
				    sync: true,
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	d.errback(new Error("Unexpected Return: ", error));
				    }
				});
		        return d;
		    }
		}),
		/* Only run if ldap can be successfully enabled
		new LoginFixture ({name: "test_LDAP_createUser1",
			userdata: '{"id":"user1","description":"desc","groups":["group1"],"password":"123"}',
			expectedRC: "CWLNA5070",
		    runTest: function() {
		    	console.debug("Starting Test: test_LDAP_createUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_LDAP_createUser1 - return data", data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
					sync: true,
					load: function(data){
						d.errback(new Error("Unexpected Success: ", data));
					},
					error: function(error, ioargs) {
						var response = JSON.parse(ioargs.xhr.response);
						d.callback(response);
					}
				});
		        return d;
		    }
		}),*/
		new LoginFixture ({name: "test_disable_LDAP1",
			ldapdata: '{"URL":"ldap://www.moreldaps.com","BaseDN":"ldapBaseDN","BindDN":"ldapBindDN","BindPassword":"password","IgnoreCase":"true","UserSuffix":"myUserFilter={12}","GroupSuffix":"myGroupFilter={12}","UserIdMap":"testUsers=awesomeUsers","GroupIdMap":"testGroups=boringGroups","GroupMemberIdMap":"testMemberGroups=boringGroups","Timeout":10,"enableCache":"true","CacheTimeout":10,"GroupCacheTimeout":10,"enabled":"false"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_modifyLDAP1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_modifyLDAP1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.ldapdata,
				    sync: true,
				    load: function(data){
				    	d.callback(data);
				    },
				    error: function(error, ioargs) {
				    	d.errback(new Error("Unexpected Return: ", error));
				    }
				});
		        return d;
		    }
		}),
		new LoginFixture ({name: "test_deleteLDAP1",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_deleteLDAP1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_deleteLDAP1 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
		        	handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	sync: true,
					load: function(data){
						d.callback(data);
					},
					error: function(error, ioargs) {
					    d.errback(new Error("Unexpected Return: ", error));
					}
		        });
		        return d;
		    }
		}),
		new LoginFixture ({	name: "test_verifyDeleteLDAP1",			
			runTest: function() {
				console.debug("Starting Test: test_verifyDeleteLDAP1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_verifyDeleteLDAP1 - return data");
	        	    console.dir(data);
	        	    var deleted = true;
	        	    for (var i = 0; i < data.length; i++) {
	        	    	if (data[i].id == "MyLDAP") {
	        	    		deleted = false;
	        	    	}
	        	    }
	        	    return deleted;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/ldap/connections/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					sync: true,
					load: function(data){
						d.callback(data);
					},
					error: function(error, ioargs) {
				  	  	d.errback(new Error("Unexpected Return: ", error));
					}
				});
				return d;
			}
		})
	]);

});
