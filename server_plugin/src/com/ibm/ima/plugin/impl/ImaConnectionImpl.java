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

package com.ibm.ima.plugin.impl;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaEndpoint;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaSubscription;
import com.ibm.ima.plugin.ImaSubscriptionType;
import com.ibm.ima.plugin.ImaTransactionListener;

/*
 * Implement the ImaConnection class
 */
public class ImaConnectionImpl implements ImaConnection {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

            int    connectID;
            int    state;
            int    index;
    private String address;
    private int    port;
            String protocol;
            String protocolFamily;
            String clientID;
    private Object userData;
    private String user;
    private String password;
    private boolean allowSteal = false;
    private boolean transactionCapable = true;
    private String certname;
    private ImaEndpointImpl endpoint;
    private int    flags;
    private byte[] savedData;
            boolean isClosed = false;
    private char    http_op;
    private Map<String,Object> http_props;
    private Map<String,Object> http_out;
    private int   keepAlive      = 60;
    private int   maxMsgInflight = 128;
    private int   processedMsgCount;
    private boolean suspended    = false;
    static final ThreadLocal < byte[] > tmpByteArray = 
            new ThreadLocal < byte[] > () {
                protected byte[] initialValue() {
                    return new byte[4096];
                }
        };
    
    ImaPluginImpl plugin;
            ImaChannel channel;
            int        seqnum;
    volatile int        needed = 0;
            ImaConnectionListener listener;
    private volatile int subscriptionCount = 1;
    static ConcurrentHashMap<Integer, ImaConnectionImpl> connects      = new ConcurrentHashMap<Integer, ImaConnectionImpl>();
            HashMap<String,ImaSubscriptionImpl> subscriptions = new HashMap<String,ImaSubscriptionImpl>();
            HashMap<Integer,ImaPluginAction> work = new HashMap<Integer,ImaPluginAction>();
            
            
    static Charset utf8 = Charset.forName("UTF-8");
    
    static final int F_Secure   = 1;
    static final int F_Reliable = 2;
    static final int F_Internal = 4;
    static final int F_Client   = 8;
    
    /*
     * Constructor from map
     */
    ImaConnectionImpl(ImaChannel channel, int connectid, Map<String,Object> map) {
        String  plugin_name;
        this.protocol   = getStringProperty(map, "Protocol");
        this.protocolFamily   = getStringProperty(map, "ProtocolFamily");
        this.clientID  = getStringProperty(map, "ClientID");
        this.address   = getStringProperty(map, "ClientAddr");
        this.endpoint  = ImaEndpointImpl.getEndpoint(getStringProperty(map, "Endpoint"), true);
        this.user      = getStringProperty(map, "User");
        this.password  = getStringProperty(map, "Password");
        this.index     = getIntProperty(map, "Index", 0);
        this.port      = getIntProperty(map, "Port", 0);
        this.certname  = getStringProperty(map, "CommonName");
        this.channel = channel;
        if (getIntProperty(map, "Internal", 0) != 0)
            flags |= F_Internal;
        if (getIntProperty(map, "Secure", 0) != 0)
            flags |= F_Secure;
        if (getIntProperty(map, "Reliable", 1) != 0)
            flags |= F_Reliable;
        plugin_name = getStringProperty(map, "Plugin");
        if (plugin_name != null)
            this.plugin = ImaPluginImpl.getPlugin(plugin_name);
        if (plugin != null && ((plugin.getCapabilities()&0x100) != 0)) 
            transactionCapable = false;
        connectID=connectid;
        state = ImaConnection.State_Handshake;
        //  System.out.println(""+this);
        connects.put(connectID, this);
    }
    
    /*
     * Constructor from parameters
     */
    ImaConnectionImpl(ImaPluginImpl plugin, int connectid, String address, int port, ImaEndpointImpl endpoint, int flags) {
        this.connectID = connectid;
        this.index     = connectid;
        this.address   = address;
        this.port      = port;
        this.endpoint  = endpoint;
        this.flags     = flags;
        this.plugin    = plugin;
        if (plugin != null && ((plugin.getCapabilities()&0x100) != 0)) 
            transactionCapable = false;
            
        connects.put(connectID, this);        
        
    }
    
