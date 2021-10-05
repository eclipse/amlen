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

package com.ibm.ima.samples.plugin.jsonmsg;

import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Map;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaPluginException;
import com.ibm.ima.plugin.ImaReliability;
import com.ibm.ima.plugin.ImaSubscription;
import com.ibm.ima.plugin.ImaSubscriptionType;
import com.ibm.ima.plugin.util.ImaBase64;
import com.ibm.ima.plugin.util.ImaJson;

/**
 * The JMConnection class is the connection listener for the json_msg plug-in.
 * This object represents the connection from the plug-in point of view.
 * <p>
 * The basic job of this listener is to convert data between the external JSON
 * representation and the MessageSight plug-in methods.  The json_msg sample 
 * protocol is conceptually very similar to the natively supported messaging 
 * protocols.  So this example demonstrates the mechanisms for a protocol 
 * plug-in but it does relatively simple conversions from one representation to 
 * another.
 */
public class JMConnection implements ImaConnectionListener {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaConnection  connect;          /* The base connection object this listener is associated with */
    ImaPlugin      plugin;           /* The plug-in object this listener is associated with         */
    ImaJson        jobj;             /* The JSON parser object                                      */
    String         savestr;          /* The saved string with a partial JSON object                 */
    int            livenesTimeoutCounter = 0;        /* The liveness count                          */
    Map<String, ImaSubscription> subscriptions = new HashMap<String, ImaSubscription>();
    static Charset utf8 = Charset.forName("UTF-8");  /* All JSON text is in UTF-8 encoding          */
    static enum ActionType {
    	Send, Connect, Ack, Subscribe, CloseSubscription, DestroySubscription,
    	Ping, Pong, Close, Connected, GetRetained, DeleteRetained
    }
    
    /**
     * Constructor for the connection listener object.
     * 
     * This is called when a connection is created and it saves the connection and plugin objects
     * for later use. 
     */
    JMConnection(ImaConnection connect, ImaPlugin plugin) {
        this.connect = connect;
        this.plugin  = plugin;
        jobj = new ImaJson();
    }

    /**
     * Handle the close of a connection.
     * 
     * If we get a close, try sending a close action.  If the physical connection is already 
     * closed this will fail silently.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onClose(int, java.lang.String)
     */
    public void onClose(int rc, String reason) {
        StringBuffer sb = new StringBuffer();
        sb.append("{ \"Action\": \"Close\", \"RC\":"+ rc);
        if (reason != null) {
            sb.append(", \"Reason\": ");
            ImaJson.put(sb, reason);
        }
        sb.append("}");
        connect.sendData(new String(sb));
    }
    
    /**
     * Handle a request to check if the connection is alive.
     * 
     * Send a ping to the client to check if the connection is still alive.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        if (livenesTimeoutCounter > 5)
            return false;
        livenesTimeoutCounter++;
        connect.sendData("{\"Action\": \"Ping\"}");
        return true;
    }

    /**
     * Handle data from the client.
     * 
     * When we receive data from the client, convert it to a string and use the JSON
     * parser to find a JSON object as a message within it.  If we find one call the
     * method associated with the action.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
        int bytes_used = length;
        try {
            String jstr;
            
            /* 
             * Check for the data not ending at a character boundary.  We use a simple
             * rule to find an ASCII7 character as a JSON message will always begin and
             * end on such a character anyway. 
             */ 
            for (; bytes_used > 0; bytes_used--) {
                if ((data[offset+bytes_used-1] & 0xff) < 0x80)
                    break;
            }

            if (savestr != null) {
                jstr = savestr + new String(data, offset, length, utf8);
                savestr = null;
            } else {
                jstr = new String(data, offset, length, utf8);
            }    
            
