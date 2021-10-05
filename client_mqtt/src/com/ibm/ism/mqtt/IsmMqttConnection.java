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

package com.ibm.ism.mqtt;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Vector;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.zip.CRC32;

import com.ibm.ism.ws.IsmWSConnection;
import com.ibm.ism.ws.IsmWSMessage;

/**
 * IsmMqttConnection represents an MQTT connection from a client to a server.
 * 
 *
 */
public class IsmMqttConnection extends IsmWSConnection implements Runnable, IsmMqttPropChecker {
    String   clientID;
    String   username;
    String   password;
    IsmMqttMessage willMessage;
    long     willDelay;
    int      port;
    int      version = 4;
    int      encoding = TCP4;
    int      keepalive = 60;
    long     expire = 0xFFFFFFFFL;
    int      maxActive = 0;
    int      serverMaxActive = 0;
    int      serverMaxTopicAlias = 0;
    int      clientMaxTopicAlias = 0;
    String[] clientTopicAlias;
    String[] serverTopicAlias;
    int      connectionTimeout = 30;
    int      maxQoS = 2;
    boolean  noRetain;
    boolean  sessionPresent;
    int      maxPacketSize = 0;
    int      serverMaxPacketSize;
    boolean  cleanSession = true;
    String   replyInfo;
    String   serverReference;
    volatile String reason;
    boolean  requestReplyInfo;
    int      requestReason = -1;
    IsmMqttListener listen;
    IsmMqttListenerAsync listenAsync;
    Object   msgidLock;
    ImaJson json;
    LinkedBlockingQueue <Object> recvq;
    HashMap<Integer,IsmMqttResult> waitlist = null;
    HashMap<Integer,IsmMqttMessage> publishlist = null; /* QoS == 1, send, between PUBLISH and PUBACK
                                                   QoS == 2, send, between PUBLISH and PUBCOMP        */
    HashMap<Integer,Object> pubrellist = null;  /* QoS == 2, send, between PUBREL and PUBCOMP         */
    HashMap<Integer,IsmMqttMessage> recvlist = null; /* QoS == 2, receive, between PUBLISH and PUBREL */
    IsmMqttResult    inuse;
    String   unique;
    int      tempcount = 0;
    ByteBuffer bb = null;
    IsmMqttPropCtx ctx;
    List<IsmMqttUserProperty> connectProps;
    List<IsmMqttUserProperty> serverProps;
    
    private IsmMqttKeepAliveThread keepAliveThread;
    private boolean timedOut = false;
    
    private boolean stopped;

    /* Legacy names for the encodings */
    public  static final int BINARY    = 1;
    public  static final int BINARYWS  = 2;
    public  static final int BINARY4   = 3;
    public  static final int BINARYWS4 = 4;
    
    /** Encoding for MQTTv3.1 TCP */
    public  static final int TCP3      = 1;
    /** Encoding for MQTTv3.1 over WebSockets */
    public  static final int WS3       = 2;
    /** Encoding for MQTTv3.1.1  TCP */
    public  static final int TCP4      = 3;
    /** Encoding for MQTTv3.1.1 over WebSockets */
    public  static final int WS4       = 4;
    /** Encoding for MQTTv5 TCP */
    public  static final int TCP5      = 5;
    /** Encoding for MQTTv5 over WebSockets */
    public  static final int WS5       = 6;
    /** Encoding for MQTTv3.1.1 using proxy protocol */
    public  static final int PROXY4    = 14;
    /** Encoding for MQTTv5 usijng proxy protocol */
    public  static final int PROXY5    = 15;
    

    /*
     * MQTT types
     */
    /** Connect packet */
    public static final int CONNECT     = 1;
    /** Connect acknowledge packet */
    public static final int CONNACK     = 2;
    /** Publish packet */
    public static final int PUBLISH     = 3;
    /** Publish acknowledge (QoS=1) */
    public static final int PUBACK      = 4;
    /** Publish received (QoS=2 first acknowledge) */
    public static final int PUBREC      = 5;
    /** Publish release (QoS=2 second acknowledge) */
    public static final int PUBREL      = 6;
    /** Publish commplete (QoS=2 third acknowledge) */
    public static final int PUBCOMP     = 7;
    /** Subscribe packet */
    public static final int SUBSCRIBE   = 8;
    /** Subscribe acknowledge packet */
    public static final int SUBACK      = 9;
    /** Unsubscribe packet */ 
    public static final int UNSUBSCRIBE = 10;
    /** Unsubscribe acknowledge */
    public static final int UNSUBACK    = 11;
    /** Ping request packet */
    public static final int PINGREQ     = 12;
    /** Ping response packet */
    public static final int PINGRESP    = 13;
    /** Disconnect packet */
    public static final int DISCONNECT  = 14;
    /** Authentication packet */
    public static final int AUTH        = 15;

    /** Publish Duplicate flag */
    static final int PUBLISH_DUP    = 0x08;
    /** Publish Retain flag */
    static final int PUBLISH_RETAIN = 0x01;
    /** Subscribe option NoLocal */
    public static final int SUBOPT_NoLocal  = 0x04;
    /** Subscribe option RetainAsPublished */
    public static final int SUBOPT_RetainAsPublished = 0x08;
    /** Subscribe option Send retain only on new subscriptions */
    public static final int SUBOPT_SendRetainNew  = 0x10;
    /** Subscribe option do not send retained at subscribe */
    public static final int SUBOPT_NoSendRetain   = 0x20;

    
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
    public IsmMqttConnection(String ip, int port, String path, String clientID) {
        super(ip, port, path, "mqtt");
        this.clientID = clientID == null ? "" : clientID;
        msgidLock = new Object();
    }
    
    /**
     * Get the version of the connection
     * @return The version (3, 4, or 5)
     */
    public int getVersion() {
        return version;
    }


    /**
     * Set a listener on the connection object.
     * The listener must be set before the connection is made.
     * If a listener is not set, the client application must call receive to get
     * messages, and handle IOExeption to handle connection closes.
     * @param listener  The listener to recieve asynchronous messages
     * @return This connection object
     */
    public IsmMqttConnection setListener(IsmMqttListener listener) {
        verifyNotActive();
        listen = listener;
        return this;
    }
    
    /**
     * Set an asynchronous listener on the connection object
     * If the async listener is set, the se4tting of a normal listener is not used 
     * @param listener  The async listener
     * @return The connection
     * @since MQTTv5
     */
    public IsmMqttConnection setAsyncListener(IsmMqttListenerAsync listener) {
        verifyNotActive();
        listenAsync = listener;
        return this;
    }


    /**
     * Set the mqtt encoding for this connection.
     * @param encoding
     * @return  This connection object
     */
    public IsmMqttConnection setEncoding(int encoding) {
        this.encoding = encoding;
        if (encoding == TCP4 || encoding == WS4 || encoding == PROXY4)
            version = 4;
        else if (encoding == TCP3 || encoding == WS3)
            version = 3;
        else if (encoding == TCP5 || encoding == WS5 || encoding == PROXY5)
            version = 5;
        else
            version = 0;
        return this;
    }
    
    /**
     * Set the WebSocket chunk size.
     * This is used to test cases where the WebSocket and MQTT packets are not aligned.
     * This is not useful in any way other than for testing.
     */
    public IsmMqttConnection setChunksize(int chunk) {
        this.chunksize = chunk;
        return this;
    }


    /**
     * Set the username and password for this connection.
     * @param username  The username for this connection
     * @param password  The pasword for this connection
     * @return This connection object.
     */
    public IsmMqttConnection setUser(String username, String password) {
        verifyNotActive();
        this.username = username;
        this.password = password;
        return this;
    }


    /**
     * Set the will for this connection.
     * @param topic   The topic to send on abnormal close
     * @param message The message to send on abnormal close
     * @param qos     The quality of service of the final message
     * @param retain  The retain flag of the final message
     * @return This connection object.
     */
    public IsmMqttConnection setWill(String topic, String message, int qos, boolean retain) {
        verifyNotActive();
        if (topic == null) {
            this.willMessage = null;
        } else {
            this.willMessage = new IsmMqttMessage(topic, message, qos, retain);
        }    
        this.willDelay = 0;
        return this;
    }


    /**
     * Set the will for this connection with delay.
     * @param topic   The topic to send on abnormal close
     * @param message The message to send on abnormal close
     * @param qos     The quality of service of the final message
     * @param retain  The retain flag of the final message
     * @param delay   The will delay in seconds
     * @return This connection object.
     * @since MQTTv5
     */
    public IsmMqttConnection setWill(String topic, String message, int qos, boolean retain, int delay) {
        verifyNotActive();
        if (topic == null) {
            this.willMessage = null;
            this.willDelay = 0;
        } else {
            this.willMessage = new IsmMqttMessage(topic, message, qos, retain);
            this.willDelay   = delay;
        }    
        return this;
    }
    
    /**
     * Set the will message for this connection as a message.
     * @param msg   The MQTT message
     * @param delay The will delay
     * @return This connection object
     * @since MQTTv5
     */
    public IsmMqttConnection setWill(IsmMqttMessage msg, long delay) {
        verifyNotActive();
        if (msg == null) {
            this.willMessage = null;
            this.willDelay = 0;
        } else {    
            this.willMessage = msg;
            this.willDelay = delay;
        }    
        return this;    
    }


    /**
     * The time in seconds to keep the connection alive in absence of
     * message activity. 
     * @param seconds
     * @return This connection object
     */
    public IsmMqttConnection setKeepAlive(int seconds) {
        verifyNotActive();
        if ((seconds < 10 || seconds > 66535) && seconds != 0) {
            throw new IllegalArgumentException("KeepAlive not valid: " + seconds);
        }
        this.keepalive = seconds;
        return this;
    }
    
    
    /*
     * Get the keepalive which could have been changed after the connect.
     */
    public int getKeepAlive() {
        return this.keepalive;
    }
    
    /**
     * Get the maximum QoS set by the server
     * @return The maximum QoS.  This is only valid after the connect is complete
     */
    public int getMaxQoS() {
        verifyActive();
        return this.maxQoS;
    }
    
    /**
     * Get the maximum packet size to sent to the server
     * @return
     */
    public int getMaxPacketSize() {
        return this.maxPacketSize;    
    }

    
    /**
     * Get the maximum packet size set by the server
     * @return The maximum packet size.  This is only valid after the connect is complete
     * @since MQTTv5
     */
    public int getServerMaxPacketSize() {
        verifyActive();
        return this.serverMaxPacketSize;
    }
    
    /**
     * Return if the session present flag is set.
     * This flag is only set after the connect is complete
     * @return true if the session is present
     */
    public boolean isSessionPresent() {
        return connected ? sessionPresent : false;
    }
    
    /**
     * Get the list of properties, or null if there are no properties
     * @return A list of user properties
     * @since MQTTv5
     */
    public synchronized List<IsmMqttUserProperty> getUserProperties() {
        return connectProps;
    }
    
    /**
     * Get the user properties the server return
     * @return A list of server properties
     * @since MQTTv5
     */
    public synchronized List<IsmMqttUserProperty> getServerProperties() {
        verifyActive();
        return serverProps;
    }
    
