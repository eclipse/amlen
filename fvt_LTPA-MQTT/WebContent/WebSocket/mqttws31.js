/*
 * Copyright (c) 2011-2021 Contributors to the Eclipse Foundation
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

// Only expose a single object name in the global namespace.
// Everything must go through this module. Global Messaging module
// only has a single public function, client, which returns
// an Messaging client object given connection details.
 
/**
 * @namespace Messaging 
 * Send and receive messages using HTML 5 browsers.
 *
 * <code><pre>
 *	var client = new Messaging.Client("mqtthost.com", 80, "clientName");
 *  client.onMessageArrived = function(message){alert(message.stringPayload);}
 *  client.connect();
 *  client.subscribe("World");
 *  var message = new Message("Hello");
 *  message.destinationName = "World";
 *  client.send(message);
 *  client.disconect();
 * </pre></code>
 * 
 * <p> 
 * This programming interface enables a Javascript client applications use the MQTT V3.1 protocol to 
 * connect to an MQTT-supporting messaging server, for example websphereMQ v7.5 with the MQ Telemetry 
 * feature installed. 
 *  
 * The function supported includes:
 * <ol>
 * <li>Connecting to and disconnecting from a server. The server is identified by its host name and port number. 
 * <li>Specifying options that relate to the communications link with the server, 
 * for example the frequency of keep-alive heartbeats, and whether SSL/TLS is required.
 * <li>Subscribing to and receiving messages from MQTT Topics.
 * <li>Publishing a message to MQTT Topics.
 * </ol>
 * <p>
 * <h2>The API consists of two main objects:</h2>
 * The <b>Messaging.Client</b> object. This contains methods that provide the functionality of the API,
 * including provision of callbacks that notify the application when a message arrives from or is delivered to the messaging server,
 * or when the status of its connection to the messaging server changes.
 * <p>
 * The <b>Messaging.Message</b> object. This encapsulates the payload of the message along with various attributes
 * associated with its delivery, in particular the destination to which it has been (or is about to be) sent. 
 * <p>
 * The programming interface validates parameters passed to it, and will throw an Error containing an error message
 * intended for developer use, if it detects an error with any parameter.
 */
