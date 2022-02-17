/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
MessagingJSON = (function (global) {

	var version = "1.0";

	/** 
	 * Unique message type identifiers, with associated
	 * associated integer values.
	 * @private 
	 */
	var MESSAGE_TYPE = {
			Connect: 1, 
			Connected: 2, 
			Send: 3,
			Ack: 4,
			Subscribe: 5, 
			CloseSubscription: 6,
			DestroySubscription: 7,
			Ping: 8,
			Pong: 9,
			Close: 10
	};

	/**
	 * Validate an object's parameter names to ensure they 
	 * match a list of expected variables name for this option
	 * type. Used to ensure option object passed into the API don't
	 * contain erroneous parameters.
	 * @param {Object} obj - User options object
	 * @param {Object} keys - valid keys and types that may exist in obj. 
	 * @throws {Error} Invalid option parameter found. 
	 * @private 
	 */
	var validate = function(obj, keys) {
		for(key in obj) {
			if (obj.hasOwnProperty(key)) {       		
				if (keys.hasOwnProperty(key)) {
					if (typeof obj[key] !== keys[key])
					   throw new Error(format(ERROR.INVALID_TYPE, [typeof obj[key], key]));
				} else {	
					var errorStr = "Unknown property, " + key + ". Valid properties are:";
					for (key in keys)
						if (keys.hasOwnProperty(key))
							errorStr = errorStr+" "+key;
					throw new Error(errorStr);
				}
			}
		}
	};

	/**
	 * Return a new function which runs the user function bound
	 * to a fixed scope. 
	 * @param {function} User function
	 * @param {object} Function scope  
	 * @return {function} User function bound to another scope
	 * @private 
	 */
	var scope = function (f, scope) {
		return function () {
			return f.apply(scope, arguments);
		};
	};
	
	/** 
	 * Unique message type identifiers, with associated
	 * associated integer values.
	 * @private 
	 */
	var ERROR = {
		OK: {code:0, text:"OK."},
		CONNECT_TIMEOUT: {code:1, text:"Connect timed out."},
		SUBSCRIBE_TIMEOUT: {code:2, text:"Subscribe timed out."}, 
		UNSUBSCRIBE_TIMEOUT: {code:3, text:"Unsubscribe timed out."},
		PING_TIMEOUT: {code:4, text:"Ping timed out."},
		INTERNAL_ERROR: {code:5, text:"Internal error."},
		CONNACK_RETURNCODE: {code:6, text:"Bad Connack return code:{0} {1}."},
		SOCKET_ERROR: {code:7, text:"Socket error: {0}."},
		SOCKET_CLOSE: {code:8, text:"Socket closed."},
		MALFORMED_UTF: {code:9, text:"Malformed UTF data:{0} {1} {2}."},
		UNSUPPORTED: {code:10, text:"{0} is not supported by this browser."},
		INVALID_STATE: {code:11, text:"Invalid state {0}."},
		INVALID_TYPE: {code:12, text:"Invalid type {0} for {1}."},
		INVALID_ARGUMENT: {code:13, text:"Invalid argument {0} for {1}."},
		UNSUPPORTED_OPERATION: {code:14, text:"Unsupported operation."},
		INVALID_STORED_DATA: {code:15, text:"Invalid data in local storage key={0} value={1}."},
		INVALID_JSON_MESSAGE_TYPE: {code:16, text:"Invalid JSON message type {0}."},
		MALFORMED_UNICODE: {code:17, text:"Malformed Unicode string:{0} {1}."},
		SUBSCRIPTION_NOT_FOUND: {code:18, text:"A subscription for the topic {0} with the name {1} does not exist."},
	};
	
	/**
	 * Format an error message text.
	 * @private
	 * @param {error} ERROR.KEY value above.
	 * @param {substitutions} [array] substituted into the text.
	 * @return the text with the substitutions made.
	 */
	var format = function(error, substitutions) {
		var text = error.text;
		if (substitutions) {
		  for (var i=0; i<substitutions.length; i++) {
			field = "{"+i+"}";
			start = text.indexOf(field);
			if(start > 0) {
				var part1 = text.substring(0,start);
				var part2 = text.substring(start+field.length);
				text = part1+substitutions[i]+part2;
			}
		  }
		}
		return text;
	};

	
	/** 
	 * Repeat keepalive requests, monitor responses.
	 * @ignore
	 */
	var Pinger = function(client, window, keepAliveInterval) { 
		this._client = client;        	
		this._window = window;
		this._keepAliveInterval = keepAliveInterval*1000;     	
		this.isReset = false;
		
		var pingReq =  { Action: "Ping" };
		
		this.pingReqMsg = JSON.stringify(pingReq);
		
		var doTimeout = function (pinger) {
			return function () {
				return doPing.apply(pinger);
			};
		};
		
		/** @ignore */
		var doPing = function() { 
			if (!this.isReset) {
				this._client._trace("Pinger.doPing", "Timed out");
				this._client._disconnected( ERROR.PING_TIMEOUT.code , format(ERROR.PING_TIMEOUT));
			} else {
				this.isReset = false;
				this._client._trace("Pinger.doPing", this.pingReqMsg);
				this._client.socket.send(this.pingReqMsg); 
				this.timeout = this._window.setTimeout(doTimeout(this), this._keepAliveInterval);
			}
		}

		this.reset = function() {
			this.isReset = true;
			this._window.clearTimeout(this.timeout);
			if (this._keepAliveInterval > 0)
				this.timeout = setTimeout(doTimeout(this), this._keepAliveInterval);
		}

		this.cancel = function() {
			this._window.clearTimeout(this.timeout);
		}
	 }; 
	
	/**
	 * Monitor request completion.
	 * @ignore
	 */
	var Timeout = function(client, window, timeoutSeconds, action, args) {
		this._window = window;
		if (!timeoutSeconds)
			timeoutSeconds = 30;
		
		var doTimeout = function (action, client, args) {
			return function () {
				return action.apply(client, args);
			};
		};
		this.timeout = setTimeout(doTimeout(action, client, args), timeoutSeconds * 1000);
		
		this.cancel = function() {
			this._window.clearTimeout(this.timeout);
		}
	};

	/*
	 * Internal implementation of the Websockets jsJSON V1.0 client.
	 * 
	 * @name Messaging.ClientImpl @constructor 
	 * @param {String} host the DNS nameof the webSocket host. 
	 * @param {Number} port the port number for that host.
	 * @param {String} clientId the MQ client identifier.
	 */
	var ClientImpl = function (uri, host, port, path, clientId) {
		// Check dependencies are satisfied in this browser.
		if (!("WebSocket" in global && global["WebSocket"] !== null)) {
			throw new Error(format(ERROR.UNSUPPORTED, ["WebSocket"]));
		}

		this.host = host;
		this.port = port;
		this.path = path;
		this.uri = uri;
		this.clientId = clientId;

	};

	// Messaging Client public instance members. 
	ClientImpl.prototype.host;
	ClientImpl.prototype.port;
	ClientImpl.prototype.path;
	ClientImpl.prototype.uri;
	ClientImpl.prototype.clientId;

	// Messaging Client private instance members.
	ClientImpl.prototype.socket;
	/* true once we have received an acknowledgment to a CONNECT packet. */
	ClientImpl.prototype.connected = false;
	ClientImpl.prototype._connectTimeout;
	ClientImpl.prototype.onMessageArrived;
	ClientImpl.prototype.connectOptions;
	ClientImpl.prototype._acksId = 1;

	ClientImpl.prototype.receiveBuffer = null;
	ClientImpl.prototype.connPinger = null;
	ClientImpl.prototype.acks = null;
	
	ClientImpl.prototype._traceBuffer = null;
	ClientImpl.prototype._MAX_TRACE_ENTRIES = 100;

	ClientImpl.prototype.connect = function (connectOptions) {
		
		this._trace("Client.connect", connectOptions, this.connected);
		if (this.connected) 
			throw new Error(format(ERROR.INVALID_STATE, ["connected state initialized"]));
		if (this.socket)
			throw new Error(format(ERROR.INVALID_STATE, ["socket already connected"]));
		
		this.connectOptions = connectOptions;
		this._doConnect(this.uri);  		
		
	};
	
	ClientImpl.prototype.createTopicSubscription = function (filter, options) {
		this._trace("Client.createTopicSubscription", filter, options);
			  
		if (!this.connected)
			throw new Error(format(ERROR.INVALID_STATE, ["not connected"]));
		
		// get a unique Ack ID
		var id = this._acksId++;
		this.acks[id] = {Type:MESSAGE_TYPE.Subscribe, Name:options.Name, Topic:filter, ID:id};
		// if there is an onFailure reference add it to the Ack entry
		if (options.onFailure) {
			this.acks[id].onFailure = options.onFailure;
		}
		
		if (options.onSuccess) {
			this.acks[id].onSuccess = options.onSuccess;
		}
		
		// create the command object
		var cmdObj = { Action: "Subscribe", Name:options.Name, Topic:filter, ID:id};
		var cmdString = JSON.stringify(cmdObj);
		
		this._trace("sending JSON subscribe message...", cmdString);
		this.socket.send(cmdString);
		
	};
	
	ClientImpl.prototype.closeTopicSubscription = function (subscriptionName, options) {
		this._trace("Client.closeSubscription", subscriptionName, options);
			  
		if (!this.connected)
			throw new Error(format(ERROR.INVALID_STATE, ["not connected"]));
		
		var id = this._acksId++;
		
		// create the Ack entry...
		this.acks[id] =  {Type:MESSAGE_TYPE.CloseSubscription,  Name:subscriptionName, ID:id};
		
		// if there is an onFailure reference add it to the Ack entry
		if (options.onFailure) {
			this.acks[id].onFailure = options.onFailure;
		}
		
		if (options.onSuccess) {
			this.acks[id].onSuccess = options.onSuccess;
		}
		
		// create the cmd action...
		var cmdObj = { Action: "CloseSubscription", ID:id, Name:subscriptionName};		
		var cmdString = JSON.stringify(cmdObj);
		this._trace("sending JSON close subscription message...", cmdString);
		this.socket.send(cmdString);
	
	};
	
	ClientImpl.prototype.send = function (message, options) {
		this._trace("Client.send");

		if (!this.connected)
		   throw new Error(format(ERROR.INVALID_STATE, ["not connected"]));
		
		var id = this._acksId++;
		
		// create the Ack entry...
		this.acks[id] =  {Type:MESSAGE_TYPE.Send, Topic:message.destinationName, ID:id};
		
		// if there is an onFailure reference add it to the Ack entry
		if (options.onFailure) {
			this.acks[id].onFailure = options.onFailure;
		}
		
		if (options.onSuccess) {
			this.acks[id].onSuccess = options.onSuccess;
		}
		
		var sendObj = {Action: "Send", Topic:message.destinationName, QoS:message.qos, ID:id, Body:message.payloadString };
		var msgString = JSON.stringify(sendObj);

		this._trace("sending JSON publish", msgString);
		this.socket.send(msgString);
		

	};
	
	ClientImpl.prototype._processAck = function (ackObj) {
		
		this._trace("Client._processAck");
		
		if (!this.acks[ackObj.ID]) {
			this._trace("no ack entry found for the id: ", ackObj.ID);
			return;
		}
		
		// look up the ack entry
		var ackEntry = this.acks[ackObj.ID];
		var type = ackEntry.Type;
		
		switch(type) {
			case MESSAGE_TYPE.Subscribe:
				if (ackObj.RC != 0) {
					if (ackEntry.onFailure) {
						ackEntry.onFailure(ackEntry.Name, ackEntry.Topic, ackObj);
					} 
				} else if (ackEntry.onSuccess) {
					ackEntry.onSuccess(ackEntry.Name, ackEntry.Topic);
				} 
				break;
			case MESSAGE_TYPE.CloseSubscription:
				if (ackObj.RC != 0) {
					if (ackEntry.onFailure) {
						ackEntry.onFailure( ackEntry.Name, ackObj);
					}
				} else if (ackEntry.onSuccess) {
					ackEntry.onSuccess(ackEntry.Name);
				} 
				break;
			case MESSAGE_TYPE.Send:
				if (ackObj.RC != 0) {
					if (ackEntry.onFailure) {
						ackEntry.onFailure( ackEntry.Topic, ackObj);
					}
				} else if (ackEntry.onSuccess) {
					ackEntry.onSuccess(ackEntry.Topic);
				} 
				break;
			default:
		};
			
		// ack reference is no longer needed
		delete this.acks[ackObj.ID];
		
	};
	
	ClientImpl.prototype.disconnect = function () {
		this._trace("Client.disconnect");

		if (!this.socket)
			throw new Error(format(ERROR.INVALID_STATE, ["not connecting or connected"]));
		
		// try to close the connection gracefully 
		var msg = { Action: "Close" };

		var msgString = JSON.stringify(msg);
		this._trace("sending JSON close message...", msgString);
		this.socket.send(msgString);
		
		if (this.socket) {
			// Cancel all socket callbacks so that they cannot be driven again by this socket.
			this.socket.onopen = null;
			this.socket.onmessage = null;
			this.socket.onerror = null;
			this.socket.onclose = null;
			if (this.socket.readyState === 1)
				this.socket.close();
			delete this.socket;           
		}
		this.connected = false;
		
		if (this.onConnectionLost)
			this.onConnectionLost();   
		
	};
	
	ClientImpl.prototype.getTraceLog = function () {
		if ( this._traceBuffer !== null ) {
			return this._traceBuffer;
		}
	};
	
	ClientImpl.prototype.startTrace = function () {
		if (this._traceBuffer) {
			delete this._traceBuffer;
		}
		this._traceBuffer = [];
		this._trace("Initializing trace...", new Date());
	};
	
	ClientImpl.prototype.stopTrace = function () {
		delete this._traceBuffer;
	};

	ClientImpl.prototype._doConnect = function (wsurl) { 
		
		this._trace("Client._doConnect", wsurl);
		this.socket = new WebSocket(wsurl, "json-msg");
		this.socket.onopen = scope(this._on_socket_open, this);
		this.socket.onmessage = scope(this._on_socket_message, this);
		this.socket.onerror = scope(this._on_socket_error, this);
		this.socket.onclose = scope(this._on_socket_close, this);
		
		this.acks = new Object();
		
		// start a pinger for keep alive timeout
		this.connPinger = new Pinger(this, window, this.connectOptions.keepAliveInterval);
		
		this._connectTimeout = new Timeout(this, window, this.connectOptions.timeout, this._disconnected,  [ERROR.CONNECT_TIMEOUT.code, format(ERROR.CONNECT_TIMEOUT)]);

	};

	/** 
	 * Called when the underlying websocket has been opened.
	 * @ignore
	 */
	ClientImpl.prototype._on_socket_open = function () {
		console.log("SOCKET OPEN!!!");
		this._trace("Client._on_socket_open");
		
		// we have an open socket to the server - try to connect
		var msg = { Action: "Connect", ClientID:this.clientId };
		if (this.connectOptions.userName) {
			msg.User = this.connectOptions.userName;
		}
		if (this.connectOptions.password) {
			msg.Password = this.connectOptions.password;
		}
		//if (this.connectOptions.keepAliveTimeout) {
			msg.KeepAliveTimeout = 60;
		//}
		var msgString = JSON.stringify(msg);
		this._trace("Sending connection request...", msgString);
		this.socket.send(msgString);

	};

	/** 
	 * Called when the underlying websocket has received a complete packet.
	 * @ignore
	 */
	ClientImpl.prototype._on_socket_message = function (event) {
		
		this._trace("Client._on_socket_message", event.data);
		
		// create an object from the JSON string
		var msgObj = JSON.parse(event.data); 
		var type = MESSAGE_TYPE[msgObj.Action];
		
		try {
			switch(type) {
				case MESSAGE_TYPE.Connected:
					this.connected = true;
					this._connectTimeout.cancel();
					if (this.connectOptions.onSuccess) {
						this.connectOptions.onSuccess();
					}
					break;
				case MESSAGE_TYPE.Send:
					if(this.onMessageArrived) {
						this.onMessageArrived(msgObj);
					}
					break;
				case MESSAGE_TYPE.Ack:
					this._processAck(msgObj);
					break;
				case MESSAGE_TYPE.Close:
					if (msgObj.RC != 0) {
						if (this.onConnectionLost) {
							this.onConnectionLost(msgObj.RC, msgObj.Reason);
						}
					}
					break;
				case MESSAGE_TYPE.Ping:
					var msg = { Action: "Pong" };
					var msgString = JSON.stringify(msg);
					this._trace("sending Pong message to server...", msgString);
					this.socket.send(msgString);
					break;
				case MESSAGE_TYPE.Pong:
					break;
				default:
					this._disconnected(ERROR.INVALID_JSON_MESSAGE_TYPE.code , format(ERROR.INVALID_JSON_MESSAGE_TYPE, [msgObj.Action]));
			};
		
		} catch (error) {
				this._disconnected(ERROR.INTERNAL_ERROR.code , format(ERROR.INTERNAL_ERROR, [error.message]));
				return;
		}
		
		// Reset the receive ping timer, we now have evidence the server is alive.
		this.connPinger.reset();		

	};


	/** @ignore */
	ClientImpl.prototype._on_socket_error = function (error) {
		
		var data = "";
		if (!error || !error.data) {
			data = "Unknown connection error";
		} else {
			data = error.data;
		}
		
		this._trace("Client._on_socket_error", data);
		this._disconnected(ERROR.SOCKET_ERROR.code , format(ERROR.SOCKET_ERROR, [data]));
		
	};

	/** @ignore */
	ClientImpl.prototype._on_socket_close = function () {
		this._trace("Client._on_socket_close");
		this._disconnected(ERROR.SOCKET_CLOSE.code , format(ERROR.SOCKET_CLOSE));
	};
	

	/**
	 * Client has disconnected either at its own request or because the server
	 * or network disconnected it. Remove all non-durable state.
	 * @param {errorCode} [number] the error number.
	 * @param {errorText} [string] the error text.
	 * @ignore
	 */
	ClientImpl.prototype._disconnected = function (errorCode, errorText) {
		this._trace("Client._disconnected", errorCode, errorText);
		
		this.connPinger.cancel();
		if (this._connectTimeout)
			this._connectTimeout.cancel();

	   
		if (this.socket) {
			
			try {
				// try and disconnect gracefully... 
				var msg = { Action: "Close" };

				var msgString = JSON.stringify(msg);
				this._trace("sending JSON close message...", msgString);
				this.socket.send(msgString);
				
			} catch (error) {
				// nothing to do here..
			}
			
			// Cancel all socket callbacks so that they cannot be driven again by this socket.
			this.socket.onopen = null;
			this.socket.onmessage = null;
			this.socket.onerror = null;
			this.socket.onclose = null;
			if (this.socket.readyState === 1)
				this.socket.close();
			delete this.socket; 
			
		}
		
		if (errorCode === undefined) {
			errorCode = ERROR.OK.code;
			errorText = format(ERROR.OK);
		}
			
		// Run any application callbacks last as they may attempt to reconnect and hence create a new socket.
		if (this.connected) {
			this.connected = false;
			// Execute the connectionLostCallback if there is one, and we were connected.       
			if (this.onConnectionLost)
				this.onConnectionLost(errorCode, errorText);      	
		} else {
			// Otherwise we never had a connection, so indicate that the connect has failed.
			if(this.connectOptions.onFailure)
				this.connectOptions.onFailure({errorCode:errorCode, errorMessage:errorText});
		}
	
	};
	
	/** @ignore */
	ClientImpl.prototype._trace = function () {
		if ( this._traceBuffer !== null ) {  
			for (var i = 0, max = arguments.length; i < max; i++) {
				if ( this._traceBuffer.length == this._MAX_TRACE_ENTRIES ) {    
					this._traceBuffer.shift();              
				}
				if (i === 0) this._traceBuffer.push(arguments[i]);
				else if (typeof arguments[i] === "undefined" ) this._traceBuffer.push(arguments[i]);
				else this._traceBuffer.push("  "+JSON.stringify(arguments[i]));
		   };
		};
	};
	

	// ------------------------------------------------------------------------
	// Public Programming interface.
	// ------------------------------------------------------------------------
	
	/** 
	 * The JavaScript application communicates with IBM MessageSight via the json_msg plug-in using a {@link MessagingJSON.Client} object. 
	 * 
	 * The send, subscribe and unsubscribe methods are implemented as asynchronous JavaScript methods 
	 * (even though the underlying protocol exchange might be synchronous in nature). 
	 * This means they signal their completion by calling back to the application 
	 * via Success or Failure callback functions provided by the application on the method in question. 
	 * These callbacks are called at most once per method invocation and do not persist beyond the lifetime 
	 * of the script that made the invocation.
	 * <p>
	 * In contrast there are some callback functions, most notably <i>onMessageArrived</i>, 
	 * that are defined on the {@link MessagingJSON.Client} object.  
	 * These callbacks might be called multiple times, and are not directly related to specific method invocations made by the client. 
	 *
	 * @name MessagingJSON.Client    
	 * 
	 * @constructor
	 *  
	 * @param {string} host - the address of the IBM MessageSight host, as a fully qualified WebSocket URI, as a DNS name or dotted decimal IP address.
	 * @param {number} port - the port number to connect to - only required if host is not a URI
	 * @param {string} path - the path on the host to connect to - only used if host is not a URI. Default: '/json-msg'.
	 * @param {string} clientId - the MessagingJSON client identifier, between 0 and 65535 characters in length.
	 * 
	 * @property {string} host - <i>read only</i> the IBM MessageSight DNS hostname or dotted decimal IP address.
	 * @property {number} port - <i>read only</i> the IBM MessageSight port.
	 * @property {string} path - <i>read only</i> the IBM MessageSight path.
	 * @property {string} clientId - <i>read only</i> used when connecting to IBM MessageSight.   
	 */
	var Client = function (host, port, path, clientId) {
		
		var uri;
	    
		if (typeof host !== "string")
			throw new Error(format(ERROR.INVALID_TYPE, [typeof host, "host"]));
	    
	    if (arguments.length == 2) {
	        clientId = port;
	        uri = host;
	        var match = uri.match(/^(wss?):\/\/((\[(.+)\])|([^\/]+?))(:(\d+))?(\/.*)$/);
	        if (match) {
	            host = match[4]||match[2];
	            port = parseInt(match[7]);
	            path = match[8];
	        } else {
	            throw new Error(format(ERROR.INVALID_ARGUMENT,[host,"host"]));
	        }
	    } else {
	        if (arguments.length == 3) {
				clientId = path;
				path = "/json-msg";
			}
			if (typeof port !== "number" || port < 0)
				throw new Error(format(ERROR.INVALID_TYPE, [typeof port, "port"]));
			if (typeof path !== "string")
				throw new Error(format(ERROR.INVALID_TYPE, [typeof path, "path"]));
			
			var ipv6AddSBracket = (host.indexOf(":") != -1 && host.slice(0,1) != "[" && host.slice(-1) != "]");
			uri = "ws://"+(ipv6AddSBracket?"["+host+"]":host)+":"+port+path;
			console.log("URI is " + uri);
		}

		var clientIdLength = 0;
		for (var i = 0; i<clientId.length; i++) {
			var charCode = clientId.charCodeAt(i);                   
			if (0xD800 <= charCode && charCode <= 0xDBFF)  {    			
				 i++; // Surrogate pair.
			}   		   
			clientIdLength++;
		}     	   	
		if (typeof clientId !== "string" || clientIdLength > 65535)
			throw new Error(format(ERROR.INVALID_ARGUMENT, [clientId, "clientId"])); 
		
		var client = new ClientImpl(uri, host, port, path, clientId);
		this._getHost =  function() { return host; };
		this._setHost = function() { throw new Error(format(ERROR.UNSUPPORTED_OPERATION)); };
			
		this._getPort = function() { return port; };
		this._setPort = function() { throw new Error(format(ERROR.UNSUPPORTED_OPERATION)); };

		this._getPath = function() { return path; };
		this._setPath = function() { throw new Error(format(ERROR.UNSUPPORTED_OPERATION)); };

		this._getURI = function() { return uri; };
		this._setURI = function() { throw new Error(format(ERROR.UNSUPPORTED_OPERATION)); };
		
		this._getClientId = function() { return client.clientId; };
		this._setClientId = function() { throw new Error(format(ERROR.UNSUPPORTED_OPERATION)); };
		
		this._getOnConnectionLost = function() { return client.onConnectionLost; };
		this._setOnConnectionLost = function(newOnConnectionLost) { 
			if (typeof newOnConnectionLost === "function")
				client.onConnectionLost = newOnConnectionLost;
			else 
				throw new Error(format(ERROR.INVALID_TYPE, [typeof newOnConnectionLost, "onConnectionLost"]));
		};
			
		this._getOnMessageArrived = function() { return client.onMessageArrived; };
		this._setOnMessageArrived = function(newOnMessageArrived) { 
			if (typeof newOnMessageArrived === "function")
				client.onMessageArrived = newOnMessageArrived;
			else 
				throw new Error(format(ERROR.INVALID_TYPE, [typeof newOnMessageArrived, "onMessageArrived"]));			
		};
		
		/** 
		 * Connect this MessagingJSON client to IBM MessageSight via the json_msg plug-in. 
		 * 
		 * @name MessagingJSON.Client#connect
		 * @function
		 * @param {Object} connectOptions - attributes used with the connection. 
		 * @param {number} connectOptions.timeout - If the connect has not succeeded within this 
		 *                    number of seconds, it is deemed to have failed.
		 *                    The default is 30 seconds.
		 * @param {string} connectOptions.userName - Authentication username for this connection.
		 * @param {string} connectOptions.password - Authentication password for this connection.
		 * @param {Number} connectOptions.keepAliveInterval - the interval in seconds to check if this
		 *                     client is still connected to IBM MessageSight - and that MessageSight can 
		 *                     respond.
		 * @param {Number} connectOptions.keepAliveTimeout - the json_msg plug-in disconnects this client if
		 *                    there is no activity for this number of seconds. If not specified the 
		 *                    server assumes 60 seconds. 
		 * @param {boolean} connectOptions.useSSL - if present and true, use an SSL Websocket connection.
		 * @param {function} connectOptions.onSuccess - called when the connect acknowledgement 
		 *                    has been received from IBM MessageSight.
		 * @config {function} [onFailure] called when the connect request has failed or timed out.
		 *                         A single response object parameter is passed to the onFailure callback 
		 *                         containing the following fields:
		 * 							<ol>     
		 * 							<li>errorCode a number indicating the nature of the error.
		 * 							<li>errorMessage text describing the error.      
		 * 							</ol>
		 * @throws {InvalidState} if the client is not in disconnected state. The client must have received connectionLost
		 * or disconnected before calling connect for a second or subsequent time.
		 */
		this.connect = function (connectOptions) {
			connectOptions = connectOptions || {} ;
			validate(connectOptions,  {timeout:"number",
									   userName:"string", 
									   password:"string",  
									   keepAliveInterval:"number",
									   keepAliveTimeout: "number",
									   useSSL:"boolean",
									   onSuccess:"function", 
									   onFailure:"function"});
			
			// If no keep alive interval is set, assume 60 seconds.
			if (connectOptions.keepAliveInterval === undefined)
				connectOptions.keepAliveInterval = 60;
			
			client.connect(connectOptions);
		
		};
		
		/** 
		 * Normal disconnect of this MessagingJSON client from IBM MessageSight.
		 * 
		 * @name MessagingJSON.Client#disconnect
		 * @function
		 * @throws {InvalidState} if the client is already disconnected.     
		 */
		this.disconnect = function () {
			client.disconnect();
		};
		
		/** 
		 * Subscribe for messages, request receipt of a copy of messages sent to the destinations described by the filter.
		 * 
		 * @name MessagingJSON.Client#createTopicSubscription
		 * @function
		 * @param {string} filter describing the destinations to receive messages from.
		 * <br>
		 * @param {object} subscribeOptions - used to control the subscription
		 *
		 * @param {number} subscribeOptions.qos - the maiximum reliability of any publications sent 
		 *                                  as a result of making this subscription. Valid settings are:
		 *                                  <dl>
		 *                                  <dt>0 - at most once.
		 *                                  <dt>1 - at least once.
		 *                                  <dt>2 - exactly once.       
		 *                                  </dl>
		 * @param {function} subscribeOptions.onSuccess - called when the subscribe acknowledgement
		 *                                  has been received from IBM MessageSight.
		 * @param {function} subscribeOptions.onFailure - called when the subscribe request has failed or timed out.
		 *                                  A single response object parameter is passed to the onFailure callback containing the following fields:
		 *                                  <ol>   
		 *                                  <li>errorCode - a number indicating the nature of the error.
		 *                                  <li>errorMessage - text describing the error.      
		 *                                  </ol>
		 * @param {number} subscribeOptions.timeout - which, if present, determines the number of
		 *                                  seconds after which the onFailure calback is called.
		 *                                  The presence of a timeout does not prevent the onSuccess
		 *                                  callback from being called when the subscribe completes.         
		 * @throws {InvalidState} if the client is not in connected state.
		 */
		this.createTopicSubscription = function (filter, subscribeOptions) {
			if (typeof filter !== "string")
				throw new Error("Invalid argument:"+filter);
			subscribeOptions = subscribeOptions || {} ;
			validate(subscribeOptions,  {QoS:"number",
										 Name: "string",
										 onSuccess:"function", 
										 onFailure:"function",
										 timeout:"number"
										});
			if (subscribeOptions.timeout && !subscribeOptions.onFailure)
				throw new Error("subscribeOptions.timeout specified with no onFailure callback.");
			if (typeof subscribeOptions.qos !== "undefined" 
				&& !(subscribeOptions.qos === 0 || subscribeOptions.qos === 1 || subscribeOptions.qos === 2 ))
				throw new Error(format(ERROR.INVALID_ARGUMENT, [subscribeOptions.qos, "subscribeOptions.qos"]));
			client.createTopicSubscription(filter, subscribeOptions);
		};
		
		/** 
		 * Close a subscription identified by a subscrption name.
		 * 
		 * @name MessagingJSON.Client#closeTopicSubscription
		 * @function
		 * @param {string} subscriptionName The name of the subscription to close.
		 * <br>
		 * @param {object} options - used to control the subscription
		 *
		 * @param {function} options.onSuccess - called when the subscribe acknowledgement
		 *                                  has been received from IBM MessageSight.
		 * @param {function} options.onFailure - called when the subscribe request has failed or timed out.
		 *                                  A single response object parameter is passed to the onFailure callback containing the following fields:
		 *                                  <ol>     
		 *                                  <li>errorCode - a number indicating the nature of the error.
		 *                                  <li>errorMessage - text describing the error.      
		 *                                  </ol>
		 * @param {number} options.timeout - which, if present, determines the number of
		 *                                  seconds after which the onFailure calback is called.
		 *                                  The presence of a timeout does not prevent the onSuccess
		 *                                  callback from being called when the subscribe completes.         
		 * @throws {InvalidState} if the client is not in connected state.
		 */
		this.closeTopicSubscription = function (subscriptionName, options) {
			if (typeof subscriptionName !== "string")
				throw new Error("Invalid argument:"+subscriptionName);
			options = options || {} ;
			validate(options,  {onSuccess:"function", 
								onFailure:"function",
								timeout:"number"
							});
			if (options.timeout && !options.onFailure)
				throw new Error("options.timeout specified with no onFailure callback.");
			client.closeTopicSubscription(subscriptionName, options);
		};
		
		/**
		 * Send a message to the consumers of the destination in the Message.
		 * 
		 * @name MessagingJSON.Client#send
		 * @function 
		 * @param {MessagingJSON.Message} message to send.
		 * @param {object} options - used to control the subscription
		 *
		 * @param {function} options.onSuccess - called when the subscribe acknowledgement
		 *                                  has been received from IBM MessageSight.
		 * @param {function} options.onFailure - called when the subscribe request has failed or timed out.
		 *                                  A single response object parameter is passed to the onFailure callback containing the following fields:
		 *                                  <ol>     
		 *                                  <li>errorCode - a number indicating the nature of the error.
		 *                                  <li>errorMessage - text describing the error.      
		 *                                  </ol>
		 * @param {number} options.timeout - which, if present, determines the number of
		 *                                  seconds after which the onFailure calback is called.
		 *                                  The presence of a timeout does not prevent the onSuccess
		 *                                  callback from being called when the subscribe completes.  
		 
		 * @throws {InvalidState} if the client is not connected.
		 */   
		this.send = function (message, options) {       	
			if (!(message instanceof Message))
				throw new Error("Invalid argument:"+typeof message);
			if (typeof message.destinationName === "undefined")
				throw new Error("Invalid parameter Message.destinationName:"+message.destinationName);
			options = options || {} ;
			validate(options,  {onSuccess:"function", 
								onFailure:"function",
								timeout:"number"
							});
			if (options.timeout && !options.onFailure)
				throw new Error("options.timeout specified with no onFailure callback.");
		   
			client.send(message, options);   
		};
		
		/** 
		 * Get the contents of the trace log.
		 * 
		 * @name MessagingJSON.Client#getTraceLog
		 * @function
		 * @return {Array.<Object>} tracebuffer containing the time ordered trace records.
		 */
		this.getTraceLog = function () {
			return client.getTraceLog();
		}
		
		/** 
		 * Start tracing.
		 * 
		 * @name MessagingJSON.Client#startTrace
		 * @function
		 */
		this.startTrace = function () {
			client.startTrace();
		};
		
		/** 
		 * Stop tracing.
		 * 
		 * @name MessagingJSON.Client#stopTrace
		 * @function
		 */
		this.stopTrace = function () {
			client.stopTrace();
		};

	};
	
	Client.prototype = {
			
		get host() { return this._getHost(); },
		set host(newHost) { this._setHost(newHost); },
				
		get port() { return this._getPort(); },
		set port(newPort) { this._setPort(newPort); },

		get path() { return this._getPath(); },
		set path(newPath) { this._setPath(newPath); },
				
		get clientId() { return this._getClientId(); },
		set clientId(newClientId) { this._setClientId(newClientId); },
		
		get onConnectionLost() { return this._getOnConnectionLost(); },
		set onConnectionLost(newOnConnectionLost) { this._setOnConnectionLost(newOnConnectionLost); },
		
		get onMessageArrived() { return this._getOnMessageArrived(); },
		set onMessageArrived(newOnMessageArrived) { this._setOnMessageArrived(newOnMessageArrived); }

	};
	
	/** 
	 * An application message, sent or received.
	 * <p>
	 * All attributes may be null, which implies the default values.
	 * 
	 * @name MessagingJSON.Message
	 * @constructor
	 * @param {String|ArrayBuffer} payload The message data to be sent.
	 * <p>
	 * @property {string} payloadString <i>read only</i> The payload as a string if the payload consists of valid UTF-8 characters.
	 * @property {ArrayBuffer} payloadBytes <i>read only</i> The payload as an ArrayBuffer.
	 * <p>
	 * @property {string} destinationName <b>mandatory</b> The name of the destination to which the message is to be sent
	 *                    (for messages about to be sent) or the name of the destination from which the message has been received.
	 *                    (for messages received by the onMessageArrived function).
	 * <p>
	 * @property {number} qos The reliability setting used to deliver the message. Applicable for destinations of type Topic only.
	 * <dl>
	 *     <dt>0 Best effort (default).
	 *     <dt>1 At least once.
	 *     <dt>2 Exactly once.     
	 * </dl>
	 * <p>
	 *                     
	 */
	var Message = function (newPayload, payloadEncoded) {  
		
		var payload;
		if (   typeof newPayload === "string" ) {
			payload = newPayload;
		} else {
			throw (format(ERROR.INVALID_ARGUMENT, [newPayload, "newPayload"]));
		}

		this._getPayloadString = function () {
				return payload;
		};
		
		var encoded = false;
		if (payloadEncoded) {
			encoded = payloadEncoded;
		}
		this._getIsEncoded = function() {
			return encoded;
		};

		var destinationName = undefined;
		this._getDestinationName = function() { return destinationName; };
		this._setDestinationName = function(newDestinationName) { 
			if (typeof newDestinationName === "string")
				destinationName = newDestinationName;
			else 
				throw new Error(format(ERROR.INVALID_ARGUMENT, [newDestinationName, "newDestinationName"]));
		};
				
		var qos = 0;
		this._getQos = function() { return qos; };
		this._setQos = function(newQos) { 
			if (newQos === 0 || newQos === 1 || newQos === 2 )
				qos = newQos;
			else 
				throw new Error("Invalid argument:"+newQos);
		};
		
		var destType = undefined;
		this._getDestinationType = function() { return destType; };
		this._setDestinationType = function(newDestType) { 
			if (newDestType === "Topic")
				destType = newDestType;
			else 
				throw new Error("Invalid destination type: " + newDestType);
		};

	};
	
	Message.prototype = {
		get payloadString() { return this._getPayloadString(); },
		
		get isEncoded() { return this._getIsEncoded(); },
		
		TOPIC : "Topic",
		
		get destinationName() { return this._getDestinationName(); },
		set destinationName(newDestinationName) { this._setDestinationName(newDestinationName); },
		
		get qos() { return this._getQos(); },
		set qos(newQos) { this._setQos(newQos); },
		
		get destinationType() { return this._getDestinationType(); },
		set destinationType(newDestType) { this._setDestinationType(newDestType); },
		
	};
	

	// Module contents.
	return {
		Client: Client,
		Message: Message
	};

})(window);