    /**
     * Add a user connect property
     * The connection properties must be set before the connect.
     * @param name  The property name
     * @param value The property value 
     * @since MQTTv5
     */
    public synchronized void addUserProperty(String name, String value) {
        verifyNotActive();
        if (connectProps == null) {
            connectProps = (List<IsmMqttUserProperty>)new Vector<IsmMqttUserProperty>();
        }
        if (name == null)
            name = "";
        if (value == null)
            value = "";
        connectProps.add(new IsmMqttUserProperty(name, value));
    }

    /*
     * Private method to set the server user properties.
     */
    private synchronized void addServerProperty(String name, String value) {
        if (serverProps == null) {
            serverProps = (List<IsmMqttUserProperty>)new Vector<IsmMqttUserProperty>();
        }
        if (name == null)
            name = "";
        if (value == null)
            value = "";
        serverProps.add(new IsmMqttUserProperty(name, value));
    }

    /**
     * Request reply info
     * @param request  Specify whether reply info should be requested
     * @since MQTTv5
     */
    public IsmMqttConnection setRequestReplyInfo(boolean request) {
        verifyNotActive();
        this.requestReplyInfo = request;
        return this;    
    }
    
    /**
     * Get the reply info.
     * @return The reply info from the server.  This is null if the server did not send it.
     * @since MQTTv5
     */
    public String getReplyInfo() {
        verifyActive();
        return this.replyInfo;
    }
    
    /**
     * Set the request reason value
     * @param reason  The request reason setting
     * <br>-1 = use the server default
     * <br>0  = Do not send the reason string except on CONNACK and DISCONNECT
     * <br>1  = Allow the reason string on all packets with a return code
     * <br>9  = Send sever identification on CONNACK and allow all reason strings.
     * @return This connection
     * @since MQTTv5
     */
    public IsmMqttConnection setRequestReason(int reason) {
        verifyNotActive();
        if ((reason < -1 || reason > 1) && reason != 9) {
            throw new IllegalArgumentException("RequestReason is not valid");
        }
        requestReason = reason;
        return this;
    }    
    
    /**
     * Get the request reason value
     * The value is -1 to indicate use the default, or the values 0-3.
     * @return the request reason value
     * @since MQTTv5
     */
    public int getRequestReason() {
        return requestReason;
    }    
    
    /**
     * Set the maximum active (flow control) for client to server
     * @param active The maximum active messages for flow control (1-65535)
     * @return this object
     * @since MQTTv5
     */
    public IsmMqttConnection setMaxReceive(int active) {
        verifyNotActive();
        if (active < 1 || active > 65535) {
            throw new IllegalArgumentException("MaxActive not valid: " + active);
        }
        this.maxActive = active;
        return this;
    }
    
    /**
     * Set the maximum packet size for packets sent from the server
     * @param maxsize  The maximum size packet the se4rver can sent to this client 
     * @return this object
     * @since MQTTv5
     */
    public IsmMqttConnection setMaxPacketSize(int maxsize) {
        verifyNotActive();
        this.maxPacketSize = maxsize;
        return this;
    }
    
    /**
     * Set the number of topic aliases to use.
     * @param clientTA  The number of topic aliases to use server to client
     *                  The server can use up to this number.
     * @param serverTA  The number of topic aliases to use client to server.  
     *                  The actual number to use is the min of this and the response from the server.
     * @return  this object
     * @since MQTTv5
     */
    public IsmMqttConnection setTopicAlias(int clientTA, int serverTA) {
        verifyNotActive();
        if (clientTA < 0 || clientTA > 255 || serverTA < 0 || serverTA > 255)
            throw new IllegalArgumentException("The topic alias count must be in the range 0 to 255");
        this.clientMaxTopicAlias = clientTA;
        this.serverMaxTopicAlias = serverTA;
        return this;
    }
    
    /**
     * Set the session expire in seconds
     * @param seconds The expire time in seconds 0-214748647
     * @return this connection
     * @since MQTTv5
     */
    public IsmMqttConnection setExpire(long seconds) {
        verifyNotActive();
        if (seconds < 0)
            throw new IllegalArgumentException("SessionExpire not valid: " + seconds);
        if (seconds > 0xffffffffl)
            this.expire = 0xFFFFFFFFL;
        else
            this.expire = seconds;
        return this;
    }
    
    /*
     * Get the session expire in seconds
     * @return The expire time in seconds
     * @since MQTTv5
     */
    public long getExpire() {
        return this.expire;
    }
    
    /**
     * Return the max active QoS>0 messages for client to server.
     * @return The max active QoS>0 messages for client to server
     * @since MQTTv5
     */
    public int getMaxReceive() {
        return this.maxActive;
    }
    
    /*
     * Return the max active QoS>0 messages for server to client.
     * @reteurn the max ative QoS>0 mesages from server to client.
     * This is zero before the connection.
     * @since MQTTv5
     */
    public int getServerMaxReceive() {
        return this.serverMaxActive;
    }
    
    /*
     * Return the ClientID
     */
    public String getClientID() {
        return this.clientID;
    }
    
    /**
     * Allow the ClientID to be set
     * @param clientID  The ClientID
     * @return This connection
     */
    public IsmMqttConnection setClientID(String clientID) {
        verifyNotActive();
        this.clientID = clientID;
        return this;
    }

    /**
     * The time in seconds to wait for response to connect().
     * @param seconds
     * @return This connection object
     */
    public IsmMqttConnection setConnectionTimeout(int seconds) {
        verifyNotActive();
        this.connectionTimeout = seconds;
        return this;
    }

    /**
     * Set the cleanSession/CleanStart bit which indicates whether to use previous subscriptions for this clientID.
     * In MQTTv5 this also sets the session expire to 0.  If you want to set this otherwise you must setClean() first
     * and then setExpire().
     * @param clean  The setting of the cleanSession flag 
     * @return This connection object
     */
    public IsmMqttConnection setClean(boolean clean) {
        verifyNotActive();
        cleanSession = clean;
        if (clean)
            expire = 0;
        return this;
    }


    /**
     * Set the verbose flag. 
     * This is used for problem determination.
     * @param verbose
     * @return This connection object
     */
    public IsmWSConnection setVerbose(boolean verbose) {
        super.verbose = verbose;
        return this;
    }


    /*
     * Make a unique string ID for this client instance
     */
    String uniqueString() {
        CRC32 crc = new CRC32();
        crc.update(clientID.getBytes());
        long now = System.currentTimeMillis();
        crc.update((int)((now/77)&0xff));
        crc.update((int)((now/79731)&0xff));
        long crcval = crc.getValue();
        char [] uniqueCh = new char[8];
        for (int i=0; i<8; i++) {
            uniqueCh[i] = "0123456789BCDFGHJKLMNPQRSTVWXYZbcdfghjklmnpqrstvwxyz".charAt((int)(crcval%52));
            crcval /= 62;
        }
        return new String(uniqueCh);
    }


    /**
     * Make a synchronous connection connection.
     * All modifications to the connection settings must be done before the connection is made.
     * This will do three levels of connection:
     * 1. A TCP connection
     * 2. A WebSockets connection handshake
     * 3. An MQTT handshake
     */
    public void connect() throws IOException {
        connect(true, true);    
    }

    /**
     * Make the connection.
     * All modifications to the connection settings must be done before the connection is made.
     * This will do three levels of connection:
     * 1. A TCP connection
     * 2. A WebSockets connection handshake
     * 3. An MQTT handshake
     * @param sync  Return the result synchron
     */
    public IsmMqttResult connect(boolean sync) throws IOException {
        return connect(sync, !sync);
    }   
    
    public IsmMqttResult reconnect(boolean sync, boolean excp) throws IOException {
        isReconnect = true;
        return connect(sync, excp);
    }
      
