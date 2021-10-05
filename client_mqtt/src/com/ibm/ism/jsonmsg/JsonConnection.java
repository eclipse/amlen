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
package com.ibm.ism.jsonmsg;

import java.io.IOException;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.IsmWSConnection;
import com.ibm.ism.ws.IsmWSMessage;

public class JsonConnection extends IsmWSConnection implements Runnable {
    String   clientID;
    String   username;
    String   password;
    int      port;
    int      encoding = JSON_TCP;
    JsonListener listen;
    Object   msgidLock;
    ImaJson  json;
    LinkedBlockingQueue <Object> recvq;
    HashMap<Integer,StringBuffer> waitlist = null;

    Long     inuse;
    String   unique;
    int      tempcount = 0;
    ByteBuffer bb = null;
    
    private boolean timedOut = false;
    
    private boolean stopped;

    public  static final int JSON_TCP  = 1;
    public  static final int JSON_WS   = 2;
    
    /**
     * Create a connection object.
     * This does not make the connection, but only sets up the object.
     * A connection object can be reused for multiple connections.
     * After a connection object is created, it can be modified using the
     * various set methods.  When the connect() method is called, the current
     * settings of the connection object are use to make the connection.
     * @param ip       The IP address or name of the server
     * @param port     The port of the server
     * @param path     The path for the WebSockets URI
     * @param clientID The client ID.  This must be unique among the various users of the server.
     */
    public JsonConnection(String ip, int port, String path, String clientID) {
        super(ip, port, path, "json-msg");
        this.clientID = clientID == null ? "" : clientID;
        msgidLock = new Object();
    }

    /**
     * Set the mqtt encoding for this connection.
     * @param encoding
     * @return  This connection object
     */
    public JsonConnection setEncoding(int encoding) {
        this.encoding = encoding;
        super.setVersion(encoding == JSON_TCP ? IsmWSConnection.JSONMSG : IsmWSConnection.WebSockets13);
        return this;
    }

    /**
     * Set a listener on the connection object.
     * The listener must be set before the connection is made.
     * If a listener is not set, the client application must call receive to get
     * messages, and handle IOExeption to handle connection closes.
     * @param listener  The listener to recieve asynchronous messages
     * @return This connection object
     */
    public JsonConnection setListener(JsonListener listener) {
        verifyNotActive();
        listen = listener;
        return this;
    }

    /**
     * Set the username and password for this connection.
     * @param username  The username for this connection
     * @param password  The pasword for this connection
     * @return This connection object.
     */
    public JsonConnection setUser(String username, String password) {
        verifyNotActive();
        this.username = username;
        this.password = password;
        return this;
    }


    /** This is in IsmWSConnection. No need to override here with same code.
     * Set the verbose flag. 
     * This is used for problem determination.
     * @param verbose
     * @return This connection object
     */
    /*public IsmWSConnection setVerbose(boolean verbose) {
        super.verbose = verbose;
        return this;
    }*/

    /*
     * Put out an optional field
     */
    static void putOption(StringBuffer sb, String name, String s) {
        if (s != null) {
            sb.append(", \"").append(name).append("\":\""); 
            ImaJson.escape(sb, s).append('"');
        }
    }
    
    /*
     * Put out a boolean option
     */
    static void putBoolOption(StringBuffer sb, String name, boolean b) {
        if (b) 
            sb.append(", \"").append(name).append("\":true");
        else
            sb.append(", \"").append(name).append("\":false");
    }
    
    /*
     * Put out a int option
     */
    static void putIntOption(StringBuffer sb, String name, int ival) {
        sb.append(", \"").append(name).append("\":").append(ival);
    }