            while (jstr != null) {
                int rc = jobj.parse(jstr);
                if (rc < -1) {          /* Bad JSON */
                    connect.close(217, "Bad JSON data: \'" + jstr + '\'');
                    jstr = null;
                    break;
                }
                if (rc == -1) { /* Incomplete JSON */
                    savestr = jstr;
                    jstr = null;
                    break;
                }
                /* One complete JSON object */
                if (jobj.getPosition() < jstr.length()) {
                    jstr = jstr.substring(jobj.getPosition()); /* Save remaining data */
                } else {
                    jstr = null;
                }
                String action = jobj.getString("Action");
                ActionType actionType = Enum.valueOf(ActionType.class, action);
                /*System.out.println("onData: Processing: "+jobj);*/
                plugin.trace(8, "JMConnection.onData: Processing: "+jobj);
                switch (actionType) {
                case Send:
                	doSend(jobj); break;
                case Ack:
                	doAck(jobj); break;
                case Connect:
                	doConnect(jobj); break;
                case Subscribe:
                	doSubscribe(jobj); break;
                case CloseSubscription:
                	doCloseSub(jobj); break;
                case DestroySubscription:
                	doDestroySub(jobj); break;
                case Close:
                	doClose(jobj); break;
                case Ping:
                	doPing(jobj); break;
                case Pong:
                	doPong(jobj); break;
                case GetRetained:
                	doGetRetained(jobj); break;
                case DeleteRetained:
                	doDeleteRetained(jobj); break;
                default:
                    connect.close(217, "Bad JSON data");
                    jstr = null;
                    break;
                }

            }
        } catch (Exception e) {
        	plugin.traceException(e);
            e.printStackTrace(System.err);
            connect.close(218, e.getMessage());
        }
        return bytes_used;
    }
    
    /**
     * Handle the completion of a connection and send a connection message to the client.
     * 
     * A non-zero return code indicates that the connection is closed, and that is handled
     * by calling close.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
    	if (rc == 0) {
    		connect.sendData("{ \"Action\": \"Connected\" }");
    	}
    }
    
    /**
     * Process a connect request from the client.
     * 
     * This must be the first action in a connection and sets the identifying information
     * for the connection.
     */
    void doConnect(ImaJson jobj) {
        String clientID = jobj.getString("ClientID");
        String user     = jobj.getString("User");
        String password = jobj.getString("Password");
        int keepAlive = jobj.getInt("KeepAliveTimeout", 600);
        connect.setKeepAlive(keepAlive);
        connect.setIdentity(clientID, user, password);
    }
    
    /**
     * Convert from flags to ImaReliability.
     * We send over the wire as a simple integer and recreate the Java enumeration.
     */
    static ImaReliability toQoS(int flags) {
        switch(flags&3) {
        case 0: return ImaReliability.qos0;
        case 1: return ImaReliability.qos1;
        case 2: return ImaReliability.qos2;
        }
        return ImaReliability.qos0;       /* Showld not happen */
    }
    
    /**
     * Convert from ImaReliability to an int value
     * From the java enumeration we create the wire format integer.
     */
    static int fromQoS(ImaReliability qos) {
    	switch(qos) {
        case qos0:  return 0;
        case qos1:  return 1;
        case qos2:  return 2;
        }
        return 0;    /* Should not happen */
    }
    
    /**
     * Process a publish or send.
     * 
     * Create a MessgeSight message and sent it on the specified destination.
     * Only topic is working for now.
     */
    void doSend(ImaJson jobj) {
        String topic = jobj.getString("Topic");
        String body  = jobj.getString("Body");
        
        if (plugin.isTraceable(9)) {
            plugin.trace("doSend Connect=" + this + " Dest=" + topic + " Body=" + body);        
        }
        if (topic != null) {
            ImaMessage msg = plugin.createMessage(ImaMessageType.text);
            int qos = jobj.getInt("QoS", 0);
            msg.setReliability(toQoS(qos));
            int id = jobj.getInt("ID", 0);
            int retain = jobj.getInt("Retain", 0);
            msg.setRetain(1 == retain ? true : false);
            
            /* If the message is binary, convert it from base64 */
            if (jobj.getBoolean("Binary", false)) {
                msg.setMessageType(ImaMessageType.bytes);
                byte [] bodybytes = ImaBase64.fromBase64(body);
                if (bodybytes == null && body != null) {
                    if (id != 0)
                        sendAck(id, 122, "The binary data is not valid.");
                    return;
                }
                msg.setBodyBytes(bodybytes);    
            } else {
                msg.setBodyText(body);
            }    
            connect.send(msg, ImaDestinationType.topic, topic, 0==id ? null : id);
        }
    }
    
    /**
     * Process an ACK from the client.
     */
    void doAck(ImaJson jobj) {
        int id = jobj.getInt("ID", 0);
        if (id != 0) {
            
        }
    }
    
    /**
     * Send ACK message to the client for Send, Subscribe, CloseSubscription, DestroySubscription
     * @param id 	client action ID.
     * @param rc	return code
     * @param message error message if any.
     */
    void sendAck(int id, int rc, String message) {
    	if (id > 0) {
    	    StringBuffer sb = new StringBuffer();
    		sb.append("{ \"Action\": \"Ack\", \"ID\": "+ id + ", \"RC\":"+ rc);
    		if (message != null) {
    		    sb.append(", \"Reason\": ");
    		    ImaJson.put(sb, message);
    		}  
    		sb.append("}");
    		connect.sendData(new String(sb));
    	}
    }
    
    /**
     * Process a subscribe from the client. 
     */
    void doSubscribe(ImaJson jobj) {
    	boolean newSub = false;
    	int id = jobj.getInt("ID", 0);
    	String name = jobj.getString("Name"); 
    	
        String destname = jobj.getString("Topic");
        ImaDestinationType desttype= ImaDestinationType.topic;
        if (destname == null){
        	destname = jobj.getString("Queue");
        	desttype= ImaDestinationType.queue;
        }
        if (destname == null) {
        	sendAck(id, -1, "The Destination is not valid.");
        	return;
        }
        
        boolean durable = jobj.getInt("Durable", 0) != 0;
        ImaSubscriptionType subType = ImaSubscriptionType.NonDurable;
        if (durable) {
            subType = ImaSubscriptionType.Durable;
        }
        // Highest currently supported QoS is currently 1, so set default 
        //   to 1.
        ImaReliability qos = toQoS(jobj.getInt("QoS", 1));
        
        
        if (plugin.isTraceable(7)) {
            plugin.trace("doSubscribe Connect=" + this + " Name=" + name + "Dest=" + destname);        
        }
        
        ImaSubscription sub = subscriptions.get(name);
        if (null == sub) {
        	newSub = true;
            sub = connect.newSubscription(desttype, destname, name, subType, false, false);
         		subscriptions.put(name, sub);
        } else if (desttype != sub.getDestinationType()
        		|| !destname.equals(sub.getDestination())
        		|| subType != sub.getSubscriptionType()) {
        	sendAck(id, -2, "The name '"+name+"' is in use for a different subscription.");
        	return;
        }
        if (!newSub && sub.isSubscribed()) {
        	if (0 != id) {
       			sendAck(id, -5, "Cannot change a subscribed subscription");
        	}
        	return;
        }
        
        
        sub.setUserData(id);
        sub.setReliability(qos);
        
        if (id != 0) {
        	sub.subscribe();
        } else {
        	sub.subscribeNoAck();
        }
    }
    
    /**
     * Process a close subscription.
     */
    void doCloseSub(ImaJson jobj) {
    	int id = jobj.getInt("ID",0);
    	int rc = 0;
    	String reason = null;
    	String name = jobj.getString("Name"); 
    	ImaSubscription subscription = subscriptions.get(name);
    	if (null == subscription) {
    		plugin.trace(5, "doCloseSub: Cannot find subscription for Name='"+name+"' Connect="+this);
    		reason = "doCloseSub: Cannot find subscription for Name='"+name+"'";
    		rc = -3;
    	} else {
            plugin.trace(7, "doCloseSub: Name="+name+" Connect=" + this);
    		subscription.close();
    		if (ImaSubscriptionType.NonDurable == subscription.getSubscriptionType()) {
    			subscriptions.remove(name);
    		}
    	}
    	if (0 != id) {
    		sendAck(id, rc, reason);
    	}
    }
    
    /**
     * Process a destroy subscription.
     */
    void doDestroySub(ImaJson jobj) {
    	int id = jobj.getInt("ID",0);
    	int rc = 0;
    	String reason = null;
    	String name = jobj.getString("Name"); 
    	ImaSubscription subscription = subscriptions.get(name);
    	if (null == subscription) {
    		plugin.trace(5, "doDestroySub: Cannot find subscription for Name='"+name+"' Connect="+this);
    		rc = -1;
    		reason = "Cannot find subscription for Name='"+name+"'";
    	} else {
        	plugin.trace(7, "doDestroySub: Name="+name+" Connect=" + this);
    		connect.destroySubscription(null, name, subscription.getSubscriptionType());
   			subscriptions.remove(name);
    	}
    	if (0 != id) {
    		sendAck(id, rc, reason);
    	}
    }
    
    /**
     * Process a disconnect from the client.
     * 
     * This is a normal close.
     */
    void doClose(ImaJson jobj) {
        plugin.trace(7, "doClose Connect=" + this);
        connect.close(0, "Connection complete");
    }
    
    /**
     * Get a retained message for a topic.
     * 
     * This is a normal close.
     */
    void doGetRetained(ImaJson jobj) {
        plugin.trace(7, "doGetRetained");
        String topic = jobj.getString("Topic");
        int ID = jobj.getInt("ID", 0);
        connect.getRetainedMessage(topic, ID);
    }
    
    /**
     * Get a retained message for a topic.
     * 
     * This is a normal close.
     */
    void doDeleteRetained(ImaJson jobj) {
        plugin.trace(7, "doDeleteRetained");
        String topic = jobj.getString("Topic");
        int ID = jobj.getInt("ID", 0);
        connect.deleteRetained(topic);
        if (0 != ID) {
        	sendAck(ID, 0, null);
        }
    }

    /**
     * Process an ping request from the client.
     * 
     * When we get a ping request we send back a pong response.
     */
    void doPing(ImaJson jobj) {
        plugin.trace(9, "doPing Connect=" + this);
        connect.sendData("{\"Action\": \"Pong\"}");
    }
    
    /**
     * Process an pong reply from the client.
     * 
     * Reset the liveness counter.
     */
    void doPong(ImaJson jobj) {
        plugin.trace(9, "doPong Connect=" + this);
        livenesTimeoutCounter--;
    }
    
    /**
     * Handle a message from the server.
     * 
     * Convert the message from the internal format to the JSON format and send the message on 
     * to the client.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
    	/*System.out.println("onMessage: sub="+sub+", msg="+msg);*/
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"Send\"");
        if (null != sub) {
        	buf.append(", \"Name\": ");
        	ImaJson.putString(buf, sub.getName());
        } else {
        	//(new Exception("onMessage: ImaSubscription is null!!!!")).printStackTrace();
        	/*System.out.println("onMessage: ImaSubscription is null!!!!");
        	System.out.println("onMessage: msg.destination="+msg.getDestination()
        			+" msg.getBodyText="+msg.getBodyText());*/
        }
        buf.append(", \"Topic\": ");
        ImaJson.putString(buf, msg.getDestination());
        if (msg.getRetain()) {
        	buf.append(", \"Retain\":"+msg.getRetain());
        }
        int qos = fromQoS(msg.getReliability()); 
        if (null != sub) {
        	int subqos = fromQoS(sub.getReliability());
        	if (subqos < qos) qos = subqos;
        }
        buf.append(", \"QoS\":"+qos);
        String msgbody = null;
        try {
            msgbody = msg.getBodyText();
        } catch (ImaPluginException ex) {
            byte [] b = msg.getBodyBytes();
            if (b != null) {
                msgbody = ImaBase64.toBase64(b);
                buf.append(", \"Binary\": true");
            }
        }
        buf.append(", \"Body\": ");
        ImaJson.putString(buf, msgbody);
        buf.append(" }");
        if (plugin.isTraceable(9)) {
            plugin.trace(9, "onMessage: " + buf);
        }
        connect.sendData(new String(buf));
        // TODO: This acknowledge should be moved to the handling of the Ack
        //   returned from the client.
        if (0 < qos) {		// If QoS is not 0, then acknowledge the message
        					//   received and sent on.
        	msg.acknowledge(0);
        }
    }
    
    /**
     * Handle a message from the server.
     * 
     * Convert the message from the internal format to the JSON format and send the message on 
     * to the client.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onGetMessage(Object correlate, int rc, ImaMessage msg) {
        plugin.trace(7, "onGetMessage rc=" + rc);
    	if(rc==0){
	        StringBuffer buf = new StringBuffer();
	        buf.append("{ \"Action\": \"Send\"");
	        buf.append(", \"Topic\": ");
	        ImaJson.putString(buf, msg.getDestination());
	        if (msg.getRetain()) {
	        	buf.append(", \"Retain\":"+msg.getRetain());
	        }
	        int qos = fromQoS(msg.getReliability()); 
	        buf.append(", \"QoS\":"+qos);
	        if (null != correlate) {
	        	if (correlate instanceof Integer) {
	        		int ID = (Integer)correlate;
	        		buf.append(", \"ID\":"+ID);
	        	}
	        }
	        String msgbody = null;
	        try {
	            msgbody = msg.getBodyText();
	        } catch (ImaPluginException ex) {
	            byte [] b = msg.getBodyBytes();
	            if (b != null) {
	                msgbody = ImaBase64.toBase64(b);
	                buf.append(", \"Binary\": true");
	            }
	        }
	        buf.append(", \"Body\": ");
	        ImaJson.putString(buf, msgbody);
	        buf.append(" }");
	        if (plugin.isTraceable(9)) {
	            plugin.trace(9, "onMessage: " + buf);
	        }
	        connect.sendData(new String(buf));
    	} else {
    		sendAck((Integer)correlate, rc, "Error returned from getRetainedMessage");
    	}
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
            plugin.trace("onComplete Connect=" + this + " Object= " + obj + " rc=" + rc);
        }
        if (obj != null) {
            if (obj instanceof Integer) {  // For Messages object is Integer
            	if (null != obj) {
            		Integer ID = (Integer)obj;
            		sendAck(ID, rc, message);
            	}
                /* For now we ignore message completion as only qos=0 (non-acknowledged) publish is supported */
            } else if (obj instanceof ImaSubscription) {
            	ImaSubscription sub = (ImaSubscription)obj;
            	Integer ID =(Integer)sub.getUserData();
            	if (null != ID) {
            		sendAck(ID.intValue(), rc, message);
            		sub.setUserData(null);
            	}
            } else {
                plugin.trace(3, "onComplete unknown complete object: Connect=" + this + " Object=" + obj);
            }
        } else {
            plugin.trace(3, "onComplete null object: Connect=" + this);
        }
    }
    
    /**
     * Give a simple string representation of this connection listener object.
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "JMConnection " + connect.getClientID();
    }

    /**
     * This plug-in does not support HTTP interactions.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data) {
        connect.close(2, "HTTP not supported");
    }
}