    /**
     * Make the connection.
     * All modifications to the connection settings must be done before the connection is made.
     * This will do three levels of connection:
     * 1. A TCP connection
     * 2. A WebSockets connection handshake
     * 3. An MQTT handshake
     */
    public IsmMqttResult connect(boolean sync, boolean excp) throws IOException {   
        int  rc = 0;
        verifyNotActive();
        if (encoding == TCP3 || encoding == TCP4 || encoding == TCP5 || encoding == PROXY4 || encoding == PROXY5)
            setVersion(IsmWSConnection.MQTT_TCP);
        if (encoding == WS3)
        	setVersion(IsmWSConnection.MQTT3WS);
        if (encoding == WS4 || encoding == WS5)
            setVersion(IsmWSConnection.MQTTWS);
        
        if (version == 5) {
            ctx = ctx5;
        } else {
            ctx = ctx0;
        }

        /*
         * Make the TCP and websockets connection
         */
        if (excp) {
            super.connect();
        } else {
            try {
                super.connect();
            } catch (IOException ex) {
                IsmMqttResult bsm = new IsmMqttResult(this, CONNECT<<4, null, 0, sync);
                bsm.setThrowable(ex);
                bsm.setReasonCode(0xff);
                bsm.setReason(ex.getMessage());
                return bsm;
            }
        }
        json = new ImaJson();
      	waitlist = new HashMap<Integer,IsmMqttResult>();
        if (publishlist == null || cleanSession)
        	publishlist = new HashMap<Integer,IsmMqttMessage>();
        if (pubrellist == null || cleanSession)
        	pubrellist = new HashMap<Integer,Object>();
        if (recvlist == null || cleanSession)
        	recvlist = new HashMap<Integer,IsmMqttMessage>();
        inuse = new IsmMqttResult(this, 999, (ByteBuffer)null, -1, true);
        
        keepAliveThread = new IsmMqttKeepAliveThread(this);
        
        /*
         * Start a receive thread
         */
        if (recvq == null || cleanSession) {
        	recvq = new LinkedBlockingQueue<Object>();
        } else {
        	/* 
        	 * If receive queue exists, it might have received,
        	 * but not acknowledged QoS 2 messages (receive, 
        	 * between PUBREL and PUBCOMP). Remove any non-message
        	 * objects from the queue, so that these messages
        	 * would be available on reconnect.
        	 */
        	Iterator<Object> it = recvq.iterator();
        	while (it.hasNext()) {
        		if (!(it.next() instanceof IsmMqttMessage)) {
        			it.remove();
        		}
        	}
        }
        Thread rcvr = new Thread(this);
        rcvr.setDaemon(true);
        rcvr.start();

        /*
         * Start a listener thread
         */
        if (listen != null || listenAsync != null) {
            Thread lstn = new Thread(new IsmMqttListenerThread(this));
            lstn.setDaemon(true);
            lstn.start();
            
        }
        
        /*
         * Send the CONNECT
         */
        
        /* Binary MQTT */
        bb = ByteBuffer.allocate(4*1024);
        if (version == 3) {
            bb = putString(bb, "MQIsdp");    /* Protocol string */
        } else {
            if (encoding == PROXY4 || encoding == PROXY5)
                bb = putString(bb, "MQTTpx");
            else
                bb = putString(bb, "MQTT");      /* Protocol string */
        }    
        bb.put((byte)version);             /* Version 5 */
        int flags = 0;
        if (cleanSession)
            flags |= 2;
        if (willMessage != null && willMessage.getTopic() != null) {
            flags |= 4;
            if (willMessage.getRetain())
                flags |= 0x20;
            flags |= willMessage.getQoS()<<3;
        }    
        if (password != null)
            flags |= 0x40;
        if (username != null)
            flags |= 0x80;

        bb.put((byte)flags);             /* Flag bytes */
        if (encoding == PROXY4 || encoding == PROXY5) {
            bb.put((byte)0);   /* No more flags for now */
        }
        bb.putShort((short)keepalive);
        
        /* Add properties for version >= 5 */
        if (version >= 5) {
            int metapos = bb.position();
            int lenlen = 0;
            int xlen;
            do {
                lenlen++;
                bb.position(metapos + lenlen);
                if (expire > 0)
                    bb = ctx5.put(bb, MPI_SessionExpire, (int)expire);
                if (maxActive > 0)
                    bb = ctx5.put(bb, MPI_MaxReceive, maxActive);
                if (maxPacketSize > 0)
                    bb = ctx5.put(bb, MPI_MaxPacketSize, maxPacketSize);
                if (clientMaxTopicAlias > 0)
                    bb = ctx5.put(bb,  MPI_MaxTopicAlias, clientMaxTopicAlias);
                if (requestReplyInfo)
                    bb = ctx5.put(bb, MPI_RequestReplyInfo, 1);
                if (requestReason >= 0)
                    bb = ctx5.put(bb, MPI_RequestReason, requestReason);
                if (connectProps != null) {
                    Iterator<IsmMqttUserProperty> it = connectProps.iterator();
                    while (it.hasNext()) {
                        IsmMqttUserProperty prp = it.next();
                        bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                    }
                    
                }
                xlen = bb.position()-metapos-lenlen;
            } while (xlen > lenmax[lenlen]);
            bb = ctx5.putVarInt(bb, metapos, xlen);    
        }
        
        bb = putString(bb, clientID);
        if ((flags & 4) != 0 && willMessage != null) {
            /* Set will properties at this point */
            if (version >= 5) {
                int metapos = bb.position();
                int lenlen = 0;
                int xlen;
                do {
                    lenlen++;
                    bb.position(metapos + lenlen);
                    if (willDelay >= 0)
                        bb = ctx5.put(bb, MPI_WillDelay, (int)willDelay);
                    if (willMessage.expire >= 0)
                        bb = ctx5.put(bb, MPI_MsgExpire, (int)willMessage.expire);
                    if (willMessage.msgtype == IsmMqttMessage.TextMessage) {
                        bb = ctx5.put(bb, MPI_PayloadFormat, 1);
                    }    
                    if (willMessage.contentType != null)
                        bb = ctx5.put(bb, MPI_ContentType, willMessage.contentType);
                    if (willMessage.corrid != null) 
                        bb = ctx5.put(bb, MPI_Correlation, willMessage.corrid);
                    if (willMessage.replyto != null) 
                        bb = ctx5.put(bb, MPI_ReplyTopic, willMessage.replyto);
                    if (willMessage.msgprops != null) {
                        Iterator<IsmMqttUserProperty> it = willMessage.msgprops.iterator();
                        while (it.hasNext()) {
                            IsmMqttUserProperty prp = it.next();
                            bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                        }
                    }
                    xlen = bb.position()-metapos-lenlen;
                } while (xlen > lenmax[lenlen]);
                bb = ctx5.putVarInt(bb, metapos, xlen);        
            }
            /* Set will topic */
            bb = putString(bb, willMessage.getTopic());
            byte [] willPayload = willMessage.getPayloadBytes();
            /* Set will paylaod */
            ensure(bb, willPayload.length + 16);
            bb.putShort((short)(willPayload.length));
            bb.put(willPayload);
        }    
        if (username != null)
            bb = putString(bb, username);
        if (password != null)
            bb = putString(bb, password);

        IsmMqttResult bsm = new IsmMqttResult(this, CONNECT<<4, bb, 0, sync);
        setwait(0, bsm);
        if (sync) {
            synchronized (bsm) {
         	    mqttSend(bb.array(), 0, bb.position(), CONNECT<<4);
                await(bsm, connectionTimeout);
            }    
            Throwable e = bsm.getThrowable();
            if (e != null) {
                if (excp) {
                    if (e instanceof RuntimeException)
                        throw (RuntimeException)e;
                    else
                        throw new RuntimeException(e.getMessage(), e);
                } else {
                    bsm.ex = e;
                }
                super.terminate();
                return bsm;
            }
            if (!bsm.isComplete()) {
                System.out.println("timeout bsm" + bsm + " " + bsm.complete + " " + bsm.isComplete());
        	/* Connection timeout */
                super.disconnect(rc, "Connect timed out waiting for a response from the server");
                IOException iex = new IOException("Connection time out: " + clientID);
                if (excp) {
                    throw iex;
                } else {
                    bsm.ex = iex;
                }
                super.terminate();
                return bsm;
            } else {    
                int crc = bsm.getReasonCode();
            	    /* Connection error */
                if ((crc&0xff) != 0) {
                    String msg = "MQTT connect failed: Client=" + clientID + " RC="+crc;
                    String reason = bsm.getReason();
                    if (reason != null)
                        msg = msg + " Reason=" + reason;
                    if (excp)
                        throw new IOException(msg);
                    else
                        bsm.ex = new IOException(msg);
                    super.terminate();
                    return bsm;
                }
            }    
        }
        
        /*
         * Start keep alive thread
         */
        new Thread(keepAliveThread).start();
        
        if (!cleanSession) {
            // resend();
        }
        return bsm;
    }
    
    /*
     * TODO:  Reimplement this
     * This support for saved state at reconnect it totally broken.
     */
    void resend() throws IOException {
        /* Potentially this is a reconnect,
         * re-send unacknowledged messages (SUBSCRIBE, UNSUBSCRIBE, PUBLISH, PUBREL)
         * 
         * 1. PUBREL (pubrellist)
         * 2. PUBLISH (messages on publishlist, but not on pubrellist)
         */
        
        /* Retry PUBRELs with DUP set to true */
        Set<Integer> pubrelIds = pubrellist.keySet();
        for (Integer msgId : pubrelIds) {
            Object obj = pubrellist.get(msgId);
            if (obj instanceof Integer) {
                int cmd = ((Integer) obj).intValue();
                cmd |= PUBLISH_DUP;
                
                ByteBuffer b = ByteBuffer.allocate(2); 
                b.putShort(msgId.shortValue());
                mqttSend(b.array(), 0, b.position(), cmd);
            }
        }

        /* Retry PUBRELs with DUP set to true */
        for (Integer msgId : publishlist.keySet()) {
            if (pubrelIds.contains(msgId)) {
                continue;
            }
            
            IsmMqttMessage msg = publishlist.get(msgId);
            msg.setDup(true);
            publish(msg);
        }
    }


    /*
     * Set to wait for a byte array comamnd
     */
    void setwait(int id, IsmMqttResult obj) {
        synchronized(msgidLock) {
            waitlist.put(id, obj);
        }
    }    
    

    /**
     * Wait for completion
     * @param obj  A result object
     */
    public void await(IsmMqttResult obj) {
        synchronized(obj) {
            while (!obj.isComplete()) {
                try {
                    obj.wait(100);
                } catch(InterruptedException ix) {
                    Thread.currentThread().interrupt();
                }
            } 
        }
    }
    


    /**
     * Wait for an ACK with a binary message with timeout
     * @param obj      A wrapped binary MQTT message
     * @param timeout  Timeout, in seconds
     */
    public void await(IsmMqttResult obj, long timeout) {
    	long currentTime = System.currentTimeMillis();
    	long wakeupTime = currentTime + timeout * 1000;
    	

       synchronized(obj) {
        	while (currentTime < wakeupTime && !obj.isComplete()) {
        		try {
        		    obj.wait(100);
        		} catch(InterruptedException ix) { 
        		    Thread.currentThread().interrupt(); 
        		}
        		currentTime = System.currentTimeMillis();
        	}
        }
    }   
    
    /**
     * Get and remove the waiter for a packetID
     * @param pkid
     * @return the waiting result object or null if there is none
     */
    IsmMqttResult getWait(int pkid) {
        IsmMqttResult bsm;
        synchronized(msgidLock) {
            bsm = waitlist.remove(pkid);
        }
        return bsm;
    }    
    
    /*
     * Respond to a message ID with a return code
     */
    void reply(IsmMqttResult bsm) {
        // System.out.println("reply " + bsm);
        synchronized(bsm) {
            bsm.complete = true;
            bsm.notify();
        }
    }


