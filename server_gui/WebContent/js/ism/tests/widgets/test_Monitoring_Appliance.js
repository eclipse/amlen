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

	doh.register("ism.tests.widgets.test_Monitoring_Appliance", [

          new LoginFixture ({name: "test_Store",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Store"}',
  					load: lang.hitch(this, function(data) {
  						doh.assertTrue(data.MemoryUsedPercent && data.MemoryUsedPercent !== "");
  						doh.assertTrue(data.DiskUsedPercent && data.DiskUsedPercent !== "");
  						doh.assertTrue(data.DiskFreeBytes && data.DiskFreeBytes !== "");
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}),
        new LoginFixture ({name: "test_Memory",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Memory"}',
  					load: lang.hitch(this, function(data) {
  						doh.assertTrue(data.CurrentActivity && data.CurrentActivity !== "");
  						doh.assertTrue(data.Destinations && data.Destinations !== "");
  						doh.assertTrue(data.MemoryFreeBytes && data.MemoryFreeBytes !== "");
  						doh.assertTrue(data.MemoryFreePercent && data.MemoryFreePercent !== "");
  						doh.assertTrue(data.MemoryTotalBytes && data.MemoryTotalBytes !== "");
  						doh.assertTrue(data.MessagePayloads && data.MessagePayloads !== "");
  						doh.assertTrue(data.PublishSubscribe && data.PublishSubscribe !== "");
  						doh.assertTrue(data.ServerResidentSetBytes && data.ServerResidentSetBytes !== "");
  						doh.assertTrue(data.ServerVirtualMemoryBytes && data.ServerVirtualMemoryBytes !== "");
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}),
        new LoginFixture ({name: "test_Memory_History_MemoryFreePercent",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Memory", "SubType":"History","StatType":"MemoryFreePercent","Duration":"86400"}',
  					load: lang.hitch(this, function(data) {
		    			console.log("Data: " + data);
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}),  
        new LoginFixture ({name: "test_Memory_History_multiples",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Memory", "SubType":"History","StatType":"MemoryTotalBytes,MemoryFreeBytes,MessagePayloads,PublishSubscribe,Destinations,CurrentActivity,ClientStates","Duration":"60"}',
  					load: lang.hitch(this, function(data) {
		    			console.log("Data: " + data);
		    			doh.assertTrue(data.MessagePayloads !== undefined);
		    			doh.assertTrue(data.PublishSubscribe !== undefined);
		    			doh.assertTrue(data.Destinations !== undefined);
		    			doh.assertTrue(data.CurrentActivity !== undefined);		    			
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}),  		  		
        new LoginFixture ({name: "test_Store_History_StoreDiskUsagePct",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
  				
  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Store", "SubType":"History","StatType":"DiskUsedPercent","Duration":"86400"}',
  					load: lang.hitch(this, function(data) {
		    			console.log("Data: " + data);
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		}), 
        new LoginFixture ({name: "test_Store_History_multiples",
  			runTest: function() {
  				var d = new doh.Deferred();
 				d.addCallback(lang.hitch(this, function(pass) {
 					doh.assertTrue(pass);
  				}));
                var metrics = "Pool1TotalBytes,Pool1UsedPercent,Pool1RecordsLimitBytes,Pool1RecordsUsedBytes,ClientStatesBytes,QueuesBytes,TopicsBytes," +
                    "SubscriptionsBytes,TransactionsBytes,MQConnectivityBytes,Pool2TotalBytes,Pool2UsedPercent,IncomingMessageAcksBytes";

  				dojo.xhrPost({
  					url: Utils.getBaseUri() + "rest/monitoring",
  					handleAs: "json",
  					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
  					postData: '{"Action":"Store", "SubType":"History","StatType":"'+metrics+'","Duration":"60"}',
  					load: lang.hitch(this, function(data) {
		    			console.log("Data: " + data);
		    			var stats = metrics.split(",");
		    			for (var i in stats) {
		    			    doh.assertTrue(data[stats[i]] !== undefined);
		    			}	    			
  						d.callback(true);
  					}),
  					error: function(error) {
  						console.dir(error);
  						d.callback(false);
  					}
  				});

  				return d;
  			}
  		})  		
    ]);
});
