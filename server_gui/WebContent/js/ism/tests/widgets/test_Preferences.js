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
define(["ism/tests/Utils", "dojo/_base/lang",	'ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {

	doh.register("ism.tests.widgets.test_Preferences", [
        new LoginFixture ({name: "test_getNone",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					doh.assertTrue(true);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/webui/preferences",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						console.error(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_setGlobalOnly",
			preferences: {
				"global":{
					"ismUserCompletedFirstSteps": true,
					"ismTaskApplianceSettings": "closed"	
				}
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/admin",
					postData: JSON.stringify(this.preferences),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						console.dir(error);
					}
				});

				return d;
			}
		}),
		new LoginFixture ({	name: "test_setUserOnly",
			preferences: {
				"user": {
					"ismHomeHideTasks": false	
				}
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");
					doh.assertTrue(data.user.ismHomeHideTasks == false);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/admin",
					postData: JSON.stringify(this.preferences),
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
			}
		}),
		new LoginFixture ({	name: "test_getUserWithNoSettings", userId: "bogus",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");
					doh.assertTrue(!data.user);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/bogus",
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
			}
		}),
		new LoginFixture ({name: "test_setUserAndGlobal", userId: "john",
		  preferences: {
				"global": {
					"somethingWeAllCareAbout": "money",
					"somethingMoreComplicated": {
						"greetings": ["hello", "goodbye"],								
					}
				},
				"user": {
					"anySettingIsOK": true,
					"somethingMoreComplicated": {
						"one": 1,
						"two": 2,
						"true": true
					}
				}
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");					
					doh.assertTrue(data.global.somethingWeAllCareAbout == "money");
					doh.assertTrue(data.global.somethingMoreComplicated.greetings[0] == "hello");
					doh.assertTrue(data.user.anySettingIsOK == true);
					doh.assertTrue(data.user.somethingMoreComplicated.one == 1);
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/john",
					postData: JSON.stringify(this.preferences),
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
			}
		}),
		new LoginFixture ({name: "test_getUserJohn", userId: "john",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");					
					doh.assertTrue(data.global.somethingWeAllCareAbout == "money");
					doh.assertTrue(data.global.somethingMoreComplicated.greetings[0] == "hello");
					doh.assertTrue(data.user.anySettingIsOK == true);
					doh.assertTrue(data.user.somethingMoreComplicated.one == 1);
				}));

				var deferred = dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/john",
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
			}
		}),
		new LoginFixture ({	name: "test_updateJohn", userId: "john",
			preferences: {
				"global": {
					"somethingWeAllCareAbout": "health",
				},
				"user": {
					"anySettingIsOK": true,
					"somethingMoreComplicated": {
						"one": 1,
						"two": 2,
						"true": true,
						"no": "no"
					}
				}
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");					
					doh.assertTrue(data.global.somethingWeAllCareAbout == "health");
					doh.assertTrue(data.global.somethingMoreComplicated.greetings[0] == "hello");
					doh.assertTrue(data.user.anySettingIsOK == true);
					doh.assertTrue(data.user.somethingMoreComplicated.one == 1);
					doh.assertTrue(data.user.somethingMoreComplicated.no == "no");
				}));

				var deferred = dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/webui/preferences/john",
					postData: JSON.stringify(this.preferences),
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
			}
		}),
		/*
		 * These are widget tests, not REST API tests. For the widget to work, we assume _PageController
		 * has been instantiated and is overriding xhr*.  We want these tests to work like a consumer
		 * would use them...
		 * 
		 * new LoginFixture ({name: "test_subscribe",
			timeout: 20000,
			runTest: function() {

				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(data) {
					console.dir(data);
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");					
					doh.assertTrue(data.global.somethingWeAllCareAbout == "health");
					doh.assertTrue(data.global.somethingMoreComplicated.greetings[0] == "hello");
					doh.assertTrue(data.user.anySettingIsOK == true);
					doh.assertTrue(data.user.somethingMoreComplicated.one == 1);
					doh.assertTrue(data.user.somethingMoreComplicated.no == "no");
				}));

				connect.subscribe(Resources.preferences.uploadedTopic, function(message) {d.callback(message)});
				var prefs = new Preferences({userId: "john"});  // this should publish!

				return d;
			}
		}),
		new LoginFixture ({	name: "test_persist",
			timeout: 20000,		
			prefs: new Preferences({userId: "john", blocking: true}),
			runTest: function() {

				var d = new doh.Deferred();								

				d.addCallback(lang.hitch(this, function(data) {							
					console.dir(data);
					
					doh.assertTrue(data.global.ismUserCompletedFirstSteps == true);
					doh.assertTrue(data.global.ismTaskApplianceSettings == "closed");					
					doh.assertTrue(data.global.somethingWeAllCareAbout == "health");
					doh.assertTrue(data.global.somethingMoreComplicated.greetings[0] == "hello");
					doh.assertTrue(data.user.anySettingIsOK == true);
					doh.assertTrue(data.user.somethingMoreComplicated.no == "maybe");
					doh.assertTrue(data.user.aNewSetting == "new");
					
				}));
				
				connect.subscribe(Resources.preferences.updatedTopic, function(message) {d.callback(message)});
				var data = this.prefs.getPreferences();
				if (data == null) {
					data = {user: {aNewSetting: "new", somethingMoreComplicated: {no: "maybe"}}};
				} else {
					data.user.aNewSetting = "new";
					data.user.somethingMoreComplicated.no = "maybe";
				}
				this.prefs.persistPreferences(data);
				
				return d;
			}
		})*/			
	]);
});