    /**
     * Send acknowledgment for PUBLISH (QoS 1) and PUBREL (QoS 2). 
     * @param msg A message to acknowledge.
     * TODO: fix QoS=2
     */
    void acknowledge(IsmMqttMessage msg, int rc, String reason) {
        /* Don't ack, if not connected */
        if (!connected) {
            return;
        }
        
        ByteBuffer b = ByteBuffer.allocate(2); 
        StringBuffer cmd = new StringBuffer();
        int qos = msg.getQoS();
        int msgid = msg.getMessageID();

        try {
            switch (qos) {
            case 1:        /* For QoS 1, send PUBACK */
                b.clear(); 
                b.putShort((short)msgid);
                mqttSend(b.array(), 0, b.position(), (IsmMqttConnection.PUBACK<<4));
                break;
            case 2:        /* For QoS 2, send PUBCOMP */
                b.clear(); 
                b.putShort((short)msgid);
                mqttSend(b.array(), 0, b.position(), (IsmMqttConnection.PUBCOMP<<4));
                break;
            default:
                break;          
            }
        } catch (IOException iox) {
            IsmMqttResult bsm = null;
            bsm = getWait(msgid);
            if (bsm != null) {
                bsm.setThrowable(iox);
                reply(bsm);
            }                
        }
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
    	IsmMqttResult bsm = null;
    	boolean alldone = false;
        while (!alldone) {
            try {
                int  id;
                
                IsmWSMessage wmsg = null;
                
                try {
	                wmsg = super.receive();
                } catch (IOException se) {
                    // se.printStackTrace();
                	if (timedOut) {
                		putq(new IOException("MQTT connection timed out", se));
                		break;
                	} else {
                		throw se;
                	}
                }

                if (wmsg != null) {
                	keepAliveThread.inboundMessage();
                	wmsg.reason = null;
                }
                
                
                 /*
                  * MQTT binary
                  */
                 {
                     int mtype = wmsg.getType();
                     int msgid = 0;
                     int pos;
                     ByteBuffer b;
                     ByteBuffer ext = null;
                     int  extlen = 0;
                     int  ackrc  = 0;
                     bsm = null;
                     byte [] payload = wmsg.getPayloadBytes();

                     // System.out.print(Integer.toHexString(mtype&0xff)+' ');
                     //  for (int i=0; i<payload.length; i++) {
                     //     System.out.print(Integer.toHexString(payload[i]&0xff)+ ' ');
                     //  }
                     //  System.out.println();

                     switch ((mtype&0xff)>>4) {
                     case CONNACK:
                         bsm = getWait(0);
                         if (bsm == null)
                             throw new RuntimeException("Wait object not found: 0");    
                         bsm.setReasonCode(payload[1]&0xff);
                         this.sessionPresent = (payload[0]&0x01) != 0;
                         if (version >= 5 && payload.length > 2) {
                             pos = 2;
                             extlen = payload[pos]&0xff;
                             if (extlen != 0) {
                                 ext = ByteBuffer.wrap(payload);
                                 ext.position(pos);
                                 extlen = ctx.getVarInt(ext);
                                 pos = ext.position();
                                 if (extlen > 0) {
                                     ext.limit(pos+extlen);                                     
                                     ctx.check(ext, CPOI_CONNACK, this);
                                 }    
                                 pos += extlen;
                             } else {
                                 pos++;
                             }
                         }
                         bsm.reason = this.reason;
                         reply(bsm);
                         if (payload[1] != 0)
                             alldone = true;
                         break;

                     case PUBLISH:
                         int qos = (mtype>>1)&3;
                         int topiclen = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         int dup = (mtype>>3)&1;
                         String topic = fromUTF8(payload, 2, topiclen, 0);   
                         pos = topiclen + 2;
                         if ((mtype&6) != 0) {
                             msgid = ((payload[pos]&0xff)<<8) | (payload[pos+1]&0xff);
                             pos += 2;
                         }
                         if (version >= 5) {
                             extlen = payload[pos]&0xff;
                             if (extlen != 0) {
                                 ext = ByteBuffer.wrap(payload);
                                 ext.position(pos);
                                 extlen = ctx.getVarInt(ext);
                                 pos = ext.position() + extlen;
                             } else {
                                 pos++;
                             }
                         }
                         byte [] body = new byte[payload.length-pos];
                         System.arraycopy(payload, pos, body, 0, body.length);
                         IsmMqttMessage mmsg = new IsmMqttMessage(topic, body, qos, (mtype&1)!=0);
                         mmsg.maxTopicAlias = clientMaxTopicAlias;
                         if (msgid != 0)
                             mmsg.setMessageID(msgid);
                         mmsg.setDup(dup == 1);
                         
                         if (extlen > 0 && ext != null) {
                             ext.limit(ext.position()+extlen);
                             ctx.check(ext, CPOI_PUBLISH, mmsg);
                         }   
                         
                         /* Implement topic alias */
                         if (mmsg.topicAlias > 0) {
                             if (clientTopicAlias == null) 
                                 clientTopicAlias = new String[clientMaxTopicAlias];
                             if (mmsg.topicAlias <= clientMaxTopicAlias) {
                                 if (mmsg.topic.length() == 0) {
                                     mmsg.topic = clientTopicAlias[mmsg.topicAlias-1];    
                                 } else {
                                     clientTopicAlias[mmsg.topicAlias-1] = mmsg.topic;
                                 }    
                             }
                         }
                         
                         int cmd;
                         switch (qos) {
                         case 1:
                             /*
                              * If qos == 1, 
                              * make it available and send PUBACK back (with message id only)
                              */
                             putq(mmsg);
                             break;
                             
                         case 2:
                             /*
                              * If qos == 2, 
                              * save it and send PUBREC back
                              */
                             putq(mmsg);
                             synchronized (recvlist) {
                                 recvlist.put(msgid, mmsg);
                             }
                             b = ByteBuffer.allocate(2); 
                             b.putShort((short)msgid);
                             cmd = (PUBREC<<4);
                             try {
                                 mqttSend(b.array(), 0, b.position(), cmd);
                             } catch (IOException iox) {
                             }
                             break;
                             
                         default:
                             putq(mmsg);
                         }
                         break;

                     case PUBACK:
                         /* qos == 1, discard published message */
                         msgid = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         bsm = getWait(msgid);
                         if (bsm == null)
                             break;
                         ackrc = 0;
                         if (version >=5 && payload.length > 2) {
                             ackrc = payload[2]&0xff;
                             if (payload.length>3) {
                                 extlen = payload[3]&0xff;
                                 if (extlen != 0) {
                                     ext = ByteBuffer.wrap(payload);
                                     ext.position(3);
                                     extlen = ctx.getVarInt(ext);
                                     if (extlen > 0)
                                         ctx.check(bb, CPOI_PUBACK, bsm);
                                 }    
                             }
                         }
                         bsm.setReasonCode(ackrc);
                         reply(bsm);
                         break;

                     case PUBREC:
                         /* 
                          * qos == 2, response to PUBLISH
                          * Response with PUBREL (qos=1, dup=false)
                          */
                         msgid = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         bsm = getWait(msgid);
                         if (bsm == null)
                             break;
                         ackrc = 0;
                         if (version >=5 && payload.length > 2) {
                             ackrc = payload[2]&0xff;
                             if (payload.length>3) {
                                 extlen = payload[3]&0xff;
                                 if (extlen != 0) {
                                     ext = ByteBuffer.wrap(payload);
                                     ext.position(3);
                                     extlen = ctx.getVarInt(ext);
                                     if (extlen > 0)
                                         ctx.check(bb, CPOI_PUBACK, bsm);
                                 }    
                             }
                         }
                         b = ByteBuffer.allocate(2); 
                         b.putShort((short)msgid);
                         cmd = (PUBREL<<4)+2;
                         pubrellist.put(msgid, cmd);
                         setwait(msgid, bsm);
                         try {
                             mqttSend(b.array(), 0, b.position(), cmd);
                         } catch (IOException iox) {
                             bsm.setThrowable(iox);
                             reply(bsm);
                         }  
                         break;
                         
                     case PUBREL:
                         /* 
                          * Last message from the server for PUBLISH with QoS == 2.
                          * Make the message available and send PUBCOMP.
                          * PUBREL has to have QoS set to 1
                          */
                         IsmMqttMessage msg = null;
                         msgid = 0;
                         if ((mtype&2) != 0) {
                             msgid = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                             if (version >=5 && payload.length > 2) {
                                 ackrc = payload[2]&0xff;
                                 if (payload.length>3) {
                                     extlen = payload[3]&0xff;
                                     if (extlen != 0) {
                                         ext = ByteBuffer.wrap(payload);
                                         ext.position(3);
                                         extlen = ctx.getVarInt(ext);
                                         if (extlen > 0)
                                             ctx.check(ext, CPOI_PUBCOMP, null);
                                     }    
                                 }
                             }
                             if (ackrc < 128) {
                                 b = ByteBuffer.allocate(2); 
                                 b.putShort((short)msgid);
                                 cmd = (PUBCOMP<<4);
                                 try {
                                     mqttSend(b.array(), 0, b.position(), cmd);
                                 } catch (IOException iox) {
                                     bsm.setThrowable(iox);
                                     reply(bsm);
                                 }    
                             }
                             synchronized (recvlist) {
                                 msg = recvlist.remove(msgid);
                             }
                             //if (msg != null) {
                             //    putq(msg);
                             //}
                         }

                         break;
                         
                     case PUBCOMP:
                         /* 
                          * qos == 2, response to PUBLISH+PUBREL
                          * discard published message 
                          */                             
                         msgid = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         bsm = getWait(msgid);
                         if (bsm == null)
                             break;
                         if (version >=5 && payload.length > 2) {
                             ackrc = payload[2]&0xff;
                             if (payload.length>3) {
                                 extlen = payload[3]&0xff;
                                 if (extlen != 0) {
                                     ext = ByteBuffer.wrap(payload);
                                     ext.position(3);
                                     extlen = ctx.getVarInt(ext);
                                     if (extlen > 0)
                                         ctx.check(ext, CPOI_PUBCOMP, bsm);
                                 }    
                             }
                         }
                    	 pubrellist.remove(msgid);
                         reply(bsm);
                         break;
                         
                     case SUBACK:
                         id = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         bsm = getWait(id);
                         if (bsm == null)
                             throw new RuntimeException("Wait object not found: " + id);
                         pos = 2;
                         if (version >=5 && payload.length > 2) {
                             extlen = payload[2]&0xff;
                             if (extlen != 0) {
                                 ext = ByteBuffer.wrap(payload);
                                 ext.position(2);
                                 extlen = ctx.getVarInt(ext);
                                 pos = ext.position() + extlen;
                                 if (extlen > 0)
                                     ctx.check(ext, CPOI_SUBACK, bsm);
                             } else {
                                 pos++;
                             }
                         }
                         int cnt = payload.length-pos;
                         if (cnt > 0) {
                             byte [] rclist = new byte[cnt];
                             System.arraycopy(payload, pos, rclist, 0, cnt);
                             bsm.rcList = rclist;
                             bsm.setReasonCode(payload[pos]&0xff);
                         }
                         reply(bsm);
                         break;

                     case UNSUBACK:
                         id = ((payload[0]&0xff)<<8) | (payload[1]&0xff);
                         bsm = getWait(id);
                         if (bsm == null)
                             throw new RuntimeException("Wait object not found: " + id);
                         //System.out.println("unsuback id=" + id);
                         pos = 2;
                         if (version >=5 && payload.length > 2) {
                             extlen = payload[2]&0xff;
                             if (extlen != 0) {
                                 ext = ByteBuffer.wrap(payload);
                                 ext.position(3);
                                 extlen = ctx.getVarInt(ext);
                                 pos = ext.position() + extlen;
                                 if (extlen > 0)
                                     ctx.check(ext, CPOI_UNSUBACK, bsm);
                             } else {
                                 pos++;
                             }
                             cnt = payload.length-pos;
                             if (cnt > 0) {
                                 byte [] rclist = new byte[cnt];
                                 System.arraycopy(payload, pos, rclist, 0, cnt);
                                 bsm.rcList = rclist;
                                 bsm.setReasonCode(payload[pos]&0xff);
                             } 
                         }
                         reply(bsm);
                         break;

                     case DISCONNECT:
                         System.out.println("got disconnect");
                         bsm = new IsmMqttResult(this, IsmMqttConnection.DISCONNECT<<4, (ByteBuffer)null, 0, true);
                         if (version >=5 && payload.length > 0) {
                             bsm.setReasonCode(payload[0]&0xff);
                             if (payload.length>1) {
                                 extlen = payload[1]&0xff;
                                 if (extlen != 0) {
                                     ext = ByteBuffer.wrap(payload);
                                     ext.position(1);
                                     extlen = ctx.getVarInt(ext);
                                     if (extlen > 0)
                                         ctx.check(ext, CPOI_DISCONNECT, bsm);
                                 }    
                             }
                         }
                         connected = false;
                         putq(bsm);
                         try {
                             Thread.sleep(25);
                             super.disconnect();
                         } catch (Exception e) {};
                         // System.err.println("**Disconnect rc="+bsm.getReasonCode()+" reason="+bsm.getReason());
                         putq(bsm);
                         alldone = true;
                         break;

                     case PINGRESP:
                    	 keepAliveThread.receivedPingResponse();
                    	 break;
                         
                     default:
                         throw new IOException("MQTT command not valid "+Integer.toHexString(mtype&0xff));
                     }
                 }
            } catch (Exception iox) {
                if (bsm != null)
                    bsm.setThrowable(iox);
                if (connected) 
                    putq(iox);
                else
                    putq(new IsmMqttResult(this, (DISCONNECT<<4), null, 0, true));
                alldone = true;
                break;
            }
        }
        synchronized (this) {
            stopped = true;
        }
    }


