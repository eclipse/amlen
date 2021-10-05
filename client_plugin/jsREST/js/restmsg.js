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
MessagingREST = (function (global) {
	var version = "2.0";
	
	/** 
	 * The JavaScript application communicates with IBM MessageSight via http plug-in using 
	 * a {@link MessagingREST.Client} object. 
	 * 
	 * The methods are implemented as asynchronous JavaScript methods (even though the 
	 * underlying protocol exchange might be synchronous in nature). This means they signal 
	 * their completion by calling back to the application via Success or Failure callback 
	 * functions provided by the application on the method in question. These callbacks are 
	 * called at most once per method invocation and do not persist beyond the lifetime of 
	 * the script that made the invocation.
	 * 
	 * @name MessagingREST.Client    
	 * 
	 * @constructor
	 * 
	 * @param {string} host - the address of the IBM MessageSight host, as a DNS name or dotted decimal IP address.
	 * @param {number} port - the port number to connect to
	 * @param {string} path - the alias on the IBM MessageSight host.
	 * @param {string} clientid - the MessagingREST client identifier
	 * @param {string} username - the username to authenticate with
	 * @param {string} password - the password for the username provided
	 *   
	 */
	var Client = function (host, port, path, protocol, clientid, username, password) {
		
		var trace = function() {};
		
		
		if (typeof host !== "string")
			throw new Error("invalid server name");
		
		if (typeof port !== "number" || port < 0)
			throw new Error("invalid port number");
			
		if (typeof path !== "string")
			throw new Error("invalid path");
		
		if (clientid && typeof clientid !== "string")
			throw new Error("The ClientID is not valid");

		var ipv6AddSBracket = (host.indexOf(":") != -1 && host.slice(0,1) != "[" && host.slice(-1) != "]");
		var url = protocol + "://"+(ipv6AddSBracket?"["+host+"]":host)+":"+port+path;
		
		trace("Base URL is: " + url);
		
		
		/**
		 * Set a trace function that the client can use for
		 * logging trace messages.
		 * 
		 * @param {function} trace - a trace function that takes one string as an argument
		 */
		this.setTrace = function(traceFunction) {
			if (traceFunction) {
				trace = traceFunction;
			}
		}

		/** 
		 * Set the necessary request headers for the a provided
		 * XMLHttpRequest object. Since the request will more than
		 * likely be a CORS request only headers that IBM MessageSight
		 * allows may be set. 
		 * @private
		 * 
		 * @name MessagingREST.Client#setRequestHeaders
		 * @function
		 * @param {object} request - The XMLHttpRequest object that headers should be set for.
		 * 
		 */
		this.setRequestHeaders = function(request) {
			// trace("Enter: Client.setRequestHeaders");
					  
			if (!request) {
				return;
			}
			
			if (username && password) {
		    	var userPass = btoa(username + ":" + password); 
		     	request.setRequestHeader("Authorization", "Basic " + userPass);
			}
			
			if (clientid) {
			    trace("setting header client-id to " + clientid);
			    request.setRequestHeader("ClientID", clientid);
			}    
		}
		
		/** 
		 * Invoke action to obtain retained messages from topic. 
		 * This API will invoke an asynchronous call on the IBM 
		 * MessageSight server that will attempt to obtain a retained
		 * message. After successful callback from this API a check
		 * for the retained message can be done with a call to 
		 * MessagingREST.Client#retrieveMessages
		 * 
		 * 
		 * @name MessagingREST.Client#getRetainedMsg
		 * @function
		 * @param {string} topicName - The topic name to obtain a retained message from.
		 * @param {function} onSuccess - Called after a response with a status code of 200 is received
		 *                              from the server. A single response parameter is passed 
		 *                              to the onSuccess callback:
		 *                              <ol>     
		 *                              <li>a string that contains the topic 
		 *                              <li>a string that contains any response from the request   
		 *                              </ol>
		 * @param {function} onFailure - Called when a status code other than 200 is received from
		 *                               the server. Two response parameters are passed to the 
		 *                               onFailure callback:
		 *                              <ol> 
		 *								<li>a number that indicates the response code from the server    
		 *                              <li>a string that contains any response text from the server
		 *                              </ol>
		 * 
		 */
		this.getRetainedMsg = function(topicName, onSuccess, onFailure) {
			// trace("Enter: Client.getRetainedMsg");
		    // GET /restmsg/topic/topicName
			var topicNameEncoded = encodeURIComponent(topicName);
			var requestUrl = url + "/message/" + topicNameEncoded;
		    var xhrRetainedMsgs = new XMLHttpRequest();
		    
		    xhrRetainedMsgs.open('GET', requestUrl, true);
		    trace("Request is GET " + requestUrl);
		    this.setRequestHeaders(xhrRetainedMsgs);
		    
		    xhrRetainedMsgs.onreadystatechange = function() {
		    	if (this.readyState != 4)  { return; }
		    	if (this.status === 200) {
		    		if (onSuccess) {
                        var topic = this.getResponseHeader("Topic");
                        onSuccess(topic, this.responseText);
		    		}
		    	} else {
		    		if (onFailure) {
		    			onFailure(this.status, this.statusText);
		    		}
		    	}
		    	this.onreadystatechange = function() { };
		    };
		    xhrRetainedMsgs.send();
		}
		
		/** 
		 * Invoke action to delete retained message from a
		 * given topic. 
		 * 
		 * 
		 * @name MessagingREST.Client#deleteRetained
		 * @function
		 * @param {string} topicName - The topic that the retained message should be removed from.
		 * @param {function} onSuccess - Called after a response with a status code of 200 is received
		 *                              from the server. A single response parameter is passed 
		 *                              to the onSuccess callback:
		 *                              <ol>     
		 *                              <li>a string that contains any response from the request   
		 *                              </ol>
		 * @param {function} onFailure - Called when a status code other than 200 is received from
		 *                               the server. Two response parameters are passed to the 
		 *                               onFailure callback:
		 *                              <ol> 
		 *								<li>a number that indicates the response code from the server    
		 *                              <li>a string that contains any response text from the server
		 *                              </ol>
		 * 
		 */
		this.deleteRetained = function(topicName, onSuccess, onFailure) {
			// trace("Enter: Client.deleteRetained");
				
		    // DELETE /restmsg/topic/topicName
			var topicNameEncoded = encodeURIComponent(topicName);
			var requestUrl = url + "/message/" + topicNameEncoded;
		    var xhrDeleteRetained = new XMLHttpRequest();
		    
		    trace("Request is DELETE " + requestUrl);
		    xhrDeleteRetained.open('DELETE', requestUrl, true);
		    this.setRequestHeaders(xhrDeleteRetained);
		    
		    xhrDeleteRetained.onreadystatechange = function() {
		    	if (this.readyState != 4)  { return; }
		    	if (this.status === 200) {
		    		if (onSuccess) {
		    			onSuccess(this.responseText);
		    		}
		    	} else {
		    		if (onFailure) {
		    			onFailure(this.status, this.statusText);
		    		}
		    	}
		    	this.onreadystatechange = function() { };
		    };
		    xhrDeleteRetained.send(null);
		}

		/** 
		 * Publish a message to a specific topic.
		 * 
		 * 
		 * @name MessagingREST.Client#createTopicSubscription
		 * @function
		 * @param {string} topicName - the topic filter that the subscription should be set to
		 * @param {boolean} persist - if the message should persist
		 * @param {boolean} retain - if the message should be retained
		 * @param {function} onSuccess - Called after a response with a status code of 200 is received
		 *                              from the server. A single response parameter is passed 
		 *                              to the onSuccess callback:
		 *                              <ol>     
		 *                              <li>a string that contains any response from the request   
		 *                              </ol>
		 * @param {function} onFailure - Called when a status code other than 200 is received from
		 *                               the server. Two response parameters are passed to the 
		 *                               onFailure callback:
		 *                              <ol> 
		 *								<li>a number that indicates the response code from the server    
		 *                              <li>a string that contains any response text from the server
		 *                              </ol>
		 */
		this.publishMessageTopic = function(topicName, persist, retain, onSuccess, onFailure) {
			// trace("Enter: Client.publishMessageTopic");

		    // POST /restmsg/topic/topicname?persist=bool&retain=bool
		    var topicNameEncoded = encodeURIComponent(topicName);
			var requestUrl;
		    var xhrPublish = new XMLHttpRequest();
		    if (retain) {
		        requestUrl = url + "/message/" + topicNameEncoded;
		        xhrPublish.open('PUT', requestUrl, true);
		        trace("Request is PUT " + requestUrl);
		    } else {
		        requestUrl = url + "/message/" + topicNameEncoded + "?persist=" + persist.toString();
		        xhrPublish.open('POST', requestUrl, true);
		        trace("Request is POST " + requestUrl);
		    }    

		    this.setRequestHeaders(xhrPublish);
		    xhrPublish.setRequestHeader("Content-Type", "text/plain");

		    xhrPublish.onreadystatechange = function() {
		    	if (this.readyState != 4)  { return; }
		    	if (this.status === 200) {
		    		if (onSuccess) {
		    			onSuccess(this.responseText);
		    		}
		    	} else {
		    		if (onFailure) {
		    			onFailure(this.status, this.responseText);
		    		}
		    	}
		    	this.onreadystatechange = function() { };
		    };
		    
		    var msgObj = {message: topicForm.textMessage.value }
		    var msgString = JSON.stringify(msgObj);
		    xhrPublish.send(topicForm.textMessage.value);
		}
	};


	// Module contents.
	return {
		Client: Client
	};

})(window);
