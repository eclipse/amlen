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

	doh.register("ism.tests.widgets.test_HA", [

	    // test GET of unconfigured appliance HA configuration
        new LoginFixture ({name: "test_getHA_start", 
	    	expected: {
	    		AllowSingleNIC: false,
	    		DiscoveryTimeout: 600,
	    		EnableHA: false,
	    		HeartbeatTimeout: 10,
	    		LocalDiscoveryNIC: "",
	    		LocalReplicationNIC: "",
	    		PreferredPrimary: false,
	    		RemoteDiscoveryNIC: "",
	    		StartupMode: "AutoDetect"
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
				
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
					load: function(data) {
						console.dir(data);
						for (var prop in this.expected) {
							console.log("checking " + prop);
							assertTrue(this.expected[prop] === data[prop]);
						}						
						d.callback(true);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						d.callback(false);
					})
				});

				return d;
			}
		}),
	    // test GET of unconfigured appliance HA role		
	    new LoginFixture ({name: "test_getHA_role_start", 
	    	expected: {
	    		ActiveNodes: "0",
	    		NewRole: "HADISABLED",
	    		OldRole: "HADISABLED",
	    		RC: 0,
	    		ReasonCode: "0",
	    		ReasonString: "",
	    		SyncNodes: "0"
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
				
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/role/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
					load: function(data) {
						console.dir(data);
						for (var prop in this.expected) {
							console.log("checking " + prop);
							assertTrue(this.expected[prop] === data[prop]);
						}						
						d.callback(true);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						d.callback(false);
					})
				});

				return d;
			}
		}),
	    // test invalid PUT of appliance HA configuration (missing group)
        new LoginFixture ({name: "test_getHA_missingGroup", 
	    	config: {
	    		DiscoveryTimeout: 600,
	    		EnableHA: true,
	    		HeartbeatTimeout: 10,
	    		LocalDiscoveryNIC: "192.168.56.32",
	    		LocalReplicationNIC: "192.168.56.33",
	    		PreferredPrimary: false,
	    		RemoteDiscoveryNIC: "192.168.56.13",
	    		StartupMode: "AutoDetect"
	    	},
	    	expectedRC: "CWLNA5012",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(rc) {
					doh.assertTrue(this.expectedRC === rc);
				}));
				
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					putData: JSON.stringify(this.config),					
					load: function(data) {
						d.callback(null);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);							
						d.callback(response.code);
					})
				});

				return d;
			}
		}),
	    // test invalid PUT of appliance HA configuration (invalid addresses)
        new LoginFixture ({name: "test_getHA_invalidAddress", 
	    	config: {
	    		Group: "TestGroup1",
	    		DiscoveryTimeout: 600,
	    		EnableHA: false,
	    		HeartbeatTimeout: 10,
	    		LocalDiscoveryNIC: "192..168.56.32",
	    		LocalReplicationNIC: "192.168.56.33",
	    		PreferredPrimary: false,
	    		RemoteDiscoveryNIC: "192.168.56.13",
	    		StartupMode: "AutoDetect"
	    	},
	    	expectedRC: "CWLNA5098",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(rc) {
					doh.assertTrue(this.expectedRC === rc);
				}));
				
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					putData: JSON.stringify(this.config),					
					load: function(data) {
						d.callback(null);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);							
						d.callback(response.code);
					})
				});

				return d;
			}
		}),
	    // test invalid PUT of appliance HA configuration (invalid timeout)
        new LoginFixture ({name: "test_getHA_invalidTimeout", 
	    	config: {
	    		Group: "TestGroup1",
	    		DiscoveryTimeout: 1,
	    		EnableHA: false,
	    		HeartbeatTimeout: 10,
	    		LocalDiscoveryNIC: "192.168.56.33",
	    		LocalReplicationNIC: "192.168.56.32",
	    		PreferredPrimary: false,
	    		RemoteDiscoveryNIC: "192.168.56.13",
	    		StartupMode: "AutoDetect"
	    	},
	    	expectedRC: "CWLNA5050",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(rc) {
					doh.assertTrue(this.expectedRC === rc);
				}));
				
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					putData: JSON.stringify(this.config),					
					load: function(data) {
						d.callback(null);
					},
					error: function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						console.dir(response);
						d.callback(response.code);
					}
				});

				return d;
			}
		}),				
	    // test valid PUT of appliance HA configuration
        new LoginFixture ({name: "test_setHA_good", 
	    	config: {
	    		Group: "TestGroup1",
	    		DiscoveryTimeout: 700,
	    		EnableHA: true,
	    		HeartbeatTimeout: 11,
	    		LocalDiscoveryNIC: "192.168.56.33",
	    		LocalReplicationNIC: "192.168.56.32",
	    		PreferredPrimary: true,
	    		RemoteDiscoveryNIC: "192.168.56.13",
	    		StartupMode: "StandAlone"
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
				
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					putData: JSON.stringify(this.config),					
					load: function(data) {
						console.dir(data);
						for (var prop in this.expected) {
							console.log("checking " + prop);
							assertTrue(this.config[prop] === data[prop]);
						}						
						d.callback(true);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						d.callback(false);
					})
				});

				return d;
			}
		}),				
        new LoginFixture ({name: "test_getHA_good", 
	    	expected: {
	    		Group: "TestGroup1",
	    		DiscoveryTimeout: 700,
	    		EnableHA: true,
	    		HeartbeatTimeout: 11,
	    		LocalDiscoveryNIC: "192.168.56.33",
	    		LocalReplicationNIC: "192.168.56.32",
	    		PreferredPrimary: true,
	    		RemoteDiscoveryNIC: "192.168.56.13",
	    		StartupMode: "StandAlone"
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
				
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
					load: function(data) {
						console.dir(data);
						for (var prop in this.expected) {
							console.log("checking " + prop);
							assertTrue(this.expected[prop] === data[prop]);
						}						
						d.callback(true);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						d.callback(false);
					})
				});

				return d;
			}
		}),
		
		
	    // test valid PUT of appliance HA configuration
        new LoginFixture ({name: "test_getHA_unconfig", 
	    	config: {
	    		Group: "TestGroup1",
	    		DiscoveryTimeout: 600,
	    		EnableHA: false,
	    		HeartbeatTimeout: 10,
	    		LocalDiscoveryNIC: "",
	    		LocalReplicationNIC: "",
	    		PreferredPrimary: false,
	    		RemoteDiscoveryNIC: "",
	    		StartupMode: "AutoDetect"
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));
				
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/appliance/highavailability/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					putData: JSON.stringify(this.config),					
					load: function(data) {
						console.dir(data);
						for (var prop in this.expected) {
							console.log("checking " + prop);
							assertTrue(this.config[prop] === data[prop]);
						}						
						d.callback(true);
					},
					error: lang.hitch(this, function(error, ioArgs) {
						d.callback(false);
					})
				});

				return d;
			}
		})		
	]);
});