    /*
     * Create a map object from a JSON object
     */
    Map<String,Object> toMap(ImaJson json, int entnum) {
        if (entnum <0 || entnum >= json.ent_count || json.ent[entnum].objtype != ImaJson.JObject.Object) 
            return null;
        HashMap<String,Object> map = new HashMap<String,Object>();
        for(int count = json.ent[entnum++].count; count > 0; count--, entnum++) {
            if (json.ent[entnum].objtype == ImaJson.JObject.Array ||
                    json.ent[entnum].objtype == ImaJson.JObject.Object ) {
                count -= json.ent[entnum].count;
                entnum += json.ent[entnum].count;
            } else {
                switch(json.ent[entnum].objtype) {
                case String:   map.put(json.ent[entnum].name, json.ent[entnum].value);   break;
                case Integer:  map.put(json.ent[entnum].name, json.ent[entnum].count);   break;
                case Number:   map.put(json.ent[entnum].name, Double.valueOf(json.ent[entnum].value));   break;
                case True:     map.put(json.ent[entnum].name, true);                     break;
                case False:    map.put(json.ent[entnum].name, false);                    break;
                case Null:     map.put(json.ent[entnum].name, (Object)null);             break;
                }
            }
        }
        return map;
    }

    /*
     * Create a List object from a JSON array
     */
    List<Object> toList(ImaJson json, int entnum) {
        if (entnum <0 || entnum >= json.ent_count || json.ent[entnum].objtype != ImaJson.JObject.Array) 
            return null;
        Vector<Object> list = new Vector<Object>();
        int count = json.ent[entnum++].count; 
        while (count-- > 0) {
            if (json.ent[entnum].objtype == ImaJson.JObject.Array ||
                    json.ent[entnum].objtype == ImaJson.JObject.Object ) {
                count -= json.ent[entnum].count;    
            } else {
                switch(json.ent[entnum].objtype) {
                case String:   list.add(json.ent[entnum].value);   break;
                case Integer:  list.add(json.ent[entnum].count);   break;
                case Number:   list.add(Double.valueOf(json.ent[entnum].value));   break;
                case True:     list.add(true);                     break;
                case False:    list.add(false);                    break;
                case Null:     list.add((Object)null);             break;
                }
            }
        }
        return list;
    }

    /**
     * Subscribe to a topic synchrnously.
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param topic   The topic to subscribe to
     * @param subopt  The requested subscription options  
     * @return the actual quality of service, which might be below what was requested. 
     * @throws IOException
     */
    public int subscribe(String topic, int subopt) throws IOException {
        String [] topics = new String [1];
        topics[0] = topic;
        int [] subopts = new int [1];
        subopts[0] = subopt;
        IsmMqttResult bsm = subscribe(topics, subopts, 0, true, null);
        return bsm.getReasonCode();
    }    

    /**
     * Subscribe to a topic.
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param topic   The topic to subscribe to
     * @param subopt  The requested subscription options
     * @param subid   The subscription ID
     * @param sync    True if the method should wait for the result  
     * @return A result object 
     * @throws IOException
     */
    public IsmMqttResult subscribe(String topic, int subopt, int subid, boolean sync) throws IOException {
        String [] topics = new String [1];
        topics[0] = topic;
        int [] subopts = new int [1];
        subopts[0] = subopt;
        return subscribe(topics, subopts, subid, sync, null);
    } 


    
    /**
     * Subscribe with selector.
     * This can only be done in a proxy connection.
     * @param topic  The topic name
     * @param select The selector as a SQL92 string
     * @param flags  Subscription flags
     * @return The return code
     */
    public int subscribeSelector(String topic, String select, int subopt) throws IOException {
        if (encoding != PROXY4 && encoding != PROXY5) 
            throw new RuntimeException("subscribeACL only supported in proxy protocol");
        verifyActive();
        int id = getMessageID();

        bb.clear();
        bb.putShort((short)id);
        if (version >= 5)
            bb.put((byte)0);    /* SubID */
        bb = putString(bb, topic);
        
        bb.putShort((short)(select.length()+6));
        bb.put((byte)0x55);
        bb = putString(bb, select);
        bb.put((byte)0x82);
        bb.putShort((short)subopt);
        IsmMqttResult bsm = new IsmMqttResult(this, (SUBSCRIBE<<4)+15, bb, id, true);
        setwait(id, bsm);
        int rc;
        synchronized (bsm) {
            mqttSend(bb.array(), 0, bb.position(), bsm.getCommand());
            await(bsm);
        }
        return bsm.getReasonCode();
    }
    
    /**
     * Subscribe to multiple topics.
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param topics  Array of topics to subscribe to
     * @param usbopt  Array of  requested qualities of service for the subscriptions  
     * @return the actual quality of service, which might be below what was requested. 
     * @throws IOException
     */
    public byte[] subscribe(String[] topics, int[] subopt) throws IOException {
        IsmMqttResult bsm = subscribe(topics, subopt, 0, true);
        byte [] rclist = bsm.getReasonCodes();
        if (rclist == null) {
            rclist = new byte[1];
            rclist[0] = (byte)bsm.getReasonCode();
        }
        return rclist;
    }
    
    /**
     * Subscribe to a set of topics
     * @param topics   An array of topic filters   
     * @param subopts  An array of subscription options (must be same length as topic array)
     * @param subid    The subscription ID
     * @param sync     If true the method waits for completion
     * @return  A result object which is probably not complete if sync=true
     * @throws IOException
     */
    public IsmMqttResult subscribe(String [] topics, int [] subopts, int subid, boolean sync) throws IOException {
        return subscribe(topics, subopts, subid, sync, null);
    }    

    /**
     * Subscribe to a set of topics.
     * If you subscribe to a topic which this connection already subscribes to,
     * the older subscription will be removed.
     * @param topics   The topic to subscribe to
     * @param subopts  The requested subscription options
     * @param subid    The subscription ID
     * @param sync     True if the method should wait for the result  
     * @param props    A list of user properties
     * @return A result object 
     * @throws IOException
     */
    public IsmMqttResult subscribe(String [] topics, int [] subopts, int subid, boolean sync, List<IsmMqttUserProperty> props) throws IOException {
    	if (!(topics.length == subopts.length))
    		throw new IllegalArgumentException("Topic array and QoS array must have the same length");
        verifyActive();
        int id = getMessageID();
        
        bb.clear();
        bb.putShort((short)id);
        
        /* For MQTTv5 put out the properties */
        if (version >= 5) {
            int pos = bb.position();
            int lenlen = 0;
            int xlen;
            do {
                lenlen++;
                bb.position(pos + lenlen);
                if (subid != 0) {
                    bb = ctx5.put(bb, MPI_SubID, subid);
                }
                if (props != null) {
                    Iterator<IsmMqttUserProperty> it = props.iterator();
                    while (it.hasNext()) {
                        IsmMqttUserProperty prp = it.next();
                        bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                    }
                }
                xlen = bb.position()-pos-lenlen;
            } while (xlen > lenmax[lenlen]);
            bb = ctx5.putVarInt(bb, pos, xlen); 
        }    
        for (int i = 0; i < topics.length; i++) {
            bb = putString(bb, topics[i]);
            bb = bb.put((byte)subopts[i]);
        }
        IsmMqttResult bsm = new IsmMqttResult(this, (SUBSCRIBE<<4)+2, bb, id, sync);
        setwait(id, bsm);
        if (sync) {
            synchronized (bsm) {
                mqttSend(bb.array(), 0, bb.position(), bsm.getCommand());
                await(bsm);
            }
        } else {
            mqttSend(bb.array(), 0, bb.position(), bsm.getCommand());
        }
        return bsm;
    }

    
    /**
     * Unsubscribe from a topic synchronously.
     * It is not an error to unsubscribe from a topic to which you are not subscribed.
     * @param topic
     */
    public void unsubscribe(String topic)throws IOException {
        String [] topics = new String[1];
        topics[0] = topic;
        unsubscribe(topics, true);
    }
    
    /**
     * Unsubscribe from a topic.
     * It is not an error to unsubscribe from a topic to which you are not subscribed.
     * @param topic
     * @deprecated use unsubscribe(String [], boolean)
     */
    public void unsubscribeMulti(String[] topic)throws IOException {
        unsubscribe(topic, true);
    }
    
    /**
     * Unsubscribe from a single topic
     * @param topic  The topic to unsubscribe from
     * @param sync   True if the method should wait for completion 
     * @return  A result object.  If sync=false this is most likely not complete
     * @throws IOException
     */
    public IsmMqttResult unsubscribe(String topic, boolean sync) throws IOException {
        String [] topics = new String[1];
        topics[0] = topic;
        return unsubscribe(topics, sync);
    }
    
    /**
     * Unsubscribe from multiple topics
     * @param topic   The topic to unsubscribe from
     * @param sync    True if the method should wait for completion
     * @return  A result object.  If sync=false this is most likely not complete
     * @throws IOException
     */
    public IsmMqttResult unsubscribe(String [] topic, boolean sync) throws IOException {
        return unsubscribe(topic, sync, null);
    }
    
    /**
     * Unsubscribe from multiple topics
     * @param topic   The topic to unsubscribe from
     * @param sync    True if the method should wait for completion
     * @param props   A list of user properties
     * @return  A result object.  If sync=false this is most likely not complete
     * @throws IOException
     */
    public IsmMqttResult unsubscribe(String [] topic, boolean sync, List<IsmMqttUserProperty> props) throws IOException {  
        verifyActive();
        Integer id = getMessageID();
        bb.clear();
        bb.putShort(id.shortValue());
        
        /* For MQTTv5 put out the properties */
        if (version >= 5) {
            int pos = bb.position();
            int lenlen = 0;
            int xlen;
            do {
                lenlen++;
                bb.position(pos + lenlen);
                if (props != null) {
                    Iterator<IsmMqttUserProperty> it = props.iterator();
                    while (it.hasNext()) {
                        IsmMqttUserProperty prp = it.next();
                        bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                    }
                }
                xlen = bb.position()-pos-lenlen;
            } while (xlen > lenmax[lenlen]);
            bb = ctx5.putVarInt(bb, pos, xlen); 
        } 
        
        
        for (int i = 0; i < topic.length; i++) {
            bb = putString(bb, topic[i]);
        }
        IsmMqttResult bsm = new IsmMqttResult(this, (UNSUBSCRIBE<<4)+2, bb, id.intValue(), sync);
        setwait(id, bsm);
        if (sync) {
            synchronized (bsm) {
                mqttSend(bb.array(), 0, bb.position(), bsm.getCommand());
                await(bsm);
            }
        } else {
            mqttSend(bb.array(), 0, bb.position(), bsm.getCommand());
        }
        return bsm;
    }

    /**
     * Publish a string QoS=0 message 
     * @param topic   The topic on which to send the message    
     * @param payload The payload of the message 
     * @return The message ID or 0 for a qos=0 message
     */
    public int publish(String topic, String payload) throws IOException {
        return publish(new IsmMqttMessage(topic, payload));
    }

