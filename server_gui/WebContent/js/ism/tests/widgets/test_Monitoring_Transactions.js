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

	doh.register("ism.tests.widgets.test_Monitoring_Transactions", [

  		// make a valid request when there are no transactions
        new LoginFixture ({name: "test_getTransactionData_none", 
     	   stat: {
     		   Action: "Transaction",
     		   StatType: "LastStateChangeTime"
     	   },
     	   expectedRC: "113", // no data
 		   runTest: function() {
 				var d = new doh.Deferred();

 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.expectedRC === result.RC);
  				}));

 				dojo.xhrPost({
 					url: Utils.getBaseUri() + "rest/monitoring",
 					postData: JSON.stringify(this.stat),
 					handleAs: "json",
 					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
 					load: function(data) {
 						d.callback(data);
 					},
 					error: function(error) {
 						console.error(error);
 						doh.assertTrue(false);
 					}
 				});

 				return d;
 		   }
 		}),
 	
		// attempt rollback on bogus transaction 
        new LoginFixture ({name: "test_transactions_rollback_bogus",
			runTest: function() {
				var d = new doh.Deferred();
		    	expectedRC: "113", // reference not found...
		    	
 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.expectedRC === result.RC);
  				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/transactions/0/rollback?bogusid",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.callback(false);
					}
				});

				return d;
			}
		}),
        
		// attempt forget on bogus transaction 
        new LoginFixture ({name: "test_transactions_forget_bogus",
			runTest: function() {
				var d = new doh.Deferred();
		    	expectedRC: "113", // reference not found...
		    	
 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.expectedRC === result.RC);
  				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/transactions/0/forget?bogusid",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.callback(false);
					}
				});

				return d;
			}
		}),
        
		// attempt commit on bogus transaction 
        new LoginFixture ({name: "test_transactions_commit_bogus",
			runTest: function() {
				var d = new doh.Deferred();
		    	expectedRC: "113", // reference not found...
		    	
 				d.addCallback(lang.hitch(this, function(result) {
 					doh.assertTrue(this.expectedRC === result.RC);
  				}));

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/transactions/0/commit?bogusid",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						console.error(error);
						d.callback(false);
					}
				});

				return d;
			}
		})	
	]);
});
