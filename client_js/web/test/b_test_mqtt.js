/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
define(
        [ "doh/runner", "js/mqttws31" ],
        function(doh, mqtt) {
            var ip = "127.0.0.1";
            var port = 16102;
            var debug = false;

            var clientType = navigator.userAgent;
            var phantomjs = clientType.indexOf("PhantomJS") + 1;
            var firefox = clientType.indexOf("Firefox") + 1;
            var chrome = clientType.indexOf("Chrome") + 1;

            socket_stub = {
                send: function() { /* Do nothing */
                },
                close: function() { /* Do nothing */
                },
                readyState: WebSocket.OPEN
            };
            pinger_stub = {
                reset: function() { /* Do nothing */
                },
                cancel: function() { /* Do nothing */
                }
            };

            // doh.register("test.test_mqtt.messagetypes",
            // [
            // function messagetypes (doh){
            // doh.assertEqual(MQTT.MESSAGE_TYPE.CONNECT,"CONNECT");
            // }
            // ]);
            doh.register("test.test_mqtt.createclient", [ function createclient(doh) {
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");

                if (debug)
                    console.log("asserting", ip, " = ", client.host);
                doh.assertEqual(ip, client.host);

                if (debug)
                    console.log("asserting ", port, " = ", client.port);
                doh.assertEqual(port, client.port);

                if (debug)
                    console.log("asserting myclient = ", client.clientId);
                doh.assertEqual("myclient", client.clientId);

                // ************************************************************
                // *** TODO: Add lines here or new tests that checks settings
                // for new callback functions ***
                // ************************************************************

                // if (debug)
                // console.log("asserting onconnect = ", null);
                // doh.assertEqual(null, client.onconnect);
                //
                // if (debug)
                // console.log("asserting onconnectionlost = ", null);
                // doh.assertEqual(null, client.onconnectionlost);
                //
                // if (debug)
                // console.log("asserting onmessage = ", null);
                // doh.assertEqual(null, client.onmessage);
            }, function createclientbadid(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                try {
                    var client = new Messaging.Client(ip, port, "myclient9012345678901234");
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = "Invalid argument:myclient9012345678901234";
                    if (debug) {
                        console.log("Asserting " + err.message + " = " + expected);
                    }
                    doh.assertEqual(expected, err.message);
                }

                if (debug)
                    console.log("Calling client constructor");
                try {
                    var client = new Messaging.Client(ip, port, "");
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = "Invalid argument:";
                    if (debug) {
                        console.log("Asserting " + err.message + " = " + expected);
                    }
                    doh.assertEqual(expected, err.message);
                }
            } ]);
            doh
                    .register(
                            "test.test_mqtt.connect",
                            [
                                    function connect_noconnopts() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;

                                        if (debug)
                                            console.log("Calling connect()");
                                        client.connect(null);

                                        console
                                                .log("client.connectOptions is "
                                                        + JSON.stringify(client.connectOptions));

                                        // Binary MQTT client forces
                                        // cleanSession set to true if nothing
                                        // is specified for this property.
                                        var expected = {
                                            "cleanSession": true
                                        };

                                        if (debug)
                                            console.log("asserting ", JSON.stringify(expected), " = ", JSON
                                                    .stringify(client.connectOptions));
                                        doh
                                                .assertEqual(JSON.stringify(expected), JSON
                                                        .stringify(client.connectOptions));
                                    },

                                    function connect_allconnopts() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;

                                        var willMessage = new Messaging.Message("will message");
                                        willMessage.qos = 0;
                                        willMessage.retained = false;
                                        willMessage.destinationName = "willtopic";

                                        var connopts = {
                                            userName: "uname",
                                            password: "pword",
                                            cleanSession: false,
                                            keepAliveInterval: 70,
                                            willMessage: willMessage
                                        };

                                        if (debug)
                                            console.log("Calling connect()");
                                        client.connect(connopts);

                                        if (debug)
                                            console.log("asserting ", JSON.stringify(connopts), " = ", JSON
                                                    .stringify(client.connectOptions));
                                        doh
                                                .assertEqual(JSON.stringify(connopts), JSON
                                                        .stringify(client.connectOptions));

                                    },

                                    function connect_alreadyconnecting() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;

                                        if (debug)
                                            console.log("Calling connect() twice");
                                        try {
                                            client.connect();
                                            client.connect();
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            if (debug)
                                                console.log("asserting error msg = Invalid state, already connecting");
                                            doh.assertEqual("Invalid state, already connecting", err.message);

                                        }
                                    },

                                    function connect_invalidconnopt() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set and invalid connection option to
                                        // force a failure
                                        var connopts = {
                                            invalidOpt: "invalidOption"
                                        };

                                        if (debug)
                                            console.log("Calling connect()");
                                        try {
                                            client.connect(connopts);
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Unknown property, invalidOpt. Valid properties are: timeout userName password willMessage keepAliveInterval cleanSession useSSL';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);

                                        }
                                    },

                                    function connect_invalidwillqos() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set an invalid value for willQoS to
                                        // force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.qos = "invalidOption";
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:invalidOption';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }
                                    },

                                    function connect_emptywillqos() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set and invalid value for willQoS to
                                        // force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.qos = "";
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }

                                        // var connopts = {
                                        // willMessage: willMessage
                                        // };
                                        //
                                        // if (debug)
                                        // console.log("Calling connect()");
                                        // try {
                                        // client.connect(connopts);
                                        // // Force test failure if error is
                                        // // not thrown
                                        // console.log("Connect options: " +
                                        // JSON.stringify(client.connectOptions));
                                        // doh.assertTrue(false);
                                        // } catch (err) {
                                        // var expected = 'Invalid QoS()
                                        // specified. QoS must be 0 (default), 1
                                        // or 2.';
                                        // if (debug)
                                        // console.log("asserting error msg = ",
                                        // expected);
                                        // doh.assertEqual(expected,
                                        // err.message);
                                        // }
                                    },

                                    function connect_invalidwillqosfloat() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set an invalid value for willQoS to
                                        // force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.qos = "0.77";
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:0.77';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }

                                        // var connopts = {
                                        // willMessage: willMessage
                                        // };
                                        //
                                        // if (debug)
                                        // console.log("Calling connect()");
                                        // try {
                                        // client.connect(connopts);
                                        // // Force test failure if error is
                                        // // not thrown
                                        // console.log("Connect options: " +
                                        // JSON.stringify(client.connectOptions));
                                        // doh.assertTrue(false);
                                        // } catch (err) {
                                        // var expected = 'Invalid QoS(0.77)
                                        // specified. QoS must be 0 (default), 1
                                        // or 2.';
                                        // ;
                                        // if (debug)
                                        // console.log("asserting error msg = ",
                                        // expected);
                                        // doh.assertEqual(expected,
                                        // err.message);
                                        // }
                                    },

                                    function connect_invalidwillretain() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set and invalid value for willQoS to
                                        // force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.retained = "invalidOption";
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:invalidOption';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }

                                        // var connopts = {
                                        // willMessage: willMessage
                                        // };
                                        //
                                        // if (debug)
                                        // console.log("Calling connect()");
                                        // try {
                                        // client.connect(connopts);
                                        // // Force test failure if error is
                                        // // not thrown
                                        // console.log("Connect options: " +
                                        // JSON.stringify(client.connectOptions));
                                        // doh.assertTrue(false);
                                        // } catch (err) {
                                        // var expected = 'Invalid
                                        // retain(invalidOption) specified.
                                        // Retain must be false (default) or
                                        // true.';
                                        // if (debug)
                                        // console.log("asserting error msg = ",
                                        // expected);
                                        // doh.assertEqual(expected,
                                        // err.message);
                                        // }
                                    },

                                    function connect_invalidwillretainfloat() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set and invalid value for willQoS to
                                        // force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.retained = 1.11;
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:1.11';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }

                                        // var connopts = {
                                        // willMessage: willMessage
                                        // };
                                        //
                                        // if (debug)
                                        // console.log("Calling connect()");
                                        // try {
                                        // client.connect(connopts);
                                        // // Force test failure if error is
                                        // // not thrown
                                        // console.log("Connect options: " +
                                        // JSON.stringify(client.connectOptions));
                                        // doh.assertTrue(false);
                                        // } catch (err) {
                                        // var expected = 'Invalid retain(1.11)
                                        // specified. Retain must be false
                                        // (default) or true.';
                                        // if (debug)
                                        // console.log("asserting error msg = ",
                                        // expected);
                                        // doh.assertEqual(expected,
                                        // err.message);
                                        // }
                                    }, function connect_invalidwillretain() {
                                        var client = new Messaging.Client(ip, port, "myclient");
                                        // debug = true;
                                        // Set an invalid value for willRetain
                                        // to force a failure

                                        var willMessage = new Messaging.Message("will message");
                                        try {
                                            willMessage.retained = "0";
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Invalid argument:0';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }

                                        // var connopts = {
                                        // willMessage: willMessage
                                        // };
                                        //
                                        // if (debug)
                                        // console.log("Calling connect()");
                                        // try {
                                        // client.connect(connopts);
                                        // // Force failure if error is not
                                        // // thrown
                                        // console.log("Connect options: " +
                                        // JSON.stringify(client.connectOptions));
                                        // doh.assertTrue(false);
                                        // } catch (err) {
                                        // var expected = 'Invalid retain(0)
                                        // specified. Retain must be false
                                        // (default) or true.';
                                        // if (debug)
                                        // console.log("asserting error msg = ",
                                        // expected);
                                        // doh.assertEqual(expected,
                                        // err.message);
                                        // }
                                    } ]);

            doh
                    .register(
                            "test.test_mqtt.subscribe",
                            [
                                    // String entries are not valid for the
                                    // binary client
                                    // function subscribe_validqosstr(doh) {
                                    // debug = true;
                                    // if (debug)
                                    // console.log("Calling client
                                    // constructor");
                                    // var client = new Messaging.Client(ip,
                                    // port, "myclient");
                                    // if (debug)
                                    // console.log("Calling subscribe");
                                    // client.subscribe("topic", {
                                    // "qos": "0"
                                    // });
                                    // client.subscribe("topic", {
                                    // "qos": "1"
                                    // });
                                    // client.subscribe("topic", {
                                    // "qos": "2"
                                    // });
                                    // },

                                    function subscribe_validqosnum(doh) {
                                        debug = true;
                                        if (debug)
                                            console.log("Calling client constructor");
                                        var client = new Messaging.Client(ip, port, "myclient");

                                        if (debug)
                                            console.log("Calling subscribe");
                                        client.subscribe("topic", {
                                            qos: 0
                                        });
                                        client.subscribe("topic", {
                                            qos: 1
                                        });
                                        client.subscribe("topic", {
                                            qos: 2
                                        });
                                    },

                                    function subscribe_invalidqosopt(doh) {
                                        debug = true;
                                        if (debug)
                                            console.log("Calling client constructor");
                                        var client = new Messaging.Client(ip, port, "myclient");

                                        if (debug)
                                            console.log("Calling subscribe");
                                        try {
                                            client.subscribe("topic", {
                                                invalidQoS: ""
                                            });
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'Unknown property, invalidQoS. Valid properties are: qos invocationContext onSuccess onFailure';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }
                                    },

                                    function subscribe_invalidqosstr(doh) {
                                        debug = true;
                                        if (debug)
                                            console.log("Calling client constructor");
                                        var client = new Messaging.Client(ip, port, "myclient");

                                        if (debug)
                                            console.log("Calling subscribe");
                                        try {
                                            client.subscribe("topic", {
                                                qos: "invalidQoS"
                                            });
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'InvalidType:string for qos';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }
                                    },

                                    function subscribe_invalidqosbool(doh) {
                                        debug = true;
                                        if (debug)
                                            console.log("Calling client constructor");
                                        var client = new Messaging.Client(ip, port, "myclient");

                                        if (debug)
                                            console.log("Calling subscribe");
                                        try {
                                            client.subscribe("topic", {
                                                qos: true
                                            });
                                            // Force test failure if error is
                                            // not thrown
                                            doh.assertTrue(false);
                                        } catch (err) {
                                            var expected = 'InvalidType:boolean for qos';
                                            if (debug)
                                                console.log("asserting error msg = ", expected);
                                            doh.assertEqual(expected, err.message);
                                        }
                                    } ]);

            doh.register("test.test_mqtt.publish", [ function publish_invalidqosstr(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setQos("invalidQoS");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid QoS(invalidQoS) specified.  QoS must be 0 (default), 1 or 2.';
                    if (debug)
                        console.log("asserting error msg = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidqosbool(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setQos(false);

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid QoS(false) specified.  QoS must be 0 (default), 1 or 2.';
                    if (debug)
                        console.log("asserting error msg = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            }, function publish_invalidqosfloatstr(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setQos("2.32");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid QoS(2.32) specified.  QoS must be 0 (default), 1 or 2.';
                    ;
                    if (debug)
                        console.log("asserting " + err.message + " starts with or = ", expected);

                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidretain(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setRetained("invalidRetain");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid retain(invalidRetain) specified.  Retain must be false (default) or true.';
                    if (debug)
                        console.log("asserting error msg = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidretainfloatstr(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setRetained("1.88");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid retain(1.88) specified.  Retain must be false (default) or true.';
                    if (debug)
                        console.log("asserting error msg = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidretain5(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic");
                var message = new Messaging.Message("message");
                message.setRetained(5);

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid retain(5) specified.  Retain must be false (default) or true.';
                    if (debug)
                        console.log("asserting error msg = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidtopicplus(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic/+");
                var message = new Messaging.Message("message");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid publish topic(topic/+). Publish topic cannot contain wildcard characters.';
                    if (debug)
                        console.log("asserting " + err.message + " = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            function publish_invalidtopichash(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                var topic = client.getTopic("topic/#");
                var message = new Messaging.Message("message");

                if (debug)
                    console.log("Calling publish");
                try {
                    topic.publish(message);
                    // Force test failure if error is not thrown
                    doh.assertTrue(false);
                } catch (err) {
                    var expected = 'Invalid publish topic(topic/#). Publish topic cannot contain wildcard characters.';
                    if (debug)
                        console.log("asserting " + err.message + " = ", expected);
                    doh.assertEqual(expected, err.message);
                }
            },

            ]);
            // doh
            // .register(
            // "test.test_mqtt.topiclisteners",
            // [
            // function topiclisteners_addwildcardplus(doh) {
            // debug = true;
            // if (debug)
            // console.log("Calling client constructor");
            // var client = new MQTT.Client(ip, port, "myclient");
            //
            // client._connected = true;
            //
            // tCallback = function(message) {
            // // Do nothing
            // };
            //
            // if (debug)
            // console.log("Calling addTopicListener");
            // try {
            // client.addTopicListener("topic/+", tCallback);
            // // Force test failure if error is
            // // not thrown
            // doh.assertTrue(false);
            // } catch (err) {
            // var expected = "Invalid listener topic(topic/+). The topic for a
            // topic listener cannot contain wildcard characters.";
            // if (debug)
            // console.log("asserting " + err.message + " = ", expected);
            // doh.assertEqual(expected, err.message);
            // }
            // },
            //
            // function topiclisteners_addwildcardhash(doh) {
            // debug = true;
            // if (debug)
            // console.log("Calling client constructor");
            // var client = new MQTT.Client(ip, port, "myclient");
            //
            // client._connected = true;
            //
            // tCallback = function(message) {
            // // Do nothing
            // };
            //
            // if (debug)
            // console.log("Calling addTopicListener");
            // try {
            // client.addTopicListener("topic/#", tCallback);
            // // Force test failure if error is
            // // not thrown
            // doh.assertTrue(false);
            // } catch (err) {
            // var expected = "Invalid listener topic(topic/#). The topic for a
            // topic listener cannot contain wildcard characters.";
            // if (debug)
            // console.log("asserting " + err.message + " = ", expected);
            // doh.assertEqual(expected, err.message);
            // }
            // },
            //
            // function topiclisteners_addtopic(doh) {
            // debug = true;
            // if (debug)
            // console.log("Calling client constructor");
            // var client = new MQTT.Client(ip, port, "myclient");
            //
            // client._connected = true;
            //
            // var tCallback = function(message) {
            // // Do nothing
            // };
            //
            // if (debug)
            // console.log("Calling addTopicListener");
            //
            // // Add first topic and confirm it was
            // // added
            // try {
            // client.addTopicListener("topic", tCallback);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_addtopic: " +
            // err.message);
            // }
            // if (debug) {
            // console.log("asserting 1 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 0 = "
            // + client._topic_listeners["topic"].indexOf(tCallback));
            // }
            // doh.assertEqual(1, client._topic_listeners["topic"].length);
            // doh.assertEqual(0,
            // client._topic_listeners["topic"].indexOf(tCallback));
            //
            // var tCallback2 = function(message) {
            // // Do nothing2
            // };
            //
            // // Add second topic and confirm it was
            // // added
            // try {
            // client.addTopicListener("topic", tCallback2);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_addtopic: " +
            // err.message);
            // }
            // if (debug) {
            // console.log("asserting 2 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // }
            // doh.assertEqual(2, client._topic_listeners["topic"].length);
            // doh.assertEqual(1,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            //
            // // Do a duplicate add second topic.
            // // Assure the array of callbacks
            // // for the topic is unchanged because
            // // the topic is already in
            // // the array.
            // try {
            // client.addTopicListener("topic", tCallback2);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_addtopic: " +
            // err.message);
            // }
            // if (debug) {
            // console.log("asserting 2 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // console.log("asserting "
            // + client._topic_listeners["topic"].indexOf(tCallback2) + " = "
            // + client._topic_listeners["topic"].lastIndexOf(tCallback2));
            // }
            // doh.assertEqual(2, client._topic_listeners["topic"].length);
            // doh.assertEqual(1,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            // doh.assertEqual(client._topic_listeners["topic"].indexOf(tCallback2),
            // client._topic_listeners["topic"].lastIndexOf(tCallback2));
            // },
            //
            // function topiclisteners_removetopic(doh) {
            // debug = true;
            // if (debug)
            // console.log("Calling client constructor");
            // var client = new MQTT.Client(ip, port, "myclient");
            //
            // client._connected = true;
            //
            // var tCallback = function(message) {
            // // Do nothing
            // };
            //
            // if (debug)
            // console.log("Calling addTopicListener");
            //
            // // Add first topic and confirm it was
            // // added
            // try {
            // client.addTopicListener("topic", tCallback);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting 1 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 0 = "
            // + client._topic_listeners["topic"].indexOf(tCallback));
            // }
            // doh.assertEqual(1, client._topic_listeners["topic"].length);
            // doh.assertEqual(0,
            // client._topic_listeners["topic"].indexOf(tCallback));
            //
            // var tCallback2 = function(message) {
            // // Do nothing2
            // };
            //
            // // Add second topic and confirm it was
            // // added
            // try {
            // client.addTopicListener("topic", tCallback2);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting 2 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // }
            // doh.assertEqual(2, client._topic_listeners["topic"].length);
            // doh.assertEqual(1,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            //
            // // Remove topic and confirm it was
            // // removed
            // var tHasCallbacks;
            // try {
            // tHasCallbacks = client.removeTopicListener("topic", tCallback2);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting true = " + tHasCallbacks);
            // console.log("asserting 1 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting -1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // }
            // // Should be true. There were two
            // // callbacks and only one was removed
            // doh.assertEqual(true, tHasCallbacks);
            // doh.assertEqual(1, client._topic_listeners["topic"].length);
            // doh.assertEqual(-1,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            //
            // // Remove second (last) topic and
            // // confirm it was removed
            // try {
            // tHasCallbacks = client.removeTopicListener("topic", tCallback);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting false = " + tHasCallbacks);
            // console.log("asserting 0 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting -1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback));
            // }
            // // Should be false. Once this second
            // // callback is removed there are no more
            // // callbacks for the topic.
            // doh.assertEqual(false, tHasCallbacks);
            // doh.assertEqual(0, client._topic_listeners["topic"].length);
            // doh.assertEqual(-1,
            // client._topic_listeners["topic"].indexOf(tCallback));
            //
            // // Attempt to remove a callback for a
            // // topic that has no callbacks
            // try {
            // tHasCallbacks = client.removeTopicListener("topic", tCallback);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting false = " + tHasCallbacks);
            // console.log("asserting 0 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting -1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback));
            // }
            // // Should be false because ther are no
            // // callbacks for this topic
            // doh.assertEqual(false, tHasCallbacks);
            // doh.assertEqual(0, client._topic_listeners["topic"].length);
            // doh.assertEqual(-1,
            // client._topic_listeners["topic"].indexOf(tCallback));
            //
            // // Add tCallback2
            // try {
            // client.addTopicListener("topic", tCallback2);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting 1 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting 0 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // }
            // doh.assertEqual(1, client._topic_listeners["topic"].length);
            // doh.assertEqual(0,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            //
            // // Attempt to remove a callback for a
            // // topic that has callbacks but not the
            // // callback specified for removal.
            // // (This topic has tCallback2 but does
            // // not have tCallback.)
            // try {
            // tHasCallbacks = client.removeTopicListener("topic", tCallback);
            // } catch (err) {
            // console.log("Unexpected error in topiclisteners_removetopic: "
            // + err.message);
            // }
            // if (debug) {
            // console.log("asserting true = " + tHasCallbacks);
            // console.log("asserting 1 = " +
            // client._topic_listeners["topic"].length);
            // console.log("asserting -1 = "
            // + client._topic_listeners["topic"].indexOf(tCallback));
            // console.log("asserting 0 = "
            // + client._topic_listeners["topic"].indexOf(tCallback2));
            // }
            // // Should be true because there are
            // // still callbacks for this topic
            // doh.assertEqual(true, tHasCallbacks);
            // doh.assertEqual(1, client._topic_listeners["topic"].length);
            // doh.assertEqual(-1,
            // client._topic_listeners["topic"].indexOf(tCallback));
            // doh.assertEqual(0,
            // client._topic_listeners["topic"].indexOf(tCallback2));
            // }, ]);
            doh.register("test.test_mqtt.connackrc", [ function connackrc_withoutonconnect(doh) {
                // Test that non-zero rc will result in automatic disconnect
                // if no onconnect is specified by the client app
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");
                client.startTrace();

                client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
                console.log(client.getTraceLog());
                client._socket = socket_stub;
                console.log(client.getTraceLog());
                client._pinger = pinger_stub;
                console.log(client.getTraceLog());

                var connack = '{"type":"CONNACK", "rc":1}';
                console.log(client.getTraceLog());
                var event = {
                    "data": connack
                };
                client._on_socket_message(event);
                console.log(client.getTraceLog());
                doh.assertFalse(client._socket_is_open());
                console.log(client.getTraceLog());
            },

            function connackrc_withonconnect(doh) {
                // Test that non-zero rc will result in call to onconnect
                // if specified by the client app
                debug = true;
                var msg = null;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");

                client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
                client._socket = socket_stub;
                client._pinger = pinger_stub;
                client.onconnect = function(rc) {
                    msg = "rc is " + rc;
                }

                var connack = '{"type":"CONNACK", "rc":1}';
                var event = {
                    "data": connack
                };
                client._on_socket_message(event);
                doh.assertTrue(client._socket_is_open());
                doh.assertFalse(client.isConnected());
                doh.assertEqual("rc is 1", msg);
            },

            function connackrc0_withoutonconnect(doh) {
                debug = true;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");

                client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
                client._socket = socket_stub;
                client._pinger = pinger_stub;

                var connack = '{"type":"CONNACK", "rc":0}';
                var event = {
                    "data": connack
                };
                client._on_socket_message(event);
                doh.assertTrue(client._socket_is_open());
                doh.assertTrue(client.isConnected());
            },

            function connackrc_withonconnect(doh) {
                debug = true;
                var msg = null;
                if (debug)
                    console.log("Calling client constructor");
                var client = new Messaging.Client(ip, port, "myclient");

                client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
                client._socket = socket_stub;
                client._pinger = pinger_stub;
                client.onconnect = function(rc) {
                    msg = "rc is " + rc;
                }

                var connack = '{"type":"CONNACK", "rc":0}';
                var event = {
                    "data": connack
                };
                client._on_socket_message(event);
                doh.assertTrue(client._socket_is_open());
                doh.assertTrue(client.isConnected());
                doh.assertEqual("rc is 0", msg);
            }, ]);
        });