Messaging = (function (global) {

    // Private variables below, these are only visibly inside the function closure
    // which is used to define the module. 

    /** 
     * Unique message type identifiers, with associated
     * associated integer values.
     * @private 
     */
    var MESSAGE_TYPE = {
        CONNECT: 1, 
        CONNACK: 2, 
        PUBLISH: 3,
        PUBACK: 4,
        PUBREC: 5, 
        PUBREL: 6,
        PUBCOMP: 7,
        SUBSCRIBE: 8,
        SUBACK: 9,
        UNSUBSCRIBE: 10,
        UNSUBACK: 11,
        PINGREQ: 12,
        PINGRESP: 13,
        DISCONNECT: 14
    };
    
    // Collection of utility methods used to simplify module code 
    // and promote the DRY pattern.  

    /**
     * Validate an object's parameter name to ensure they 
     * match a list of expected variables name for this option
     * type. Used to ensure option object passed into the API don't
     * contain erroneous parameters.
     * @param {Object} obj User options object
     * @param {key:type, key2:type, ...} valid keys and types that may exist in obj. 
     * @throws {Error} Invalid option parameter found. 
     * @private 
     */
    var validate = function(obj, keys) {
        for(key in obj) {
        	if (obj.hasOwnProperty(key) && keys.hasOwnProperty(key)) {
        	  if (typeof obj[key] !== keys[key])
        		 throw new Error("InvalidType:"+typeof obj[key] +" for "+key);
        	} else {	
            	var errorStr = "Unknown property, " + key + ". Valid properties are:";
            	for (key in keys)
            		if (keys.hasOwnProperty(key))
            		    errorStr = errorStr+" "+key;
            	throw new Error(errorStr);
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
    	CONNECT_TIMEOUT: {code:1, text:"AMQJSC0001E Connect timed out."},
        SUBSCRIBE_TIMEOUT: {code:2, text:"AMQJS0002E Subscribe timed out."}, 
        UNSUBSCRIBE_TIMEOUT: {code:3, text:"AMQJS0003E Unsubscribe timed out."},
        PING_TIMEOUT: {code:4, text:"AMQJS0004E Ping timed out."},
        INTERNAL_ERROR: {code:5, text:"AMQJS0005E Internal error."},
        CONNACK_RETURNCODE: {code:6, text:"AMQJS0006E Bad Connack return code:{0}"},
        SOCKET_ERROR: {code:7, text:"AMQJS0007E Socket error:{0}"},
        SOCKET_CLOSE: {code:8, text:"AMQJS0008I Socket closed."},
        MALFORMED_UTF: {code:9, text:"AMQJS0009E Malformed UTF data:{0} {1} {2}"},
        UNSUPPORTED: {code:10, text:"AMQJS0010E {0} is not supported by this browser"}
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
    
    //MQTT protocol and version          6    M    Q    I    s    d    p    3
    const MqttProtoIdentifier = [0x00,0x06,0x4d,0x51,0x49,0x73,0x64,0x70,0x03];
    
    /**
     * Construct an MQTT wire protocol message.
     * @param type MQTT packet type.
     * @param options optional wire message attributes.
     * 
     * Optional properties
     * 
     * messageIdentifier: message ID in the range [0..65535]
     * payloadMessage:	Application Message - PUBLISH only
     * connectStrings:	array of 0 or more Strings to be put into the CONNECT payload
     * topics:			array of strings (SUBSCRIBE, UNSUBSCRIBE)
     * requestQoS:		array of QoS values [0..2]
     *  
     * "Flag" properties 
     * cleanSession:	true if present / false if absent (CONNECT)
     * willMessage:  	true if present / false if absent (CONNECT)
     * isRetained:		true if present / false if absent (CONNECT)
     * userName:		true if present / false if absent (CONNECT)
     * password:		true if present / false if absent (CONNECT)
     * keepAliveInterval:	integer [0..65535]  (CONNECT)
     *
     * @private
     */
    var WireMessage = function (type, options) {
        this.type = type;
        for(name in options) {
            if (options.hasOwnProperty(name)) {
                this[name] = options[name];
            }
        }
    };
        
    WireMessage.prototype.encode = function() {
    	// Compute the first byte of the fixed header
    	var first = ((this.type & 0x0f) << 4);
    	
    	/*
    	 * Now calculate the length of the variable header + payload by adding up the lengths
    	 * of all the component parts
    	 */

    	remLength = 0;
    	topicStrLength = new Array();
    	
    	// if the message contains a messageIdentifier then we need two bytes for that
    	if (this.messageIdentifier != undefined)
    		remLength += 2;

    	switch(this.type) {
    	    // If this a Connect then we need to include 12 bytes for its header
	        case MESSAGE_TYPE.CONNECT:
	        	remLength += MqttProtoIdentifier.length + 3;
                remLength += UTF8Length(this.clientId) + 2;
			    if (this.willMessage != undefined) {
			    	remLength += UTF8Length(this.willMessage.destinationName) + 2;
                    // Will message is always a string, sent as UTF-8 characters with a preceding length.
				    var willMessagePayloadBytes = this.willMessage.payloadBytes;
				    if (!(willMessagePayloadBytes instanceof Uint8Array))
		        		willMessagePayloadBytes = new Uint8Array(payloadBytes);
                    remLength += willMessagePayloadBytes.byteLength +2;
    	        }
                if (this.userName != undefined)
                    remLength += UTF8Length(this.userName) + 2;
                if (this.password != undefined)
                    remLength += UTF8Length(this.password) + 2;
			break;

			// Subscribe, Unsubscribe can both contain topic strings
	        case MESSAGE_TYPE.SUBSCRIBE:	        	
	        	first |= 0x02; // Qos = 1;
	        	for ( var i = 0; i < this.topics.length; i++) {
	        		topicStrLength[i] = UTF8Length(this.topics[i]);
	        		remLength += topicStrLength[i] + 2;
	        	}
	        	remLength += this.requestedQos.length; // 1 byte for each topic's
	        	// QoS on Subscribe only
	        	break;

	        case MESSAGE_TYPE.UNSUBSCRIBE:
	        	first |= 0x02; // Qos = 1;
	        	for ( var i = 0; i < this.topics.length; i++) {
	        		topicStrLength[i] = UTF8Length(this.topics[i]);
	        		remLength += topicStrLength[i] + 2;
	        	}
	        	break;

	        case MESSAGE_TYPE.PUBLISH:
	        	if (this.payloadMessage.duplicate) first |= 0x08;
	        	first  = first |= (this.payloadMessage.qos << 1);
	        	if (this.payloadMessage.retained) first |= 0x01;
	        	destinationNameLength = UTF8Length(this.payloadMessage.destinationName);
	        	remLength += destinationNameLength + 2;	   
	        	var payloadBytes = this.payloadMessage.payloadBytes;
	        	remLength += payloadBytes.byteLength;  
	        	if (payloadBytes instanceof ArrayBuffer)
	        		payloadBytes = new Uint8Array(payloadBytes);
	        	else if (!(payloadBytes instanceof Uint8Array))
	        		payloadBytes = new Uint8Array(payloadBytes.buffer);
	        	break;

	        case MESSAGE_TYPE.DISCONNECT:
	        	break;

	        default:
	        	;
    	}

    	// Now we can allocate a buffer for the message

    	var mbi = encodeMBI(remLength);  // Convert the length to MQTT MBI format
    	var pos = mbi.length + 1;        // Offset of start of variable header
    	var buffer = new ArrayBuffer(remLength + pos);
    	var byteStream = new Uint8Array(buffer);    // view it as a sequence of bytes

    	//Write the fixed header into the buffer
    	byteStream[0] = first;
    	byteStream.set(mbi,1);

    	// If this is a PUBLISH then the variable header starts with a topic
    	if (this.type == MESSAGE_TYPE.PUBLISH)
    		pos = writeString(this.payloadMessage.destinationName, destinationNameLength, byteStream, pos);
    	// If this is a CONNECT then the variable header contains the protocol name/version, flags and keepalive time
    	
    	else if (this.type == MESSAGE_TYPE.CONNECT) {
    		byteStream.set(MqttProtoIdentifier, pos);
    		pos += MqttProtoIdentifier.length;
    		var connectFlags = 0;
    		if (this.cleanSession) 
    			connectFlags = 0x02;
    		if (this.willMessage != undefined ) {
    			connectFlags |= 0x04;
    			connectFlags |= (this.willMessage.qos<<3);
    			if (this.willMessage.retained) {
    				connectFlags |= 0x20;
    			}
    		}
    		if (this.userName != undefined)
    			connectFlags |= 0x80;
            if (this.password != undefined)
    		    connectFlags |= 0x40;
    		byteStream[pos++] = connectFlags; 
    		pos = writeUint16 (this.keepAliveInterval, byteStream, pos);
    	}

    	// Output the messageIdentifier - if there is one
    	if (this.messageIdentifier != undefined)
    		pos = writeUint16 (this.messageIdentifier, byteStream, pos);

    	switch(this.type) {
    	    case MESSAGE_TYPE.CONNECT:
    		    pos = writeString(this.clientId, UTF8Length(this.clientId), byteStream, pos); 
    		    if (this.willMessage != undefined) {
    		        pos = writeString(this.willMessage.destinationName, UTF8Length(this.willMessage.destinationName), byteStream, pos);
    		        pos = writeUint16(willMessagePayloadBytes.byteLength, byteStream, pos);
    		        byteStream.set(willMessagePayloadBytes, pos);
		        	pos += willMessagePayloadBytes.byteLength;
    		        
    	        }
    		if (this.userName != undefined) 
    			pos = writeString(this.userName, UTF8Length(this.userName), byteStream, pos);
    		if (this.password != undefined) 
    			pos = writeString(this.password, UTF8Length(this.password), byteStream, pos);
    		break;

    	    case MESSAGE_TYPE.PUBLISH:	
    	    	// PUBLISH has a text or binary payload, if text do not add a 2 byte length field, just the UTF characters.	
    	    	byteStream.set(payloadBytes, pos);
    	    		
    	    	break;

//    	    case MESSAGE_TYPE.PUBREC:	
//    	    case MESSAGE_TYPE.PUBREL:	
//    	    case MESSAGE_TYPE.PUBCOMP:	
//    	    	break;

    	    case MESSAGE_TYPE.SUBSCRIBE:
    	    	// SUBSCRIBE has a list of topic strings and request QoS
    	    	for (var i=0; i<this.topics.length; i++) {
    	    		pos = writeString(this.topics[i], topicStrLength[i], byteStream, pos);
    	    		byteStream[pos++] = this.requestedQos[i];
    	    	}
    	    	break;

    	    case MESSAGE_TYPE.UNSUBSCRIBE:	
    	    	// UNSUBSCRIBE has a list of topic strings
    	    	for (var i=0; i<this.topics.length; i++)
    	    		pos = writeString(this.topics[i], topicStrLength[i], byteStream, pos);
    	    	break;

    	    default:
    	    	// Do nothing.
    	}

    	return buffer;
    }	

    function decodeMessage(input) {
    	//var msg = new Object();  // message to be constructed
    	var first = input[0];
    	var type = first >> 4;
    	var messageInfo = first &= 0x0f;
    	var pos = 1;
    	

    	// Decode the remaining length (MBI format)

    	var digit;
    	var remLength = 0;
    	var multiplier = 1;
    	do {
    		digit = input[pos++];
    		remLength += ((digit & 0x7F) * multiplier);
    		multiplier *= 128;
    	} while ((digit & 0x80) != 0);

    	var wireMessage = new WireMessage(type);
    	switch(type) {
            case MESSAGE_TYPE.CONNACK:
    	    	wireMessage.topicNameCompressionResponse = input[pos++];
    	        wireMessage.returnCode = input[pos++];
    		    break;
    	    
    	    case MESSAGE_TYPE.PUBLISH:     	    	
    	    	var qos = (messageInfo >> 1) & 0x03;
    	    	   		    
    	    	var len = readUint16(input, pos);
    		    pos += 2;
    		    var topicName = parseUTF8(input, pos, len);
    		    pos += len;
    		    // If QoS 1 or 2 there will be a messageIdentifier
                if (qos > 0) {
    		        wireMessage.messageIdentifier = readUint16(input, pos);
    		        pos += 2;
                }
                
                var message = new Messaging.Message(input.subarray(pos));
                if ((messageInfo & 0x01) == 0x01) 
    	    		message.retained = true;
    	    	if ((messageInfo & 0x08) == 0x08)
    	    		message.duplicate =  true;
                message.qos = qos;
                message.destinationName = topicName;
                wireMessage.payloadMessage = message;	
    		    break;
    	    
    	    case  MESSAGE_TYPE.PUBACK:
    	    case  MESSAGE_TYPE.PUBREC:	    
    	    case  MESSAGE_TYPE.PUBREL:    
    	    case  MESSAGE_TYPE.PUBCOMP:
    	    case  MESSAGE_TYPE.UNSUBACK:    	    	
    	    	wireMessage.messageIdentifier = readUint16(input, pos);
        		break;
    		    
    	    case  MESSAGE_TYPE.SUBACK:
    	    	wireMessage.messageIdentifier = readUint16(input, pos);
        		pos += 2;
    	        wireMessage.grantedQos = input.subarray(pos);	
    		    break;
    	
    	    default:
    	    	;
    	}
    	    	
    	return wireMessage;	
    }

    function writeUint16(input, buffer, offset) {
    	buffer[offset++] = input >> 8;      //MSB
    	buffer[offset++] = input % 256;     //LSB 
    	return offset;
    }	

    function writeString(input, utf8Length, buffer, offset) {
    	offset = writeUint16(utf8Length, buffer, offset);
    	stringToUTF8(input, buffer, offset);
    	return offset + utf8Length;
    }	

    function readUint16(buffer, offset) {
    	return 256*buffer[offset] + buffer[offset+1];
    }	

    /**
     * Encodes an MQTT Multi-Byte Integer
     * @private 
     */
    function encodeMBI(number) {
    	var output = new Array(1);
    	var numBytes = 0;

    	do {
    		var digit = number % 128;
    		number = number >> 7;
    		if (number > 0) {
    			digit |= 0x80;
    		}
    		output[numBytes++] = digit;
    	} while ( (number > 0) && (numBytes<4) );

    	return output;
    }

    /**
     * Takes a String and calculates its length in bytes when encoded in UTF8.
     * @private
     */
    function UTF8Length(input) {
    	var output = 0;
    	for (var i = 0; i<input.length; i++) 
    	{
    		var charCode = input.charCodeAt(i);
    		if (charCode > 0x7FF)
    			output +=3;
    		else if (charCode > 0x7F)
    			output +=2;
    		else
    			output++;
    	} 
    	return output;
    }
    
    /**
     * Takes a String and writes it into an array as UTF8 encoded bytes.
     * @private
     */
    function stringToUTF8(input, output, start) {
    	var pos = start;
    	for (var i = 0; i<input.length; i++) {
    		var charCode = input.charCodeAt(i);
    		if (charCode <= 0x7F) {
    			output[pos++] = charCode;
    		} else if (charCode <= 0x7FF) {
    			output[pos++] = charCode>>6  & 0x1F | 0xC0;
    			output[pos++] = charCode     & 0x3F | 0x80;
    		} else if (charCode <= 0xFFFF) {    				    
    	        output[pos++] = charCode>>12 & 0x0F | 0xE0;
        		output[pos++] = charCode>>6  & 0x3F | 0x80;   
        		output[pos++] = charCode     & 0x3F | 0x80;   
    		} else {
    			output[pos++] = charCode>>18 & 0x07 | 0xF0;
        		output[pos++] = charCode>>12 & 0x3F | 0x80;
        		output[pos++] = charCode>>6  & 0x3F | 0x80;
        		output[pos++] = charCode     & 0x3F | 0x80;
    		};
    	} 
    	return output;
    }
    
    function parseUTF8(input, offset, length) {
    	var output = "";
    	var utf16;
    	var pos = offset;

    	while (pos < offset+length)
    	{
    		var byte1 = input[pos++];
    		if (byte1 < 128)
    			utf16 = byte1;
    		else 
    		{
    			var byte2 = input[pos++]-128;
    			if (byte2 < 0) 
    				throw new Error(format(MALFORMED_UTF, [byte1.toString(16),byte2.toString(16),""]));
    			if (byte1 < 0xE0)             // 2 byte character
    				utf16 = 64*(byte1-0xC0) + byte2;
    			else 
    			{ 
    				var byte3 = input[pos++]-128;
    				if (byte3 < 0) 
    					throw new Error(format(MALFORMED_UTF, [byte1.toString(16), byte2.toString(16),byte3.toString(16)]));
    				if (byte1 < 0xF0)        // 3 byte character
    					utf16 = 4096*(byte1-0xE0) + 64*byte2 + byte3;
    				else                     // longer encodings are not supported  
    					throw new Error(format(MALFORMED_UTF, [byte1.toString(16), byte2.toString(16), byte3.toString(16)]));
    			}; 
    		}  
    		output += String.fromCharCode(utf16);
    	}
    	return output;
    }
    
    var pingReq = new WireMessage(MESSAGE_TYPE.PINGREQ).encode(); 
    
    /** @ignore Repeat keepalive requests, monitor responses.*/
    var Pinger = function(client, window, keepAliveInterval) { 
    	this._client = client;        	
     	this._window = window;
     	this._keepAliveInterval = keepAliveInterval*1000;     	
        this.isReset = false;
        
        var doTimeout = function (pinger) {
	        return function () {
	            return doPing.apply(pinger);
	        };
	    };
        var doPing = function() { 
        	if (!this.isReset) {
        		this._client._trace("Pinger.doPing", "Timed out");
        		this._client._disconnected( ERROR.PING_TIMEOUT.code , format(ERROR.PING_TIMEOUT));
        	} else {
        	    this.isReset = false;
        	    this._client._trace("Pinger.doPing", "send PINGREQ");
                this._client.socket.send(pingReq); 
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

	/** @ignore Monitor request completion. */
	var Timeout = function(client, window, timeoutSeconds, action, args) {
		this._window = window;
		/*if (!timeoutSeconds)*/
			timeoutSeconds = 300;
		
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
	 * Internal implementation of the Websockets MQTT V3.1 client.
	 * 
	 * @name Messaging.ClientImpl @constructor @param {String} host the DNS name
	 * of the webSocket host. @param {Number} port the port number kin the host.
	 * @param {String} clientId the MQ client identifier.
	 */
    var ClientImpl = function (host, port, clientId) {
        this._trace("Messaging.Client", host, port, clientId);

        if (!("WebSocket" in global && global["WebSocket"] !== null)) {
        	throw new Error(format(UNSUPPORTED, ["WebSocket"]));
	    } 
        if (!("localStorage" in global && global["localStorage"] !== null)) {
        	throw new Error(format(UNSUPPORTED, ["localStorage"]));
        }
        if (!("ArrayBuffer" in global && global["ArrayBuffer"] !== null)) {
        	throw new Error(format(UNSUPPORTED, ["ArrayBuffer"]));
        }

        this.host = host;
        this.port = port;
        this.clientId = clientId;

        // Local storagekeys are qualified with the following string.
        this._localKey=host+":"+port+":"+clientId+":";

        // Create private instance-only message queue
        // Internal queue of messages to be sent, in sending order. 
        this._msg_queue = [];

        // Messages we have sent and expecting a response for indexed by their respective message ids. 
        this._sentMessages = {};

        // Messages we have received and acknowleged and are expecting a confirm message for
        // indexed by their respective message ids. 
        this._receivedMessages = {};
 
        // Internal list of callbacks to be executed when messages
        // have been successfully sent over web socket, e.g. disconnect
        // when it doesn't have to wait for ACK, just message is dispatched.
        this._notify_msg_sent = {};

        // Unique identifier for SEND messages, incrementing
        // counter as messages are sent.
        this._message_identifier = 1;

        // Load the local state, if any,  from the saved version.   	
        for(key in localStorage)
        	this.restore(key);
    };

    // Messaging Client public instance members. 
    ClientImpl.prototype.host;
    ClientImpl.prototype.port;
    ClientImpl.prototype.clientId;

    // Messaging Client private instance members.
    ClientImpl.prototype.socket;
    /* true once we have received an acknowledgement to a CONNECT packet. */
    ClientImpl.prototype.connected = false;
    /* The largest message identifier allowed, may not be larger than 2**16 but 
     * if set smaller reduces the maximum number of outbound messages allowed.
     */ 
    ClientImpl.prototype.maxMessageIdentifier = 65536;
    ClientImpl.prototype.connectOptions;
    ClientImpl.prototype.onConnectionLost;
    ClientImpl.prototype.onMessageDelivered;
    ClientImpl.prototype.onMessageArrived;
    ClientImpl.prototype._msg_queue = null;
    ClientImpl.prototype._connectTimeout;
    /* Send keep alive messages. */
    ClientImpl.prototype.pinger = null;
    
    ClientImpl.prototype._traceBuffer = null;
    ClientImpl.prototype._MAX_TRACE_ENTRIES = 100;

    ClientImpl.prototype.connect = function (connectOptions) {
    	this._trace("Client.connect", connectOptions, this.socket, this.connected);
        
    	if (this.connected) 
        	throw new Error("Invalid state, already connected.");
    	if (this.socket)
    		throw new Error("Invalid state, already connecting.");
        
    	this.connectOptions = connectOptions;
        
        // When the socket is open, this client will send the CONNECT WireMessage using the saved parameters. 
        if (connectOptions.useSSL)
          wsurl = ["wss://", this.host, ":", this.port, "/mqtt"].join("");
        else
          wsurl = ["ws://", this.host, ":", this.port, "/mqtt"].join("");
        this.connected = false;
        this.socket = new WebSocket(wsurl, 'mqttv3.1');
        this.socket.binaryType = 'arraybuffer';
        this.socket.onopen = scope(this._on_socket_open, this);
        this.socket.onmessage = scope(this._on_socket_message, this);
        this.socket.onerror = scope(this._on_socket_error, this);
        this.socket.onclose = scope(this._on_socket_close, this);
        
        // If no keep alive interval is set in the connect options, the server assumes 60 seconds.
        if (connectOptions.keepAliveInterval === undefined)
        	this.pinger = new Pinger(this, window, 60);
        else
            this.pinger = new Pinger(this, window, connectOptions.keepAliveInterval);
        
        this._connectTimeout = new Timeout(this, window, connectOptions.timeout, this._disconnected,  [ERROR.CONNECT_TIMEOUT.code, format(ERROR.CONNECT_TIMEOUT)]);
    };

    ClientImpl.prototype.subscribe = function (filter, subscribeOptions) {
    	this._trace("Client.subscribe", filter, subscribeOptions);
              
    	if (!this.connected)
    	    throw new Error("InvalidState, not connected.");
    	
        var wireMessage = new WireMessage(MESSAGE_TYPE.SUBSCRIBE);
        wireMessage.topics=[filter];
        if (subscribeOptions.qos != undefined)
        	wireMessage.requestedQos = [subscribeOptions.qos];
        else 
        	wireMessage.requestedQos = [0];
        
        if (subscribeOptions.onSuccess) {
            wireMessage.callback = function() {subscribeOptions.onSuccess(subscribeOptions.invocationContext);};
        }
        if (subscribeOptions.timeout) {
        	wireMessage.timeOut = new Timeout(this, window, subscribeOptions.timeout, subscribeOptions.onFailure
        			, [subscribeOptions.invocationContext, ERROR.SUBSCRIBE_TIMEOUT.code , format(ERROR.SUBSCRIBE_TIMEOUT)]);
        }
        
        // All subscriptions return a SUBACK. 
        this._requires_ack(wireMessage);
        this._schedule_message(wireMessage);
    };

    /** @ignore */
    ClientImpl.prototype.unsubscribe = function(filter, unsubscribeOptions) {  
    	this._trace("Client.unsubscribe", filter, unsubscribeOptions);
        
    	if (!this.connected)
    	   throw new Error("InvalidState, not connected.");
    	
    	var wireMessage = new WireMessage(MESSAGE_TYPE.UNSUBSCRIBE);
        wireMessage.topics = [filter];
        
        if (unsubscribeOptions.onSuccess) {
        	wireMessage.callback = function() {unsubscribeOptions.onSuccess(unsubscribeOptions.invocationContext);};
        }
        if (unsubscribeOptions.timeout) {
        	wireMessage.timeOut = new Timeout(this, window, unsubscribeOptions.timeout, unsubscribeOptions.onFailure
        			, [unsubscribeOptions.invocationContext, ERROR.UNSUBSCRIBE_TIMEOUT.code , format(ERROR.UNSUBSCRIBE_TIMEOUT)]);
        }
     
        // All unsubscribes return a SUBACK.         
        this._requires_ack(wireMessage);
        this._schedule_message(wireMessage);
    };
    
    
    ClientImpl.prototype.send = function (message) {
        this._trace("Client.send", message);

        if (!this.connected)
           throw new Error("InvalidState, not connected.");
        
        wireMessage = new WireMessage(MESSAGE_TYPE.PUBLISH);
        wireMessage.payloadMessage = message;
        
        if (message.qos > 0)
            this._requires_ack(wireMessage);
        else if (this.onMessageDelivered)
        	this._notify_msg_sent[wireMessage] = this.onMessageDelivered(wireMessage.payloadMessage);
        this._schedule_message(wireMessage);
    };
    
    ClientImpl.prototype.disconnect = function () {
        this._trace("Client.disconnect");

        if (!this.socket)
    		throw new Error("Invalid state, not connecting or connected.");
        
        wireMessage = new WireMessage(MESSAGE_TYPE.DISCONNECT);

        // Run the disconnected call back as soon as the message has been sent,
        // in case of a failure later on in the disconnect processing.
        // as a consequence, the _disconected call back may be run several times.
        this._notify_msg_sent[wireMessage] = scope(this._disconnected, this);

        this._schedule_message(wireMessage);
    };
    
   ClientImpl.prototype.getTraceLog = function () {
        if ( this._traceBuffer !== null ) {
            this._trace("Client.getTraceLog", new Date());
            this._trace("Client.getTraceLog in flight messages", this._sentMessages.length);
            for (key in this._sentMessages)
                this._trace("_sentMessages ",key, this._sentMessages[key]);
            for (key in this._receivedMessages)
                this._trace("_receivedMessages ",key, this._receivedMessages[key]);

            return this._traceBuffer;
        }
    };

    ClientImpl.prototype.startTrace = function () {
        if ( this._traceBuffer === null ) {
            this._traceBuffer = [];
        }
        this._trace("Client.startTrace", new Date());
    };

    ClientImpl.prototype.stopTrace = function () {
        delete this._traceBuffer;
    };

    // Schedule a new message to be sent over the WebSockets
    // connection. CONNECT messages cause WebSocket connection
    // to be started. All other messages are queued internally
    // until this has happened. When WS connection starts, process
    // all outstanding messages. 
    ClientImpl.prototype._schedule_message = function (message) {
        this._msg_queue.push(message);
        // Process outstanding messages in the queue if we have an  open socket, and have received CONNACK. 
        if (this.connected) {
            this._process_queue();
        }
    };

    ClientImpl.prototype.store = function(prefix, wireMessage) {
    	storedMessage = {type:wireMessage.type, messageIdentifier:wireMessage.messageIdentifier, version:1};
    	switch(wireMessage.type) {
	      case MESSAGE_TYPE.PUBLISH:
	    	  if(wireMessage.pubRecReceived)
	    		  storedMessage.pubRecReceived = true;
	    	  
	    	  // Convert the payload tro a hex string.
	    	  storedMessage.payloadMessage = {};
	    	  var hex = "";
	          var messageBytes = wireMessage.payloadMessage.payloadBytes;
	          for (i=0;i<messageBytes.length;i++) {
	            if (messageBytes[i] <= 0xF)
	              hex = hex+"0"+messageBytes[i].toString(16);
	            else 
	              hex = hex+messageBytes[i].toString(16);
	          }
	    	  storedMessage.payloadMessage.payloadHex = hex;
	    	  
	    	  storedMessage.payloadMessage.qos = wireMessage.payloadMessage.qos;
	    	  storedMessage.payloadMessage.destinationName = wireMessage.payloadMessage.destinationName;
	    	  if (wireMessage.payloadMessage.duplicate) 
	    		  storedMessage.payloadMessage.duplicate = true;
	    	  if (wireMessage.payloadMessage.retained) 
	    		  storedMessage.payloadMessage.retained = true;	        	
	          break;    
	          
	        default:
	        	throw Error("Invalid type.");
  	    }
    	localStorage.setItem(prefix+this._localKey+wireMessage.messageIdentifier, JSON.stringify(storedMessage));
    };
    
    ClientImpl.prototype.restore = function(key) {
    	var storedMessage = JSON.parse(localStorage.getItem(key));
    	
    	var wireMessage = new WireMessage(storedMessage.type, storedMessage);
    	
    	switch(storedMessage.type) {
	      case MESSAGE_TYPE.PUBLISH:
	    	  // Replace the payload message with a Message object.
	    	  var hex = storedMessage.payloadMessage.payloadHex;
	    	  var buffer = new ArrayBuffer((hex.length)/2);
              var byteStream = new Uint8Array(buffer); 
              var i = 0;
              while (hex.length >= 2) { 
            	  var x = parseInt(hex.substring(0, 2), 16);
	              hex = hex.substring(2, hex.length);
	              byteStream[i++] = x;
	          }
              var payloadMessage = new Messaging.Message(byteStream);
	      	  
	    	  payloadMessage.qos = storedMessage.payloadMessage.qos;
	    	  payloadMessage.destinationName = storedMessage.payloadMessage.destinationName;
	    	  if (storedMessage.payloadMessage.duplicate) 
	    		  payloadMessage.duplicate = true;
	    	  if (storedMessage.payloadMessage.retained) 
	    		  payloadMessage.retained = true;	 
	    	  wireMessage.payloadMessage = payloadMessage;
	          break;    
	          
	        default:
	        	throw Error("Invalid type.");
	    }
    	
    	
//    	var buffer = new ArrayBuffer((hex.length)/2);
//    	var byteStream = new Uint8Array(buffer); 
//    	var i = 0;
//    	while (storedMessage.hex.length >= 2) { 
//          var x = parseInt(storedMessage.hex.substring(0, 2), 16);
//          storedMessage.hex = message.hex.substring(2, storedMessage.hex.length);
//          byteStream[i++] = x;
//        }
//    	var payloadMessage = new Messaging.Message(byteStream);
//        if ((messageInfo & 0x01) == 0x01) 
//    		payloadMessage.retained = true;
//    	if ((messageInfo & 0x08) == 0x08)
//    		payloadMessage.duplicate =  true;
//        payloadMessage.qos = qos;
//        payloadMessage.destinationName = topicName;
       
    	
    	
    		    	
    	if (key.indexOf("Sent:"+this._localKey) == 0) {      
    		this._sentMessages[wireMessage.messageIdentifier] = wireMessage;    		    
    	} else if (key.indexOf("Received:"+this._localKey) == 0) {
    		this._receivedMessages[wireMessage.messageIdentifier] = wireMessage;
    	}
    };
    
    ClientImpl.prototype._process_queue = function () {
        var message = null;
        // Process messages in order they were added
        var fifo = this._msg_queue.reverse();

        // Send all queued messages down socket connection
        while ((message = fifo.pop())) {
            this._socket_send(message);
            // Notify listeners that message was successfully sent
            if (this._notify_msg_sent[message]) {
                this._notify_msg_sent[message]();
                delete this._notify_msg_sent[message];
            }
        }
    };

    /**
     * @ignore
     * Expect an ACK response for this message. Add message to the set of in progress
     * messages and set an unused identifier in this message.
     */
    ClientImpl.prototype._requires_ack = function (wireMessage) {
    	var messageCount = Object.keys(this._sentMessages).length;
        if (messageCount > this.maxMessageIdentifier)
            throw Error ("Too many messages:"+messageCount);

        while(this._sentMessages[this._message_identifier] !== undefined) {
            this._message_identifier++;
        }
        wireMessage.messageIdentifier = this._message_identifier;
        this._sentMessages[wireMessage.messageIdentifier] = wireMessage;
        if (wireMessage.type === MESSAGE_TYPE.PUBLISH) {
        	this.store("Sent:", wireMessage);
        }
        if (this._message_identifier === this.maxMessagIdentifier) {
            this._message_identifier = 1;
        }
    };

    /** 
     * @ignore
     * Called when the underlying websocket has been opened.
     */
    ClientImpl.prototype._on_socket_open = function () {        
        // Create the CONNECT message object.
        var wireMessage = new WireMessage(MESSAGE_TYPE.CONNECT, this.connectOptions); 
        wireMessage.clientId = this.clientId;
        this._socket_send(wireMessage);
    };

    /** 
     * @ignore
     * Called when the underlying websocket has received a complete packet.
     */
    ClientImpl.prototype._on_socket_message = function (event) {
        this._trace("Client._on_socket_message", event.data);
        
        // Reset the ping timer.
        this.pinger.reset();
        var byteArray = new Uint8Array(event.data);
        try {
            var wireMessage = decodeMessage(byteArray);
        } catch (error) {
        	this._disconnected(ERROR.INTERNAL_ERROR.code , format(ERROR.INTERNAL_ERROR, [error.message]));
        	return;
        }
        this._trace("Client._on_socket_message", wireMessage);

        switch(wireMessage.type) {
            case MESSAGE_TYPE.CONNACK:
            	this._connectTimeout.cancel();
            	
            	// If we have started using clean session then clear up the local state.
            	if (this.connectOptions.cleanSession) {
    		    	for (key in this._sentMessages) {	    		
    		    	    var sentMessage = this._sentMessages[key];
    					localStorage.removeItem("Sent:"+this._localKey+sentMessage.messageIdentifier);
    		    	}
    				this._sentMessages = {};

    				for (key in this._receivedMessages) {
    					var receivedMessage = this._receivedMessages[key];
    					localStorage.removeItem("Received:"+this._localKey+receivedMessage.messageIdentifier);
    				}
    				this._receivedMessages = {};
            	}
            	// Client connected and ready for business.
            	if (wireMessage.returnCode === 0) {
        	        this.connected = true;
                } else {
                    this._disconnected(ERROR.CONNACK_RETURNCODE.code , format(ERROR.CONNACK_RETURNCODE, [wireMessage.returnCode]));
                    break;
                }
            	
        	    // Resend messages.
        	    // TODO sort sentMessages into time order.
        	    for (key in this._sentMessages) {
        	    	var sentMessage = this._sentMessages[key];
        	    	if (sentMessage.type == MESSAGE_TYPE.PUBLISH && sentMessage.pubRecReceived) {
        	    	    var pubRelMessage = new WireMessage(MESSAGE_TYPE.PUBREL, {messageIdentifier:sentMessage.messageIdentifier});
        	            this._schedule_message(pubRelMessage);
        	    	} else {
        	    		this._schedule_message(sentMessage);
        	    	};
        	    }

        	    // Execute the connectOptions.onSuccess callback if there is one.
        	    if (this.connectOptions.onSuccess) {
        	        this.connectOptions.onSuccess(this.connectOptions.invocationContext);
        	    }

        	    // Process all queued messages now that the connection is established. 
        	    this._process_queue();
        	    break;
        
            case MESSAGE_TYPE.PUBLISH:
                this._receivePublish(wireMessage);
                break;

            case MESSAGE_TYPE.PUBACK:
            	var sentMessage = this._sentMessages[wireMessage.messageIdentifier];
                 // If this is a re flow of a PUBACK after we have restarted receivedMessage will not exist.
            	if (sentMessage) {
                    delete this._sentMessages[wireMessage.messageIdentifier];
                    localStorage.removeItem("Sent:"+this._localKey+wireMessage.messageIdentifier);
                    if (this.onMessageDelivered)
                    	this.onMessageDelivered(sentMessage.payloadMessage);
                }
            	break;
            
            case MESSAGE_TYPE.PUBREC:
                var sentMessage = this._sentMessages[wireMessage.messageIdentifier];
                // If this is a re flow of a PUBREC after we have restarted receivedMessage will not exist.
                if (sentMessage) {
                	sentMessage.pubRecReceived = true;
                    var pubRelMessage = new WireMessage(MESSAGE_TYPE.PUBREL, {messageIdentifier:wireMessage.messageIdentifier});
                    this.store("Sent:", sentMessage);
                    this._schedule_message(pubRelMessage);
                }
                break;
            	            	
            case MESSAGE_TYPE.PUBREL:
                var receivedMessage = this._receivedMessages[wireMessage.messageIdentifier];
                localStorage.removeItem("Received:"+this._localKey+wireMessage.messageIdentifier);
                // If this is a re flow of a PUBREL after we have restarted receivedMessage will not exist.
                if (receivedMessage) {
                    this._receiveMessage(receivedMessage);
                    pubCompMessage = new WireMessage(MESSAGE_TYPE.PUBCOMP, {messageIdentifier:wireMessage.messageIdentifier});
                    this._schedule_message(pubCompMessage);                    
                    delete this._receivedMessages[wireMessage.messageIdentifier];
                }
                
                break;

            case MESSAGE_TYPE.PUBCOMP: 
            	var sentMessage = this._sentMessages[wireMessage.messageIdentifier];
            	delete this._sentMessages[wireMessage.messageIdentifier];
                localStorage.removeItem("Sent:"+this._localKey+wireMessage.messageIdentifier);
                if (this.onMessageDelivered)
                	this.onMessageDelivered(sentMessage.payloadMessage);
                break;
                
            case MESSAGE_TYPE.SUBACK:
                var sentMessage = this._sentMessages[wireMessage.messageIdentifier];
                if (sentMessage) {
                	if(sentMessage.timeOut)
                	    sentMessage.timeOut.cancel();
                    if (sentMessage.callback) {
                        sentMessage.callback();
                    }
                    delete this._sentMessages[wireMessage.messageIdentifier];
                }
                break;
        	    
            case MESSAGE_TYPE.UNSUBACK:
            	var sentMessage = this._sentMessages[wireMessage.messageIdentifier];
                if (sentMessage) { 
                	if (sentMessage.timeOut)
                        sentMessage.timeOut.cancel();
                    if (sentMessage.callback) {
                        sentMessage.callback();
                    }
                    delete this._sentMessages[wireMessage.messageIdentifier];
                }

                break;
                
            case MESSAGE_TYPE.PINGRESP:
            	break;
            	
            case MESSAGE_TYPE.DISCONNECT:
            	//TODO
            	break;

            default:
                throw new Error("Invalid message type:"+wireMessage.type); 
        }; 
    };
    
    /** @ignore */
    ClientImpl.prototype._on_socket_error = function (error) {
    	this._disconnected(ERROR.SOCKET_ERROR.code , format(ERROR.SOCKET_ERROR, [error.data]));
    };

    /** @ignore */
    ClientImpl.prototype._on_socket_close = function () {
        this._disconnected(ERROR.SOCKET_CLOSE.code , format(ERROR.SOCKET_CLOSE));
    };

    /** @ignore */
    ClientImpl.prototype._socket_send = function (wireMessage) {
        this._trace("Client._socket_send", wireMessage);
        
        this.socket.send(wireMessage.encode());
        this.pinger.reset();
    };
    
    /** @ignore */
    ClientImpl.prototype._receivePublish = function (wireMessage) {
        switch(wireMessage.payloadMessage.qos) {
            case "undefined":
            case 0:
                this._receiveMessage(wireMessage);
                break;

            case 1:
                var pubAckMessage = new WireMessage(MESSAGE_TYPE.PUBACK, {messageIdentifier:wireMessage.messageIdentifier});
                this._schedule_message(pubAckMessage);
                this._receiveMessage(wireMessage);
                break;

            case 2:
                this._receivedMessages[wireMessage.messageIdentifier] = wireMessage;
                this.store("Received:", wireMessage);
                var pubRecMessage = new WireMessage(MESSAGE_TYPE.PUBREC, {messageIdentifier:wireMessage.messageIdentifier});
                this._schedule_message(pubRecMessage);

                break;

            default:
                throw Error("Invalid qos="+wireMmessage.payloadMessage.qos);
        };
    };

    /** @ignore */
    ClientImpl.prototype._receiveMessage = function (wireMessage) {
        if (this.onMessageArrived) {
            this.onMessageArrived(wireMessage.payloadMessage);
        }
    };

    /**
     * @ignore
     * Client has disconnected either at its own request or because the server
     * or network disconnected it. Remove all non-durable state.
     * @param {errorCode} [number] the error number.
     * @param {errorText} [string] the error text.
     */
    ClientImpl.prototype._disconnected = function (errorCode, errorText) {
    	this.pinger.cancel();
    	if (this._connectTimeout)
    	    this._connectTimeout.cancel();
    	// Clear message buffers.
        this._msg_queue = [];
        this._notify_msg_sent = {};
       
        if (this.socket) {
            // Cancel all socket callbacks so that they cannot be driven again by this socket.
            this.socket.onopen = null;
            this.socket.onmessage = null;
            this.socket.onerror = null;
            this.socket.onclose = null;
            this.socket.close();
            delete this.socket;           
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
            	this.connectOptions.onFailure(this.connectOptions.invocationContext, errorCode, errorText);
        }
    };

    /** @ignore */
    ClientImpl.prototype._trace = function () {
        if ( this._traceBuffer !== null ) {  
            for (var i = 0, max = arguments.length; i < max; i++) {
                if ( this._traceBuffer == this._MAX_TRACE_ENTRIES ) {    
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
     * The Javascript application communicates to the server using a Messaging.Client object. 
     * Most applications will create just one Client object and then call its connect() method,
     * however applications can create more than one Client object if they wish. 
     * In this case the combination of host, port and clientId attributes must be different for each Client object.
     * <p>
     * The send, subscribe and unsubscribe methods are implemented as asynchronous Javascript methods 
     * (even though the underlying protocol exchange might be synchronous in nature). 
     * This means they signal their completion by calling back to the application, 
     * via Success or Failure callback functions provided by the application on the method in question. 
     * Such callbacks are called at most once per method invocation and do not persist beyond the lifetime 
     * of the script that made the invocation.
     * <p>
     * In contrast there are some callback functions <i> most notably onMessageArrived</i> 
     * that are defined on the Messaging.Client object.  
     * These may get called multiple times, and aren't directly related to specific method invocations made by the client. 
     *
     * @name Messaging.Client    
     * 
     * @constructor
     * Creates a Messaging.Client object that can be used to communicate with a Messaging server.
     *  
     * @param {string} host the address of the messaging server, as a DNS name or dotted decimal IP address.
     * @param {number} port the port number in the host to connect to.
     * @param {string} clientId the Messaging client identifier, between 1 and 23 characters in length.
     * 
     * @property {string} host <i>read only</i> the servers DNS hostname or dotted decimal IP address.
     * @property {number} port <i>read only</i> the servers port.
     * @property {string} clientId <i>read only</i> used when connecting to the server.
     * @property {function} onConnectionLost called when a connection has been lost, 
     * after a connect() method has succeeded.
     * Establish the call back used when a connection has been lost. The connection may be
     * lost because the client initiates a disconnect or because the server or network 
     * cause the client to be disconnected. The disconnect call back may be called without 
     * the connectionComplete call back being invoked if, for example the client fails to 
     * connect.
     * Parameters passed to the onConnectionLost callback are:
     * <ol>   
     * <li>reasonCode
     * <li>reasonMessage       
     * </ol>
     * @property {function} onMessageDelivered called when a message has been delivered. 
     * All processing that this Client will ever do has been completed. So, for example,
     * in the case of a Qos=2 message sent by this client, the PubComp flow has been received from the server
     * and the message has been removed from persistent storage before this callback is invoked. 
     * Parameters passed to the onMessageDelivered callback are:
     * <ol>   
     * <li>Messaging.Message that was delivered.
     * </ol>    
     * @property {function} onMessageArrived called when a message has arrived in this Messaging.client. 
     * Parameters passed to the onMessageArrived callback are:
     * <ol>   
     * <li>Messaging.Message that has arrived.
     * </ol>    
     */
    var Client = function (host, port, clientId) {
    	if (typeof host !== "string")
        	throw new Error("Invalid argument:"+host);
    	if (typeof port !== "number" || port < 0)
        	throw new Error("Invalid argument:"+port);
        if (typeof clientId !== "string" || clientId.length < 1 | clientId.length > 23)
        	throw new Error("Invalid argument:"+clientId); 
    	
        var client = new ClientImpl(host, port, clientId);
        this.__defineGetter__("host", function() { return client.host; });
    	this.__defineSetter__("host", function() { throw new Error("Unsupported operation."); });
         	
        this.__defineGetter__("port", function() { return client.port; });
    	this.__defineSetter__("port", function() { throw new Error("Unsupported operation."); });
    	
    	this.__defineGetter__("clientId", function() { return client.clientId; });
    	this.__defineSetter__("clientId", function() { throw new Error("Unsupported operation."); });
        

        this.__defineGetter__("onConnectionLost", function() { return client.onConnectionLost; });
         	this.__defineSetter__("onConnectionLost", function(newOnConnectionLost) { 
            if (typeof newOnConnectionLost === "function")
            	client.onConnectionLost = newOnConnectionLost;
            else 
    			throw new Error("Invalid argument:"+newOnConnectionLost);
            });

        this.__defineGetter__("onMessageDelivered", function() { return client.onMessageDelivered; });
    	this.__defineSetter__("onMessageDelivered", function(newOnMessageDelivered) { 
    		if (typeof newOnMessageDelivered === "function")
    			client.onMessageDelivered = newOnMessageDelivered;
    		else 
    			throw new Error("Invalid argument:"+newOnMessageDelivered);
    		});
       
        this.__defineGetter__("onMessageArrived", function() { return client.onMessageArrived; });
    	this.__defineSetter__("onMessageArrived", function(newOnMessageArrived) { 
    		if (typeof newOnMessageArrived === "function")
    			client.onMessageArrived = newOnMessageArrived;
    		else 
    			throw new Error("Invalid argument:"+newOnMessageArrived);
    		});
        
        /** 
         * Connect this Messaging client to its server. 
         * 
         * @name Messaging.Client#connect
         * @function
         * @param {Object} [connectOptions] attributes used with the connection. 
         * <p>
         * Properties of the connect options are: 
         * @config {number} [timeout] If the connect has not succeeded within this number of seconds, it is deemed to have failed.
         *                            The default is 30 seconds.
         * @config {string} [userName] Authentication username for this connection.
         * @config {string} [password] Authentication password for this connection.
         * @config {Messaging.Message} [willMessage] sent by the server when the client disconnects abnormally.
         * @config {Number} [keepAliveInterval] the server disconnects this client if there is no activity for this
         *                number of seconds. The default value of 60 seconds is assumed by the server if not set.
         * @config {boolean} [cleanSession] if true(default) the client and server persistent state is deleted on succesful connect.
         * @config {boolean} [useSSL] if present and true, use an SSL Websocket connection.
         * @config {object} [invocationContext] passed to the onSuccess callback or onFailure callback.
         * @config {function} [onSuccess] called when the message has been written to the network
         * parameters passed to the onSuccess callback are:
         * <ol>
         * <li>invovcationContext as passed in to the send method in the sendOptions.       
         * </ol>
         * @config {function} [onFailure] called when the unsubscribe request has failed or timed out.
         * parameters passed to the onFailure callback are:
         * <ol>
         * <li>invovcationContext as passed in to the send method in the sendOptions.       
         * <li>errorCode
         * <li>errorMessage       
         * </ol>
         * @throws {InvalidState} if the client is not in disconnected state. The client must have received connectionLost
         * or disconnected before calling connect for a second or subsequent time.
         */
        this.connect = function (connectOptions) {
        	connectOptions = connectOptions || {} ;
        	validate(connectOptions,  {timeout:"number",
        			                   userName:"string", 
        		                       password:"string", 
        		                       willMessage:"object", 
        		                       keepAliveInterval:"number", 
        		                       cleanSession:"boolean", 
        		                       useSSL:"boolean",
        		                       invocationContext:"object", 
      		                           onSuccess:"function", 
      		                           onFailure:"function"});
        	if (connectOptions.willMessage) {
                if (!(connectOptions.willMessage instanceof Message))
            	    throw new Error("Invalid argument:"+connectOptions.willMessage);
                // The will message must have a payload that can be represented as a string.
                // Cause the willMessage to throw an exception if this is not the case.
            	connectOptions.willMessage.stringPayload;
            	
            	if (typeof connectOptions.willMessage.destinationName === "undefined")
                	throw new Error("Invalid parameter connectOptions.willMessage.destinationName:"+connectOptions.willMessage.destinationName);
        	}
        	if (typeof connectOptions.cleanSession === "undefined")
        		connectOptions.cleanSession = true;

        	client.connect(connectOptions);
        };
     
        /** 
         * Subscribe for messages, request receipt of a copy of messages sent to the destinations described by the filter.
         * 
         * @name Messaging.Client#subscribe
         * @function
         * @param {string} filter describing the destinations to receive messages from.
         * <br>
         * @param {object} [subscribeOptions] used to control the subscription, as follows:
         * <p>
         * @config {number} [qos] the maiximum qos of any publications sent as a result of making this subscription.
         * @config {object} [invocationContext] passed to the onSuccess callback or onFailure callback.
         * @config {function} [onSuccess] called when the message has been written to the network
         * parameters passed to the onSuccess callback are:
         * <ol>
         * <li>invovcationContext if set in the subscribeOptions.       
         * </ol>
         * @config {function} [onFailure] called when the unsubscribe request has failed or timed out.
         * parameters passed to the onFailure callback are:
         * <ol>
         * <li>invovcationContext if set in the subscribeOptions.       
         * <li>errorCode
         * <li>errorMessage       
         * </ol>
         * @config {number} [timeout] which if present determines the nuber of seconds after which the onFailure calback is called
         * the presence of a timeout does not prevent the onSuccess callback from being called when the MQTT Suback is eventually received.         
    	 * @throws {InvalidState} if the client is not in connected state.
         */
        this.subscribe = function (filter, subscribeOptions) {
        	if (typeof filter !== "string")
        		throw new Error("Invalid argument:"+filter);
        	subscribeOptions = subscribeOptions || {} ;
        	validate(subscribeOptions,  {qos:"number", 
        		                         invocationContext:"object", 
        		                         onSuccess:"function", 
        		                         onFailure:"function",
        		                         timeout:"number"
        		                        });
        	if (subscribeOptions.timeout && !subscribeOptions.onFailure)
        		throw new Error("subscribeOptions.timeout specified with no onFailure callback.");
        	if (typeof subscribeOptions.qos !== "undefined" 
        		&& !(subscribeOptions.qos === 0 || subscribeOptions.qos === 1 || subscribeOptions.qos === 2 ))
    			throw new Error("Invalid option:"+subscribeOptions.qos);
            client.subscribe(filter, subscribeOptions);
        };

        /**
         * Unsubscribe for messages, stop receiving messages sent to destinations described by the filter.
         * 
         * @name Messaging.Client#unsubscribe
         * @function
         * @param {string} filter describing the destinations to receive messages from.
         * @param {object} [unsubscribeOptions] used to control the subscription, as follows:
         * <p>
         * @config {object} [invocationContext] passed to the onSuccess callback or onFailure callback.
         * @config {function} [onSuccess] called when the message has been written to the network
         * parameters passed to the onSuccess callback are:
         * <ol>
         * <li>invovcationContext if set in the unsubscribeOptions.     
         * </ol>
         * @config {function} [onFailure] called when the unsubscribe request has failed or timed out.
         * parameters passed to the onFailure callback are:
         * <ol>
         * <li>invocationContext if set in the unsubscribeOptions.       
         * <li>errorCode
         * <li>errorMessage       
         * </ol>
         * @config {number} [timeout] which if present determines the number of seconds after whch the onFailure calback is called, the
         * presence of a timeout does not prevent the onSuccess callback from being called when the MQTT UnSuback is eventually received.
         * @throws {InvalidState} if the client is not in connected state.
         */
        this.unsubscribe = function (filter, unsubscribeOptions) {
        	if (typeof filter !== "string")
        		throw new Error("Invalid argument:"+filter);
        	unsubscribeOptions = unsubscribeOptions || {} ;
        	validate(unsubscribeOptions,  {invocationContext:"object", 
        		                           onSuccess:"function", 
        		                           onFailure:"function",
        		                           timeout:"number"
        		                          });
        	if (unsubscribeOptions.timeout && !unsubscribeOptions.onFailure)
        		throw new Error("unsubscribeOptions.timeout specified with no onFailure callback.");
            client.unsubscribe(filter, unsubscribeOptions);
        };

        /**
         * Send a message to the consumers of the destination in the Message.
         * 
         * @name Messaging.Client#send
         * @function 
         * @param {Messaging.Message} message to send.
         
         * @throws {InvalidState} if the client is not in connected state.
         */   
        this.send = function (message) {       	
            if (!(message instanceof Message))
                throw new Error("Invalid argument:"+typeof message);
            if (typeof message.destinationName === "undefined")
            	throw new Error("Invalid parameter Message.destinationName:"+message.destinationName);
           
            client.send(message);   
        };
        
        /** 
         * Normal disconnect of this Messaging client from its server.
         * 
         * @name Messaging.Client#disconnect
         * @function
         * @throws {InvalidState} if the client is not in connected or connecting state.     
         */
        this.disconnect = function () {
        	client.disconnect();
        };
        
        /** 
         * Get the contents of the trace log.
         * 
         * @name Messaging.Client#getTraceLog
         * @function
         * @return {Object[]} tracebuffer containing the time ordered trace records.
         */
        this.getTraceLog = function () {
        	return client.getTraceLog();
        }
        
        /** 
         * Start tracing.
         * 
         * @name Messaging.Client#startTrace
         * @function
         */
        this.startTrace = function () {
        	client.startTrace();
        };
        
        /** 
         * Stop tracing.
         * 
         * @name Messaging.Client#stopTrace
         * @function
         */
        this.stopTrace = function () {
            client.stopTrace();
        };
    };

    /** 
     * An application message, sent or received.
     * All attributes may be null, which implies the default values.
     * 
     * @name Messaging.Message
     * @constructor
     * @param {String|ArrayBuffer} payload The message data to be sent.
     * <p>
     * @property {string} payloadString <i>read only</i> The payload as a string if the payload consists of valid UTF-8 characters.
     * @property {ArrayBuffer} payloadBytes <i>read only</i> The payload as an ArrayBuffer.
     * <p>
     * @property {string} destinationName <b>mandatory</b> The name of the destination to which the message is to be sent
     *                    (for messages about to be sent) or the name of the destination from which the message has been received.
     *                    (for messages received by the onMessage function).
     * <p>
     * @property {number} qos The Quality of Service used to deliver the message.
     * <dl>
     *     <dt>0 Best effort (default).
     *     <dt>1 At least once.
     *     <dt>2 Exactly once.     
     * </dl>
     * <p>
     * @property {Boolean} retained If true, the message is to be retained by the server and delivered 
   	 *                     to both current and future subscriptions.
   	 *                     If false the server only delivers the message to current subscribers, this is the default for new Messages. 
   	 *                     A received message has the retained boolean set to true if the message was published 
   	 *                     with the retained boolean set to true
   	 *                     and the subscrption was made after the message has been published. 
   	 * <p>
     * @property {Boolean} duplicate <i>read only</i> If true, this message might be a duplicate of one which has already been received. 
     *                     This is only set on messages received from the server.
     */
    var Message = function (newPayload) {  
    	var payload;
    	if (   typeof newPayload === "string" 
    		|| newPayload instanceof ArrayBuffer
    		|| newPayload instanceof Int8Array
    		|| newPayload instanceof Uint8Array
    		|| newPayload instanceof Int16Array
    		|| newPayload instanceof Uint16Array
    		|| newPayload instanceof Int32Array
    		|| newPayload instanceof Uint32Array
    		|| newPayload instanceof Float32Array
    		|| newPayload instanceof Float64Array
    	   ) {
            payload = newPayload;
        } else {
            throw ("Invalid argument:"+newPayload);
        }

    	this.__defineGetter__("payloadString", function() {
    		if (typeof payload === "string")
    			return payload;
    		else
    			return parseUTF8(payload, 0, payload.length); 
    	});

    	this.__defineGetter__("payloadBytes", function() {
    		if (typeof payload === "string") {
    			var buffer = new ArrayBuffer(UTF8Length(payload));
    			var byteStream = new Uint8Array(buffer); 
    			stringToUTF8(payload, byteStream, 0);

    			return byteStream;
    		} else {
    			return payload;
    		};
    	});

    	var destinationName=undefined;
    	this.__defineGetter__("destinationName", function() { return destinationName; });
    	this.__defineSetter__("destinationName", function(newDestinationName) { 
    		if (typeof newDestinationName === "string")
    		    destinationName = newDestinationName;
    		else 
    			throw new Error("Invalid argument:"+newDestinationName);
    		});
    	
    	
    	var qos = 0;
    	this.__defineGetter__("qos", function() { return qos; });
    	this.__defineSetter__("qos", function(newQos) { 
    		if (newQos === 0 || newQos === 1 || newQos === 2 )
    			qos = newQos;
    		else 
    			throw new Error("Invalid argument:"+newQos);
    		});

    	var retained = false;
    	this.__defineGetter__("retained", function() { return retained; });
    	this.__defineSetter__("retained", function(newRetained) { 
    		if (typeof newRetained === "boolean")
    		    retained = newRetained;
    		else 
    			throw new Error("Invalid argument:"+newRetained);
    		});
    	
    	var duplicate = false;
    	this.__defineGetter__("duplicate", function() { return duplicate; });
    	this.__defineSetter__("duplicate", function(newDuplicate) { duplicate = newDuplicate; });
    };
    
    // Module contents.
    return {
        Client: Client,
        Message: Message,
    };
})(window);