    /**
     * Publish a string message. 
     * This is a convenience method which publishes a synchronous message  
     * @param topic    The topic on which to send the message
     * @param payload  The payload of the message
     * @param qos      The quality of service of the message
     * @param retain   The retain flag of the message
     * @return The message ID or 0 for a qos=0 message
     */
    public int publish(String topic, String payload, int qos, boolean retain) throws IOException {
        return publish(new IsmMqttMessage(topic, payload, qos, retain));
    }

    static final int [] lenmax = new int [] {0, 127, 16383, 2097151, 0x7ffffff};

    /**
     * Publish a synchronous MQTT message.
     * @param msg  An MQTT message
     * @return The message ID or 0 for a qos=0 message, -1=error
     * #deprecated use IsmMqttConnection.publish(IsmMqttMesssage msg, boolean sync)
     */
    public int publish(IsmMqttMessage msg) throws IOException {
        IsmMqttResult result;
        result = publish(msg, true);
        return result != null ? result.getPacketID() : -1;
    }
    
    /**
     * Publish an MQTT message
     * @param  msg  An MQTT message
     * @param  sync  false=asynchronous, 1=synchronous
     */
    public IsmMqttResult publish(IsmMqttMessage msg, boolean sync) throws IOException {
        int  topicalias = 0;
        verifyActive();
        Integer msgid = null;
        bb.clear();
        
        if (version >= 5 && serverMaxTopicAlias > 0) {
             if (serverTopicAlias == null) {
                 serverTopicAlias = new String[serverMaxTopicAlias];
             }
             for (int i=0; i<serverMaxTopicAlias; i++) {
                 if (serverTopicAlias[i] == null) {
                     serverTopicAlias[i] = msg.topic;
                     topicalias = i+1;
                     break;
                 } 
                 if (serverTopicAlias[i].equals(msg.topic)) {
                     topicalias = i+1;
                     msg.topic = "";
                     break;
                 }
             }
        }
        bb = putString(bb, msg.topic);
        if (msg.qos > 0) {
            msgid = getMessageID();
            bb.putShort(msgid.shortValue());
        }
        
        /* 
         * Write the publish properties
         */
        if (version >= 5) {
            int metapos = bb.position();
            int lenlen = 0;
            int xlen;
            do {
                lenlen++;
                bb.position(metapos + lenlen);
                // Temp for testing only
                // bb = ctx5.put(bb,  MPI_SubID,  1);
                if (topicalias > 0)
                    bb = ctx5.put(bb, MPI_TopicAlias, topicalias);
                if (msg.expire >= 0)
                    bb = ctx5.put(bb, MPI_MsgExpire, (int)msg.expire);
                if (msg.msgtype == IsmMqttMessage.TextMessage) {
                    bb = ctx5.put(bb, MPI_PayloadFormat, 1);
                }    
                if (msg.contentType != null)
                    bb = ctx5.put(bb, MPI_ContentType, msg.contentType);
                if (msg.corrid != null) 
                    bb = ctx5.put(bb, MPI_Correlation, msg.corrid);
                if (msg.replyto != null) 
                    bb = ctx5.put(bb, MPI_ReplyTopic, msg.replyto);
                if (msg.msgprops != null) {
                    Iterator<IsmMqttUserProperty> it = msg.msgprops.iterator();
                    while (it.hasNext()) {
                        IsmMqttUserProperty prp = it.next();
                        bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                    }
                }
                xlen = bb.position()-metapos-lenlen;
            } while (xlen > lenmax[lenlen]);
            bb = ctx5.putVarInt(bb, metapos, xlen);    
        }
        
        /* Put out the payload */
        byte [] paybytes = msg.getPayloadBytes();
        if (paybytes != null) {
            ensure(bb, paybytes.length);
            bb.put(paybytes);
        }    
        int cmd = (PUBLISH<<4)+(msg.qos<<1)+(msg.retain?PUBLISH_RETAIN:0)+(msg.dup?PUBLISH_DUP:0);
        
        IsmMqttResult bsm = new IsmMqttResult(this, cmd, bb, msgid==null ? 0 : msgid.intValue(), sync);
        try {
            if (msg.qos > 0) {
                setwait(msgid, bsm);
                if (sync) {
                    /* QoS>0 sync */
                    synchronized (bsm) {
                        mqttSend(bb.array(), 0, bb.position(), cmd);
                        /* Wait for PUBACK (qos 1) or PUBCOMP (qos 2) */
                        try {bsm.wait();} catch(InterruptedException ix){}
                    }
                } else {
                    /* QoS>0 async */
                    mqttSend(bb.array(), 0, bb.position(), cmd);
                }
            } else {
                /* QoS=0 always returns right away */
                mqttSend(bb.array(), 0, bb.position(), cmd);
            }
        } catch (IOException ioe) {
            bsm.setThrowable(ioe);
        }
        return bsm;
    }

    static int messageID = 1;
    /*
     * Get an available message ID
     * TODO: implement flow control 
     */
    public int getMessageID() throws IOException {
        synchronized (msgidLock) {
            for (int cnt=0; cnt<64000; cnt++) {
                if (++messageID >= 65536)
                    messageID= 1;
                if (!waitlist.containsKey(messageID)) {
                    waitlist.put(messageID, inuse);
                    return messageID;
                }
            }    
        }
        throw new IOException("too many IDs in use");
    }
    
    void wakeWaiters(IsmMqttResult result, Throwable ex) {
        synchronized(msgidLock) {
            Set<Integer> waiters = waitlist.keySet();
            Iterator<Integer> it = waiters.iterator();
            while (it.hasNext()) {
                Integer id = it.next();
                IsmMqttResult bsm = waitlist.get(id);
                if (ex != null) {
                    bsm.ex = ex;
                    bsm.rc = 255;
                } else {
                    bsm.rc = result.rc;
                    bsm.reason = result.reason;
                }    
                bsm.complete = true;
                it.remove();
                synchronized(bsm) {
                    bsm.complete = true;
                    bsm.notify();
                }    
            }
        }    
    }    

