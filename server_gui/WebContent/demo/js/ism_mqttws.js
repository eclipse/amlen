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
* @overview An MQTT client, for sending and receiving MQTT v3 messages using a WebSocket gateway.
* @name MQTT Client.
*/

/**
* @namespace MQTT
*/
MQTT = (function (global) {
      var MESSAGE_TYPE = {
         CONNECT:"CONNECT",
         CONNACK:"CONNACK",
         PUBLISH:"PUBLISH",
         PUBACK:"PUBACK",
         PUBREC:"PUBREC",
         PUBREL:"PUBREL",
         PUBCOMP:"PUBCOMP",
         SUBSCRIBE:"SUBSCRIBE",
         SUBACK:"SUBACK",
         UNSUBSCRIBE:"UNSUBSCRIBE",
         UNSUBACK:"UNSUBACK",
         PINGREQ:"PINGREQ",
         PINGRESP:"PINGRESP",
         DISCONNECT:"DISCONNECT"
      }

      
      
      /**
      * Return a new function which runs the user function bound
      * to a fixed scope. 
      * @param {Function} User function
      * @param {Object} Function scope  
      * @return {Function} User function bound to another scope
      * @private 
      */
      var scope = function (f, scope) {
         return function () {
            return f.apply(scope, arguments);
         };
      };
      
      /** @ignore */
      _Pinger = (function() { 
            var _client;        	
            var _window;
            var _keepAliveInterval;
            
            var JSONMessage = JSON.stringify({type: MESSAGE_TYPE.PINGREQ});
            var timeout = null;
            var pingOutstanding = false;
            
            var Pinger_ctor = function (client,window,keepAliveInterval) {
               _client = client;        	
               _window = window;
               _keepAliveInterval = keepAliveInterval*1000;
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
            }
            Pinger_ctor.prototype.reset = function() {
               _window.clearTimeout(timeout);
               if (_keepAliveInterval > 0) {
                  timeout = setTimeout(doPing, _keepAliveInterval);
               }
            }
            
            Pinger_ctor.prototype.responseReceived = function() {
               pingOutstanding = false;
            }
            
            Pinger_ctor.prototype.cancel = function() {
               _window.clearTimeout(timeout);
               timeout = null;
            }
            
            return Pinger_ctor;
      }) (); 
      

      /** @ignore */
      function uniqueString() {
         var unique = "";
         var chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
         for (var i = 0; i < 5; i++) {
             unique += chars[Math.floor(Math.random() * 62)];
         }
         //console.log("Unique string: " + unique);
         return unique;
      }

      /** 
      * An MQTT client, for sending and receiving MQTT v3 messages using a
      * WebSocket gateway.
      *
      * @example
      * <code>
      * var client = new MQTT.Client(mqtthost.co.uk, 80, "clientName");
      * client.onmessage = function(message) {
      *    alert(message.payload+" on topic "+message.topic);
      * };
      * client.onconnect = function() {
      *    client.subscribe("World");
      *    client.publish("World", "Hello world");
      * };
      * client.connect();
      * </code>
      * @name MQTT.Client
      * @constructor
      * @param {String} host the address of the WebSocket gateway.
      * @param {Number} port the port number of the WebSocket gateway.
      * @param {String} clientId the MQTT client identifier.
      */
      var client_ctor = function (host, port, clientId) {
         
         if ('MozWebSocket' in global && global['MozWebSocket'] !== null) {
            global['WebSocket'] = MozWebSocket;
         }
         if (!('WebSocket' in global && global['WebSocket'] !== null)) {
            throw (new Error("WebSockets is not supported by this browser"));
         } 
         
         this.host = host;
         this.port = port;
         this.clientId = clientId;
         
         // Messages we have sent and expecting a response for
         // indexed by their respective message ids. 
         this._sentMessages = {};
         
         // Messages we have received and acknowleged and are expecting a confirm message for
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
      };
      
      /**
       * Settable callback for when a connection is established.
       * @name MQTT.Client#onconnect
       * @field
       */
      client_ctor.prototype.onconnect = null;
      
      /**
       * Settable callback for when the connection is unexpectedly lost. The callback
       * function receives a single argument that identifies the cause of the connection loss.
       * @name MQTT.Client#onconnectionlost
       * @field
       */
      client_ctor.prototype.onconnectionlost = null;

      /**
       * Settable callback for when a message arrives. The callback function receives
       * a single argument that is the message. The message is an Object containing the following
       * properties:
       * <dl>
       * <dt>topic</dt><dd>(String) the topic the message was published to.</dd>
       * <dt>payload</dt><dd>(String) the payload of the message.</dd>
       * <dt>qos</dt><dd>(Number) the qos the message was received at.</dd>
       * <dt>retained</dt><dd>(Boolean) whether the message was retained.</dd>
       * </dl>
       * @name MQTT.Client#onmessage
       * @field
       */
      client_ctor.prototype.onmessage = null;

      /** 
      * Connect this MQTT client to its server. <br />
      * 
      * The following options can be passed to this function as members of the connectOptions
      * Object:
      * <dl>
      * <dt>cleanSession (Boolean)</dt><dd>
      *         If true (default), the server should not persistent state any state associated with 
      *         the client. When the client disconnects, all messages in-flight are deleted and all 
      *         subscriptions are removed. <br/>
      *         If false, the server persists this state between connections
      * </dd>
      * <dt>keepAliveInterval (Number)</dt><dd>
      *         The number of seconds of inactivity between client and server before the connection
      *         is deemed to be broken. The client automatically sends a heart-beat message to ensure
      *         the connection remains active. <br/>
      *         If not specified, a default of value of 60 seconds is used.
      * </dd>
      * <dt>userName (String)</dt><dd>
      *         Authentication username for this connection.
      * </dd>
      * <dt>password (String)</dt><dd>
      *         Authentication password for this connection.
      * </dd>
      * <dt>willTopic (String)</dt>
      * <dt>willMessage (String)</dt>
      * <dt>willQoS (Number, default: 0)</dt>
      * <dt>willRetain (Boolean, default false)</dt><dd>
      *         The details of a Will message, to be sent by the server if the client disconnects
      *         abnormally.
      * </dd>
      * </dl>
      *
      * @name MQTT.Client#connect
      * @function
      * @param {Object} [connectOptions] (optional) attributes used with the connection.
      */
      client_ctor.prototype.connect = function (connectOptions) {
         if (this._connected) {
            throw new Error("Already Connected");
         }
         // If user hasn't provided any option, use empty obj.
         connectOptions = connectOptions || {}; 
         var keys = [ "userName", "password", "willTopic", "willMessage",
         "willQoS","willRetain","keepAliveInterval","cleanSession" ];
         
         this._connectMessage = {type: MESSAGE_TYPE.CONNECT, clientId: this.clientId};
         var key;
         for(key in connectOptions) {
            if (connectOptions.hasOwnProperty(key)) {
               if (keys.indexOf(key) == -1) {
                  var errorStr = "Unknown property: " + key +". Valid properties include: \"" + keys.join("\", \"") + "\".";
                  throw (new Error(errorStr));
               } else {
                  this._connectMessage[key] = connectOptions[key];
               }
            }
         }
         
         // If no keep alive interval is set in the connect options, the server assumes 60 seconds.
         if (connectOptions.keepAliveInterval === undefined)
            this._pinger = new _Pinger(this, window, 60);
         else
            this._pinger = new _Pinger(this, window, connectOptions.keepAliveInterval);
         
         // If the socket is closed, attempt to open one.
         if (!this._socket_is_open()) {
            // When the socket is open, 
            // this client will send the CONNECT message using the saved parameters. 
            this._openSocket();
         }             
      };
      
      /** 
      * Disconnect this MQTT client from its server.
      * 
      * @name MQTT.Client#disconnect
      * @function
      */
      client_ctor.prototype.disconnect = function () {
         if (!this._connected) {
            throw new Error("Not Connected");
         }
         var message = {type: MESSAGE_TYPE.DISCONNECT};
         this._send(message);
      };
      
      /** 
      * Indicates if the client is connected to its server.
      * 
      * @name MQTT.Client#isConnected
      * @function
      * @return {boolean} true if the client has an active connection.
      */
      client_ctor.prototype.isConnected = function () {        
         return this._connected;
      };
      
      /** 
      * Publishes a message to the server.
      * 
      * @name MQTT.Client#publish
      * @function
      * @param {String} [topic] the topic to publish to.
      * @param {String} [payload] the payload to publish.
      * @param {Number} [qos] (optional) the qos to publish at. Default: 0
      * @param {Boolean} [retained] (Optional) whether to publish a retained message. Default: false
      */
      client_ctor.prototype.publish = function (topic, payload, qos, retained) {
         
         if (!this._connected) {
            throw new Error("Not Connected");
         }

         qos = qos || 0;
         
         var message = {
            type: MESSAGE_TYPE.PUBLISH,
            topic: topic,
            payload: payload,
         };
         
         if (qos && qos > 0) {
            message['qos'] = qos;
         }
         if (retained) {
            message['retained'] = true;
         }
         
         if (message.qos > 0) {
            this._requires_ack(message);
         }
         
         this._send(message);
      };
      

      /**
       * Generates a temporary topic (optionally, subscribes)
       *
       * @name MQTT.Client#createTemporaryTopic
       * @function
       * @return {String} a string which can be used as a temporary topic
       */
      client_ctor.prototype.createTemporaryTopic = function () {
          this._temporaryTopicCount++;
          return "_TT/" + this._unique + this._temporaryTopicCount;
      }

      /** 
      * Publishes a message to the server with a replyTo topic and correlation
      * id.  If replyTo is not specified, a temporary topic will be generated
      * and subscribed to, with an unsubscribe following receipt of the
      * message.  If replyTo is specified but not subscribed to, a subscription
      * will be attempted.  If corrId is not provided, a random string
      * will be generated.
      *
      * When 
      * 
      * @name MQTT.Client#publish
      * @function
      * @param {String} [topic] the topic to publish to.
      * @param {String} [payload] the payload to publish.
      * @param {String} [replyTo] (optional) the topic on which the response will be sent.
      * @param {String} [corrId] (optional) correlation ID. 
      * @param {Number} [qos] (optional) the qos to publish at. Default: 0
      * @param {Boolean} [retained] (optional) whether to publish a retained message. Default: false
      */
      client_ctor.prototype.publishWithReply = function (topic, payload, replyto, corrid, qos, retained) {
         
         if (!this._connected) {
            throw new Error("Not Connected");
         }

         qos = qos || 0;
         
         var message = {
            type: MESSAGE_TYPE.PUBLISH,
            topic: topic,
         };

         message['payload'] = payload;

         if (replyto && replyto != "") {
            message['replyto'] = replyto;
         }
         
         if (corrid && corrid != "") {
            message['corrid'] = corrid;
         }
         
         if (qos && qos > 0) {
            message['qos'] = qos;
         }
         if (retained) {
            message['retained'] = true;
         }
         
         if (message.qos > 0) {
            this._requires_ack(message);
         }
         
         this._send(message);
      };
      
      /** 
      * Subscribe to a topic.
      * 
      * @name MQTT.Client#subscribe
      * @function
      * @param {String} [topicFilter] the topic filter to subscribe to. May contain wildcards.
      * @param {Function} [callback] (optional) called when a message is delivered for the topic.
      * @param {Number} [qos] (optional) the qos to subscribe at. Default: 0 
      * @param {Function} [subscribeCompleteCallback] (optional) called when the subscription is complete.     
      */
      client_ctor.prototype.subscribe = function (topicFilter, callback, qos, subscribeCompleteCallback) {
         if (!this._connected) {
            throw new Error("Not Connected");
         }
         qos = qos || 0;
         var message = {
            type: MESSAGE_TYPE.SUBSCRIBE,
            topic: topicFilter,
            qos: qos
         };
         
         // All subscriptions return an ACK or NACK. 
         this._requires_ack(message);
         
         if (callback && typeof callback == "function") {
            this._add_listener(topicFilter, callback);
         }
         
         if (subscribeCompleteCallback && typeof subscribeCompleteCallback == "function") {
            message.callback = subscribeCompleteCallback;
         }
         this._send(message);
      };
      
      /**
      * Unsubscribe from a topic.
      *
      * @name MQTT.Client#unsubscribe
      * @function
      * @param {String} [topicFilter] the topic filter to unsubscribe from. 
      * @param {Function} [callback] (optional) the callback previously registered for this topic. If
      *        none is specified, all registered callbacks for this topic are removed.
      */
      client_ctor.prototype.unsubscribe = function (topicFilter, callback) {
         if (!this._connected) {
            throw new Error("Not Connected");
         }
         // If we have a callback remove from listeners and check is listeners
         // are empty
         this._remove_listener(topicFilter, callback);
         
         var message = {
            type: MESSAGE_TYPE.UNSUBSCRIBE,
            topic: topicFilter,
            qos: 0
            // was this:  // qos: qos
         };
         
         this._requires_ack(message);
         if (callback && typeof callback == "function") {
            message.callback = callback;
         }
         this._send(message);
      };
      
      // MQTT Client private instance methods.
      // -------------------------------------
      
      
      // Remove subscription callback from the list of callbacks registered
      // for a valid topic. Returns boolean indicating whether topic still has
      // listeners
      //  
      client_ctor.prototype._remove_listener = function (topic, callback) {
         if (callback) {
            var tl = this._topic_listeners[topic];
            var idx = !!tl ? tl.indexOf(callback) : -1;
            if (idx !== -1) {
               tl.splice(idx, 1);    
            }
            return tl.length === 0;
         } else {
            delete this._topic_listeners[topic];
         }
         return false;
      };
      
      // Register a callback function to be notified when a messaged is
      // received on the following topic string. If there are already
      // listeners for this topic, push callback into the queue, otherwise
      // create new list with callback as sole member
      client_ctor.prototype._add_listener = function (topic, callback) {
         this._topic_listeners[topic] ? this._topic_listeners[topic].push(callback) : 
         this._topic_listeners[topic] = [callback];
      };
      
      // Socket's been open and connected to remote server
      client_ctor.prototype._socket_is_open = function () {
         return this._socket !== null && 
         this._socket.readyState === WebSocket.OPEN;
      };
      
      // Expect an ACK response for this message. Add message to the set of in progress
      // messages and set an unused identifier in this message.
      client_ctor.prototype._requires_ack = function (message) {
         if (this._sentMessages.length > 65536) {
            throw Error ("Too many messages in-flight:"+this._sentMessages.length);
         }
         while(this._sentMessages[this._message_identifier] !== undefined) {
            this._message_identifier++;
         }
         message.id = this._message_identifier;
         this._sentMessages[message.id] = message;
         if (this._message_identifier === 65536) {
            this._message_identifier = 1;
         }
      };
      
      // Initiate the websocket connection.
      client_ctor.prototype._openSocket = function () {
         if (window.ismPortSecure && window.ismPortSecure == true) {
            var wsurl = ["wss://", this.host, ":", this.port, "/mqtt-json"].join("");
         } else {
            var wsurl = ["ws://", this.host, ":", this.port, "/mqtt-json"].join("");
         }
         this._connected = false;
         this._disconnectedDone = false;
         this._socket = new WebSocket(wsurl, "mqtt-json");
         this._socket.onopen = scope(this._on_socket_open, this);
         this._socket.onmessage = scope(this._on_socket_message, this);
         this._socket.onerror = scope(this._on_socket_error, this);
         this._socket.onclose = scope(this._on_socket_close, this);
      };
      
      // Called when the underlying websocket has been opened.
      client_ctor.prototype._on_socket_open = function () {        
         this._send(this._connectMessage);
      };
      
      // Called when the underlying websocket has received a complete packet.
      client_ctor.prototype._on_socket_message = function (event) {
         
         var message = JSON.parse(event.data);
         
         switch(message.type) {
         case MESSAGE_TYPE.CONNACK:
            this._connected = true;
            if (this.onconnect !== null && typeof this.onconnect == "function") {
               this.onconnect();
            }
            break;
            
         case MESSAGE_TYPE.SUBACK:
            var sentMessage = this._sentMessages[message.id];
            if (sentMessage !== null) {
               if (sentMessage.callback !== null && sentMessage.callback !== undefined) {
                  sentMessage.callback();
               }
               delete this._sentMessages[message.id];
            }
            break;
            
         case MESSAGE_TYPE.UNSUBACK:
            var sentMessage = this._sentMessages[message.id];
            if (sentMessage !== null) {
               var topic = sentMessage.topic;
               this._topic_listeners[topic] = null;
               
               if (sentMessage.callback !== null && sentMessage.callback !== undefined) {
                  sentMessage.callback();
               }
               delete this._sentMessages[message.id];
            }           
            break;
         case MESSAGE_TYPE.PUBACK:
            delete this._sentMessages[message.id];
            break;
         case MESSAGE_TYPE.PUBREC:                      
            var pubrelMessage = {type: MESSAGE_TYPE.PUBREL, id:message.id};
            this._sentMessages[message.id] = pubrelMessage;
            this._send(pubrelMessage);
            break;
         case MESSAGE_TYPE.PUBCOMP:
            delete this._sentMessages[message.id];
            break;
         case MESSAGE_TYPE.PUBREL:
            var receivedMessage = this._receivedMessages[message.id];
            if (receivedMessage !== null) {
               this._receiveMessage(receivedMessage);
            }
            var ackMessage = {type:MESSAGE_TYPE.PUBCOMP,id:message.id};
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
            throw new Error("Invalid message type:"+message.type); 
         } 
      };
      
      client_ctor.prototype._on_socket_error = function (error) {
         this._disconnected("Socker error: "+error);
      };
      
      client_ctor.prototype._on_socket_close = function () {
         this._disconnected("Socket closed");
      };
      
      client_ctor.prototype._send = function (message) {
         var JSONMessage = JSON.stringify(message); 
         //console.log("_send (" + message + "):      " + JSONMessage);
         this._socket.send(JSONMessage); 
         this._pinger.reset();
         if (message.type == MESSAGE_TYPE.DISCONNECT) {
            this._disconnectedDone = true;
            this._disconnected();
         }
      };
      
      client_ctor.prototype._receiveSend = function (message) {
         switch(message.qos) {
         case undefined:
         case 0:
            this._receiveMessage(message);
            break;
            
         case 1:
            var ackMessage = {type:MESSAGE_TYPE.PUBACK,id:message.id};
            this._send(ackMessage);
            this._receiveMessage(message);
            break;
            
         case 2:
            this._receivedMessages[message.id] = message;
            var ackMessage = {type:MESSAGE_TYPE.PUBREC, id:message.id};
            this._send(ackMessage);
            break;
            
         default:
            throw Error("Invalid qos="+message.qos);
         }
      };
      
      client_ctor.prototype._receiveMessage = function (message) {
         // Extract topic and message from MQTT packet
         var topic = message.topic, data = message.payload, 
         listeners = this._topic_listeners[topic], i =  0;
         
         // If we have some registered listeners, iterate through the list,
         // executing each with the message content.
         while(listeners && i < listeners.length) {
            listeners[i](message);
            i++;
         }
         if (this.onmessage !== null && typeof this.onmessage == "function") {
            this.onmessage(message);
         }
      };
      
      
      // Client has disconnected either at its own request or because the server
      // or network disconnected it. Remove all non-durable state.
      client_ctor.prototype._disconnected = function (reason) {
         this._pinger.cancel();
         // Execute the connectionLostCallback if there is one, and we are connected.
         if (this.onconnectionlost !== null && typeof this.onconnectionlost == "function" && !this._disconnectedDone) {
            this.onconnectionlost(this, reason);
         }
         if (this._socket_is_open()) {
            this._socket.close();
         }
         this._connected = false;
         this._disconnectedDone = true;
         this._socket = null;
      };
      
      return {
         Client: client_ctor,
      };
})(window);
