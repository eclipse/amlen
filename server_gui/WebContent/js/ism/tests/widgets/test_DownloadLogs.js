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

	doh.register("ism.tests.widgets.test_DownloadLogs", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getLogs",
			timeout: 60000,
			expectedLogs: ["default-log", "default-trace", "nslcd-log", "imaserver-connection.log", "imaserver-security.log",
			               "imaserver-admin.log", "imaserver-mqconnectivity.log", "imaserver-default.log", "webui-messages.log"],
			expectedProperties: ["name", "lastModified", "size", "uri"],
			runTest: function() {
		          var d = new doh.Deferred();		          
		          d.addCallback(lang.hitch(this,function(data) {
	        		 for (var i in data) {
	        			 var logRecord = data[i];
	        			 // verify each record has the expected properties
	        			 for (var prop in this.expectedProperties) {
	        				 var property = this.expectedProperties[prop];
	        				 console.log("checking " + i +", name=" + logRecord.name + " for property " + property);
	        				 doh.assertTrue(logRecord[property] !== undefined && logRecord[property] !== "");
	        			 }
	        			 // if it's one of the ones we expect, remove it from the list
	        			 var index = this.expectedLogs.indexOf(logRecord.name);
	        			 if (index > -1 && this.expectedLogs.length > index) {
		        			 console.log("removing " + logRecord.name + " from the list of expected logs");
		        			 this.expectedLogs.splice(index, 1);
	        			 } else {
	        				 console.log(logRecord.name + " isn't in the list");
	        			 }
	        			 
	        			 // see if we can get the log
	        			 console.log("trying to get file: " + logRecord.name);
		   		         dojo.xhrGet({
							    url: logRecord.uri,
							    handleAs: "text",
								headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},							    
				                load: function(data) {
				                	console.log("retreived file");
				                },
				                error: function(error) {
				                	console.dir(error);
				                	doh.assertTrue(false);
				                }
						 });         			 
	        		 }
	        		 // make sure all expected logs where found
	        		 console.log("expected logs: " + this.expectedLogs);
	        		 doh.assertTrue(this.expectedLogs.length === 0);
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/logs",
					    handleAs: "json",
		        	 	headers: {"Content-Type": "application/json", "X-Session-Verify": dojo.cookie("xsrfToken")},	  	  
		                load: function(data) {
		                      d.callback(data);
		                },
		                error: function(error) {
		                	console.dir(error);
		                	d.callback({});
		                }
				 }); 
		         return d;
			}
		})
	]);
});