    /**
     * Receive a message.
     * This call will block until a message is available or the connection closes.
     * This can be done in 
     * @return The returned message
     */
    public IsmMqttMessage receive() throws IOException {
        verifyActive();
        if (listen != null || listenAsync != null) {
            throw new IllegalStateException("receive() cannot be used with a message listener");
        }
        for (;;) {
            try {
                Object obj = recvq.take();
                if (obj instanceof IsmMqttMessage) {
                    IsmMqttMessage msg = (IsmMqttMessage)obj; 
                    if (msg.getQoS() > 0)
                        acknowledge(msg, 0, null);
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
     */
    public IsmMqttMessage receive(long millis) throws IOException {
        verifyActive();
        if (listen != null || listenAsync != null) {
            throw new IllegalStateException("receive() cannot be used with a message listener");
        }
        for (;;) {
            try {
                Object obj = recvq.poll(millis, TimeUnit.MILLISECONDS);
                if (obj == null)
                    return null;
                if (obj instanceof IsmMqttMessage) {
                    IsmMqttMessage msg = (IsmMqttMessage)obj; 
                    if (msg.getQoS() > 0)
                        acknowledge(msg, 0, null);
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
     * Normal disconnect from the connection.
     * After a disconnect, the connection object can be reused 
     */
    public void disconnect() {
        disconnect(0, null, -1, null);
    }
    
    public void disconnect(int code, String reason) {
        disconnect(code, reason, -1, null);
    }


    /**
     * Send a DISCONNECT with a return code and reason string
     * @param code   The return code which must be a valid DISCONNECT return code
     * @param reason The reason string (which can be null)
     * @param expire The
     */
    public void disconnect(int code, String reason, long expire, List<IsmMqttUserProperty> userprops) {
        if (!connected) {
            keepAliveThread.stop();
            return;
        }    
        IsmMqttResult bsm = new IsmMqttResult(this, IsmMqttConnection.DISCONNECT<<4, (ByteBuffer)null, 0, true);
        bsm.setReason(reason);
        bsm.setReasonCode(code);
        try {
            if (version >= 5) {
                /* Send the rc with optional reason */
                int exlen = reason == null ? 0  : sizeUTF8(reason) + 3;
                if (expire >= 0)
                    exlen += 5;
                bb = ByteBuffer.allocate(exlen+16);
                bb.put((byte)code);
                bb = ctx5.putVarInt(bb, exlen);
                if (expire >= 0)
                    bb = ctx5.put(bb, MPI_SessionExpire, (int)expire);
                if (reason != null)
                    bb = ctx5.put(bb, MPI_Reason, reason);
                if (userprops != null) {
                    Iterator<IsmMqttUserProperty> it = userprops.iterator();
                    while (it.hasNext()) {
                        IsmMqttUserProperty prp = it.next();
                        System.out.println("disconnect prop "+prp.name() + "=" + prp.value());
                        bb = ctx5.put(bb, MPI_UserProperty, prp.name(), prp.value());
                    }
                }    
                mqttSend(bb.array(), 0, bb.position(), DISCONNECT<<4);
            } else {
                mqttSend((byte[])null, 0, 0, DISCONNECT<<4);  
            }
        } catch (IOException ex) {
            bsm.setThrowable(ex);
        }

        try {
            super.disconnect(code, reason);
        } catch (IOException ex) {
            if (bsm.getThrowable() == null)
                bsm.setThrowable(ex);
        }
        putq(bsm);
        
        keepAliveThread.stop();
        
        synchronized (this) {
            while (!stopped) {
                try { 
                    this.wait(50);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    /*
     * Send PINGREQ to the server
     */
    public void sendPing() {
        try {
            mqttSend((byte[])null, 0, 0, PINGREQ<<4);
        } catch (IOException ex) {
        }
    }
    
    /*
     * Terminate the connection on timeout.
     */
    void terminateOnTimeout() {
    	timedOut = true;
        keepAliveThread.stop();
		super.terminate();
		
		for (Integer msgId : waitlist.keySet()) {
			Object obj = waitlist.get(msgId);
			synchronized (obj) {
				obj.notify();
			}
		}
		
		for (Integer msgId : publishlist.keySet()) {
			Object obj = publishlist.get(msgId);
			synchronized (obj) {
				obj.notify();
			}
		}
    }
    

    /*
     * Send an MQTT packet.
     * The framing is done at the4 parent level
     */
	private void mqttSend(byte[] payload, int start, int len, int mtype) throws IOException {
    	try {
    		super.send(payload, start, len, mtype);
    	} catch (IOException iox) {
    		if (timedOut) {
    			throw new IOException("MQTT connection timed out", iox);
    		} else {
    			throw iox;
    		}
    	}
        keepAliveThread.outboundMessage();
    }

    
    /*
     * Put a string into a byte buffer in UTF8.
     * This also ensures that there are 32 bytes extra apace left in the byte buffer
     */
    public static ByteBuffer putString(ByteBuffer bb, String s) {
        int len = sizeUTF8(s);
        if (bb.capacity()-bb.position() < (len+2)) {
            int newlen = bb.capacity() + len + 200;
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
     * Ensure we have enough space
     */
    public static ByteBuffer ensure(ByteBuffer bb, int len) {
        if (bb.capacity()-bb.position() < len) {
            int newlen = bb.capacity() + len + 250;
            ByteBuffer newbb = ByteBuffer.allocate(newlen);
            bb.flip();
            newbb.put(bb);
            bb = newbb;
        }    
        return bb;    
    }

    /*
     * Temporary buffer per thread
     */
    static final ThreadLocal<char[]> tempChar = new ThreadLocal<char[]>() {
        protected char[] initialValue() {
            return new char[120];
        }
    };

    static final int CHECK_NULL    = 1;
    static final int CHECK_C1      = 3;
    static final int CHECK_C2      = 7;
    static final int CHECK_NONCHAR = 8;
    static final int CHECK_CC_NON  = 15;
    
    /*
     * Check if a UTF-16 string is valid
     * 
     * @param str   The String to check
     * @param check The bitmask of 
     * @throws IOException if the String is not valid according to the selected
     * 
     */
    static void validUTF16(String str, int check) throws IOException {
        int strlen = str.length();
        int c;
        int c2;
        int i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c < 0x30) {
                if (c == 0 && ((check & CHECK_NULL) != 0))
                    throw new IOException("null character");
                if ((check & CHECK_C1) != 0)
                    throw new IOException("control character");
            }
            if (c <= 0x007F) {
                if (c >= 0x80 && c <= 0x9f)
                    throw new IOException("control character");
            } else if (c > 0x07FF) {
                if (c >= 0xd800 && c <= 0xdbff) {
                    c2 = str.charAt(++i);
                    if (c2 < 0xdc00 || c2 > 0xdfff)
                        throw new IOException("bad surrogate");
                    c = ((c & 0x3ff) << 10) + (str.charAt(++i) & 0x3ff) + 0x10000;
                } else {
                    if (((check & CHECK_NONCHAR) != 0) && ((c & 0xffff) == 0xfffe || (c & 0xffff) == 0xffff))
                        throw new IOException("non-character");
                }
            } else {
                if (c >= 0xfdd0 && ((check & CHECK_NONCHAR) != 0)) {
                    if (c < 0xfffe) {
                        if (c >= 0xfdd0 && c <= 0xfdef)
                            throw new IOException("non-character");
                    } else {
                        if (c == 0xfffe || c == 0xffff)
                            throw new IOException("non-character");
                    }
                }    
            }
        }    
    }
    
    /*
     * Make a valid UTF-8 byte array from a string. Before invoking this method
     * you must call sizeUTF8 using the same string to find the length of the
     * UTF-8 buffer and to check the validity of the UTF-8 string.
     */
    static void makeUTF8(String str, int utflen, ByteBuffer ba) {
        int strlen = str.length();
        int c;
        int i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007F) {
                ba.put((byte) c);
            } else if (c > 0x07FF) {
                if (c >= 0xd800 && c <= 0xdbff) {
                    c = ((c & 0x3ff) << 10) + (str.charAt(++i) & 0x3ff) + 0x10000;
                    ba.put((byte) (0xF0 | ((c >> 18) & 0x07)));
                    ba.put((byte) (0x80 | ((c >> 12) & 0x3F)));
                    ba.put((byte) (0x80 | ((c >> 6) & 0x3F)));
                    ba.put((byte) (0x80 | (c & 0x3F)));
                } else {
                    ba.put((byte) (0xE0 | ((c >> 12) & 0x0F)));
                    ba.put((byte) (0x80 | ((c >> 6) & 0x3F)));
                    ba.put((byte) (0x80 | (c & 0x3F)));
                }
            } else {
                ba.put((byte) (0xC0 | ((c >> 6) & 0x1F)));
                ba.put((byte) (0x80 | (c & 0x3F)));
            }
        }
    }

    /* Starter states for UTF8 */
    private static final int States[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
            2, 2, 3, 3, 4, 1, };

    /* Initial byte masks for UTF8 */
    private static final int StateMask[] = { 0, 0, 0x1F, 0x0F, 0x07 };

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
                    ret = false;
                break;
            case 3:
                if (byte1 == 0 && byte2 < 0xa0)
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
     */
    static String fromUTF8(byte[] ba, int pos, int len, int check) throws IOException {
        char[] tbuf = tempChar.get();
        if (tbuf.length < len * 2) {
            tbuf = new char[len * 2];
            tempChar.set(tbuf);
        }
        int byte1 = 0;
        int byteoff = pos;
        int byteend = pos + len;
        int charoff = 0;
        int state = 0;
        int value = 0;
        int inputsize = 0;

        while (byteoff < byteend) {
            if (state == 0) {
                /* Fast loop in single byte mode */
                for (;;) {
                    byte1 = ba[byteoff];
                    if (byte1 < 0)
                        break;
                    if (byte1 < 0x20) {
                        if (((check & CHECK_C1)) != 0) 
                            throw new IOException("control character");
                        if (byte1 == 0 && ((check & CHECK_NULL) != 0))
                            throw new IOException("null character");
                    }
                    tbuf[charoff++] = (char) byte1;
                    if (++byteoff >= byteend)
                        return new String(tbuf, 0, charoff);
                }

                byteoff++;
                state = States[(byte1 & 0xff) >> 3];
                value = byte1 = byte1 & StateMask[state];
                inputsize = 1;
                if (state == 1)
                    throw new IOException("The initial UTF-8 byte is not valid.");
            } else {
                int byte2 = ba[byteoff++] & 0xff;
                if ((inputsize == 1 && !validSecond(true, state, byte1, byte2))
                        || (inputsize > 1 && byte2 < 0x80 || byte2 > 0xbf))
                    throw new IOException("The continuing UTF-8 byte is not valid.");
                value = (value << 6) | (byte2 & 0x3f);
                if (inputsize + 1 >= state) {
                    if (value < 0x10000) {
                        if ((check & CHECK_C2) != 0  && value < 0x9f) 
                            throw new IOException("control character");
                        if (value >= 0xfdd0 && ((check & CHECK_NONCHAR) != 0)) {
                            if ((value  >= 0xfdd0 && value <= 0xfdef) || value >= 0xfffe)
                                throw new IOException("non-character");
                        }
                        tbuf[charoff++] = (char) value;
                    } else {
                        if (((check & CHECK_NONCHAR) != 0) && ((value & 0xffff) == 0xfffe || (value & 0xffff) == 0xffff)) 
                            throw new IOException("non-character");
                        tbuf[charoff++] = (char) (((value - 0x10000) >> 10) | 0xd800);
                        tbuf[charoff++] = (char) (((value - 0x10000) & 0x3ff) | 0xdc00);
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
     * 
     * @param str The string to find the UTF8 size of
     * 
     * @returns The length of the string in UTF-8 bytes, or -1 to indicate an
     * input error
     */
    static int sizeUTF8(String str) {
        int strlen = str.length();
        int utflen = 0;
        int c;
        int i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007f) {
                utflen++;
            } else if (c <= 0x07ff) {
                utflen += 2;
            } else if (c >= 0xdc00 && c <= 0xdfff) { /*
                                                      * High surrogate is an
                                                      * error
                                                      */
                return -1;
            } else if (c >= 0xd800 && c <= 0xdbff) { /* Low surrogate */
                if ((i + 1) >= strlen) { /* At end of string */
                    return -1;
                } else { /* Check for valid surrogate */
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
    
    /*
     * MQTT property support
     */
    static final int MPI_PayloadFormat     = 0x01;
    static final int MPI_MsgExpire         = 0x02;    
    static final int MPI_ContentType       = 0x03;
    static final int MPI_ReplyTopic        = 0x08;
    static final int MPI_Correlation       = 0x09;
    static final int MPI_SubID             = 0x0B;
    static final int MPI_SessionExpire     = 0x11;
    static final int MPI_ClientID          = 0x12;
    static final int MPI_KeepAlive         = 0x13;
    static final int MPI_AuthMethod        = 0x15;
    static final int MPI_AuthData          = 0x16;
    static final int MPI_RequestReason     = 0x17;
    static final int MPI_WillDelay         = 0x18;
    static final int MPI_RequestReplyInfo  = 0x19;
    static final int MPI_ReplyInfo         = 0x1A;
    static final int MPI_ServerReference   = 0x1C;
    static final int MPI_Reason            = 0x1F;
    static final int MPI_MaxReceive        = 0x21;
    static final int MPI_MaxTopicAlias     = 0x22;
    static final int MPI_TopicAlias        = 0x23;
    static final int MPI_MaximumQoS        = 0x24;
    static final int MPI_RetainUnavailable = 0x25;
    static final int MPI_UserProperty      = 0x26;
    static final int MPI_MaxPacketSize     = 0x27;
    static final int MPI_WildcardAvailable = 0x28;
    static final int MPI_SubIDAvailable    = 0x29;
    static final int MPI_SharedAvailable   = 0x2A;

    
    /*
     * Control packet masks
     */
    static final int CPOI_CONNECT         = 0x0001;
    static final int CPOI_CONNACK         = 0x0002;
    static final int CPOI_CONNECT_CONNACK = 0x0003;
    static final int CPOI_S_PUBLISH       = 0x0004;
    static final int CPOI_C_PUBLISH       = 0x0008;
    static final int CPOI_PUBLISH         = 0x000C;
    static final int CPOI_PUBACK          = 0x0010;
    static final int CPOI_PUBCOMP         = 0x0020;
    static final int CPOI_SUBSCRIBE       = 0x0040;
    static final int CPOI_SUBACK          = 0x0080;
    static final int CPOI_UNSUBSCRIBE     = 0x0100;
    static final int CPOI_UNSUBACK        = 0x0200;
    static final int CPOI_S_DISCONNECT    = 0x0400;
    static final int CPOI_C_DISCONNECT    = 0x0800;
    static final int CPOI_DISCONNECT      = 0x0C00;
    static final int CPOI_AUTH            = 0x1000;
    static final int CPOI_WILLPROP        = 0x2000;
    static final int CPOI_PUBLISHWILL     = 0x200C;
    static final int CPOI_AUTH_CONN_ACK   = 0x1003;
    static final int CPOI_ACK             = 0x0EB2;
    static final int CPOI_CONNACK_DISC    = 0x0602;
    static final int CPOI_USERPROPS       = 0x11FFF;     /* All packets and repeats */

    
    /*
     * Create the MQTT property contexts
     */
    static IsmMqttPropCtx ctx0;         /* Context for versions before 5 */
    static IsmMqttPropCtx ctx5;         /* Context for version 5 */
    
    /*
     * Property list (CSD02)
     * This inludes properties in both directions wo we can check
     */
    static {
        IsmMqttProperty [] mxfields = new IsmMqttProperty [] {
            new IsmMqttProperty(MPI_PayloadFormat,    IsmMqttProperty.Int1,    5, CPOI_PUBLISHWILL,     "PayloadFormat"     ),
            new IsmMqttProperty(MPI_MsgExpire,        IsmMqttProperty.Int4,    5, CPOI_PUBLISHWILL,     "MessageExpire"     ),
            new IsmMqttProperty(MPI_ContentType,      IsmMqttProperty.String,  5, CPOI_PUBLISHWILL,     "ContentType"       ),
            new IsmMqttProperty(MPI_ReplyTopic,       IsmMqttProperty.String,  5, CPOI_PUBLISHWILL,     "ReplyTopic"        ),
            new IsmMqttProperty(MPI_Correlation,      IsmMqttProperty.Bytes,   5, CPOI_PUBLISHWILL,     "CorrelationID"     ),
            new IsmMqttProperty(MPI_SubID,            IsmMqttProperty.VarInt,  5, CPOI_S_PUBLISH | CPOI_SUBSCRIBE,          "SubscriptionID" ),
            new IsmMqttProperty(MPI_SessionExpire,    IsmMqttProperty.Int4,    5, CPOI_CONNECT_CONNACK | CPOI_C_DISCONNECT, "SessionExpire"  ) ,
            new IsmMqttProperty(MPI_ClientID,         IsmMqttProperty.String,  5, CPOI_CONNACK,         "AssignedClientID"  ),
            new IsmMqttProperty(MPI_KeepAlive,        IsmMqttProperty.Int2,    5, CPOI_CONNACK,         "ServerKeepAlive"   ),
            new IsmMqttProperty(MPI_AuthMethod,       IsmMqttProperty.String,  5, CPOI_AUTH_CONN_ACK,   "AuthMethod"        ),
            new IsmMqttProperty(MPI_AuthData,         IsmMqttProperty.Bytes,   5, CPOI_AUTH_CONN_ACK,   "AuthData"          ),
            new IsmMqttProperty(MPI_RequestReason,    IsmMqttProperty.Int1,    5, CPOI_CONNECT,         "RequestReason"     ),
            new IsmMqttProperty(MPI_WillDelay,        IsmMqttProperty.Int4,    5, CPOI_WILLPROP,        "WillDelay"         ),
            new IsmMqttProperty(MPI_RequestReplyInfo, IsmMqttProperty.Int1,    5, CPOI_CONNECT,         "RequestReplyInfo"  ),
            new IsmMqttProperty(MPI_ReplyInfo,        IsmMqttProperty.String,  5, CPOI_CONNACK,         "ReplyInfo"         ),
            new IsmMqttProperty(MPI_ServerReference,  IsmMqttProperty.String,  5, CPOI_CONNACK_DISC,    "ServerReference"   ),
            new IsmMqttProperty(MPI_Reason,           IsmMqttProperty.String,  5, CPOI_ACK,             "Reason"            ),
            new IsmMqttProperty(MPI_MaxReceive,       IsmMqttProperty.Int2,    5, CPOI_CONNECT_CONNACK, "MaxReceive"        ),
            new IsmMqttProperty(MPI_MaxTopicAlias,    IsmMqttProperty.Int2,    5, CPOI_CONNECT_CONNACK, "TopicAliasMaximum" ),
            new IsmMqttProperty(MPI_TopicAlias,       IsmMqttProperty.Int2,    5, CPOI_PUBLISH,         "TopicAlias"        ),
            new IsmMqttProperty(MPI_MaximumQoS,       IsmMqttProperty.Int1,    5, CPOI_CONNACK,         "MaximumQoS"        ),
            new IsmMqttProperty(MPI_RetainUnavailable,IsmMqttProperty.Boolean, 5, CPOI_CONNACK,         "MaxReceive"        ),
            new IsmMqttProperty(MPI_UserProperty,     IsmMqttProperty.NamePair,5, CPOI_USERPROPS,       "UserProperty"      ),
            new IsmMqttProperty(MPI_MaxPacketSize,    IsmMqttProperty.Int4,    5, CPOI_CONNECT_CONNACK, "MaxPacketSize"     ),
            new IsmMqttProperty(MPI_WildcardAvailable,IsmMqttProperty.Int1,    5, CPOI_CONNACK,         "WildcardAvailable" ),
            new IsmMqttProperty(MPI_SubIDAvailable,   IsmMqttProperty.Int1,    5, CPOI_CONNECT_CONNACK, "MaxPacketSize"     ),
            new IsmMqttProperty(MPI_SharedAvailable,  IsmMqttProperty.Int1,    5, CPOI_CONNECT_CONNACK, "MaxPacketSize"     ),
        };
        ctx5 = new IsmMqttPropCtx(mxfields, 5);
        ctx0 = new IsmMqttPropCtx(mxfields, 0);
    }
    
   /*
    * Return the allowed control packets for a return code
    */
   static public int reasonCodeAllowed(int rc) {
       int allow;
       switch (rc) {
       case MqttRC.OK                 :   /* 0x00   0 */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_SUBACK | CPOI_UNSUBACK | CPOI_AUTH | CPOI_DISCONNECT | CPOI_AUTH;  break;
       case MqttRC.QoS1               :   /* 0x01   1 QoS=1 return on SUBSCRIBE */
       case MqttRC.QoS2               :   /* 0x02   2 QoS=2 return on SUBSCRIBE */
           allow = CPOI_SUBACK;
       case MqttRC.KeepWill           :   /* 0x04   4 Normal close of connection but keep the will */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.NoSubscription     :   /* 0x10  16 There was no matching subscription on a PUBLISH */
           allow = CPOI_PUBACK;                           break;
       case MqttRC.NoSubExisted       :   /* 0x11  17 No subscription existed on UNSUBSCRIBE */
           allow = CPOI_UNSUBACK;                         break;
       case MqttRC.ContinueAuth       :   /* 0x18  24 Continue authentication */
       case MqttRC.Reauthenticate     :   /* 0x19  25 Start a re-authentication */
           allow = CPOI_AUTH;                             break;
       case MqttRC.UnspecifiedError   :   /* 0x80 128 Unspecified error */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_UNSUBACK | CPOI_AUTH;  break;
       case MqttRC.MalformedPacket    :   /* 0x81 129 The packet is malformed */
       case MqttRC.ProtocolError      :   /* 0x82 130 Protocol error */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.ImplError          :   /* 0x83 131 Implementation specific error */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_UNSUBACK | CPOI_AUTH;  break;
       case MqttRC.UnknownVersion     :   /* 0x84 132 Unknown MQTT version */
       case MqttRC.IdentifierNotValid :   /* 0x85 133 ClientID not valid */
       case MqttRC.BadUserPassword    :   /* 0x86 134 Username or Password not valid */
           allow = CPOI_CONNACK;                          break;
       case MqttRC.NotAuthorized      :   /* 0x87 135 Not authorized */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_SUBACK | CPOI_UNSUBACK | CPOI_DISCONNECT;   break;
       case MqttRC.ServerUnavailable  :   /* 0x88 136 The server in not available */
           allow = CPOI_CONNACK;                          break;
       case MqttRC.ServerBusy         :   /* 0x89 137 The server is busy */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.ServerBanned       :   /* 0x8A 138 The server is banned from connecting */
           allow = CPOI_CONNACK;                          break;
       case MqttRC.ServerShutdown     :   /* 0x8B 139 The server is being shut down */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.BadAuthMethod      :   /* 0x8C 140 The authentication method is not valid */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.KeepAliveTimeout   :   /* 0x8D 141 The Keep Alive time has been exceeded */
       case MqttRC.SessionTakenOver   :   /* 0x8E 142The ClientID has been taken over */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.TopicFilterInvalid :   /* 0x8F 143 The topic filter is not valid for this server */
           allow = CPOI_SUBACK | CPOI_UNSUBACK | CPOI_DISCONNECT;   break;
       case MqttRC.TopicInvalid       :   /* 0x90 144 The topic name is not valid for this server */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_DISCONNECT;    break;
       case MqttRC.PacketIDInUse      :   /* 0x91 145 The PacketID is in use */
           allow = CPOI_PUBACK | CPOI_SUBACK | CPOI_UNSUBACK;   break;
       case MqttRC.PacketIDNotFound   :   /* 0x92 146 The PacketID is not found */
           allow = CPOI_PUBCOMP;                          break;
       case MqttRC.ReceiveMaxExceeded :   /* 0x93 147 The Receive Maximum value is exceeded */
       case MqttRC.TopicAliasInvalid  :   /* 0x94 148 The topic alias is not valid */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.PacketTooLarge     :   /* 0x95 149 The packet is too large */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.MsgRateTooHigh     :   /* 0x96 150 The message rate is too high */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.QuotaExceeded      :   /* 0x97 151 A user quota is exceeded */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_SUBACK | CPOI_DISCONNECT;    break;
       case MqttRC.AdminAction        :   /* 0x98 152 The connection is closed due to an administrative action */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.PayloadInvalid     :   /* 0x99 153 The payload format is invalid */
           allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_DISCONNECT;         break;
       case MqttRC.RetainNotSupported :   /* 0x9A 154 Retain not supported */
       case MqttRC.QoSNotSupported    :   /* 0x9B 155 The QoS is not supported */
       case MqttRC.UseAnotherServer   :   /* 0x9C 156 Temporary use another server */
       case MqttRC.ServerMoved        :   /* 0x9D 157 Permanent use another server */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.SharedNotSupported :   /* 0x9E 158 Shared subscriptions are not allowed */
           allow = CPOI_SUBACK | CPOI_DISCONNECT;         break;
       case MqttRC.ConnectRate        :   /* 0x9F 159 Connection rate exceeded */
           allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
       case MqttRC.MaxConnectTime     :   /* 0xA0 160 Shared subscriptions are not allowed */
           allow = CPOI_DISCONNECT;                       break;
       case MqttRC.SubIDNotSupported  :   /* 0xA1 161 Subscription IDs are not supportd */
       case MqttRC.WildcardNotSupported : /* 0xA2 163 Wildcard subscriptions are not supported */
           allow = CPOI_SUBACK | CPOI_DISCONNECT;         break;
       default :
           allow = 0;
       }
       return allow;
   }

    /*
     * Check integer fields 
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, int value) {
        boolean valid = true;
        switch (fld.id) {

        case MPI_MaximumQoS:
            if (value < 0 || value > 2)
                valid = false;
            this.maxQoS = value;
            break;
        case MPI_SessionExpire:
            this.expire = value;
            break;
        case MPI_RetainUnavailable:
            this.noRetain = value==1;
            break;
        case MPI_MaxReceive:
            if (value == 0)
                valid = false;
            if (value < serverMaxActive)
                 serverMaxActive = value;
            break;
        case MPI_MaxTopicAlias:
            if (value < serverMaxTopicAlias)
                serverMaxTopicAlias = value;
            break;
        case MPI_MaxPacketSize:
            if (value < 100)
                valid = false;
            else 
                serverMaxPacketSize = value;
            break;
        case MPI_KeepAlive:
            keepalive = value;
            break;

        }
        if (!valid)
            throw new IllegalArgumentException("The " + fld.name + " property is not valid: " + value);
    }


    /*
     * Check for string fields.
     * Note that we have already checked that the Unicode is correct
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String str) {
        switch (fld.id) {
        case MPI_Reason:
            this.reason = str;
            break;
        case MPI_ReplyInfo:
            this.replyInfo = str;
            break;
        case MPI_ServerReference:
            serverReference = str;
            break;
        case MPI_ClientID:
            clientID = str;
            break;
        }    
    }


    /*
     * Check for byte array fields
     * It is common to have nothing to check about byte arrays, but this also allows them to be stored
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, byte[] ba) {
    }
    
    /*
     * Check a user property
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String name, String value) {
        addServerProperty(name, value);    
    }
            
}
