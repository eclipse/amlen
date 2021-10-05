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
define(["doh/runner", "js/ism_mqttws"], function(doh, mqtt) {	
	var ip = "127.0.0.1";
	var port = 16102;
	var debug = false;
	
	var clientType = navigator.userAgent;
	var phantomjs = clientType.indexOf("PhantomJS") + 1;
	var firefox = clientType.indexOf("Firefox") + 1;
	var chrome = clientType.indexOf("Chrome") + 1;
	
	socket_stub = { send:function() { /* Do nothing */ },
			        close:function() { /* Do nothing */ },
			        readyState: WebSocket.OPEN
	};
	pinger_stub = { reset:function() { /* Do nothing */ },
	                cancel:function() { /* Do nothing */ }		        
	};

//	doh.register("test.test_mqtt.messagetypes", 
//		    [
//	      	function messagetypes (doh){
//	      		doh.assertEqual(IsmMqtt.MESSAGE_TYPE.CONNECT,"CONNECT");
//	      	}
//	      	]);
	doh.register("test.test_mqtt.createclient", 
	    [
      	function createclient (doh){
    		if (debug)
    		    console.log("Calling client constructor");
    		var client = new IsmMqtt.Client(ip,port,"myclient");

    	    if (debug)
    		    console.log("asserting", ip," = ", client.host);
    		doh.assertEqual(ip, client.host);
    		
    		if (debug)
    		    console.log("asserting ",port, " = ", client.port);	
    		doh.assertEqual(port, client.port);

    		if (debug)
    		    console.log("asserting myclient = ", client.clientId);
    		doh.assertEqual("myclient", client.clientId);
    		
    		if (debug)
    		    console.log("asserting onconnect = ", null);
    		doh.assertEqual(null, client.onconnect);
    		
    		if (debug)
    		    console.log("asserting onconnectionlost = ", null);
    		doh.assertEqual(null, client.onconnectionlost);
    		
    		if (debug)
    		    console.log("asserting onmessage = ", null);
    		doh.assertEqual(null, client.onmessage);
    	},
		function createclientbadid (doh){
      		debug = true;
    		if (debug)
    		    console.log("Calling client constructor");
    		try {
    		    var client = new IsmMqtt.Client(ip,port,"myclient9012345678901234");
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
    		} catch (err) {
    			var expected = "Invalid clientId.  ClientId must be between 1 and 23 characters long.";
    			if (debug) {
    				console.log("Asserting "+err.message+" = "+expected);
    			}
    			doh.assertEqual(expected,err.message);
    		}
    		
    		if (debug)
    		    console.log("Calling client constructor");
    		try {
    		    var client = new IsmMqtt.Client(ip,port,"");
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
    		} catch (err) {
    			var expected = "Invalid clientId.  ClientId must be between 1 and 23 characters long.";
    			if (debug) {
    				console.log("Asserting "+err.message+" = "+expected);
    			}
    			doh.assertEqual(expected,err.message);
    		}
    	}
	    ]);
	doh.register("test.test_mqtt.connect", 
	    [
	 	function connect_noconnopts () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;

			if (debug)
			    console.log("Calling connect()");
			client.connect();
			
			var expected = {"type":"CONNECT","clientId":"myclient"};
			
			if (debug)
			    console.log("asserting ",JSON.stringify(expected)," = ",JSON.stringify(client._connectMessage));
			doh.assertEqual(expected,client._connectMessage);
		},
	 	
	 	function connect_allconnopts () {
	 		//var client = new IsmMqtt.Client("9.41.62.164",16102,"myclient");
	 		var client = new IsmMqtt.Client(ip,port,"myclient");
	 		//debug = true;
	 		var connopts = {
	 				userName:"uname",
	 				password:"pword",
	 				clean:false,
	 				keepAliveInterval:70,
	 				willTopic:"wtopic",
	 				willMessage:"wmessage",
	 				willQoS:1,
	 				willRetain:true
	 		};

	 		if (debug)
	 		    console.log("Calling connect()");
	 		client.connect(connopts);
	 		
	 		var expected = {"type":"CONNECT","clientId":"myclient","userName":"uname","password":"pword","clean":false,"keepAliveInterval":70,"willTopic":"wtopic","willMessage":"wmessage","willQoS":1,"willRetain":true};
	 		
	 		if (debug)
	 		    console.log("asserting ",JSON.stringify(expected)," = ",JSON.stringify(client._connectMessage));
	 		doh.assertEqual(expected,client._connectMessage);
	 		
	 	},
	 	
	 	function connect_alreadyconnected () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set _connected to true to force failure
			client._connected = true;

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect();
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				if (debug)
				    console.log("asserting error msg = Already Connected");
				doh.assertEqual("Already Connected",err.message);
				
				
			}
		},
		
	 	function connect_invalidconnopt () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set and invalid connection option to force a failure
			var connopts = {
	 				invalidOpt:"invalidOption"
			}

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Unknown property: invalidOpt. Valid properties include: "userName", "password", "willTopic", "willMessage", "willQoS", "willRetain", "keepAliveInterval", "clean".';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);
				
				
			}
		},
		
	 	function connect_invalidwillqos () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set an invalid value for willQoS to force a failure
			var connopts = {
					willQoS:"invalidOption"
			};

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid QoS(invalidOption) specified.  QoS must be 0 (default), 1 or 2.';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		},
		
	 	function connect_emptywillqos () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set and invalid value for willQoS to force a failure
			var connopts = {
					willQoS:""
			};

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid QoS() specified.  QoS must be 0 (default), 1 or 2.';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		},
		
	 	function connect_invalidwillqosfloat () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set an invalid value for willQoS to force a failure
			var connopts = {
					willQoS:0.77
			};

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid QoS(0.77) specified.  QoS must be 0 (default), 1 or 2.';;
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		},
		
		
	 	function connect_invalidwillretain () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set and invalid value for willQoS to force a failure
			var connopts = {
					willRetain:"invalidOption"
			};

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid retain(invalidOption) specified.  Retain must be false (default) or true.';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		},
		
	 	function connect_invalidwillretainfloat () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set and invalid value for willQoS to force a failure
			var connopts = {
					willRetain:1.11
			};

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
    		    // Force test failure if error is not thrown
    		    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid retain(1.11) specified.  Retain must be false (default) or true.';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		},
	 	function connect_invalidwillretain () {
			var client = new IsmMqtt.Client(ip,port,"myclient");
			//debug = true;
			// Set an invalid value for willRetain to force a failure
			var connopts = {
					willRetain:"0"
			}

			if (debug)
			    console.log("Calling connect()");
			try {
			    client.connect(connopts);
			    // Force failure if error is not thrown
			    doh.assertTrue(false);
			} catch (err) {
				var expected = 'Invalid retain(0) specified.  Retain must be false (default) or true.';
				if (debug)
				    console.log("asserting error msg = ", expected);				
				doh.assertEqual(expected,err.message);				
			}
		}
	    ]);	
	
	doh.register("test.test_mqtt.subscribe", 
		[
	      	function subscribe_validqosstr (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = socket_stub;
				client._pinger = pinger_stub;
				
				var connack = '{"type":"CONNACK", "rc":0}';
				var event = { "data":connack };
				client._on_socket_message(event);
				if (debug)
	    		    console.log("Calling subscribe");
			    client.subscribe("topic", "0");
			    client.subscribe("topic", "1");
			    client.subscribe("topic", "2");			
	      	},
	      	
	      	function subscribe_validqosnum (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = { send:function() { /* Do nothing */ }
				};
				client._pinger = { reset:function() { /* Do nothing */ }
				};
				
				var connack = '{"type":"CONNACK", "rc":0}';
				var event = { "data":connack };
				client._on_socket_message(event);
				if (debug)
	    		    console.log("Calling subscribe");
			    client.subscribe("topic", 0);
			    client.subscribe("topic", 1);
			    client.subscribe("topic", 2);			
	      	},
		      	
	      	function subscribe_invalidqosstr (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling subscribe");
				try {
					client.subscribe("topic", "invalidQoS");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid QoS(invalidQoS) specified.  QoS must be 0 (default), 1 or 2.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function subscribe_invalidqosbool (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
	    		client._connected = true;
	    		
	    		if (debug)
	    		    console.log("Calling subscribe");
	    		try {
	    			client.subscribe("topic", true);
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
	    		} catch (err) {
	    			var expected = 'Invalid QoS(true) specified.  QoS must be 0 (default), 1 or 2.';
	    			if (debug)
	    			    console.log("asserting error msg = ", expected);				
	    			doh.assertEqual(expected,err.message);				
	    		}
	      	}
		    ]);

	doh.register("test.test_mqtt.publish", 
		    [
	      	function publish_invalidqosstr (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", "invalidQoS");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid QoS(invalidQoS) specified.  QoS must be 0 (default), 1 or 2.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function publish_invalidqosbool (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", false);
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid QoS(false) specified.  QoS must be 0 (default), 1 or 2.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	function publish_invalidqosfloatstr (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", "2.32");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid QoS(2.32) specified.  QoS must be 0 (default), 1 or 2.';;
					if (debug)
					    console.log("asserting "+err.message+" starts with or = ", expected);

					doh.assertEqual(expected,err.message);
				}
	      	},
	      	
	      	function publish_invalidretain (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", "0", "invalidRetain");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid retain(invalidRetain) specified.  Retain must be false (default) or true.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function publish_invalidretainfloatstr (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", "0", "1.88");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid retain(1.88) specified.  Retain must be false (default) or true.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function publish_invalidretain5 (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic", "message", "0", 5);
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid retain(5) specified.  Retain must be false (default) or true.';
					if (debug)
					    console.log("asserting error msg = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function publish_invalidtopicplus (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic/+", "message");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid publish topic(topic/+). Publish topic cannot contain wildcard characters.';
					if (debug)
					    console.log("asserting "+err.message+" = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
	      	function publish_invalidtopichash (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				if (debug)
	    		    console.log("Calling publish");
				try {
					client.publish("topic/#", "message");
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = 'Invalid publish topic(topic/#). Publish topic cannot contain wildcard characters.';
					if (debug)
					    console.log("asserting "+err.message+" = ", expected);				
					doh.assertEqual(expected,err.message);				
				}
	      	},
	      	
		    ]);
	doh.register("test.test_mqtt.topiclisteners", 
		    [
	      	function topiclisteners_addwildcardplus (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				tCallback = function(message) {
					// Do nothing
				};
				
				if (debug)
	    		    console.log("Calling addTopicListener");
				try {
					client.addTopicListener("topic/+", tCallback);
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = "Invalid listener topic(topic/+). The topic for a topic listener cannot contain wildcard characters.";
					if (debug)
					    console.log("asserting "+err.message+" = ", expected);				
					doh.assertEqual(expected,err.message);
				}	      		
	      	},
	      	
	      	function topiclisteners_addwildcardhash (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				tCallback = function(message) {
					// Do nothing
				};
				
				if (debug)
	    		    console.log("Calling addTopicListener");
				try {
					client.addTopicListener("topic/#", tCallback);
	    		    // Force test failure if error is not thrown
	    		    doh.assertTrue(false);
				} catch (err) {
					var expected = "Invalid listener topic(topic/#). The topic for a topic listener cannot contain wildcard characters.";
					if (debug)
					    console.log("asserting "+err.message+" = ", expected);				
					doh.assertEqual(expected,err.message);
				}	      		
	      	},
	      	
	      	function topiclisteners_addtopic (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				var tCallback = function(message) {
					// Do nothing
				};
								
				if (debug)
	    		    console.log("Calling addTopicListener");
				
				// Add first topic and confirm it was added
				try {
					client.addTopicListener("topic", tCallback);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_addtopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 1 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 0 = "+client._topic_listeners["topic"].indexOf(tCallback));
				}
				doh.assertEqual(1, client._topic_listeners["topic"].length);
				doh.assertEqual(0, client._topic_listeners["topic"].indexOf(tCallback));
				
				var tCallback2 = function(message) {
					// Do nothing2
				};
				
				// Add second topic and confirm it was added
				try {
					client.addTopicListener("topic", tCallback2);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_addtopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 2 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 1 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				}
				doh.assertEqual(2, client._topic_listeners["topic"].length);
				doh.assertEqual(1, client._topic_listeners["topic"].indexOf(tCallback2));
				
				// Do a duplicate add second topic.  Assure the array of callbacks
				// for the topic is unchanged because the topic is already in 
				// the array.
				try {
					client.addTopicListener("topic", tCallback2);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_addtopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 2 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 1 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				    console.log("asserting "+client._topic_listeners["topic"].indexOf(tCallback2)+" = "+client._topic_listeners["topic"].lastIndexOf(tCallback2));
				}
				doh.assertEqual(2, client._topic_listeners["topic"].length);
				doh.assertEqual(1, client._topic_listeners["topic"].indexOf(tCallback2));
				doh.assertEqual(client._topic_listeners["topic"].indexOf(tCallback2), client._topic_listeners["topic"].lastIndexOf(tCallback2));
	      	},
	      	
	      	function topiclisteners_removetopic (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connected = true;
				
				var tCallback = function(message) {
					// Do nothing
				};
								
				if (debug)
	    		    console.log("Calling addTopicListener");
				
				// Add first topic and confirm it was added
				try {
					client.addTopicListener("topic", tCallback);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 1 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 0 = "+client._topic_listeners["topic"].indexOf(tCallback));
				}
				doh.assertEqual(1, client._topic_listeners["topic"].length);
				doh.assertEqual(0, client._topic_listeners["topic"].indexOf(tCallback));
				
				var tCallback2 = function(message) {
					// Do nothing2
				};
				
				// Add second topic and confirm it was added
				try {
					client.addTopicListener("topic", tCallback2);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 2 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 1 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				}
				doh.assertEqual(2, client._topic_listeners["topic"].length);
				doh.assertEqual(1, client._topic_listeners["topic"].indexOf(tCallback2));
				
				// Remove topic and confirm it was removed
				var tHasCallbacks;
				try {
					tHasCallbacks = client.removeTopicListener("topic", tCallback2);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
					console.log("asserting true = "+tHasCallbacks);
				    console.log("asserting 1 = "+client._topic_listeners["topic"].length);
				    console.log("asserting -1 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				}
				// Should be true.  There were two callbacks and only one was removed
				doh.assertEqual(true,tHasCallbacks);
				doh.assertEqual(1, client._topic_listeners["topic"].length);
				doh.assertEqual(-1, client._topic_listeners["topic"].indexOf(tCallback2));
				
				// Remove second (last) topic and confirm it was removed
				try {
					tHasCallbacks = client.removeTopicListener("topic", tCallback);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
					console.log("asserting false = "+tHasCallbacks);
				    console.log("asserting 0 = "+client._topic_listeners["topic"].length);
				    console.log("asserting -1 = "+client._topic_listeners["topic"].indexOf(tCallback));
				}
				// Should be false.  Once this second callback is removed there are no more callbacks for the topic.
				doh.assertEqual(false, tHasCallbacks);
				doh.assertEqual(0, client._topic_listeners["topic"].length);
				doh.assertEqual(-1, client._topic_listeners["topic"].indexOf(tCallback));
				
				// Attempt to remove a callback for a topic that has no callbacks
				try {
					tHasCallbacks = client.removeTopicListener("topic", tCallback);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
					console.log("asserting false = "+tHasCallbacks);
				    console.log("asserting 0 = "+client._topic_listeners["topic"].length);
				    console.log("asserting -1 = "+client._topic_listeners["topic"].indexOf(tCallback));
				}
				// Should be false because ther are no callbacks for this topic
				doh.assertEqual(false, tHasCallbacks);
				doh.assertEqual(0, client._topic_listeners["topic"].length);
				doh.assertEqual(-1, client._topic_listeners["topic"].indexOf(tCallback));
				
				// Add tCallback2
				try {
					client.addTopicListener("topic", tCallback2);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
				    console.log("asserting 1 = "+client._topic_listeners["topic"].length);
				    console.log("asserting 0 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				}
				doh.assertEqual(1, client._topic_listeners["topic"].length);
				doh.assertEqual(0, client._topic_listeners["topic"].indexOf(tCallback2));
				
				// Attempt to remove a callback for a topic that has callbacks but not the callback specified for removal.
				// (This topic has tCallback2 but does not have tCallback.)
				try {
					tHasCallbacks = client.removeTopicListener("topic", tCallback);
				} catch (err) {
					console.log("Unexpected error in topiclisteners_removetopic: "+err.message);
				}
				if (debug) {
					console.log("asserting true = "+tHasCallbacks);
				    console.log("asserting 1 = "+client._topic_listeners["topic"].length);
				    console.log("asserting -1 = "+client._topic_listeners["topic"].indexOf(tCallback));
				    console.log("asserting 0 = "+client._topic_listeners["topic"].indexOf(tCallback2));
				}
				// Should be true because there are still callbacks for this topic
				doh.assertEqual(true, tHasCallbacks);
				doh.assertEqual(1, client._topic_listeners["topic"].length);
				doh.assertEqual(-1, client._topic_listeners["topic"].indexOf(tCallback));
				doh.assertEqual(0, client._topic_listeners["topic"].indexOf(tCallback2));
	      	},
		    ]);
	doh.register("test.test_mqtt.connackrc", 
		    [
	      	function connackrc_withoutonconnect (doh){
	      		// Test that non-zero rc will result in automatic disconnect
	      		// if no onconnect is specified by the client app
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = socket_stub;
				client._pinger = pinger_stub;
				
				var connack = '{"type":"CONNACK", "rc":1}';
				var event = { "data":connack };
				client._on_socket_message(event);
				doh.assertFalse(client._socket_is_open());
	      	},
	      	
	      	function connackrc_withonconnect (doh){
	      		// Test that non-zero rc will result in call to onconnect
	      		// if specified by the client app
	      		debug = true;
	      		var msg = null;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = socket_stub;
				client._pinger = pinger_stub;
				client.onconnect = function(rc) {
					msg = "rc is "+rc;
				}
				
				var connack = '{"type":"CONNACK", "rc":1}';
				var event = { "data":connack };
				client._on_socket_message(event);
				doh.assertTrue(client._socket_is_open());
				doh.assertFalse(client.isConnected());
				doh.assertEqual("rc is 1",msg);
	      	},
	      	
	      	function connackrc0_withoutonconnect (doh){
	      		debug = true;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = socket_stub;
				client._pinger = pinger_stub;
				
				var connack = '{"type":"CONNACK", "rc":0}';
				var event = { "data":connack };
				client._on_socket_message(event);
				doh.assertTrue(client._socket_is_open());
				doh.assertTrue(client.isConnected());
	      	},
	      	
	      	function connackrc_withonconnect (doh){
	      		debug = true;
	      		var msg = null;
	    		if (debug)
	    		    console.log("Calling client constructor");
	    		var client = new IsmMqtt.Client(ip,port,"myclient");
	    		
				client._connectMessage = '{"type":"CONNECT", "id":client.clientId}';
				client._socket = socket_stub;
				client._pinger = pinger_stub;
				client.onconnect = function(rc) {
					msg = "rc is "+rc;
				}
				
				var connack = '{"type":"CONNACK", "rc":0}';
				var event = { "data":connack };
				client._on_socket_message(event);
				doh.assertTrue(client._socket_is_open());
				doh.assertTrue(client.isConnected());
				doh.assertEqual("rc is 0",msg);
	      	},
		    ]);
});