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

	doh.register("ism.tests.widgets.test_Ethernet", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getEthernet_eth0",
		    timeout: 10000,
            expectedName: "eth0", 
            expectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expectedRC); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_not_defined", expectedName: "eth9", notExpectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC != this.notExpectedRC);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_malformed", expectedName: "eth99", notExpectedRC: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC != this.notExpectedRC);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.expectedName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_getEthernet_all", value: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		              doh.assertTrue(data.instanceValueMap.length>this.value);
		          }));
		          dojo.xhrGet({
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_postEthernet_eth1",
		    timeout: 10000,
		    instanceName: "eth1",
		    valueSet: {
		    	"address":"192.168.56.10/24,2607:f0d0:1002:51::12/24",
		    	"updatedAddress": "192.168.56.10/24",
		    	"updatedV6Address": "2607:f0d0:1002:51::12/24",
		    	"ipv4-gateway":"192.168.56.1",
		    	"ipv6-gateway":"2607:f0d0:1002:51::100"},
		    expected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		          }));
		          console.log(JSON.stringify(this.valueSet));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName,
					    handleAs: "json",
					    putData: JSON.stringify(this.valueSet),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    },
		    tearDown: function() {
		    	 //return back to original value
		    	 var d = new doh.Deferred();
		    	 var valueEmpty = {"address":"","ipv4-gateway":"","ipv6-gateway":""};

		    	 d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		         }));
		         console.log(JSON.stringify(valueEmpty));
		         dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		        	 	url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName,
		        	 	handleAs: "json",
		        	 	putData: JSON.stringify(valueEmpty),
		                load: function(data) {
		                     d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name: "test_postEthernet_testData_ping_success",
		    timeout: 20000,
	    	instanceName: "ha0",
	    	expected: 0,
	    	value: {"address":"192.168.56.12/24","AdminState":"Enabled","use-arp":"true"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expected);
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		}),  
		new LoginFixture ({name: "test_postEthernet_testData_ping_failed",
		    timeout: 20000,
	    	instanceName: "ha0",
	    	expectedRC: "CWLNA5029",
	    	value: {"address":"192.168.56.6/24", "updatedAddress":"192.168.56.6/24", "ipv4-gateway":"192.168.56.100"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
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
		new LoginFixture ({name: "test_postEthernet_testData_ping_failed_ipv6",
		    timeout: 20000,
	    	instanceName: "ha0",
	    	expectedRC: "CWLNA5029",
	    	value: {"address":"2607:f0d0:1002:51::6/24", "updatedV6Address":"2607:f0d0:1002:51::6/24","ipv6-gateway":"2607:f0d0:1002:51::100"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		   		  }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
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
		new LoginFixture ({name: "test_postEthernet_testData_ping_failed_v4v6",
		    timeout: 20000,
	    	instanceName: "ha0",
	    	expectedRC: "CWLNA5029",
	    	value: {
	    			"address":"2607:f0d0:1002:51::6/24,192.168.56.100/24",
	    			"updatedAddress":"192.168.56.100/24",
	    			"updatedV6Address":"2607:f0d0:1002:51::6/24",
	    			"ipv6-gateway":"2607:f0d0:1002:51::100"
		    },
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        	    doh.assertTrue(data.code == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
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
		new LoginFixture ({name: "test_postEthernet_testData_incorrect_IP",
		    timeout: 10000,
	    	instanceName: "ha0",
	    	expectedRC: "CWLNA5006",
	    	value: {"name":"eth1","address":"1.2.3.4","updatedAddress":"1.2.3.4","ipv4-gateway":"192.168.56.10"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          		d.addCallback(lang.hitch(this,function(data) {
		          	  	doh.assertTrue(data.code == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
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
		new LoginFixture ({name: "test_postEthernet_testData_unidentified_instance",
		    timeout: 10000,
	    	instanceName: "eth9",
	    	expectedRC: "CWLNA5004",
	    	value: {"address":"192.168.56.12/24","ipv4-gateway":"192.168.56.10","AdminState":"Enabled",
	    			"use-arp":"true"
		    },
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		        		doh.assertTrue(data.code == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.value));
		          dojo.xhrPost({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/"+this.instanceName+"/actions/testConnection",
					    handleAs: "json",
					    putData: JSON.stringify(this.value),
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
		new LoginFixture ({name: "test_pingHost",
		    timeout: 10000,
		    expectedRC: 0,
		    address: {"address":"192.168.56.1"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.RC == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.address));
		          dojo.xhrPut({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/actions/test",
					    handleAs: "json",
					    putData: JSON.stringify(this.address),
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
		new LoginFixture ({name: "test_pingHost_failed",
		    timeout: 30000,
		   	expectedRC: "CWLNA5029",
		   	address: {"address":"12.18.56.1"},
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(response) {
		                doh.assertTrue(response.code == this.expectedRC);
		          }));
		          console.log(JSON.stringify(this.address));
		          dojo.xhrPut({
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					    url: Utils.getBaseUri() + "rest/config/appliance/ethernet-interfaces/actions/test",
					    handleAs: "json",
					    putData: JSON.stringify(this.address),
		                load: function(data) {
		                	// it's an error if we get here
							d.callback(data);
		                },
		                error: function(error, args) {
							var response = JSON.parse(error.responseText);
							d.callback(response);
						}
				 }); 
		         return d;
		    }
		})
	]);
});