    /**
     * Make the connection.
     * All modifications to the connection settings must be done before the connection is made.
     * This will do three levels of connection:
     * 1. A TCP connection
     * 2. A WebSockets connection handshake
     * 3. An MQTT handshake
     */
    public void connect() throws IOException {
        int  rc = 0;
        verifyNotActive();

        /*
         * Make the TCP and websockets connection
         */
        super.connect();
        json = new ImaJson();
        waitlist = new HashMap<Integer,StringBuffer>();
        inuse = new Long(1);
        
        /*
         * Start a receive thread
         */
        recvq = new LinkedBlockingQueue<Object>();
        new Thread(this).start();

        /*
         * Start a listener thread
         */
        if (listen != null) {
            new Thread(new JsonListenerThread(this)).start();
        }
        
        /*
         * Send the CONNECT
         */
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"Connect\"");
        putOption(buf, "ClientID", clientID);
        if (username != null && username.length()>0) 
            putOption(buf, "User", username);
        putOption(buf, "Password", password);
        buf.append(" }");
        
        setwait(0, buf);
        synchronized (buf) {
            send(new String(buf));
            rc = await(buf);
        }
        if (rc < 0) {
            /* Connection timeout */
            super.disconnect(rc, "Connect timed out waiting for a response from the server");
            throw new IOException("Connect timed out waiting for a response from the server");
        } else if (rc > 0) {
            /* Connection error */
            throw new IOException("Connection failed: rc=" + rc);
        }
    }
        

    /*
     * Set to wait for a string command
     */
    void setwait(Integer id, StringBuffer obj) {
        synchronized(msgidLock) {
            waitlist.put(id, obj);
        }
    }
    

    /*
     * Wait for an ACK with a string
     */
    int await(StringBuffer obj) {
        synchronized(obj) {
            try {obj.wait();} catch(InterruptedException ix){}
        }
        return obj.charAt(0);
    }


    /**
     * Wait for an ACK with a StringBuffer with timeout
     * @param obj      A wrapped StringBuffer
     * @param timeout  Timeout, in seconds
     * @return  0 - good response received
     *         -1 - timeout
     *         >0 - bad response received. 
     */
    int await(StringBuffer obj, long timeout) {
        long currentTime = System.currentTimeMillis();
        long wakeupTime = currentTime + timeout * 1000;

       synchronized(obj) {
            while ((currentTime < wakeupTime) && obj.charAt(0) == 99) {
                try {
                    obj.wait(wakeupTime - currentTime);
                } catch(InterruptedException ix) { 
                    Thread.currentThread().interrupt(); 
                }
                currentTime = System.currentTimeMillis();
            }
        }

        int rc = obj.charAt(0); 
        return (currentTime >= wakeupTime && rc != 0)?-1:rc;
    }
    
    /*
     * Respond to a message ID with a return code
     */
    void reply(int msgid, int rc) {
        Object obj;
        synchronized(msgidLock) {
            obj = waitlist.remove(msgid);
        }
        System.out.println("reply obj="+obj+ " msgid=" + msgid + " rc=" + rc);

        if (obj != null) {
            synchronized(obj) {
                if (obj instanceof StringBuffer)
                    ((StringBuffer)obj).setCharAt(0, (char) rc);;
                obj.notify();
            }
        }
    }


    /**
     * Send acknowledgment 
     * @param msg A message to acknowledge.
     */
    void acknowledge(JsonMessage msg, int rc) {
        /* Don't ack, if not connected */
        if (!connected) {
            return;
        }
        /* TODO: send the ACK */
    }
    
    /*
     * Put an object onto the receive queue
     */
    void putq(Object obj) {
        for (;;) {
            try {
                recvq.put(obj);
                break;
            } catch(InterruptedException iex) { }
        }    
    }


    /*
     * Run the receive thread for this connection.
     * If this is a message, either queue it
     * @see java.lang.Runnable#run()
     */
    public void run() {
        stopped = false;
        String savestr = null;
        String jstr;
        ImaJson jobj = new ImaJson();
        boolean msgLeftover = false;
        for (;;) {
            try {
                IsmWSMessage wmsg = null;
                try {
                    /* This might get just a fragment */
                    if (!msgLeftover) {
                    	wmsg = super.receive();
                    	System.out.println("jmreceive: " + wmsg.getPayload());
                    	if (savestr != null)
                    		jstr = savestr + wmsg.getPayload();
                    	else
                    		jstr = wmsg.getPayload();
                    } else {
                    	jstr = savestr;
                    	msgLeftover = false;
                    }
                    savestr = null;
                    int rc = jobj.parse(jstr);
                    if (rc < -1) {          /* Bad JSON */
                    	System.out.println("Bad JSON: '"+jstr+"'");
                        closeConnection();
                    	savestr = jstr;
                    } else if (rc == -1) {  /* Incomplete JSON */ 
                        savestr = jstr;
                    } else {                /* One complete JSON object */
                        if (jobj.getPosition() < jstr.length()) {
                            savestr = jstr.substring(jobj.getPosition());   /* Save remaining data */
                            msgLeftover = true;
                        }
                        String action = jobj.getString("Action");
                        if ("Send".equals(action)) {
                            doSend(jobj);
                        } else if ("Ack".equals(action)) {    
                            doAck(jobj);
                        } else if ("Connected".equals(action)) {    
                            doConnected(jobj);
                        } else if ("Ping".equals(action)) {
                            doPing(jobj);
                        } else if ("Pong".equals(action)) {
                            doPong(jobj);
                        } else {
                            closeConnection();
                        }
                    }    
                    /* TODO: fix up message */
                } catch (SocketException se) {
                    if (timedOut) {
                        putq(new IOException("Json-msg connection timed out", se));
                        break;
                    } else {
                        throw se;
                    }
                }
            } catch (Exception iox) {
                if (connected) 
                    putq(iox);
                else
                    putq(0);
                break;
            }
        }
        synchronized (this) {
            stopped = true;
        }
    }
    
    /*
     * Process a publish
     */
    void doSend(ImaJson jobj) {
    	System.out.println("In doSend with: '"+jobj.toString()+"'");
        String dest = jobj.getString("Topic");
        String body  = jobj.getString("Body");
        String name  = jobj.getString("Name");
        boolean persist = jobj.getBoolean("Persistent", false);
        boolean retain = jobj.getBoolean("Retain", false);
        int id = jobj.getInt("ID", 0);
        int qos = jobj.getInt("QoS", 0);
        boolean  isQueue = false;
        if (dest == null) {
            dest = jobj.getString("Queue");
            if (dest != null)
                isQueue = true;
        }
        if (dest != null) {
            JsonMessage msg = new JsonMessage(name, dest, isQueue, body, persist, retain);
            msg.msgid = id;
            msg.setQos(qos);
            putq(msg);
        } else {
            closeConnection();
        }
    }
    
    /*
     * Process an ACK
     */
    void doAck(ImaJson jobj) {
        int  id = jobj.getInt("ID", 0);
        System.out.println("doAck: for ID="+id);
        if (id != 0) {
        	int RC = jobj.getInt("RC", 0);
        	reply(id, RC);
        }
    }   
    
    /*
     * Process an ACK
     */
    void doConnected(ImaJson jobj) {
        reply(0, 0);
    } 
    
    /*
     * Process a disconnect
     */
    void doClose(ImaJson jobj) {
        closeConnection();
    }
    
    /*
     * Process an ACK
     */
    void doPing(ImaJson jobj) {
        sendPong();
    }
    
    /*
     * Process an ACK
     */
    void doPong(ImaJson jobj) {
    }

    /*
     * Create a map object from a JSON object
     */
    Map<String,Object> toMap(ImaJson json, int entnum) {
        ImaJson.Entry [] ent = json.getEntries();
        if (entnum <0 || entnum >= json.getEntryCount() || ent[entnum].objtype != ImaJson.JObject.Object) 
            return null;
        HashMap<String,Object> map = new HashMap<String,Object>();
        for(int count = ent[entnum++].count; count > 0; count--, entnum++) {
            if (ent[entnum].objtype == ImaJson.JObject.Array ||
                    ent[entnum].objtype == ImaJson.JObject.Object ) {
                count -= ent[entnum].count;
                entnum += ent[entnum].count;
            } else {
                switch(ent[entnum].objtype) {
                case String:   map.put(ent[entnum].name, ent[entnum].value);   break;
                case Integer:  map.put(ent[entnum].name, ent[entnum].count);   break;
                case Number:   map.put(ent[entnum].name, Double.valueOf(ent[entnum].value));   break;
                case True:     map.put(ent[entnum].name, true);                     break;
                case False:    map.put(ent[entnum].name, false);                    break;
                case Null:     map.put(ent[entnum].name, (Object)null);             break;
                }
            }
        }
        return map;
    }

    /*
     * Create a List object from a JSON array
     */
    List<Object> toList(ImaJson json, int entnum) {
        ImaJson.Entry [] ent = json.getEntries();
        if (entnum <0 || entnum >= json.getEntryCount() || ent[entnum].objtype != ImaJson.JObject.Array) 
            return null;
        Vector<Object> list = new Vector<Object>();
        int count = ent[entnum++].count; 
        while (count-- > 0) {
            if (ent[entnum].objtype == ImaJson.JObject.Array ||
                    ent[entnum].objtype == ImaJson.JObject.Object ) {
                count -= ent[entnum].count;    
            } else {
                switch(ent[entnum].objtype) {
                case String:   list.add(ent[entnum].value);   break;
                case Integer:  list.add(ent[entnum].count);   break;
                case Number:   list.add(Double.valueOf(ent[entnum].value));   break;
                case True:     list.add(true);                break;
                case False:    list.add(false);               break;
                case Null:     list.add((Object)null);        break;
                }
            }
        }
        return list;
    }

    /**
     * Simple form of topic subscribe
     * <p>
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param topic   The topic or queue name 
     * @param durble  The subscription is durable if true
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, String topic, boolean durable) throws IOException {
        return subscribe(name, false, topic, durable, 0, false, 2 ,0);
    }

    /**
     * Simple form of topic subscribe
     * <p>
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param topic   The topic or queue name 
     * @param durable The subscription is durable if true
     * @param qos     The QoS for the subscription
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, String topic, boolean durable, int qos) throws IOException {
        return subscribe(name, false, topic, durable, 0, false, qos, 0);
    }

    /**
     * Simple form of topic subscribe
     * <p>
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param topic   The topic or queue name 
     * @param durable The subscription is durable if true
     * @param qos     The QoS for the subscription
     * @param id      id for subscribe message to request acknowledgement
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, String topic, boolean durable, int qos, int id) throws IOException {
        return subscribe(name, false, topic, durable, 0, false, qos, id);
    }


    /**
     * Subscribe to a topic.
     * <p>
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param isQueue True if the destination is a queue
     * @param dest    The topic or queue name 
     * @param durble  The subscription is durable if true
     * @param shared  The share setting: 0=not shared, 1=shared, 2=global shared.  This is ignored if the destination is a queue.
     * @param nolocal If true do not receive messages from the same clientID.  This is ignored if shared is not 0.
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, boolean isQueue, String dest, boolean durable, int shared, boolean nolocal) throws IOException {
    	return subscribe(name, isQueue, dest, durable, shared, nolocal, 2, 0);
    }

    /**
     * Subscribe to a topic.
     * <p>
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param isQueue True if the destination is a queue
     * @param dest    The topic or queue name 
     * @param durble  The subscription is durable if true
     * @param shared  The share setting: 0=not shared, 1=shared, 2=global shared.  This is ignored if the destination is a queue.
     * @param nolocal If true do not receive messages from the same clientID.  This is ignored if shared is not 0.
     * @param qos     The QoS for the subscription 
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, boolean isQueue, String dest, boolean durable, int shared, boolean nolocal, int qos) throws IOException {
    	return subscribe(name, isQueue, dest, durable, shared, nolocal, qos, 0);
    }

    /**
     * Subscribe to a topic.
     * <p>
     * @param name    The name of the subscriptions. The name is required and must not currently be in use.
     * @param isQueue True if the destination is a queue
     * @param dest    The topic or queue name 
     * @param durble  The subscription is durable if true
     * @param shared  The share setting: 0=not shared, 1=shared, 2=global shared.  This is ignored if the destination is a queue.
     * @param nolocal If true do not receive messages from the same clientID.  This is ignored if shared is not 0.
     * @param qos     The QoS for the subscription 
     * @param id      id for subscribe message to request acknowledgement
     * @return A return code indicating success or failure 
     * @throws IOException if the send to the server fails
     */
    public int subscribe(String name, boolean isQueue, String dest, boolean durable, int shared, boolean nolocal, int qos, int id) throws IOException {
        int  rc = 0;
        verifyActive();
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"Subscribe\"");
        putOption(buf, "Name", name);
        if (isQueue) 
            putOption(buf, "Queue", dest);
        else
            putOption(buf, "Topic", dest);
        if (durable)
            putBoolOption(buf, "Durable", durable);
        if (shared > 0 && shared <= 2)
            putIntOption(buf, "Shared", shared);
        if (nolocal)
            putBoolOption(buf, "Nolocal", nolocal);
        putIntOption(buf, "QoS", qos);
        if (0 != id)
        	putIntOption(buf, "ID", id);
        buf.append(" }");
        
        if (id != 0)
            setwait(id, buf);
        synchronized (buf) {
        	System.out.println("subscribe: Sending Subscribe: "+buf);
            send(new String(buf));
            if (id != 0)
            	System.out.println("subscribe: waiting for return from Subscribe");
                rc = await(buf);
        }    
        return rc;
    }


    /**
     * Close a subscription from a topic.
     * It is not an error to unsubscribe from a topic to which you are not subscribed.
     * @param name  The name of the topic
     * @throws IOException if the send to the server fails
     */
    public void closeSubscription(String name) throws IOException {
        verifyActive();
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"CloseSubscription\"");
        putOption(buf, "Name", name);
        buf.append(" }");
        
        setwait(0, buf);
        synchronized (buf) {
            send(new String(buf));
        }    
    }
  
    /**
     * Destroy a subscription
     * This is used to fully terminate a durable subscription
     */
    public void destroySubscription(String name) throws IOException {
    	verifyActive();
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"DestroySubscription\"");
        putOption(buf, "Name", name);
        buf.append(" }");
        
        setwait(0, buf);
        synchronized (buf) {
            send(new String(buf));
        }    
    }

    /**
     * Publish a string message 
     * @param topic   The topic on which to send the message    
     * @param payload The payload of the message 
     * @return The message ID or 0 for a qos=0 message
     * @throws IOException if the send to the server fails
     */
    public int publish(String topic, String payload) throws IOException {
        return publish(new JsonMessage(topic, payload));
    }


    /**
     * Publish a message
     * @param msg  A JSON message
     * @return The sequence number or 0 if the messages is not ACKed
     * @throws IOException if the send to the server fails
     */
    public int publish(JsonMessage msg) throws IOException {
        return publish(msg, 0);
    }


    /**
     * Publish a message
     * @param msg  A JSON message
     * @return The sequence number or 0 if the messages is not ACKed
     * @throws IOException if the send to the server fails
     */
    public int publish(JsonMessage msg, int id) throws IOException {
        int  rc;
        verifyActive();
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"Send\"");
        if (msg.isQueue()) 
            putOption(buf, "Queue", msg.getQueue());
        else
            putOption(buf, "Topic", msg.getTopic());
        if (msg.persist)
            putBoolOption(buf, "Persistent", msg.persist);
        //if (msg.qos > 0)
        	putIntOption(buf, "QoS", msg.qos);
        if (0 != id)
        	putIntOption(buf, "ID", id);
        if (msg.getRetain())
        	putBoolOption(buf, "Retain", true);
        
        String payload = msg.getPayload();
        buf.append(", \"Body\":");
        if (msg.msgtype == JsonMessage.MapMessage || msg.msgtype == JsonMessage.ListMessage) {
            buf.append(payload);
        } else {
            buf.append('"');
            ImaJson.escape(buf, payload).append("\"");
        }
        buf.append(" }");
        System.out.println("Send message: "+(new String(buf)));
        
        if (id != 0)
            setwait(id, buf);
        synchronized (buf) {
            send(new String(buf));
            if (id != 0)
                rc = await(buf);
            else
                rc = 0;
        }    
        return rc;
    }

    /**
     * Receive a message.
     * This call will block until a message is available or the connection closes.
     * This can be done in 
     * @return The returned message
     * @throws IOException if the send to the server fails
     */
    public JsonMessage receive() throws IOException {
        verifyActive();
        if (listen != null) {
            throw new IllegalStateException("receive() cannot be used with a message listener");
        }
        for (;;) {
            try {
                Object obj = recvq.take();
                if (obj instanceof JsonMessage) {
                    JsonMessage msg = (JsonMessage)obj; 
                    if (msg.getQoS() > 0)
                        acknowledge(msg, 0);
                    return msg;
                } else if (obj instanceof IOException)
                    throw (IOException)obj;
                else if (obj instanceof RuntimeException) 
                    throw (RuntimeException)obj;
                else {
                    String rc = obj instanceof Integer ? " rc="+(Integer)obj : "";
                    IOException iox = new IOException("Disconnect" + rc);
                    if (obj instanceof Throwable)
                        iox.initCause((Throwable)obj);
                    throw iox;
                }    
            } catch (InterruptedException iex) {
            }
        }
    }


    /**
     * Receive a message with a timeout.
     * This call will block until a message is available or the connection closes.
     * This can be done in 
     * @return The returned message or null for a timeout
     * @throws IOException if the send to the server fails
     */
    public JsonMessage receive(long millis) throws IOException {
        verifyActive();
        if (listen != null) {
            throw new IllegalStateException("receive() cannot be used with a message listener");
        }
        for (;;) {
            try {
                Object obj = recvq.poll(millis, TimeUnit.MILLISECONDS);
                if (obj == null)
                    return null;
                if (obj instanceof JsonMessage) {
                    JsonMessage msg = (JsonMessage)obj; 
                    if (msg.getQoS() > 0)
                        acknowledge(msg, 0);
                    return msg;
                } else if (obj instanceof IOException)
                    throw (IOException)obj;
                else if (obj instanceof RuntimeException) 
                    throw (RuntimeException)obj;
                else {
                    String rc = obj instanceof Integer ? " rc="+(Integer)obj : "";
                    IOException iox = new IOException("Disconnect" + rc);
                    if (obj instanceof Throwable)
                        iox.initCause((Throwable)obj);
                    throw iox;
                }    
            } catch (InterruptedException iex) {
            }
        }
    }

    
    /**
     * Delete the Retained message for a topic
     */
    public void getRetained(String topic, int id) throws IOException {
    	StringBuffer buf = new StringBuffer();
        
        buf.append("{ \"Action\": \"GetRetained\", \"Topic\": \"");
        buf.append(topic);
        buf.append("\"");
        if (id != 0) {
        	putIntOption(buf, "ID", id);
        }
        buf.append(" }");
        send(new String(buf));
        return;
    }
    
    /**
     * Delete the Retained message for a topic
     */
    public int deleteRetained(String topic, int id) throws IOException {
    	StringBuffer buf = new StringBuffer();
    	int rc;
        
        buf.append("{ \"Action\": \"DeleteRetained\", \"Topic\": \"");
        buf.append(topic);
        buf.append("\"");
        if (id != 0) {
        	putIntOption(buf, "ID", id);
        }
        buf.append(" }");
        if (id != 0)
            setwait(id, buf);
        synchronized (buf) {
            send(new String(buf));
            if (id != 0)
                rc = await(buf);
            else
                rc = 0;
        }    
        return rc;
    }

    /**
     * Normal disconnect from the connection.
     * After a disconnect, the connection object can be reused 
     */
    public void closeConnection() {
        closeConnection(0, "");
    }


    /**
     * 
     * @param code
     * @param reason
     */
    public void closeConnection(int code, String reason) {
        try {
            send("{ \"Action\": \"Close\" }");
        } catch (IOException ex) {
        }
        try {
            super.disconnect(code, reason);
        } catch (IOException ex) {
        }
        try {
            synchronized (this) {
                while (!stopped) {
                    try { 
                        this.wait(50);
                    } catch (InterruptedException e) {
                        
                    }
                }
            }
        } catch (Exception e) {
        }
    }

    /**
     * Send ping to the server.
     * This is a request to respond.
     */
    public void sendPing() {
        try {
            send("{\"Action\":\"Ping\"}");            
        } catch (IOException ex) {
        }
    }

    /**
     * Send pong to the server.
     * This is the normal response to a ping
     */
    public void sendPong() {
        try {
            send("{\"Action\":\"Pong\"}");            
        } catch (IOException ex) {
        }
    }

    
    /*
     * Put a string into a byte buffer in UTF8.
     * This also ensures that there are 32 bytes extra apace left in the byte buffer
     */
    public ByteBuffer putString(ByteBuffer bb, String s) {
        int len = sizeUTF8(s);
        if (bb.capacity() < (len+2)) {
            int newlen = bb.capacity() + len + 34;
            ByteBuffer newbb = ByteBuffer.allocate(newlen);
            bb.flip();
            newbb.put(bb);
            bb = newbb;
        }
        bb.putShort((short)len);
        makeUTF8(s, len, bb);
        return bb;
    }


    /*
     * Make a valid UTF-8 byte array from a string.  
     * Before invoking this method you must call sizeUTF8 using the same string to find the length of the UTF-8
     * buffer and to check the validity of the UTF-8 string.
     */
    static void makeUTF8(String str, int utflen, ByteBuffer ba) {
        int  strlen = str.length();
        int  c;
        int  i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007F) {
                ba.put((byte) c);
            } else if (c > 0x07FF) {
                if (c >= 0xd800 && c <= 0xdbff) {
                    c = ((c&0x3ff)<<10) + (str.charAt(++i)&0x3ff) + 0x10000;
                    ba.put((byte) (0xF0 | ((c >> 18) & 0x07)));
                    ba.put((byte) (0x80 | ((c >> 12) & 0x3F)));
                    ba.put((byte) (0x80 | ((c >>  6) & 0x3F)));
                    ba.put((byte) (0x80 | (c & 0x3F)));
                } else {    
                    ba.put((byte) (0xE0 | ((c >> 12) & 0x0F)));
                    ba.put((byte) (0x80 | ((c >>  6) & 0x3F)));
                    ba.put((byte) (0x80 | (c & 0x3F)));
                }
            } else {
                ba.put((byte) (0xC0 | ((c >>  6) & 0x1F)));
                ba.put((byte) (0x80 | (c & 0x3F)));
            }
        }
    }

    /* Starter states for UTF8 */
    private static final int States[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
    };

    /* Initial byte masks for UTF8 */
    private static final int StateMask[] = {0, 0, 0x1F, 0x0F, 0x07};

    /*
     * Check if the second and subsequent bytes of a UTF8 string are valid.
     */
    private static boolean validSecond(boolean checkValid, int state, int byte1, int byte2) {
        if (byte2 < 0x80 || byte2 > 0xbf)
            return false;
        boolean ret = true;
        if (checkValid) {
            switch (state) {
            case 2:
                if (byte1 < 2)
                    ret =false;
                break;
            case 3:
                if (byte1== 0 && byte2 < 0xa0)
                    ret = false;
                break;
            case 4:
                if (byte1 == 0 && byte2 < 0x90)
                    ret = false;
                else if (byte1 == 4 && byte2 > 0x8f)
                    ret = false;
                else if (byte1 > 4)
                    ret = false;
            }
        }
        return ret;
    }

    /*
     * Fast UTF-8 to char conversion.
     * This code does a fast conversion from a UTF-8 byte array to a String.
     * If checkValid is specified, the function matches the Unicode specification for checking
     * encoding, but does not guarantee that there are no unmatched surrogates.  The modified
     * UTF-8 will fail when checkValid is specified if there is a zero byte encoded.
     */
    static String fromUTF8(byte [] ba, int pos, int len, boolean checkValid) throws IOException {
        char [] tbuf = new char[len*2];
        int  byte1 = 0;
        int  byteoff = pos;
        int  byteend = pos+len;
        int  charoff = 0;
        int  state = 0;
        int  value = 0;
        int  inputsize = 0;

        while (byteoff < byteend) {
            if (state == 0) {
                /* Fast loop in single byte mode */
                for (;;) {
                    byte1 = ba[byteoff];
                    if (byte1 < 0)
                        break;
                    tbuf[charoff++] = (char)byte1;
                    if (++byteoff >= byteend)
                        return new String(tbuf, 0, charoff);
                }

                byteoff++;
                state = States[(byte1&0xff) >>3];
                value = byte1 = byte1 & StateMask[state];
                inputsize = 1;
                if (state == 1)
                    throw new IOException("The initial UTF-8 byte is not valid.");
            } else {
                int byte2 = ba[byteoff++]&0xff;
                if ((inputsize==1 && !validSecond(checkValid, state, byte1, byte2)) ||
                        (inputsize > 1 && byte2 < 0x80 || byte2 > 0xbf)) 
                    throw new IOException("The continuing UTF-8 byte is not valid.");
                value = (value<<6) | (byte2&0x3f);
                if (inputsize+1 >= state) {
                    if (value < 0x10000) {
                        tbuf[charoff++] = (char)value;
                    } else {
                        tbuf[charoff++] = (char)(((value-0x10000) >> 10) | 0xd800);
                        tbuf[charoff++] = (char)(((value-0x10000) & 0x3ff) | 0xdc00);
                    }
                    state = 0;
                } else {
                    inputsize++;
                }
            }
        }    
        return new String(tbuf, 0, charoff);
    }


    /* 
     * Return the length of a valid UTF-8 string including surrogate handling.
     * @param str  The string to find the UTF8 size of
     * @returns The length of the string in UTF-8 bytes, or -1 to indicate an input error
     */
    static int sizeUTF8(String str) {
        int  strlen = str.length();
        int  utflen = 0;
        int  c;
        int  i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007f) {
                utflen++;
            } else if (c <= 0x07ff) {
                utflen += 2;
            } else if (c >= 0xdc00 && c <= 0xdfff) {   /* High surrogate is an error */
                return -1;
            } else if (c >= 0xd800 && c <= 0xdbff) {   /* Low surrogate */
                if ((i+1) >= strlen) {      /* At end of string */
                    return -1;
                } else {                    /* Check for valid surrogate */
                    c = str.charAt(++i);
                    if (c >= 0xdc00 && c <= 0xdfff) {
                        utflen += 4;
                    } else {
                        return -1;
                    }    
                }
            } else {     
                utflen += 3;
            }
        }
        return utflen;
    }

}
