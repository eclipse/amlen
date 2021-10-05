/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
    
    var restUri = Utils.getBaseUri() + "rest/config/cluster/clusterMembership/0";

    var getClusterMembership = function(d, expectedResult) {
        dojo.xhrGet({
            url: restUri,
            handleAs: "json",
            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},                           
            load: function(data) {
                doh.assertEqual("OK", Utils.compareObjects(expectedResult, data));
                d.callback(true);
            },
            error: lang.hitch(this, function(error, ioArgs) {
                var errorResult = error;
                try {
                    errorResult = JSON.parse(error.responseText);
                } catch (e) {
                    console.debug(e);
                }
                d.errback(new Error("Unexpected error getting ClusterMembership: " + JSON.stringify(errorResult)));                        
            })
        });
    };
    
    var putClusterMembership = function(d, config, expectedResult, expectedError) {
        dojo.xhrPut({
            headers: { 
                "Content-Type": "application/json",
                "X-Session-Verify": dojo.cookie("xsrfToken")
            },        
            url: restUri,
            handleAs: "json",
            putData: JSON.stringify(config),
            load: function(data) {
                if (expectedError) {
                    d.errback(new Error("Update should have failed but did not: " + JSON.stringify(data)));
                } else {
                    getClusterMembership(d, expectedResult);               
                }
            },
            error: lang.hitch(this, function(error, ioArgs) {
                if (expectedError) {
                    if (error.responseText != null) {
                        var response = JSON.parse(error.responseText);
                        if (response.code == expectedError) {
                            d.callback(true);
                        } else {
                            d.errback(new Error("Actual code (" + response.code + ") doesn't match expected (" + expectedError + ")"));
                        }
                    } else {
                        d.errback(new Error("Unexpected Return: ", error));
                    }
                } else {
                    d.errback(new Error("Unexpected error updating ClusterMembership: " + JSON.stringify(JSON.parse(error.responseText))));
                }
            })
        }); 
    };
    
	doh.register("ism.tests.widgets.test_ClusterMembership", [

	    // test GET of unconfigured ClusterMembership configuration
        new LoginFixture ({name: "test_getClusterMembership_1", 
	    	expected: {
	    	    ControlPort: "9102",
	    	    EnableClusterMembership: false,
	    	    MessagingPort: "9103",
	    	    MessagingUseTLS: false,
	    	    MulticastDiscoveryTTL: 1,
	    	    UseMulticastDiscovery: true
	    	},
			runTest: function() {
				var d = new doh.Deferred();
				getClusterMembership(d, this.expected);				
				return d;
			}
		}),
        // minimal ClusterMembership configuration
        new LoginFixture ({name: "test_setClusterMembership_minimal",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30"
            },
            expected: {
                ControlPort: "9102",
                EnableClusterMembership: false,
                MessagingPort: "9103",
                MessagingUseTLS: false,
                MulticastDiscoveryTTL: 1,
                UseMulticastDiscovery: true,
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30"
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, this.expected);
                return d;
            }
        }),
        // invalid control port - out of range
        new LoginFixture ({name: "test_setClusterMembership_invalid_port",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30",
                ControlPort: "65536"                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_OUT_OF_RANGE);  
                return d;
            }
        }),
        // invalid control external port - out of range
        new LoginFixture ({name: "test_setClusterMembership_invalid_external_port",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30",
                ControlExternalPort: "0"                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_OUT_OF_RANGE);  
                return d;
            }
        }),
        // invalid messaging port - out of range
        new LoginFixture ({name: "test_setClusterMembership_invalid_messaging_port",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30",
                MessagingPort: "66000"                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_OUT_OF_RANGE);  
                return d;
            }
        }),
        // invalid messaging external port - out of range
        new LoginFixture ({name: "test_setClusterMembership_invalid_messaging_external_port",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.56.30",
                MessagingExternalPort: "0"                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_OUT_OF_RANGE);  
                return d;
            }
        }),
        // invalid cluster name - too long
        new LoginFixture ({name: "test_setClusterMembership_invalid_clusterName",
            config: {
                ClusterName: Utils.createString(257),
                ControlAddress: "192.168.56.30",                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_TOO_LONG); 
                return d;
            }
        }),
        // invalid address - too long
        new LoginFixture ({name: "test_setClusterMembership_invalid_controlAddress",
            config: {
                ClusterName: "testCluster",
                ControlAddress: "192.168.156.130.192.168.156.130.192.168.156.130",                    
            },
            runTest: function() {
                var d = new doh.Deferred();
                putClusterMembership(d, this.config, null, Utils.validationErrors.VALUE_TOO_LONG); 
                return d;
            }
        }),        
	]);
});
