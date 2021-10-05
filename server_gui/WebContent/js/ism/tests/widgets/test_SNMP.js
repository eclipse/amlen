/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
    
    var compareObjects = function(expected, actual) {
        if (!expected || !actual) {
            return "expected or actual are not defined";
        } 
        for (var prop in expected) {
            if (expected[prop] !== actual[prop]) {
                return "Mismatch for prop="+prop+"; expected: " + expected[prop] + ", actual: " + actual[prop];
            }
        }
        return "OK";
    };


	doh.register("ism.tests.widgets.test_SNMP", [
		//
		// Tests defined with the DOH test fixture
		//

		/****************************************************************************************************
         * 
         * SNMP Enabled State
         * 
         ****************************************************************************************************/
		
		new LoginFixture ({name: "test_setSNMPEnabled_true", runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {			        	  
						  console.log("data.SNMPEnabled (expecting true) = " + data.SNMPEnabled);
						  doh.assertTrue(data.SNMPEnabled === true || data.SNMPEnabled.toLowerCase() === "true" );
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/snmp/enabled",
						    handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
			                load: function(data) {
			                      d2.callback(data);
			                }
					 }); 
			         return d2;
	        		  
		          }));
		          dojo.xhrPut({
		        	 	headers: { 
		        	 		"Content-Type": "application/json",
		        	 		"X-Session-Verify": dojo.cookie("xsrfToken")
		        	 	},	  	  
					    url:Utils.getBaseUri() + "rest/config/snmp/enabled",
					    handleAs: "json",
					    putData: JSON.stringify({"SNMPEnabled":true}),
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		          return d;
		}}),
        new LoginFixture ({name: "test_setSNMPEnabled_false", runTest: function() {
            var d = new doh.Deferred();
            d.addCallback(lang.hitch(this,function(data) {
                var d2 = new doh.Deferred();
                d2.addCallback(lang.hitch(this,function(data) {                         
                    console.log("data.SNMPEnabled (expecting false) = " + data.SNMPEnabled);
                    doh.assertTrue(data.SNMPEnabled === false || data.SNMPEnabled.toLowerCase() === "false" );
                }));
                dojo.xhrGet({
                      url:Utils.getBaseUri() + "rest/config/snmp/enabled",
                      handleAs: "json",
                      headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},                           
                      load: function(data) {
                            d2.callback(data);
                      }
               }); 
               return d2;
                
            }));
            dojo.xhrPut({
                  headers: { 
                      "Content-Type": "application/json",
                      "X-Session-Verify": dojo.cookie("xsrfToken")
                  },        
                  url:Utils.getBaseUri() + "rest/config/snmp/enabled",
                  handleAs: "json",
                  putData: JSON.stringify({"SNMPEnabled":false}),
                  load: function(data) {
                        d.callback(data);
                  }
           }); 
            return d;
        }}),

        /****************************************************************************************************
         * 
         * SNMPCommunities and Trap Subscribers
         * 
         ****************************************************************************************************/
        
        new LoginFixture ({name: "test_addCommunity1",
            resource: {
                Name: "UnitTest1",
                HostRestriction: ""
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/communities",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var community = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Name === resource.Name) {
                                    community = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(community !== undefined);
                            doh.assertEqual("OK", compareObjects(resource,community));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting community: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrPost({
                    url: Utils.getBaseUri() + "rest/config/snmp/communities",
                    postData: JSON.stringify(this.resource),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error creating community: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        new LoginFixture ({name: "test_addCommunity2",
            resource: {
                Name: "UnitTest2",
                HostRestriction: "192.168.56.1,fe80::6437:9686:c3ce:216a,192.168.56.20/24"
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/communities",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var community = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Name === resource.Name) {
                                    community = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(community !== undefined);
                            doh.assertEqual("OK", compareObjects(resource,community));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting community: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrPost({
                    url: Utils.getBaseUri() + "rest/config/snmp/communities",
                    postData: JSON.stringify(this.resource),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error creating community: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),

        new LoginFixture ({name: "test_addTrapSubscriber1",
            resource: {
                Host: "192.168.56.100",
                ClientPortNumber: 162,
                Community: "UnitTest1"
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var trapSubscriber = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Host === resource.Host) {
                                    trapSubscriber = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(trapSubscriber !== undefined);
                            doh.assertEqual("OK", compareObjects(resource,trapSubscriber));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrPost({
                    url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                    postData: JSON.stringify(this.resource),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error creating trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        new LoginFixture ({name: "test_addTrapSubscriber2",
            resource: {
                Host: "192.168.56.200",
                ClientPortNumber: 162,
                Community: "UnitTest2"
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var trapSubscriber = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Host === resource.Host) {
                                    trapSubscriber = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(trapSubscriber !== undefined);
                            doh.assertEqual("OK", compareObjects(resource,trapSubscriber));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrPost({
                    url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                    postData: JSON.stringify(this.resource),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error creating trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),

        new LoginFixture ({name: "test_deleteTrapSubscriber1",
            resource: {
                Host: "192.168.56.100",
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var trapSubscriber = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Host === resource.Host) {
                                    trapSubscriber = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(trapSubscriber === undefined);
                            doh.assertEqual("OK", compareObjects(resource,trapSubscriber));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrDelete({
                    url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers/" + encodeURIComponent(this.resource.Host),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error deleting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        
        new LoginFixture ({name: "test_deleteCommunity1",
            resource: {
                Name: "UnitTest1",
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/communities",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var community = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Name === resource.Name) {
                                    community = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(community === undefined);
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting community: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrDelete({
                    url: Utils.getBaseUri() + "rest/config/snmp/communities/" + encodeURIComponent(this.resource.Name),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error deleting community: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        new LoginFixture ({name: "test_deleteCommunity2_FAIL",
            resource: {
                Name: "UnitTest2",
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/communities",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var community = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Name === resource.Name) {
                                    community = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(community !== undefined);
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting community: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrDelete({
                    url: Utils.getBaseUri() + "rest/config/snmp/communities/" + encodeURIComponent(this.resource.Name),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(false);
                    },
                    error: function(error) {
                        d.callback(true);
                    }
                });

                return d;
            }
        }),
        new LoginFixture ({name: "test_deleteTrapSubscriber2",
            resource: {
                Host: "192.168.56.200",
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var trapSubscriber = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Host === resource.Host) {
                                    trapSubscriber = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(trapSubscriber === undefined);
                            doh.assertEqual("OK", compareObjects(resource,trapSubscriber));
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrDelete({
                    url: Utils.getBaseUri() + "rest/config/snmp/trapSubscribers/" + encodeURIComponent(this.resource.Host),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error deleting trapSubscriber: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        
        new LoginFixture ({name: "test_deleteCommunity2",
            resource: {
                Name: "UnitTest2",
            },
            runTest: function() {
                var d = new doh.Deferred();
                var resource = this.resource;

                d.addCallback(lang.hitch(this, function(pass) {
                    doh.assertTrue(pass);
                    dojo.xhrGet({
                        url: Utils.getBaseUri() + "rest/config/snmp/communities",
                        handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                        load: function(data) {
                            // Find the one we just created
                            var community = undefined;
                            for (var i = 0; i < data.length; i++) {
                                if (data[i].Name === resource.Name) {
                                    community = data[i];
                                    break;
                                }
                            }
                            doh.assertTrue(community === undefined);
                        },
                        error: function(error) {
                            d.errback(new Error("Unexpected error getting community: " + JSON.stringify(JSON.parse(error.responseText))));
                        }
                    });
                    
                }));

                dojo.xhrDelete({
                    url: Utils.getBaseUri() + "rest/config/snmp/communities/" + encodeURIComponent(this.resource.Name),
                    handleAs: "json",
                    headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                    load: function(data, ioArgs) {
                        d.callback(true);
                    },
                    error: function(error) {
                        d.errback(new Error("Unexpected error deleting community: " + JSON.stringify(JSON.parse(error.responseText))));                        
                    }
                });

                return d;
            }
        }),
        

	]);
});