    /*
     * Utility to return a string property
     */
    static String getStringProperty(Map<String,Object>map, String name) {
        Object obj = map.get(name);
        if (obj == null)
            return null;
        if (obj instanceof String)
            return (String)obj;
        return ""+obj;
    }
    
    /*
     * Utility to return an int property
     */
    static int getIntProperty(Map<String,Object>map, String name, int defval) {
        Object obj = map.get(name);
        if (obj == null)
            return defval;
        if (obj instanceof Number)
            return ((Number)obj).intValue();
        return defval;
    }


    /*
     * Accept a connection
     */
    private void accept() {
        protocolFamily = plugin.getProtocolFamily();
        if ((protocol == null) || protocol.isEmpty())
            protocol = protocolFamily;
        state = ImaConnection.State_Accepted;
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Accept, 256, 4);
        action.setObject(this);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        action.bb = ImaPluginUtils.putStringValue(action.bb, protocol);
        action.bb = ImaPluginUtils.putStringValue(action.bb, plugin.getProtocolFamily());
        action.bb = ImaPluginUtils.putStringValue(action.bb, plugin.getName());
        action.send(channel);
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#setProtocol(java.lang.String)
     */
    public void setProtocol(String protocol) {
        if (state != State_Handshake && state != State_Accepted) {
            throw new IllegalStateException("The protocol must be set before authenticate.");
        }
        this.protocol = protocol;
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#setKeepAlive(int)
     */
    public void setKeepAlive(int timeout) {
        keepAlive = timeout;
        if (state == ImaConnection.State_Handshake || state == ImaConnection.State_Accepted ||
            state == ImaConnection.State_Closed)
            return;
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.SetKeepAlive, 256, 2);
        action.bb = ImaPluginUtils.putIntValue(action.bb, connectID);
        action.bb = ImaPluginUtils.putIntValue(action.bb, keepAlive);
        action.send(channel);
    }

    /*
     * Set work for this connection
     */
    public synchronized int setWork(ImaPluginAction action) {
        int seqnum;
        if (work.isEmpty()) {
            this.seqnum = 1;
        }
        seqnum = ++this.seqnum;
        work.put(seqnum, action);
        return seqnum;
    }
    
