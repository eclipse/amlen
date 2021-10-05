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

package com.ibm.ima.samples.plugin.restmsg;

import java.nio.charset.Charset;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaReturnCode;
import com.ibm.ima.plugin.ImaSubscription;

/**
 * The RestMsgConnection class is the connection listener for the restmsg plug-in.
 * This object represents the connection from the plug-in point of view.
 * <p>
 * The basic job of this listener is to convert data between the external REST 
 * requests and the MessageSight plug-in methods.  The restmsg sample 
 * protocol is conceptually very similar to the natively supported messaging 
 * protocols.  So this example demonstrates the mechanisms for a protocol 
 * plug-in but it does relatively simple conversions from HTTP / REST to  
 * to pub/sub messaging.
 */
public class RestMsgConnection implements ImaConnectionListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaConnection  connect;          /* The base connection object this listener is associated with */
    ImaPlugin      plugin;           /* The plug-in object this listener is associated with         */
    Map<String, ImaSubscription> subscriptions = new HashMap<String, ImaSubscription>();
    LinkedBlockingQueue<ImaMessage>receivedMessages = new LinkedBlockingQueue<ImaMessage>();
    static Charset utf8 = Charset.forName("UTF-8");  /* UTF-8 encoding  */
    
    String op = null;
    String path = null;
    String query = null;
    String content_type = null;
    byte[] data = null;
    boolean atServer = false;
    
    /**
     * Constructor for the connection listener object.
     * 
     * This is called when a connection is created and it saves the connection and plugin objects
     * for later use. 
     */
    RestMsgConnection(ImaConnection connect, ImaPlugin plugin) {
        this.connect = connect;
        this.plugin  = plugin;
    }

    /**
     * Handle the close of a connection.
     * 
     * No action is required.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onClose(int, java.lang.String)
     */
    public void onClose(int rc, String reason) {
    }
    
    /**
     * Handle a request to check if the connection is alive.
     *
     * Allow the connection to be disconnected if we timeout while not processing a request.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        return atServer;
    }
    
    /**
     * Parse the query string of a request into key value pairs. 
     * 
     * @param query  The query string from the REST request. 
     * @return		 A Map backed by a HashMap of key value pairs. The
     * 				 key value pairs represent the query string passed in.
     */
    private Map<String, String> parseQuery(String query) {
        if (query == null || query.length() == 0)
            return null;
    	HashMap<String, String> queryMap = new HashMap<String, String>();
    	
    	if (query != null && query.length() > 0) {
    		String[] qArray = query.split("&");   
    		if (qArray != null && qArray.length > 0) {
    			for (int i=0; i<qArray.length; i++) {
    				String[] keyValue = qArray[i].split("=");
    				if (keyValue != null && keyValue.length == 2) {
    					queryMap.put(keyValue[0], keyValue[1]);
    				}
    			}
    		}
    	}
    	
    	return queryMap;
    }
    
    static char [] base62 = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
        'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
        'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
    };
    
    /*
     * Generate a random ClientID
     */
    String generateClientID() {
        char [] cidbuf = new char[12];
        cidbuf[0] = '_';    /* First char of generated ClientID should be underscore */
        cidbuf[1] = 'R';    
        cidbuf[2] = 'M';
        cidbuf[3] = '_';    /* Use one more underscore */ 
        int where = 4;
        
        /*
         * Use secure random to create about 48 bits of randomness
         */
        byte [] rbuf = new byte [6];
        SecureRandom rnd = new SecureRandom();
        rnd.nextBytes(rbuf);
        long rval = 0;
        for (int i=0; i<6; i++) {
            rval = (rval<<8) + (rbuf[i]&0xff);
        }    
        for (int i=0; i<8; i++) {
            cidbuf[where++] = base62[(int)(rval%62)];
            rval /= 62;
        }
        return new String(cidbuf);
    }
    
    /* 
     * Get the boolean value of a query parameter
     */
    boolean getBooleanQuery(Map<String, String> queryMap, String keyword, boolean defval) {
        if (queryMap == null)
            return defval;
        String val = queryMap.get(keyword);
        if (val == null || val.length()==0)
            return defval;
        if (val.equalsIgnoreCase("true"))
            return true;
        if (val.equalsIgnoreCase("false"))
            return false;
        int ival;
        try {
            ival = Integer.parseInt(val);
            return ival != 0;
        } catch (Exception e) {
            return defval;
        }
    }
    
    /*
     * Make a response in JSON format
     */
    String makeResponse(int rc, String reason) {
        return "{ \"Code\":\"CWLNA0"+rc+"\", \"Message\":\"" + reason + "\" }";
    }
    
    
    /**
     * Handle data from the client.
     * 
     * When we receive data from the client determine the appropriate action to take.
     * If we find one call the method associated with the action.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data) {
        Map<String, String> queryMap = parseQuery(query);
        if (plugin.isTraceable(9)) {
        	plugin.trace("onHttpData op " + op);
        	plugin.trace("onHttpData path " + path);
        	plugin.trace("onHttpData query " + query);
        	plugin.trace("onHttpData content_type " + content_type);
        }
        
        atServer = true;
        
        if (connect.getState() == ImaConnection.State_Accepted) { 
        	this.op = op;
        	this.path = path;
        	this.query = query;
        	this.content_type = content_type; 
        	this.data = data;
        	String clientID = null;
        	if (queryMap != null)
        	    clientID = queryMap.get("ClientID");
        	if (clientID == null)
        	    clientID = connect.getHttpHeader("ClientID");
        	if (clientID == null)
        	    clientID = generateClientID();
        	try {
        	    connect.setKeepAlive(300);     /* five minutes */
                connect.setIdentity(clientID, connect.getUser(), connect.getPassword());
        	} catch (Exception e) {
        	    plugin.traceException(2, e);
        	}
            return;
        }
        
        int aliaspos = path.indexOf('/', 1);
        if (aliaspos > 0)
            path = path.substring(aliaspos);
        if (aliaspos < 0 || path.length() < 9 || !path.startsWith("/message/")) {
            atServer = false;
            try {
                connect.sendHttpData(404, (String)null, makeResponse(404, "The path is not valid"));
            } catch (Exception e) {
                plugin.traceException(2, e);
            }
            return;
        }  
        
        String userpath = path.substring(9);
        if (userpath.length()<1 || userpath.indexOf('+')>=0 || userpath.indexOf('#')>= 0) {
            try {
                connect.sendHttpData(400, (String)null, makeResponse(400, "The topic is not valid"));
            } catch (Exception e) {
                plugin.traceException(2, e);
            }
            return;
        }

    	if ("GET".equals(op)) {
			onHttpGET(userpath);
    	} else if ("POST".equals(op)) {
    	    boolean persist = getBooleanQuery(queryMap, "persist", false); 
			onHttpPOST(userpath, persist, content_type, data);
    	} else if ("PUT".equals(op)) {
    		onHttpPUT(userpath, content_type, data);
    	} else if ("DELETE".equals(op)) {
    	    onHttpDELETE(userpath);
    	} else {
    	    atServer = false;
    	    try {
			    connect.sendHttpData(400, (String)null, makeResponse(400, "The HTTP operation is not known"));
            } catch (Exception e) {
                plugin.traceException(2, e);
            }
		}

    }

    /**
     * Get the retained message from a topic. - 
     * 
     * @param path           The remaining path which is the topic
     */
    void onHttpGET(String path) {
        try {
            connect.getRetainedMessage(path, connect);
        } catch (Exception e) {
            plugin.traceException(2, e);
        }
    }    
    
    /**
     * Handle a REST PUT request which sends a retained message. 
     * 
     * @param path           The remaining part of the path which is the topic 
     * @param content_type   Content type of bytes data
     * @param data           byte array of data associated with request.
     */
    void onHttpPUT(String path, String content_type, byte[] data) {
        doSend(path, true, true, content_type, data);
    }
    
    /**
     * Handle a REST POST request which published a message- 
     * 
     * @param path           The remaining portion of the path which is the topic 
     * @param content_type   Content type of bytes data
     * @param data           byte array of data associated with request.
     */
    void onHttpPOST(String path, boolean persist, String content_type, byte[] data) {
        doSend(path, persist, false, content_type, data);
    }
    
    /**
     * Handle a REST DELETTE request which deletes a retained message
     * @param path       The remaining portion of the path which is the topic
     */
    void onHttpDELETE(String path) {
        try {
            connect.deleteRetained(path, this);
        } catch (Exception e) {
            plugin.traceException(2, e);
        }
    }
    
    /**
     * Handle data from the client.
     * 
     * This should never be called as we do not accept data.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
        return length;
    }
    
    /**
     * Handle the completion of a connection and send a connection message to the client.
     * 
     * On the first HTTP header in the connection we went async to authenticate.
     * Do the saved operation on success.
     *
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
        try {
            if (rc == 0) {
        	     onHttpData(this.op, this.path, this.query, this.content_type, this.data);
            } else {
                 connect.sendHttpData(403, "text/plain;charset=utf-8", reason);
            }
        } catch (Exception e) {
            plugin.traceException(2, e);
        }
    }
  
    /**
     * Process a publish or send.
     * 
     * Create a MessgeSight message and sent it on the specified destination.
     * Only topic is working for now.
     
     * @param topic				The topic to send a message to
     * @param persist           Specifies that the message should be persistent
     * @param retain            Specifies if the message is of type retained
     * @param content_type		The content type of the bytes data associated
     * 							with the sending of a messages
     * @param data				Byte array of data
     */
    void doSend(String topic, boolean persist, boolean retain, String content_type, byte[] data) {
        try {
        	/*
        	 * Check for known text content-types, and otherwise make it a binary
        	 */
        	ImaMessageType mtype = ImaMessageType.bytes;
            if (content_type != null) {
            	if (content_type.startsWith("text/")) {
            	    mtype = ImaMessageType.text;
            	} else if (content_type.startsWith("application/json")) {
            	    mtype = ImaMessageType.jsonObject;
            	} else if (content_type.startsWith("application/xml")) {
                    mtype = ImaMessageType.text;
            	}
            }
            
            if (plugin.isTraceable(9)) {
                plugin.trace("doSend Connect=" + this + " Dest=" + topic);
                if (mtype != ImaMessageType.bytes) {
                	plugin.trace("doSend textData=" + new String(data, utf8));
                }
            }
            
            /* Create a message with the appropriate message type.  */
            ImaMessage msg = plugin.createMessage(mtype);
         	msg.setBodyBytes(data);
            
            /* Set attributes and send the message  */
            msg.setRetain(retain);
            msg.setPersistent(persist);
    
            /* 
             * For persist or retain we respond only on the ACK.
             * This means that for a POST without persist, we do not wait for the
             * return code and always return 200. 
             */
            if (persist | retain) {
                connect.send(msg, ImaDestinationType.topic, topic, this);
            } else {
                connect.send(msg, ImaDestinationType.topic, topic, null);
                atServer = false;
            	connect.sendHttpData(200, "text/plain;charset=utf-8", "Success");
            }  	
        } catch (Exception e) {
             plugin.traceException(2, e);
        }
    }
    
    
    /**
     * Handle a message from the server.
     * 
     * Enqueue the message for a later get.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
    }

    /**
     * Handle a completion event.
     * 
     * The completion can come in on a message or subscription.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onComplete(java.lang.Object, int, java.lang.String)
     */
    public void onComplete(Object obj, int rc, String message) {
        if (plugin.isTraceable(9)) {
            plugin.trace("onComplete obj="+obj+" rc="+rc+" message="+message);
        }    
        if (obj instanceof RestMsgConnection) {
            if (rc == 0)
                rc = 200;
            else if (rc == ImaReturnCode.IMARC_NotAuthorized)
                rc = 403;
            else 
                rc = 400;
            try {
                atServer = false;
                connect.sendHttpData(rc, (String)null, message==null ? "Success" : message);
            } catch (Exception e) {
                plugin.traceException(e);
            }
        } else {
            plugin.trace(4, "onComplete unknown Connect=" + this + " Object= " + obj + " rc=" + rc);
        }
    }
    
    /**
     * Give a simple string representation of this connection listener object.
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "RestMsgConnection " + connect.getClientID();
    }
    
    /**
     * Handle a message from the server.
     * 
     * Return the message as the response.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
	public void onGetMessage(Object correlate, int rc, ImaMessage msg) {
	    atServer = false;
	    try {
    	    if (rc != 0 || msg == null) {
    	        if (rc == ImaReturnCode.IMARC_NotAuthorized) {
                    connect.sendHttpData(403, (String)null, "The get is not authorized");
                } else {
                    connect.sendHttpData(204, (String)null, "No message is available");
                }
    	    } else {
    	        connect.setHttpHeader("Topic", msg.getDestination());
                connect.sendHttpData(200, (String)null, msg.getBodyBytes());
    	    }
	    } catch (Exception e) {
            plugin.traceException(2, e);
	    }
	}

}
