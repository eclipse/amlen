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
/**
 * @overview The IBM Messaging Appliance JavaScript library for sending and receiving MQTT v3 messages over WebSockets.
 * @name ism_mqttws.js
 */

/**
 * @namespace IsmMqtt is the JavaScript namespace for the IBM Messaging 
 *            Appliance MQTT client API.
 * @name IsmMqtt
 */
IsmMqtt = (function(global) {
    var MESSAGE_TYPE = {
        CONNECT: "CONNECT",
        CONNACK: "CONNACK",
        PUBLISH: "PUBLISH",
        PUBACK: "PUBACK",
        PUBREC: "PUBREC",
        PUBREL: "PUBREL",
        PUBCOMP: "PUBCOMP",
        SUBSCRIBE: "SUBSCRIBE",
        SUBACK: "SUBACK",
        UNSUBSCRIBE: "UNSUBSCRIBE",
        UNSUBACK: "UNSUBACK",
        PINGREQ: "PINGREQ",
        PINGRESP: "PINGRESP",
        DISCONNECT: "DISCONNECT"
    };

    /**
     * Return a new function which runs the user function bound to a fixed
     * scope.
     * 
     * @param {Function}
     *            User function
     * @param {Object}
     *            Function scope
     * @return {Function} User function bound to another scope
     * @private
     */
    var scope = function(f, scope) {
        return function() {
            return f.apply(scope, arguments);
        };
    };

    /** @private */
    _Pinger = (function() {
        var _client;
        var _window;
        var _keepAliveInterval;

        var JSONMessage = JSON.stringify({
            type: MESSAGE_TYPE.PINGREQ
        });
        var timeout = null;
        var pingOutstanding = false;

        var Pinger_ctor = function(client, window, keepAliveInterval) {
            _client = client;
            _window = window;
            _keepAliveInterval = keepAliveInterval * 1000;
        };

        var doPing = function() {
            if (_client.isConnected()) {
                if (!pingOutstanding) {
                    _client._socket.send(JSONMessage);
                    pingOutstanding = true;
                    timeout = _window.setTimeout(doPing, _keepAliveInterval);
                } else {
                    _client._disconnected("Connection timed out");
                }
            }
        };

        Pinger_ctor.prototype.reset = function() {
            _window.clearTimeout(timeout);
            if (_keepAliveInterval > 0) {
                timeout = setTimeout(doPing, _keepAliveInterval);
            }
        };

        Pinger_ctor.prototype.responseReceived = function() {
            pingOutstanding = false;
        };

        Pinger_ctor.prototype.cancel = function() {
            _window.clearTimeout(timeout);
            timeout = null;
        };

        return Pinger_ctor;
    })();

    /** @private */
    function uniqueString() {
        var unique = "";
        var chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        for ( var i = 0; i < 5; i++) {
            unique += chars[Math.floor(Math.random() * 62)];
        }
        // console.log("Unique string: " + unique);
        return unique;
    }

    /**
     * @private Token Store is used to store the delivery token. Each client has
     *          an instance of the token store. There is a timer task which
     *          wakes up every second to go through the tokens in the store and
     *          to determine whether there are messages that need to be resent.
     */
    var _TokenStore = function(client, window) {
        // Array that store the token
        this.PERSISTENTSTORE_PREFIX = "ISM_TOKENSTORE_" + client.clientId + "_";
        this._tokens = {};
        this._message_identifier = 1;
        this._client = client;
        this._window = window;
        this.timerid = null;
        this.isLocalStorage = false;
        this.msgpendingcnt = 0;
        this.msgcompletedcnt = 0;

        try {
            if ('localStorage' in window && window['localStorage'] !== null) {
                this.isLocalStorage = true;
            } else {
                this.isLocalStorage = false;

            }
        } catch (e) {
            this.isLocalStorage = false;
        }
        if (this.isLocalStorage == false)
            console
                    .log("WARN: the browser doesn't support HTML5 localStorage. Persistent Store for the delivery token is disabled.");
    };

    /**
     * @private saveToken function creates a new token if the message is not a
     *          duplicate. If the message is a duplicate, the token that
     *          contains the message will be returned.
     * 
     */
    _TokenStore.prototype.saveToken = function(message) {
        var token = null;
        if (this._tokens.length > 65536) {
            throw new Error("Too many messages in-flight:" + this._tokens.length);
        }

        // check if message is dup. No need to put the message
        // into another slot if the message is dup.
        if (message.dup == false || message.dup == undefined) {
            while (this._tokens[this._message_identifier] !== undefined) {
                this._message_identifier++;
            }
            message.id = this._message_identifier;
            token = new DeliveryToken(message);

            this._tokens[message.id] = token;

            if (this._message_identifier === 65536) {
                this._message_identifier = 1;
            }
        } else {
            token = this._tokens[message.id];
        }

        this.persistTokenInPersistentStore(token);
        if (message.type == MESSAGE_TYPE.PUBLISH && this._client._deliverytrackingmode == "CLIENT") {
            this.msgpendingcnt++;
        }
        return token;
    };

    /**
     * @private The removeToken function removes the token from the array of
     *          tokens. The token will be marked complete.
     * 
     */
    _TokenStore.prototype.removeToken = function(message) {
        var token = this._tokens[message.id];
        if (token != null) {
            token.completed = true;
            if (token.message.type == MESSAGE_TYPE.PUBLISH) {
                this.removeTokenInPersistentStore(message.id);
                if (this._client._deliverytrackingmode = "CLIENT") {
                    this.msgpendingcnt--;
                    this.msgcompletedcnt++;
                }

            }
            delete this._tokens[message.id];

        }
    };

    /** @private */
    _TokenStore.prototype.getToken = function(message) {
        var token = this._tokens[message.id];
        return token;
    };

    /**
     * @private The token store timer task goes through the tokens in the array
     *          and determines if the timeout has expired. If the timeout has
     *          expired, the original message will be resent and marked as a
     *          duplicate.
     * 
     */
    _TokenStore.prototype.tokenStoreTimerTask = function() {

        // Need to check if the client is connected before processing
        if (!this._client._connected) {
            return;
        }
        for ( var tokenKey in this._tokens) {

            if (this._tokens.hasOwnProperty(tokenKey)) {
                var token = this._tokens[tokenKey];
                // Check if the Message is Expired. If so, redeliver
                // Get CurrentTime:
                var d = new Date();
                var currentTime = d.getTime();
                if (token.message.type == MESSAGE_TYPE.PUBLISH) {
                    if (currentTime > (token.msgResendTimeout + token.lastsenttime)) {
                        // console.log("Need to redeliver this message: msgID:
                        // "+ token.message.id);

                        // If PUBREC is already received, but not PUBCOMP.
                        // Only resend the PUBREL ack mesage for the message.
                        if (token.message.qos == 2 && token.pubrecreceived == true) {
                            var pubrelMessage = {
                                type: MESSAGE_TYPE.PUBREL,
                                id: token.message.id
                            };
                            this._client._send(pubrelMessage);
                        } else {
                            if (token.message.replyto == null || token.message.replyto == undefined) {
                                this._client._publish(token.message.topic, token.message.payload, token.message.qos,
                                        token.message.retained, true, token.message.id);
                            } else {
                                // Call publishWithReply
                                this._client._publishWithReply(token.message.topic, token.message.payload,
                                        token.message.replyto, token.message.corrid, token.message.qos,
                                        token.message.retained, true, token.message.id);

                            }
                        }
                    }
                }
            }
        }
    };

    /**
     * @private The startTokenStoreTaskTimer function starts the task timer for
     *          the token store
     * 
     */
    _TokenStore.prototype.startTokenStoreTaskTimer = function() {
        var inst = this;
        console.log("Start the token store task timer: clientid=" + inst._client.clientId);
        this.timerid = setInterval(function() {
            inst.tokenStoreTimerTask();
        }, 10000);

    };

    /**
     * @private The stopTokenStoreTaskTimer function stops the task timer for
     *          the token store
     * 
     */
    _TokenStore.prototype.stopTokenStoreTaskTimer = function() {
        var inst = this;
        console.log("Stop the token store task timer: clientid=" + inst._client.clientId);
        clearInterval(this.timerid);
    };

    /**
     * @private Persist, update, or remove token from storage.
     */
    _TokenStore.prototype.persistTokenInPersistentStore = function(token) {
        if (this.isLocalStorage == false)
            return;

        // Only store publish messages with QoS 1 & 2
        if (token.message.type != MESSAGE_TYPE.PUBLISH || token.message.qos < 1)
            return;

        // Convert to JSON String
        // Need to add the ISM PREFIX. the local storage might be shared with
        // other apps

        var id = this.PERSISTENTSTORE_PREFIX + token.message.id;
        var tokenString = JSON.stringify(token);

        try {
            localStorage.setItem(id, tokenString); // store the item in the
            // database
        } catch (e) {
            if (e == QUOTA_EXCEEDED_ERR) {
                console.log("ERROR: Failed to persist the delivery token due to the maximum storage is reached.");
            }
        }

    };

    /**
     * @private Get the token from persistent storage
     * @param id
     *            the id of the item
     * @return the object
     */
    _TokenStore.prototype.getTokenInPersistentStore = function(tid) {
        if (this.isLocalStorage == false)
            return null;

        var id = this.PERSISTENTSTORE_PREFIX + tid;
        var tokenString = localStorage.getItem(id);
        return tokenString;
    };

    /**
     * @private Remove the token from the persistent storage
     */
    _TokenStore.prototype.removeTokenInPersistentStore = function(tid) {
        if (this.isLocalStorage == false)
            return;

        var id = this.PERSISTENTSTORE_PREFIX + tid;
        localStorage.removeItem(id);

    };

    /**
     * @private Restore all the tokens in the store. This is case of restoring
     *          the client.
     */
    _TokenStore.prototype.restoreTokensInPersistentStore = function() {
        if (this.isLocalStorage == false)
            return;

        var storageLength = localStorage.length - 1;
        // now we are going to loop through each item in the database
        for (i = 0; i <= storageLength; i++) {
            // let's setup some variables for the key and values
            var itemKey = localStorage.key(i);
            var value = localStorage.getItem(itemKey);
            if (value != null && itemKey.indexOf(this.PERSISTENTSTORE_PREFIX) > -1) {
                // Found a existing token
                var token = JSON.parse(value);
                this._tokens[token.message.id] = token;
            }

        }
        // Invoke the store timer task to process the tokens.
        this.tokenStoreTimerTask();
    };

    /**
     * @private Clear all the tokens for this client from the store. This is
     *          used when the client connects with clean session (clean) set to
     *          true.
     */
    _TokenStore.prototype.cleanTokensInPersistentStore = function() {
        if (this.isLocalStorage == false)
            return;
        var storageLength = localStorage.length - 1;
        // now we are going to loop through each item in the database
        for (i = 0; i <= storageLength; i++) {
            // lets setup some variables for the key and values
            var itemKey = localStorage.key(i);
            if (itemKey.indexOf(this.PERSISTENTSTORE_PREFIX) > -1) {
                // Found a existing token
                this.removeTokenInPersistentStore(itemKey);
            }

        }
    };

    /** @private */
    var MessageDeliveryStatus = function(tokenStore) {
        this.pendingcount = tokenStore.msgpendingcnt;
        this.completedcount = tokenStore.msgcompletedcnt;

        tokenStore.msgcompletedcnt = 0;

    };

    /**
     * @private Get the Message Delivery Stats
     */
    _TokenStore.prototype.getMsgDeliveryStats = function() {
        return new MessageDeliveryStatus(this);
    };

    /**
     * @private DeliveryToken is the object that contains the information about
     *          the messages that have been sent to the server.
     */
    var DeliveryToken = function(message) {

        this.completed = false;
        this.message = message;
        this.msgResendTimeout = 10000;
        this.pubrecreceived = false;

        var d = new Date();
        var n = d.getTime();
        this.lastsenttime = n;
    };

    /**
     * MQTT client for sending and receiving MQTT v3 messages using the IBM
     * Messaging Appliance server.
     * 
     * <p>
     * The client provides APIs to support connections, subscriptions and
     * publish actions using MQTT v3 messages. It also provides extensions to
     * MQTT for replying to received messages and for creating temporary topics.
     * </p>
     * 
     * <b>NOTE:</b> Client applications must be run in a browser that supports
     * WebSockets/HTML 5.
     * 
     * @name IsmMqtt.Client
     * @constructor
     * @param {String}
     *            host the address of the IBM Messaging Appliance server.
     * @param {Number}
     *            port the server port number.
     * @param {String}
     *            clientId the MQTT client identifier.
     * @param {Number}
     *            [msgResendTimeout] optional setting for the time to wait
     *            before resending an unacknowledged message with QoS 1 or 2.
     * @throws {Error}
     *             error thrown if client is run in a browser that does not
     *             support WebSockets
     * @throws {Error}
     *             error thrown if the client ID is not between 1 and 23
     *             characters long
     * 
     * @example Example: Creating a client <code>
     * var client = new IsmMqtt.Client(serverhost.co.com, 80, "clientName");
     * client.onmessage = function(message) {
     *    alert(message.payload+" on topic "+message.topic);
     * };
     * client.onconnect = function(rc) {
     *    if (rc == 0)
     *       client.subscribe("World");
     *       client.publish("World", "Hello world");
     *    } else {
     *       client.disconnect();
     *    }
     * };
     * client.connect();
     * </code>
     */
    var client_ctor = function(host, port, clientId, msgResendTimeout) {

        if ('MozWebSocket' in global && global['MozWebSocket'] !== null) {
            global['WebSocket'] = MozWebSocket;
        }
        if (!('WebSocket' in global && global['WebSocket'] !== null)) {
            throw (new Error("WebSockets is not supported by this browser"));
        }

        if (clientId.length == 0 || clientId.length > 23) {
            throw new Error("Invalid clientId.  ClientId must be between 1 and 23 characters long.");
        }

        this.host = host;
        this.port = port;
        this.clientId = clientId;

        this._tokens = {};

        // Messages we have received and acknowleged and are expecting a confirm
        // message for
        // indexed by their respective message ids.
        this._receivedMessages = {};

        // Unique identifier for PUBLISH messages, incrementing
        // counter as messages are sent.
        this._message_identifier = 1;

        // Internal list of callbacks to be notified when we
        // receive a message topic.
        this._topic_listeners = {};

        // Unique identifier for client, used for temporary topic creation
        this._unique = uniqueString();

        // Counter for generating unique temporary topics
        this._temporaryTopicCount = 0;

        this.onconnect = null;
        this.onconnectionlost = null;
        this.onmessage = null;
        this._socket = null;
        this._connected = false;
        this._disconnectedDone = true;
        this._pinger = null;
        this._tokenStore = null;
        if (msgResendTimeout) {
            this._msgResendTimeout = msgResendTimeout;
        } else {
            this._msgResendTimeout = 10000;
        }

        // Delivery Tracking Mode. Default is CLIENT.
        // If CLIENT mode, the JS Client will do the resend.
        // If APP mode, the application will do the resend.
        this._deliverytrackingmode = "CLIENT";
    };

    /**
     * @private Get the message delivery status.
     * @return the object which has the count for completed and pending
     *         messages.
     */
    client_ctor.prototype.getMessageDeliveryStatus = function() {
        if (this._tokenStore != null) {
            return this._tokenStore.getMsgDeliveryStats();
        } else {
            return null;
        }

    };

    /**
     * Callback for actions to take when a connection is established. This
     * function takes the MQTT CONNACK return code as an argument. If the return
     * code is not zero then the client connection is not yet fully functional.
     * If the application does not provide an onconnect callback function, then
     * a non-zero CONNACK return code will result in an automatic disconnect.
     * Otherwise, the client application's onconnect function must use the
     * non-zero return code to determine what action to take. The application
     * can either retry the call to connect() after making the necessary updates
     * to the connect request or the application can call disconnect().
     * 
     * @name IsmMqtt.Client#onconnect
     * @field
     * @example Example: Callback for connection established <code>
     * client.onconnect = function(rc) {
     *     if (rc == 0) {
     *         alert("Client connected");
     *     } else {
     *         alert("Connect returned "+rc+" client is disconnecting");
     *         client.disconnect();
     *     }
     * }
     * </code>
     */
    client_ctor.prototype.onconnect = null;

    /**
     * Callback for actions to take when the connection is unexpectedly lost.
     * The callback function receives a single string argument that identifies
     * the cause of the connection loss.
     * 
     * @name IsmMqtt.Client#onconnectionlost
     * @field
     * @example Example: Callback for connection lost <code>
     * client.onconnectionlost = function(reason) {
     *     alert("Client connection lost: "+reason);
     * }
     * </code>
     */
    client_ctor.prototype.onconnectionlost = null;

    /**
     * Callback for actions to take when a message arrives. The callback
     * function receives a single argument that is the message. The message is
     * an Object containing the following properties:
     * <dl>
     * <dt>topic</dt>
     * <dd>(String) the topic the message was published to.</dd>
     * <dt>payload</dt>
     * <dd>(String) the payload of the message.</dd>
     * <dt>qos</dt>
     * <dd>(Number) the quality of service for the message received.</dd>
     * <dt>retained</dt>
     * <dd>(Boolean) whether the message was retained.</dd>
     * </dl>
     * 
     * @name IsmMqtt.Client#onmessage
     * @field
     * @example Example: Callback for message received <code>
     * client.onmessage = function(message) {
     *     alert("Received: "+message.payload);
     * } 
     * </code>
     */
    client_ctor.prototype.onmessage = null;

    /**
     * Connect this MQTT client to the IBM Messaging Appliance server.
     * <br />
     * 
     * The following options can be passed to this function as members of the
     * connectOptions Object:
     * <dl>
     * <dt>clean (Boolean)</dt>
     * <dd> If true (default), the server should not persist any state
     * associated with the client. When the client disconnects, all messages
     * in-flight are deleted and all subscriptions are removed. If false, the
     * server persists this state between connections. <br/> Default: true </dd>
     * <dt>keepAliveInterval (Number)</dt>
     * <dd> The time in seconds of inactivity between client and server before
     * the connection is deemed to be broken. The client automatically sends a
     * heart-beat message to ensure the connection remains active. <br/>
     * Default: 60 seconds </dd>
     * <dt>userName (String)</dt>
     * <dd> Authentication username for this connection. </dd>
     * <dt>password (String)</dt>
     * <dd> Authentication password for this connection. </dd>
     * <dt>willTopic (String)</dt>
     * <dd>Topic to receive will message</dd>
     * <dt>willMessage (String)</dt>
     * <dd>Message to send if connection is lost unexpectedly</dd>
     * <dt>willQoS (Number)</dt>
     * <dd>The quality of service for the will message<br/>Default: 0</dd>
     * <dt>willRetain (Boolean)</dt>
     * <dd> Whether the will message that is sent should be retained after
     * sending.<br/> Default: false </dd>
     * </dl>
     * 
     * @example Example: Connect with no connection options <code>
     * client.connect();
     * </code>
     * @example Example: Connect with connection options <code>
     * var connopts = {
     *     userName:"uname",
     *     password:"pword",
     *     keepAliveInterval:90
     * }
     * client.connect(connopts);
     * </code>
     * @name IsmMqtt.Client#connect
     * @function
     * @param {Object}
     *            [connectOptions] optional attributes used with the connection.
     * @throws {Error}
     *             error thrown if the client is already connected
     * @throws {Error}
     *             error thrown if the specified willTopic contains any wildcard
     *             characters
     * @throws {Error}
     *             error thrown if an invalid value is specified for willQoS
     * @throws {Error}
     *             error thrown if an invalid value is specified for willRetain
     * @throws {Error}
     *             error thrown if an invalid value is specified for
     *             keepAliveInterval
     * @throws {Error}
     *             error thrown if an invalid value is specified for clean
     */
    client_ctor.prototype.connect = function(connectOptions) {
        if (this._connected) {
            throw new Error("Already Connected");
        }
        // If user hasn't provided any option, use empty obj.
        connectOptions = connectOptions || {};
        var keys = [ "userName", "password", "willTopic", "willMessage", "willQoS", "willRetain", "keepAliveInterval",
                "clean" ];

        this._connectMessage = {
            type: MESSAGE_TYPE.CONNECT,
            clientId: this.clientId
        };
        var key;
        for (key in connectOptions) {
            if (connectOptions.hasOwnProperty(key)) {
                if (keys.indexOf(key) == -1) {
                    var errorStr = "Unknown property: " + key + ". Valid properties include: \"" + keys.join("\", \"")
                            + "\".";
                    throw (new Error(errorStr));
                } else {
                    if (key == "willTopic") {
                        var topic = connectOptions[key];
                        if ((topic.indexOf("#") > -1) || (topic.indexOf("+") > -1))
                            throw new Error("Invalid will topic(" + topic
                                    + "). Publish topic cannot contain wildcard characters.");
                        this._connectMessage[key] = topic;
                    }
                    if (key === "willQoS") {
                        var willQoS = _validateQoS(connectOptions[key]);
                        this._connectMessage[key] = willQoS;
                    } else if (key === "willRetain") {
                        var willRetain = _validateRetain(connectOptions[key]);
                        this._connectMessage[key] = willRetain;
                    } else if (key === "clean") {
                        var cleanSession = connectOptions[key];
                        switch (cleanSession) {
                        case false:
                        case "false":
                            cleanSession = false;
                            break;
                        case true:
                        case "true":
                            cleanSession = true;
                            break;
                        default:
                            throw new Error("Invalid clean(" + cleanSession
                                    + ") specified.  Clean must be true (default) or false.");
                        }
                        this._connectMessage[key] = cleanSession;
                    } else
                        this._connectMessage[key] = connectOptions[key];
                }
            }
        }

        // If no keep alive interval is set in the connect options, the server
        // assumes 60 seconds.
        if (connectOptions.keepAliveInterval === undefined)
            this._pinger = new _Pinger(this, window, 60);
        else if (!isNaN(connectOptions.keepAliveInterval))
            this._pinger = new _Pinger(this, window, connectOptions.keepAliveInterval);
        else
            throw new Error("Invalid keepAliveInterval(" + connectOptions.keepAliveInterval
                    + ") specified.  KeepAliveInterval must be a number (default 60).");

        // If the socket is closed, attempt to open one.
        if (!this._socket_is_open()) {
            // When the socket is open,
            // this client will send the CONNECT message using the saved
            // parameters.
            this._openSocket();
        } else {
            this._send(this._connectMessage);
        }
    };

    /**
     * Disconnects this MQTT client from the IBM Messaging Appliance
     * server.
     * 
     * @name IsmMqtt.Client#disconnect
     * @function
     * @throws {Error}
     *             error thrown if the client is not connected
     */
    client_ctor.prototype.disconnect = function() {
        if (!(this._connected || this._socket_is_open())) {
            throw new Error("Not Connected");
        }
        var message = {
            type: MESSAGE_TYPE.DISCONNECT
        };
        this._send(message);

        /* Stop the token store timer */
        this._tokenStore.stopTokenStoreTaskTimer();
    };

    /**
     * Indicates whether the client is fully connected to the server.
     * 
     * @name IsmMqtt.Client#isConnected
     * @function
     * @return {boolean} true if the client has an active connection.
     */
    client_ctor.prototype.isConnected = function() {
        return this._connected;
    };

    /**
     * Publishes a message to the server.
     * 
     * @name IsmMqtt.Client#publish
     * @function
     * @param {String}
     *            topic the topic to publish to.
     * @param {String}
     *            payload the message content to publish.
     * @param {Number}
     *            [qos] optional setting for quality of service for the
     *            published message.<br/> Default: 0
     * @param {Boolean}
     *            [retained] optional setting for whether to retain the message
     *            after it is published.<br/> Default: false
     * @throws {Error}
     *             error thrown if the client is not connected
     * @throws {Error}
     *             error thrown if the topic contains any wildcard characters
     * @throws {Error}
     *             error thrown if an invalid QoS value is specified
     * @throws {Error}
     *             error thrown if an invalid retained value is specified
     * @example Example: Simple publish request <code>
     * client.publish("test_topic", "This is a test message");
     * </clode>
     * @example
     * Example: Publish request specifying all parameters
     * <code>
     * client.publish("test_topic", "This is a test message", 0, false);
     * </clode>
     */
    client_ctor.prototype.publish = function(topic, payload, qos, retained) {
        return this._publish(topic, payload, qos, retained, false);
    };

    /**
     * @private Internal function for publishing a message to the server.
     * 
     * @name IsmMqtt.Client#_publish
     * @function
     * @param {String}
     *            topic the topic to publish to.
     * @param {String}
     *            payload the message content to publish.
     * @param {Number}
     *            [qos] optional setting for quality of service for the
     *            published message.<br/> Default: 0
     * @param {Boolean}
     *            [retained] optional setting for whether to retain the message
     *            after it is published.<br/> Default: false
     * @param {Boolean}
     *            [dup] optional indicator of whether the message is a
     *            duplicate.<br/> Default: false
     * @param {Number}
     *            [msgid] message id for a duplicate message (required when dup
     *            is true).<br/>
     */
    client_ctor.prototype._publish = function(topic, payload, qos, retained, dup, msgid) {

        if (!this._connected) {
            throw new Error("Not Connected");
        }

        if ((topic.indexOf("#") > -1) || (topic.indexOf("+") > -1))
            throw new Error("Invalid publish topic(" + topic + "). Publish topic cannot contain wildcard characters.");

        var message = {
            type: MESSAGE_TYPE.PUBLISH,
            topic: topic,
            payload: payload
        };

        if (qos != undefined) {
            qos = _validateQoS(qos);
            if (qos)
                message['qos'] = qos;
        }

        if (retained != undefined) {
            retained = _validateRetain(retained);
            if (retained)
                message['retained'] = retained;
        }
        if (message.qos > 0) {
            message['dup'] = dup;
            if (dup == true) {
                message.id = msgid;
            }
        } else {
            // Set the default to false for dup
            message['dup'] = false;
        }
        // Save the message in a token
        if (message.qos > 0) {
            deliveryToken = this._tokenStore.saveToken(message);
            var d = new Date();
            var n = d.getTime();
            deliveryToken.lastsenttime = n;
            deliveryToken.msgResendTimeout = this._msgResendTimeout;
        } else
            deliveryToken = null;

        this._send(message);

        return deliveryToken;
    };

    /**
     * Generates a temporary topic.
     * 
     * 
     * @name IsmMqtt.Client#createTemporaryTopic
     * @function
     * @return {String} a string which can be used as a temporary topic
     */
    client_ctor.prototype.createTemporaryTopic = function() {
        this._temporaryTopicCount++;
        return "_TT/" + this._unique + this._temporaryTopicCount;
    };

    /**
     * Generates a correlation ID.
     * 
     * 
     * @name IsmMqtt.Client#generateCorrId
     * @function
     * @return {String} a string for use as a message correlation Id
     */
    client_ctor.prototype.generateCorrId = function() {
        var chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        var corrid = "";
        for ( var i = 0; i < 16; i++) {
            corrid += chars[Math.floor(Math.random() * 62)];
        }
        return corrid;
    };

    // TODO: Add automatic temp topic generation and subscription here
    // TODO: Add jsdoc example once this is added
    /**
     * Publishes a message to the server with a replyTo topic and correlation
     * id. If a replyTo topic is not specified, a temporary topic is generated
     * and a subscription is opened. When subscribed to, with an unsubscribe
     * following receipt of the message. If a replyTo topic is specified but not
     * subscribed to, a subscription will be attempted. If corrId is not
     * provided, a random string will be generated.
     * 
     * 
     * @name IsmMqtt.Client#publishWithReply
     * @function
     * @param {String}
     *            [topic] the topic to publish to.
     * @param {String}
     *            [payload] the message content to publish.
     * @param {String}
     *            [replyTo] (optional) the topic on which the response will be
     *            sent.
     * @param {String}
     *            [corrId] (optional) correlation ID.
     * @param {Number}
     *            [qos] (optional) the quality of service for the published
     *            message.<br/> Default: 0
     * @param {Boolean}
     *            [retained] (optional) whether to retained the message after it
     *            is published.<br/> Default: false
     * @throws {Error}
     *             error thrown if the client is not connected
     * @throws {Error}
     *             error thrown if the topic contains any wildcard characters
     * @throws {Error}
     *             error thrown if the replyto topic contains any wildcard
     *             characters
     * @throws {Error}
     *             error thrown if an invalid QoS value is specified
     * @throws {Error}
     *             error thrown if an invalid retained value is specified
     */
    client_ctor.prototype.publishWithReply = function(topic, payload, replyto, corrid, qos, retained) {
        return this._publishWithReply(topic, payload, replyto, corrid, qos, retained, false);
    };

    /*
     * Internal Publishes a message to the server with a replyTo topic and
     * correlation id. If a replyTo topic is not specified, a temporary topic is
     * generated and a subscription is opened. When subscribed to, with an
     * unsubscribe following receipt of the message. If a replyTo topic is
     * specified but not subscribed to, a subscription will be attempted. If
     * corrId is not provided, a random string will be generated.
     * 
     * 
     * @name IsmMqtt.Client#publishWithReply @function @param {String} [topic]
     * the topic to publish to. @param {String} [payload] the message content to
     * publish. @param {String} [replyTo] (optional) the topic on which the
     * response will be sent. @param {String} [corrId] (optional) correlation
     * ID. @param {Number} [qos] (optional) the quality of service for the
     * published message.<br/> Default: 0 @param {Boolean} [retained]
     * (optional) whether to retained the message after it is published.<br/>
     * Default: false @param {Boolean} [dup] (optional) indicate the message is
     * a duplicate.<br/> Default: false @param {Number} [msgid] (optional) the
     * message id for the duplicate message.<br/>
     */
    client_ctor.prototype._publishWithReply = function(topic, payload, replyto, corrid, qos, retained, dup, msgid) {

        if (!this._connected) {
            throw new Error("Not Connected");
        }

        if ((topic.indexOf("#") > -1) || (topic.indexOf("+") > -1))
            throw new Error("Invalid publish topic(" + topic + "). Publish topic cannot contain wildcard characters.");

        var message = {
            type: MESSAGE_TYPE.PUBLISH,
            topic: topic
        };

        message['payload'] = payload;

        if (replyto && replyto != "") {
            if ((replyto.indexOf("#") > -1) || (replyto.indexOf("+") > -1))
                throw new Error("Invalid publish replyto topic(" + topic
                        + "). Publish topic cannot contain wildcard characters.");
            message['replyto'] = replyto;
        }

        if (corrid && corrid != "") {
            message['corrid'] = corrid;
        }

        if (qos != undefined) {
            qos = _validateQoS(qos);
            if (qos)
                message['qos'] = qos;
        }

        if (retained != undefined) {
            retained = _validateRetain(retained);
            if (retained)
                message['retained'] = retained;
        }

        if (message.qos > 0) {
            message['dup'] = dup;
            if (dup == true) {
                message.id = msgid;
            }
        } else {
            // Set the default to false for dup
            message['dup'] = false;
        }

        if (message.qos > 0) {
            // Save the message into a token
            deliveryToken = this._tokenStore.saveToken(message);
            var d = new Date();
            var n = d.getTime();
            deliveryToken.lastsenttime = n;
            deliveryToken.msgResendTimeout = this._msgResendTimeout;
        } else
            deliveryToken = null;

        this._send(message);

        return deliveryToken;
    };

    /**
     * Subscribe to a topic.
     * 
     * @name IsmMqtt.Client#subscribe
     * @function
     * @param {String}
     *            topicFilter the topic filter to subscribe to. May contain
     *            wildcards if no topic callback is specified.
     * @param {Number}
     *            [qos] optional setting for the requested quality of service
     *            for the subscription.<br/> Default: 0<br/> Note: The server
     *            might grant a lower quality of service than requested.
     * @param {Function}
     *            [subscribeCompleteCallback] optional callback for actions to
     *            take when the subscription request is complete.
     * @throws {Error}
     *             error thrown if the client is not connected
     * @throws {Error}
     *             error thrown if an invalid QoS value is specified
     * @throws {Error}
     *             error thrown if the subscribeCompleteCallback argument is set
     *             and references content that is not a function.
     * @example Example: Subscribe to a topic <code>
     * client.subscribe("test_topic");
     * </code>
     * @example Example: Subscribe to a topic specifying topic callback for
     *          published messages and subscribeCompleteCallback for
     *          subscription request. <code>  
     * var sccallback = function() {
     *     alert("subscription request complete for "+this.topic);
     * };  
     * subscribe("test_topic", 0, sccallback);
     * </code>
     */
    client_ctor.prototype.subscribe = function(topicFilter, qos, subscribeCompleteCallback) {
        if (!this._connected) {
            throw new Error("Not Connected");
        }

        if (qos != undefined) {
            qos = _validateQoS(qos);
        }

        var message = {
            type: MESSAGE_TYPE.SUBSCRIBE,
            topic: topicFilter,
            qos: qos
        };

        if (subscribeCompleteCallback && typeof subscribeCompleteCallback == "function") {
            message.callback = subscribeCompleteCallback;
        } else if (subscribeCompleteCallback) {
            throw new Error("Invalid callback. The subscribeCompleteCallback must be a function.");
        }

        deliveryToken = this._tokenStore.saveToken(message);
        var d = new Date();
        var n = d.getTime();
        deliveryToken.lastsenttime = n;
        deliveryToken.msgResendTimeout = this._msgResendTimeout;

        this._send(message);

        return deliveryToken;

    };

    /**
     * Unsubscribe from a topic.
     * 
     * @name IsmMqtt.Client#unsubscribe
     * @function
     * @param {String}
     *            topicFilter the topic filter to unsubscribe to. May contain
     *            wildcards and must match a subscribed topic filter or the
     *            unsubscribe action will have no effect.
     * @param {Function}
     *            [unsubscribeCompleteCallback] optional callback for actions to
     *            take when the unsubscribe request is complete.
     * @throws {Error}
     *             error thrown if the client is not connected
     * @throws {Error}
     *             error thrown if the unsubscribeCompleteCallback argument is
     *             set and references content that is not a function.
     */
    client_ctor.prototype.unsubscribe = function(topicFilter, unsubscribeCompleteCallback) {
        if (!this._connected) {
            throw new Error("Not Connected");
        }

        var message = {
            type: MESSAGE_TYPE.UNSUBSCRIBE,
            topic: topicFilter
        };

        if (unsubscribeCompleteCallback && typeof unsubscribeCompleteCallback == "function") {
            message.callback = unsubscribeCompleteCallback;
        } else if (unsubscribeCompleteCallback) {
            throw new Error("Invalid callback. The unsubscribeCompleteCallback must be a function.");
        }

        deliveryToken = this._tokenStore.saveToken(message);
        var d = new Date();
        var n = d.getTime();
        deliveryToken.lastsenttime = n;
        deliveryToken.msgResendTimeout = this._msgResendTimeout;

        this._send(message);

        return deliveryToken;
    };

    /**
     * Add a listener for a topic.
     * 
     * @name IsmMqtt.Client#addTopicListener
     * @function
     * @description The addTopicListener function specifies a callback function
     *              to be invoked when messages are received for a particular
     *              topic. In order for the callback to be invoked, the topic
     *              specified must match a subscribed topic filter. If
     *              specified, the client's onmessage callback will be executed
     *              for a given message before any topic listeners. If more than
     *              one listener is specified for a given topic, the order in
     *              which each listener is executed is non-deterministic.
     * @param {String}
     *            topic the topic for which the callback is to be invoked. This
     *            topic string must not contain wildcards.
     * @param {Function}
     *            callback callback for actions to take when messages are
     *            received for this topic.
     * @throws {Error}
     *             error thrown if the topic specified includes any wildcards.
     * @throws {Error}
     *             error thrown if the callback argument is not provided or if
     *             it is not a function.
     */
    client_ctor.prototype.addTopicListener = function(topic, callback) {
        if ((topic.indexOf("#") > -1) || (topic.indexOf("+") > -1))
            throw new Error("Invalid listener topic(" + topic
                    + "). The topic for a topic listener cannot contain wildcard characters.");
        if (callback && typeof callback == "function") {
            this._add_listener(topic, callback);
        } else {
            throw new Error("Invalid callback.  A callback must be specified and must be a function.");
        }
    };

    /**
     * Remove one or all listeners for a topic.
     * 
     * @name IsmMqtt.Client#removeTopicListener
     * @function
     * @description The removeTopicListener function removes a previously added
     *              callback function from a list of callbacks associated with a
     *              specific topic. If this function is called and there are no
     *              matches are found for the topic/callback pair, no error is
     *              reported.
     * @param {String}
     *            topic the topic for which the specified callback will no
     *            longer be invoked.
     * @param {Function}
     *            [callback] optional name of a callback function to remove from
     *            the list of callbacks for the specified topic. If no callback
     *            is specified, then all callbacks associated with the topic are
     *            removed.
     * @returns {Boolean} true if there are still callbacks for the topic, false
     *          otherwise.
     * @throws {Error}
     *             error thrown if the topic specified includes any wildcards.
     */
    client_ctor.prototype.removeTopicListener = function(topic, callback) {
        if ((topic.indexOf("#") > -1) || (topic.indexOf("+") > -1))
            throw new Error("Invalid listener topic(" + topic
                    + "). The topic for a topic listener cannot contain wildcard characters.");
        return this._remove_listener(topic, callback);
    };

    // MQTT Client private instance methods.
    // -------------------------------------

    // Remove subscription callback from the list of callbacks registered
    // for a valid topic. Returns boolean indicating whether topic still has
    // listeners
    //  
    client_ctor.prototype._remove_listener = function(topic, callback) {
        if (callback) {
            var tl = this._topic_listeners[topic];
            var idx = !!tl ? tl.indexOf(callback) : -1;
            if (idx !== -1) {
                tl.splice(idx, 1);
            }
            return tl.length !== 0;
        } else {
            delete this._topic_listeners[topic];
        }
        return false;
    };

    // Register a callback function to be notified when a messaged is
    // received on the following topic string. If there are already
    // listeners for this topic, push callback into the queue, otherwise
    // create new list with callback as sole member
    client_ctor.prototype._add_listener = function(topic, callback) {
        if (!this._topic_listeners[topic]) {
            this._topic_listeners[topic] = [ callback ];
        } else {
            // Convert functions to string form to assure the same function
            // is not added twice.
            if (String(this._topic_listeners[topic]).indexOf(String(callback)) == -1) {
                this._topic_listeners[topic].push(callback);
            }
            // Else: This callback is already in the array. No need
            // to add it again.
        }
    };

    // Socket's been opened and connected to remote server
    client_ctor.prototype._socket_is_open = function() {
        return this._socket !== null && this._socket.readyState === WebSocket.OPEN;
    };

    // Initiate the websocket connection.
    client_ctor.prototype._openSocket = function() {
        var wsurl = [ "ws://", this.host, ":", this.port, "/mqtt" ].join("");
        this._connected = false;
        this._disconnectedDone = false;
        this._socket = new WebSocket(wsurl, "mqtt");
        this._socket.onopen = scope(this._on_socket_open, this);
        this._socket.onmessage = scope(this._on_socket_message, this);
        this._socket.onerror = scope(this._on_socket_error, this);
        this._socket.onclose = scope(this._on_socket_close, this);
    };

    // Called when the underlying websocket has been opened.
    client_ctor.prototype._on_socket_open = function() {
        this._send(this._connectMessage);
    };

    // Called when the underlying websocket has received a complete packet.
    client_ctor.prototype._on_socket_message = function(event) {

        var message = JSON.parse(event.data);

        switch (message.type) {
        case MESSAGE_TYPE.CONNACK:
            /* Start Task timer */
            // Start token store task timer for this client
            this._tokenStore = new _TokenStore(this, window);

            if (message['rc'] == 0) {
                this._connected = true;
                if (this.onconnect !== null && typeof this.onconnect == "function") {
                    this.onconnect(0);
                }
                // localStorage.clear();
                if (this._connectMessage['clean'] && this._connectMessage['clean'] == false) {
                    this._tokenStore.restoreTokensInPersistentStore();
                } else {
                    this._tokenStore.cleanTokensInPersistentStore();
                }
                // Start Task Timer
                this._tokenStore.startTokenStoreTaskTimer();
            } else if (this.onconnect !== null && typeof this.onconnect == "function") {
                this.onconnect(message['rc']);
            } else { // Client application did not provide an onconnect
                // function
                console.log("CONNACK returned " + message['rc']);
                this.disconnect();
            }
            break;

        case MESSAGE_TYPE.SUBACK:
            var token = this._tokenStore.getToken(message);
            if (token != null) {
                if (token.message.callback !== null && token.message.callback !== undefined) {
                    token.message.callback();
                }
                this._tokenStore.removeToken(message);
            }

            break;

        case MESSAGE_TYPE.UNSUBACK:
            var token = this._tokenStore.getToken(message);
            if (token != null) {
                var topic = token.message.topic;

                if (token.message.callback !== null && token.message.callback !== undefined) {
                    token.message.callback();
                }
                this._tokenStore.removeToken(message);
            }
            break;

        case MESSAGE_TYPE.PUBACK:
            this._tokenStore.removeToken(message);
            break;
        case MESSAGE_TYPE.PUBREC:
            var token = this._tokenStore.getToken(message);
            if (token != null) {
                // Mark the token that the PUBREC is received.
                token.pubrecreceived = true;
                var pubrelMessage = {
                    type: MESSAGE_TYPE.PUBREL,
                    id: message.id
                };
                this._send(pubrelMessage);
            }
            break;

        case MESSAGE_TYPE.PUBCOMP:
            this._tokenStore.removeToken(message);
            break;

        case MESSAGE_TYPE.PUBREL:
            var receivedMessage = this._receivedMessages[message.id];
            if (receivedMessage !== null) {
                this._receiveMessage(receivedMessage);
            }
            var ackMessage = {
                type: MESSAGE_TYPE.PUBCOMP,
                id: message.id
            };
            this._send(ackMessage);
            delete this._receivedMessages[message.id];
            break;

        case MESSAGE_TYPE.PUBLISH:
            this._receiveSend(message);
            break;

        case MESSAGE_TYPE.PINGRESP:
            this._pinger.responseReceived();
            break;

        default:
            throw new Error("Invalid message type:" + message.type);
        }
    };

    client_ctor.prototype._on_socket_error = function() {
        this._disconnected("Socket error");
    };

    client_ctor.prototype._on_socket_close = function() {
        this._disconnected("Socket closed");
    };

    client_ctor.prototype._send = function(message) {
        var JSONMessage = JSON.stringify(message);
        // console.log("_send (" + message + "): " + JSONMessage);
        this._socket.send(JSONMessage);
        this._pinger.reset();
        if (message.type == MESSAGE_TYPE.DISCONNECT) {
            this._disconnectedDone = true;
            this._disconnected();
        }
    };

    client_ctor.prototype._receiveSend = function(message) {
        switch (message.qos) {
        case undefined:
        case 0:
            this._receiveMessage(message);
            break;

        case 1:
            var ackMessage = {
                type: MESSAGE_TYPE.PUBACK,
                id: message.id
            };
            this._send(ackMessage);
            this._receiveMessage(message);
            break;

        case 2:
            this._receivedMessages[message.id] = message;
            var ackMessage = {
                type: MESSAGE_TYPE.PUBREC,
                id: message.id
            };
            this._send(ackMessage);
            break;

        default:
            throw new Error("Invalid qos=" + message.qos);
        }
    };

    client_ctor.prototype._receiveMessage = function(message) {
        // Extract topic and message from MQTT packet
        var topic = message.topic, data = message.payload, listeners = this._topic_listeners[topic], i = 0;

        if (this.onmessage !== null && typeof this.onmessage == "function") {
            this.onmessage(message);
        }

        // If we have some registered listeners, iterate through the list,
        // executing each with the message content.
        while (listeners && i < listeners.length) {
            listeners[i](message);
            i++;
        }
    };

    // Client has disconnected either at its own request or because the server
    // or network disconnected it. Remove all non-durable state.
    client_ctor.prototype._disconnected = function(reason) {
        this._pinger.cancel();
        // Execute the connectionLostCallback if there is one, and we are
        // connected.
        if (this.onconnectionlost !== null && typeof this.onconnectionlost == "function" && !this._disconnectedDone) {
            if (!(this._connected))
                reason = "Failed to connect";
            this.onconnectionlost(reason);
        }
        if (this._socket_is_open()) {
            this._socket.close();
        }
        this._connected = false;
        this._disconnectedDone = true;
        this._socket = null;
    };

    return {
        Client: client_ctor
    };

    /** @private */
    function _validateQoS(qos) {
        switch (qos) {
        case 0:
        case "0":
            return 0;
        case 1:
        case "1":
            return 1;
        case 2:
        case "2":
            return 2;
        default:
            throw new Error("Invalid QoS(" + qos + ") specified.  QoS must be 0 (default), 1 or 2.");
        }
    }
    ;

    /** @private */
    function _validateRetain(retain) {
        switch (retain) {
        case false:
        case "false":
            return false;
        case true:
        case "true":
            return true;
        default:
            throw new Error("Invalid retain(" + retain + ") specified.  Retain must be false (default) or true.");
        }
    }
    ;
})(window);