    /*
     * Get work for this connection.
     * This will unlink it from the list.
     */
    public synchronized ImaPluginAction getWork(int seqnum) {
        ImaPluginAction ret = work.get(seqnum);
        if (ret != null)
            work.remove(seqnum);
        return ret;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaConnection#setIdentity(java.lang.String, java.lang.String, java.lang.String)
     */
    public void setIdentity(String clientid, String user, String password) {
        setIdentity(clientid, user, password, ContinueAuthentication, false);
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#setIdentity(java.lang.String, java.lang.String, java.lang.String, int)
     */
    public void setIdentity(String clientid, String user, String password, int auth) {
        setIdentity(clientID, user, password, auth, false);
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#setIdentity(java.lang.String, java.lang.String, java.lang.String, int,
     * boolean)
     */
    public void setIdentity(String clientid, String user, String password, int auth, boolean allowSteal) {
        if (state != State_Accepted) 
            throw new IllegalStateException("The connection must be in accepted state.");
            
        /* Check for valid state */
        this.clientID = clientid;
        this.user     = user;
        this.password = password;
        this.allowSteal = allowSteal;
        authenticate(auth);
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#setCommonName(java.lang.String)
     */
    public void setCommonName(String certname) {
        if (state != ImaConnection.State_Accepted && state != ImaConnection.State_Handshake)
            throw new IllegalStateException("The connection must be in handshake or accepted state.");
        this.certname = certname;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#authenticate(int)
     */
    public void authenticate(int authaction) {
        if (state != ImaConnection.State_Accepted)
            throw new IllegalStateException("The connection must be in acceted state to authenticate.");
        if (authaction == ImaConnection.Authenticated)
            state = ImaConnection.State_Open;
        else
            state = ImaConnection.State_Authenticating;
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Identify, 1024, 5);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        ImaPluginUtils.putIntValue(action.bb, seqnum);   /* seqnum */
        ImaPluginUtils.putByteValue(action.bb, (byte)authaction);   /* auth */
        ImaPluginUtils.putIntValue(action.bb, keepAlive); /* keepAlive */
        ImaPluginUtils.putIntValue(action.bb, maxMsgInflight); /* maxMsgInflight */
        HashMap<String,Object> cmap = new HashMap<String,Object>(6);
        cmap.put("ClientID", clientID);
        if (this.user != null)
            cmap.put("User", this.user);
        if (this.password != null)   /* TODO: obfuscate */
            cmap.put("Password", this.password);
        if (this.certname != null)
            cmap.put("CommonName", this.certname);
        cmap.put("Protocol", this.protocol);
        cmap.put("AllowSteal", this.allowSteal);
        cmap.put("TransactionCapable", this.transactionCapable);
        action.bb = ImaPluginUtils.putMapValue(action.bb, cmap);
        action.send(channel);
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#close()
     */
    public void close() {
        close(0, "Connection closed");
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#close(int, java.lang.String)
     */
    public synchronized void close(int rc, String reason) {
        if (!isClosed) {
            state = ImaConnection.State_Closed;
            isClosed = true;
            connects.remove(connectID);            
            ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Close, 256, 4);
            ImaPluginUtils.putIntValue(action.bb, connectID);
            ImaPluginUtils.putIntValue(action.bb, 0); /* seqnum */
            ImaPluginUtils.putIntValue(action.bb, rc); /* rc */
            action.bb = ImaPluginUtils.putStringValue(action.bb, reason);
            action.send(channel);
        }
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#getConnectionListener()
     */
    public ImaConnectionListener getConnectionListener() {
        return listener;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaConnection#getCertName()
     */
    public String getCertName() {
        return certname;
    }


    /*
     * @see com.ibm.ima.ext.ImaConnection#getClientAddress()
     */
    public String getClientAddress() {
        return address;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#getClientID()
     */
    public String getClientID() {
        return clientID;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaConnection#getClientPort()
     */
    public int getClientPort() {
        return port;
    }
    /*
     * @see com.ibm.ima.ext.ImaConnection#getEndpoint()
     */
    public ImaEndpoint getEndpoint() {
        return endpoint;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#getIndex()
     */
    public int getIndex() {
        return index;
    }

    /*
     * The password returned is obfuscated.
     * @see com.ibm.ima.ext.ImaConnection#getPassword()
     */
    public String getPassword() {
        return password;    
    }
    
    /*
     * @see com.ibm.ima.ext.ImaConnection#getProtocol()
     */
    public String getProtocol() {
        return protocol;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#getProtocolFamily()
     */
    public String getProtocolFamily() {
        return protocolFamily;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#getState()
     */
    public int getState() {
        return state;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaConnection#getUser()
     */
    public String getUser() {
        return user;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#isClient()
     */
    public boolean isClient() {
        return (flags&F_Client) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#isInternal()
     */
    public boolean isInternal() {
        return (flags&F_Internal) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#isReliable()
     */
    public boolean isReliable() {
        return (flags&F_Reliable) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#isSecure()
     */
    public boolean isSecure() {
        return (flags&F_Secure) != 0;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#simpleSubscriptionName()
     */
    public String simpleSubscriptionName() {
        return "" + (subscriptionCount++);
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#newSubscription(java.lang.String)
     */
    public ImaSubscription newSubscription(String topic) {
        ImaSubscriptionImpl sub = new ImaSubscriptionImpl(ImaDestinationType.topic, topic, simpleSubscriptionName(), 
                ImaSubscriptionType.NonDurable, false, false, this);
        return sub;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#newSubscription(com.ibm.ima.ext.ImaDestinationType, java.lang.String, java.lang.String, com.ibm.ima.ext.ImaSubscfriptonType, com.ibm.ima.ext.ImaReliability, boolean)
     */
    public ImaSubscription newSubscription(ImaDestinationType desttype, String dest, String name, 
            ImaSubscriptionType subType, boolean nolocal, boolean transacted) {
        if (transacted && !transactionCapable)
            throw new RuntimeException("Connection is not transaction capable");
        ImaSubscriptionImpl sub = new ImaSubscriptionImpl(desttype, dest, name, subType, nolocal, transacted, this);
        return sub;
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#receiveRetained(int, java.lang.String)
     */
    public ImaMessage receiveRetained(int seqnum, String topic) {
        return null;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#requiredData(int)
     */
    public void requiredData(int needed) {
        this.needed = needed;    
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#send(com.ibm.ima.ext.ImaMessage, com.ibm.ima.ext.ImaDestinationType, java.lang.String, java.lang.Object)
     */
    public void send(ImaMessage message, ImaDestinationType desttype, String dest, Object ackObj) {
        send(message, desttype, dest, ackObj, null);
    }

    /*
     * @see com.ibm.ima.ext.ImaConnection#send(com.ibm.ima.ext.ImaMessage, com.ibm.ima.ext.ImaDestinationType,
     * java.lang.String, java.lang.Object)
     */
    public void send(ImaMessage message, ImaDestinationType desttype, String dest, Object ackObj,
            ImaTransactionImpl transaction) {
        if (!(message instanceof ImaMessageImpl))
            throw new IllegalArgumentException("The message is not valid");
        if (state != State_Open) {
            throw new IllegalStateException("The connection must be open to send a message.");
        }
        long handle = 0;
        if (transaction != null) {
            if (!(transaction instanceof ImaTransactionImpl))
                throw new IllegalArgumentException("The transaction is not valid");
            if (transaction.inUse() || !transaction.isValid())
                throw new RuntimeException("ImaTransaction object is not valid: inUse=" + transaction.inUse() + " isValid="
                        + transaction.isValid());
            handle = ((ImaTransactionImpl) transaction).handle;
        }
        ImaMessageImpl msg = (ImaMessageImpl)message;
        if (desttype == null) {
            desttype = msg.destType;
            if (desttype == null)
                desttype = ImaDestinationType.topic;
        }    
        if (dest == null)
            dest = msg.dest;
        int flags = ImaMessageImpl.fromQoS(msg.qos) + (msg.retain<<3);
        if (msg.persistent)
            flags |= 0x04;
        if (desttype == ImaDestinationType.queue)
            flags |= 0x20;
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Send, 1024, 6);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        if (ackObj != null) {
            int seqnum = setWork(action);
            ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
            action.setObject(ackObj);
        } else
            ImaPluginUtils.putIntValue(action.bb, 0); /* Seqnum */
        ImaPluginUtils.putByteValue(action.bb, (byte)(ImaMessageImpl.fromMessageType(message.getMessageType())));
        ImaPluginUtils.putByteValue(action.bb, (byte)flags);

        if (desttype == ImaDestinationType.queue)
            ImaPluginUtils.putStringValue(action.bb, dest);
        else    
            ImaPluginUtils.putNullValue(action.bb);
        ImaPluginUtils.putLongValue(action.bb, handle);
        action.bb = ImaPluginUtils.putMapValue(action.bb, msg.props, msg, desttype, dest);
        try {
            action.send(channel, msg.body, msg.body != null ? msg.body.length : 0);
        } catch (IOException ioe) {
            ioe.printStackTrace(System.out);
        }
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#publish(com.ibm.ima.plugin.ImaMessage, java.lang.String, Java.lang.Object)
     */
    public void publish(ImaMessage message, String topic, Object ackObj) {
        send(message, ImaDestinationType.topic, topic, ackObj);
    }
    
    /*
     * Send text output data on the connection.
     * The data will be marked as text.
     * @param data   The string representing the data.
     */
    public void sendData(String data) {
        byte [] b = data.getBytes(utf8);
        sendData(b, 0, b.length, true);
    }
    
    /*
     * Send binary output data on the connection as a byte array.
     * The data will be marked as binary. 
     * @param data   The data to send    
     */
    public void sendData(byte [] data) {
        sendData(data, 0, data.length, false);
    }
    
    /*
     * Send output data on the connection
     * @param data    The data to send    
     * @param offset  The starting offset in the data
     * @param len     The length of data to send 
     * @param isText  If true the data represents text data in UTF-8 encoding.
     */
    public void sendData(byte [] b, int offset, int len, boolean isText) {
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.SendData, len+16, 2);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        ImaPluginUtils.putByteValue(action.bb, isText ? (byte)0x01 : (byte)0x02);
        ImaPluginUtils.putNullValue(action.bb);
        ImaPluginUtils.putByteArrayValue(action.bb, b, offset, len);
        action.send(channel);
    }

    
    /*
     * Send a subscribe request to this connection
     */
    void invokeSubscription(ImaSubscriptionImpl sub, boolean ack) {
        int flags = 0;
        switch (sub.getReliability()) {
        case qos1:    flags = 4;      break;
        case qos2:    flags = 8;      break;
        default:                      break;
        }
        if (sub.getNoLocal())
            flags |= 2;
        ImaDestinationType desttype = sub.getDestinationType();
        if (desttype != null && desttype == ImaDestinationType.queue)
            flags |= 1;

        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Subscribe, 1024, 8);
        int seqnum = 0;
        if (ack) {
            action.setObject(sub);
            seqnum = setWork(action);
        }
        ImaPluginUtils.putIntValue(action.bb, connectID);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* seqnum */
        ImaPluginUtils.putByteValue(action.bb, (byte) flags);
        ImaPluginUtils.putByteValue(action.bb, (byte) sub.getShareType());
        ImaPluginUtils.putByteValue(action.bb, (byte) (sub.transacted ? 1 : 0));
        action.bb = ImaPluginUtils.putStringValue(action.bb, sub.getDestination());
        action.bb = ImaPluginUtils.putStringValue(action.bb, sub.getName());
        action.bb = ImaPluginUtils.putNullValue(action.bb); /* Selector */
        action.send(channel);
        if (!ack) {
            subscriptions.put(sub.getName(), sub);
            sub.isSubscribed = true;
        }
    }

    /*
     * Send a subscription close to this connection
     */
    void closeSubscription(ImaSubscriptionImpl sub) {
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.CloseSub, 256, 4);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        int seqnum = setWork(action);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
        action.setObject(sub);
        ImaPluginUtils.putStringValue(action.bb, sub.getName());
        ImaPluginUtils.putIntValue(action.bb, sub.getShareType());
        action.send(channel);
        return;
    }
    
    /*
     * Destroy a subscription.
     * 
     * @see com.ibm.ima.plugin.ImaConnection#destroySubscription(java.lang.Object, java.lang.String, com.ibm.ima.plugin.ImaSubscriptionType)
     */
    public void destroySubscription(Object waiter, String name, ImaSubscriptionType subType) {
        if (state != ImaConnection.State_Open)
            throw new IllegalStateException("The connection is not open.");
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.DestroySub, 256, 4);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        int seqnum = setWork(action);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
        action.setObject(waiter);
        ImaPluginUtils.putStringValue(action.bb, name);
        ImaPluginUtils.putIntValue(action.bb, ImaSubscriptionImpl.fromSubscriptionType(subType));
        action.send(channel);
        return;
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#log(java.lang.String, java.lang.String, java.lang.String, java.lang.Object[])
     */
    public void log(String msgid, int level, String category, String msgformat, Object... args) {
        plugin.log(msgid, level, category, msgformat, args);
    }

    /*
     * Get the retained message for a topic.
     * This is not yet implemented.
     * @see com.ibm.ima.plugin.ImaConnection#receiveRetained(java.lang.String)
     */
    public ImaMessage receiveRetained(String topic) {
        return null;
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#deleteRetained(java.lang.String)
     */
    public void deleteRetained(String topic, Object correlate) {
        if (state != ImaConnection.State_Open)
            throw new IllegalStateException("The connection must be open.");
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.DeleteRetain, 256, 3);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        if (correlate != null) {
            int seqnum = setWork(action);
            ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
            action.setObject(correlate);
        } else
            ImaPluginUtils.putIntValue(action.bb, 0); /* Seqnum */
        ImaPluginUtils.putStringValue(action.bb, topic);
        action.send(channel);
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#deleteRetained(java.lang.String)
     */
    public void deleteRetained(String topic) {
        deleteRetained(topic, null);
    }

    /*
     * Find which plug-in supports this connection
     */
    boolean pluginLookup(byte[] body, int length) {
        if (isClosed)
            return false;
        if (length == 0) {
            if (protocol == null)
                protocol = "";
            /* This is a connect request for WS or HTTP connection type - use protocol for plug-in lookup */
            plugin = ImaPluginAction.findPlugin(protocol);
            if (plugin == null) {
                close(0, "No plugin defined for protocol: " + protocol);
                return false;
            }
        } else {
            byte[] data = Arrays.copyOfRange(body, 0, length);
            int index = ImaPluginAction.findPlugin(this, data);
            if (index < 0) {
                if (index == -1) {
                    StringBuffer sb = new StringBuffer(1024);
                    int maxLength = (data.length < 64) ? data.length : 64;
                    sb.append("[ ");
                    for (int i = 0; i < maxLength; i++) {
                        sb.append(data[i]);
                        if ((data[i] > 31) && (data[i] < 127))
                            sb.append('(').append((char) data[i]).append(')');
                        sb.append(' ');
                    }
                    sb.append(']');
                    close(0, "No plugin defined for: " + sb.toString());
                }
                else
                    savedData = data;
                return false;
                    
            }
            plugin = ImaPluginAction.plugins[index];
            if (protocol == null)
                protocol = plugin.getName();
        }
        listener = plugin.plugin.onConnection(this, protocol);
        if (listener == null) {
            close(0, "Connection is rejected by the plugin");
            return false;
        }
        accept();
        return true;
    }

    /*
     * Handle data received from an input connection
     */
    void onData(byte[] body) {
        if (!isClosed) {
            byte[] data = body;
            int dataLength = body.length;
            if (savedData != null) {
                /* There is a saved data - append new data to it */
                data = tmpByteArray.get();
                dataLength = savedData.length + body.length;
                if (data.length < dataLength) {
                    data = new byte[dataLength];
                    tmpByteArray.set(data);
                }
                System.arraycopy(savedData, 0, data, 0, savedData.length);
                System.arraycopy(body, 0, data, savedData.length, body.length);
                savedData = null;
            }
            if (state == State_Handshake) {
                if (!pluginLookup(data, dataLength))
                    return;
            }
            int offset = 0;
            while ((dataLength > 0) && !isClosed) {
                if (dataLength < needed)
                    break;
                needed = 0;
                int used = listener.onData(data, offset, dataLength);
                if (used > 0) {
                    offset += used;
                    dataLength -= used;
                    continue;
                }
                if (used < 0)
                    dataLength = 0;
                break;
            }
            if (dataLength > 0) {
                /* We need to save some data */
                savedData = Arrays.copyOfRange(data, offset, dataLength);
            }
        }
    }

    /*
     * Receive a message from the server and send to the plug-in message listener.
     */
    void onMessage(String subname, ImaMessageImpl msg) {
        ImaSubscriptionImpl sub = subscriptions.get(subname);
        listener.onMessage(sub, msg);
        processedMsgCount++;
        if (!suspended && (processedMsgCount >= (maxMsgInflight >> 2))) {
            sendResumeRequest();
            processedMsgCount = 0;
        }
    }

    /*
     * Receive HTTP data
     */
    void onHttpData(byte [] body, char op, String path, String query, Map<String,Object> props) {
        String httpop = null;
        String content_type = null;
        switch (op) {
        case 'H':
        case 'G':  httpop = "GET";  body = null;   break;        
        case 'P':  httpop = "POST";                break;
        case 'U':  httpop = "PUT";                 break;
        case 'D':  httpop = "DELETE";              break;
        }
        
        if (body != null) {
            if (props != null)
                content_type = ""+props.get("]Content-Type");
        }
        http_props   = props;
        this.http_op = op;
        if (state == State_Handshake) {
            if (!pluginLookup(null, 0))
                return;
        } 
        listener.onHttpData(httpop, path, query, content_type, body);
    }
    
    /*
     * Acknowledge a message
     */
    void acknowledge(ImaMessageImpl msg, int rc, ImaTransactionImpl transaction) {
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Acknowledge, 128, 4);
        ImaPluginUtils.putIntValue(action.bb, connectID);
        ImaPluginUtils.putIntValue(action.bb, msg.seqnum); /* connection seqnum */
        ImaPluginUtils.putIntValue(action.bb, rc);
        if (transaction != null) {
            if (transaction.inUse() || !transaction.isValid())
                throw new RuntimeException("ImaTransaction object is not valid: inUse=" + transaction.inUse() + " isValid="
                        + transaction.isValid());
            ImaPluginUtils.putLongValue(action.bb, transaction.handle);
        } else
            ImaPluginUtils.putLongValue(action.bb, 0);
        action.send(channel);
    }
    
    /*
     * Process a liveness check
     */
    void onLivenessCheck() {
        boolean keepAlive = false;
        if (!isClosed) {
            if (listener != null) {
                keepAlive = listener.onLivenessCheck();
            }
            if (keepAlive)
                return;
            close(160, "The connection timed out");
        }
    }

    void onComplete(Object[] hdrs) {
        int seqnum = ((Number) hdrs[1]).intValue();
        int rc = ((Number) hdrs[2]).intValue();
        final ImaPluginAction action = getWork(seqnum);
        if (action != null) {
            if (action.obj instanceof ImaTransactionImpl) {
                ImaTransactionImpl transaction = (ImaTransactionImpl) action.obj;
                transaction.onComplete(action.action, rc, hdrs[3]);
                return;
            }
            String reason = (String) hdrs[3];
            switch(action.action) {
            case ImaPluginAction.Send:
                listener.onComplete(action.obj, rc, reason);
                break;
            case ImaPluginAction.Subscribe:
                onSubscribe((ImaSubscriptionImpl) action.obj, rc, reason);
                break;
            case ImaPluginAction.CloseSub:
                onCloseSubscription((ImaSubscriptionImpl) action.obj, rc, reason);
                break;
            default:
                listener.onComplete(action.obj, rc, reason);
                break;
            }
        } else {
            String reason = (String) hdrs[3];
            listener.onComplete(listener, rc, reason);
        }
    }

    /*
     * Add a subscription to the list for this connection 
     */
    void onSubscribe(ImaSubscriptionImpl sub, int rc, String reason) {
        if(rc == 0) {
            subscriptions.put(sub.getName(), sub);
            sub.isSubscribed = true;
        }
        listener.onComplete(sub, rc, reason);
//      System.err.println("onSubscribe: add subscription to list: " + sub);
    }

    /*
     * Remove a subscription from the list for this connections 
     */
    void onCloseSubscription(ImaSubscriptionImpl sub, int rc, String reason) {
        subscriptions.remove(sub.getName());
        sub.isSubscribed = false;
        listener.onComplete(sub, rc, reason);
        
//      System.err.println("onCloseSubscription: remove subscription from the list: " + sub);
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnection#sendHttpData(int, java.lang.String, byte[])
     */
    public void sendHttpData(int rc, String content_type, byte [] bytes) {
        sendHttpData(rc, 0, content_type, bytes);
    }

    /**
     * Send HTTP data.
     * 
     * This can only be done as a result of an onHttpData() call.  Each onHttpData() call but result in
     * one and only one sendHttpData().
     * @param rc  An HTTP return code.  Only a small set of HTTP return codes are allowed
     * @param content_type  The content type of the result.  If null, "text/plain" is used.
     * @param bytes  The byte content of the result.  If null an empty string is used.
     */
    public void sendHttpData(int rc, int options, String content_type, byte [] bytes) {
        if (http_op == 0)
            throw new IllegalStateException("HTTP calls are only allowed while processing an onHttpData");
        switch (rc) {
        case 200:  /* OK */
        case 201:  /* Created  */
        case 202:  /* Accepted */
        case 203:  /* Non-authoritative */
        case 204:  /* No content */
        case 205:  /* Reset content */
        case 400:  /* Bad request */
        case 403:  /* Forbidden */
        case 404:  /* Not Found */
        case 405:  /* Method not allowed */
        case 406:  /* Not acceptable */
        case 409:  /* Conflict */
        case 410:  /* Gone */
        case 413:  /* Request entity too large */
        case 415:  /* Unsupported media type */
        case 500:  /* Server error */
        case 501:  /* Not implemented */
        case 503:  /* Service Unavailable */
            if (rc == 204) {
                content_type = null;
                bytes = new byte[0];
            } else {
                if (content_type == null || content_type.length()==0) {
                    content_type = "text/plain";
                    if (bytes == null) 
                        bytes = new byte[0];
                }
            }    
            /* TODO */  
            break;
        default:
            throw new IllegalArgumentException("Invalid HTTP return code");
        }    
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.SendHttp, 1024, 4); 
        ImaPluginUtils.putIntValue(action.bb, connectID);
        ImaPluginUtils.putIntValue(action.bb, rc);
        action.bb = ImaPluginUtils.putStringValue(action.bb, content_type);
        action.bb = ImaPluginUtils.putIntValue(action.bb, options);
        action.bb = ImaPluginUtils.putMapValue(action.bb, http_out);
        action.bb = ImaPluginUtils.putByteArrayValue(action.bb, bytes, 0, bytes.length);
        action.send(channel);
        
        /* We are no longer processing an HTTP op */
        http_op = 0;
        http_props = null;
        http_out   = null;
    }
    
    /**
     * Send HTTP data as a String.
     */
    public void sendHttpData(int rc, String content_type, String data) {
        sendHttpData(rc, 0, content_type, data.getBytes(utf8));
    }
    
    /*
     * lSend HTTP data as a String with options.
     */
    public void sendHttpData(int rc, int options, String content_type, String data) {
        sendHttpData(rc, options, content_type, data.getBytes(utf8));
    }
    
    /**
     * Get an HTTP cookie by name.
     * 
     * @param name  The name of the cookie
     * @return The value of the coolie or null if the cookie is not set 
     */
    public String getHttpCookie(String name) {
        if (http_props == null)
            return null;
        return ""+http_props.get(name);
    }

    
    /**
     * Get a Set of HTTP cookie names.
     * If there are no cookies or this is not an HTTP connection, return null.
     * @return A set of the names of cookies from the current HTTP connection.
     */
    public Set<String> getHttpCookieSet() {
        if (http_props == null)
            return null;
        Set<String> keyset = http_props.keySet();
        HashSet<String> myset = new HashSet<String>(keyset);
        Iterator<String> it = myset.iterator();
        while (it.hasNext()) {
            String key = it.next();
            if (key.length()>0 && key.charAt(0)==']')
                myset.remove(key);
        }  
        return myset;
    }
    
    /**
     * Get an HTTP header from the current HTTP connection
     * @param name   The name of the header
     * @return  The value of the header
     */
    public String getHttpHeader(String name) {
        if (http_props == null)
            return null;
        Object obj = http_props.get("]"+name);
        if (obj == null || !(obj instanceof String))
            return null;
        return ""+obj;
    }
    
    /**
     * Set an HTTP header field for the following sendHttpData().
     * @param name   The name of the header
     * @param value  The value of the header
     */
    public void setHttpHeader(String name, String value) {
        if (http_out == null) {
            http_out = new HashMap<String,Object>();
            if (http_op == 0)
                throw new IllegalStateException("HTTP calls are only allowed while processing an onHttpData");
        }    
        http_out.put("]"+name, value);        
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#setUserData()
     */
    public Object getUserData() {
        return userData;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#setUserData()
     */
    public void setUserData(Object userData) {
        this.userData = userData;
    }

    /*
     * 
     */
    public void suspendMessageDelivery() {
        suspended = true;
    }

    /*
     * 
     */
    public void resumeMessageDelivery() {
        suspended = false;
        if (processedMsgCount >= (maxMsgInflight >> 2)) {
            sendResumeRequest();
            processedMsgCount = 0;
        }
    }

    public void createTransaction(ImaTransactionListener l) {
        new ImaTransactionImpl(this, l);
    }

    /*
     * 
     */
    private void sendResumeRequest() {
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.ResumeDelivery, 64, 2);
        action.bb = ImaPluginUtils.putIntValue(action.bb, connectID);
        action.bb = ImaPluginUtils.putIntValue(action.bb, processedMsgCount);
        action.send(channel);
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnection#getRetainedMessage()
     */
    public void getRetainedMessage(String topic, Object correlate) {
    	if(topic==null){
    		 throw new NullPointerException("Topic is null.");
    	}
		ImaPluginAction action = new ImaPluginAction(ImaPluginAction.GetMessage, 64, 3);
        action.bb = ImaPluginUtils.putIntValue(action.bb, connectID);
        int seqnum = setWork(action);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
        action.setObject(correlate);
        action.bb = ImaPluginUtils.putStringValue(action.bb, topic);
        action.send(channel);
	}
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuffer sb = new StringBuffer(512);
        sb.append("ImaConnection Connect=").append(connectID).append(" ClientAddr=")
            .append(address).append(" Port=").append(port).append(" Protocol=").append(protocol);
        if (endpoint != null)
            sb.append(" Endpoint=\"").append(endpoint.getName()).append('\"');
        if (clientID != null)
            sb.append(" ClientID=\"").append(clientID).append('\"');
        if (user != null)
            sb.append(" User=\"").append(user).append('\"');
        if (certname != null)
            sb.append(" CommonName=\"").append(certname).append('\"');
        return new String(sb);
    }

	
	

}
