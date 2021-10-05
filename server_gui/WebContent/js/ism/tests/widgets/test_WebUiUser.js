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

	var compareArrays = function(array1, array2) {
		console.debug("--- Comparing Arrays ---");
		console.dir(array1);
		console.dir(array2);
		console.debug("--- ---");
		if (array1.length != array2.length) {
			return false;
		}
		
		for (var i = 0; i < array1.length; i++) {
			if (array1[i] != array2[i]) {
				return false;
			}
		}
		
		return true;
	};
	
	doh.register("ism.tests.widgets.test_WebUiUser", [
		//
		// WebUI Users [Golden Path]
		//
		new LoginFixture ({name: "test_webui_createUser1", 
			userdata:'{"id":"user1","description":"desc","groups":["Users"],"password":"123"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_createUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_createUser1 - return data");
		        	console.dir(data);
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
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
		new LoginFixture ({name: "test_webui_verifyCreateUser1",			
			runTest: function() {
				console.debug("Starting Test: test_webui_verifyCreateUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_webui_verifyCreateUser1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "desc");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["Users"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		new LoginFixture ({name: "test_webui_modifyUser1", userdata: '{"description":"modified","groups":["Users", "SystemAdministrators"]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_modifyUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_modifyUser1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/user1",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
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
		new LoginFixture ({name: "test_webui_verifyModifyUser1",			
			runTest: function() {
				console.debug("Starting Test: test_webui_verifyModifyUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_webui_verifyModifyUser1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "modified");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["SystemAdministrators", "Users"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		new LoginFixture ({name: "test_webui_deleteUser1",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_deleteUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_webui_deleteUser1 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/user1",
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
		new LoginFixture ({name: "test_webui_verifyDeleteUser1",			
			runTest: function() {
				console.debug("Starting Test: test_webui_verifyDeleteUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_webui_verifyDeleteUser1 - return data");
	        	    console.dir(data);
	        	    var deleted = true;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		deleted = false;
	        	    	}
	        	    }
	        	    return deleted;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		new LoginFixture ({name: "test_webui_deleteAdminUser", expectedRC: "CWLNA5033",
			runTest: function() {
				console.debug("Starting Test: test_webui_deleteAdminUser");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_webui_deleteAdminUser - return data", data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/admin",
		        	handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
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
		}),
		//
		// WebUI Users - Other Tests
		//
		new LoginFixture ({name: "test_webui_createUser2",
			userdata: '{"id":"user2","description":"desc","groups":["Users"],"password":"123"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_createUser2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_createUser2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
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
		new LoginFixture ({name: "test_webui_renameUser2", userdata: '{"id":"user3"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_renameUser2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_renameUser2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/user2",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
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
		new LoginFixture ({name: "test_webui_verifyRenameUser2",			
			runTest: function() {
				console.debug("Starting Test: test_webui_verifyRenameUser2");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_webui_verifyRenameUser2 - return data");
	        	    console.dir(data);
	        	    var noUser2 = true;
	        	    var foundUser3 = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user3") {
	        	    		foundUser3 = true;
	        	    	}
	        	    	if (data.data[i].id == "user2") {
	        	    		noUser2 = false;
	        	    	}
	        	    }
	        	    return noUser2 && foundUser3;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		new LoginFixture ({name: "test_webui_nonExistantGroupUser3", 
			userdata: '{"groups":["Users", "MyGroup"]}',
			expectedRC: "CWLNA5056",
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_nonExistantGroupUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_nonExistantGroupUser3 - return data");
		        	console.dir(data);
		        	console.debug("Data: " + this.userdata);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/user3",
				    handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.userdata,
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
		new LoginFixture ({name: "test_webui_verifyNonExistantGroupUser3",			
			runTest: function() {
				console.debug("Starting Test: test_webui_verifyNonExistantGroupUser3");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_webui_verifyNonExistantGroupUser3 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user3") {
	        	    		found = true;
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["Users"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		new LoginFixture ({name: "test_webui_createAnotherUser3",
		    userdata: '{"id":"user3","description":"desc","groups":["Users"],"password":"123"}',
		    expectedRC: "CWLNA5030",
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_createAnotherUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_webui_createAnotherUser3 - return data", data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/webui/users",
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
		}),
		new LoginFixture ({name: "test_webui_deleteUser3",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_webui_deleteUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_webui_deleteUser3 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/webui/users/user3",
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
