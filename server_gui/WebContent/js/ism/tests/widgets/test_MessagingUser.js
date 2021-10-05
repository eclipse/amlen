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
	
	doh.register("ism.tests.widgets.test_MessagingUser", [
		//
		// Messaging Groups
		//

		new LoginFixture ({name: "test_messaging_noGroups",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_noGroups");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_noGroups - return data");
	        	    console.dir(data);
	        	    doh.assertTrue(data.data.length == 0);
	        	    
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_createGroup1", groupdata: '{"id":"group1","description":"desc","groups":[]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createGroup1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createGroup1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.groupdata,
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
		new LoginFixture ({name: "test_messaging_verifyCreateGroup1",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyCreateGroup1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyCreateGroup1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group1") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "desc");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,[]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_createGroup2", groupdata: '{"id":"group2","description":"desc","groups":["group1"]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createGroup2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createGroup2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					postData: this.groupdata,
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
		new LoginFixture ({name: "test_messaging_verifyCreateGroup2",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyCreateGroup2");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyCreateGroup2 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group2") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "desc");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["group1"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_modifyGroup2", userdata: '{"description":"modified","groups":[]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_modifyGroup2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_modifyGroup2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group2",
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
		new LoginFixture ({name: "test_messaging_verifyModifyGroup2",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyModifyGroup2");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyModifyGroup2 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group2") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "modified");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,[]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		//
		// Messaging Groups - Other Tests
		//
		new LoginFixture ({name: "test_messaging_createGroup3", 
			userdata: '{"id":"group3","description":"desc","groups":[]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createGroup3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createGroup3 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_createUser1InGroup3", 
			userdata: '{"id":"user1","description":"desc","groups":["group3"],"password":"123"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createUser1InGroup3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createUser1InGroup3 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_renameGroup3",	userdata: '{"id":"group4"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_renameGroup3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_renameGroup3 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group3",
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
		new LoginFixture ({name: "test_messaging_verifyRenameGroup3",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyRenameGroup3");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyRenameGroup3 - return data");
	        	    console.dir(data);
	        	    var noGroup3 = true;
	        	    var foundGroup4 = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group4") {
	        	    		foundGroup4 = true;
	        	    	}
	        	    	if (data.data[i].id == "group3") {
	        	    		noGroup3 = false;
	        	    	}
	        	    }
	        	    return noGroup3 && foundGroup4;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_verifyUser1InGroup4",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyUser1InGroup4");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyUser1InGroup4 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		found = true;
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["group4"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_deleteUser1",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteUser1 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user1",
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
		new LoginFixture ({name: "test_messaging_nonExistantGroupGroup4", 
			userdata: '{"groups":["MyGroup"]}', 
			expectedRC: "CWLNA5056",
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_nonExistantGroupGroup4");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_nonExistantGroupGroup4 - return data");
		        	console.dir(data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group4",
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
		new LoginFixture ({name: "test_messaging_verifyNonExistantGroupGroup4",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyNonExistantGroupGroup4");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyNonExistantGroupGroup4 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group4") {
	        	    		found = true;
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,[]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_createAnotherGroup4",
		    userdata: '{"id":"group4","description":"desc","groups":["group1"]}', 
		    expectedRC: "CWLNA5030",
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createAnotherGroup4");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createAnotherGroup4 - return data", data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_deleteGroup4",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteGroup4");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteGroup4 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group4",
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
		//
		// Messaging Users
		//

		new LoginFixture ({	name: "test_messaging_noUsers",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_noUsers");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_noUsers - return data");
	        	    console.dir(data);
	        	    doh.assertTrue(data.data.length == 0);
	        	    
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_createUser1",	
			userdata: '{"id":"user1","description":"desc","groups":["group1"],"password":"123"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createUser1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_verifyCreateUser1",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyCreateUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyCreateUser1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "desc");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["group1"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_modifyUser1",
			userdata: '{"description":"modified","groups":["group1", "group2"]}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_modifyUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_modifyUser1 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user1",
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
		new LoginFixture ({name: "test_messaging_verifyModifyUser1",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyModifyUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyModifyUser1 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user1") {
	        	    		found = true;
	        	    		doh.assertTrue(data.data[i].description == "modified");
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["group1", "group2"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_deleteUser1",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteUser1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteUser1 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user1",
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
		new LoginFixture ({name: "test_messaging_verifyDeleteUser1",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyDeleteUser1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyDeleteUser1 - return data");
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
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		//
		// Messaging Users - Other Tests
		//
		new LoginFixture ({name: "test_messaging_createUser2", 
			userdata: '{"id":"user2","description":"desc","groups":["group1"],"password":"123"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createUser2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createUser2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({ name: "test_messaging_renameUser2", userdata: '{"id":"user3"}',
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_renameUser2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_renameUser2 - return data");
		        	console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user2",
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
		new LoginFixture ({	name: "test_messaging_verifyRenameUser2",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyRenameUser2");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyRenameUser2 - return data");
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
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({ name: "test_messaging_nonExistantGroupUser3",
			userdata: '{"groups":["group1", "MyGroup"]}',
			expectedRC: "CWLNA5056",
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_nonExistantGroupUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_nonExistantGroupUser3 - return data");
		        	console.dir(data);
		            doh.assertTrue(data.code == this.expectedRC);
		            
		        }));
		          
		        var deferred = dojo.xhrPut({
				    url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user3",
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
		new LoginFixture ({	name: "test_messaging_verifyNonExistantGroupUser3",
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyNonExistantGroupUser3");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyNonExistantGroupUser3 - return data");
	        	    console.dir(data);
	        	    var found = false;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "user3") {
	        	    		found = true;
	        	    		doh.assertTrue(compareArrays(data.data[i].groups,["group1"]));
	        	    	}
	        	    }
	        	    return found;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users",
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
		new LoginFixture ({name: "test_messaging_createAnotherUser3",
			userdata: '{"id":"user3","description":"desc","groups":["Users"],"password":"123"}', 
			expectedRC: "CWLNA5030",
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_createAnotherUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		        	console.debug("test_messaging_createAnotherUser3 - return data", data);
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
		}),
		new LoginFixture ({name: "test_messaging_deleteUser3",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteUser3");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteUser3 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/users/user3",
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
		//
		// Clean up Messaging Groups
		//

		new LoginFixture ({name: "test_messaging_deleteGroup1",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteGroup1");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteGroup1 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group1",
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
		new LoginFixture ({name: "test_messaging_verifyDeleteGroup1",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyDeleteGroup1");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyDeleteGroup1 - return data");
	        	    console.dir(data);
	        	    var deleted = true;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group1") {
	        	    		deleted = false;
	        	    	}
	        	    }
	        	    return deleted;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
		new LoginFixture ({name: "test_messaging_deleteGroup2",		    
		    runTest: function() {
		    	console.debug("Starting Test: test_messaging_deleteGroup2");
				console.debug("--- ---");
		        var d = new doh.Deferred();
		          
		        d.addCallback(lang.hitch(this,function(data) {
		            console.debug("test_messaging_deleteGroup2 - return data");
		            console.dir(data);
		            
		        }));
		          
		        var deferred = dojo.xhrDelete({
		        	url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups/group2",
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
		new LoginFixture ({name: "test_messaging_verifyDeleteGroup2",			
			runTest: function() {
				console.debug("Starting Test: test_messaging_verifyDeleteGroup2");
				console.debug("--- ---");
				var d = new doh.Deferred();
				
				d.addCallback(lang.hitch(this,function(data) {
	        	    console.debug("test_messaging_verifyDeleteGroup1 - return data");
	        	    console.dir(data);
	        	    var deleted = true;
	        	    for (var i = 0; i < data.data.length; i++) {
	        	    	if (data.data[i].id == "group2") {
	        	    		deleted = false;
	        	    	}
	        	    }
	        	    return deleted;
				}));
	          
				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/userregistry/messaging/groups",
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
