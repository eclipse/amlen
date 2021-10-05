/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.poc.udp;
import java.nio.charset.Charset;
import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaReturnCode;
import com.ibm.ima.plugin.ImaSubscription;

/**
 * The ATBConnection represents either an incoming ATB HTTP connection or
 * an outgoing server connections.
 * 
 * One of the issues with the data and especially the periodic data is that it is
 * not byte aligned.  Some amount of effort should be taken to help the performance
 * of this parsing.  Some study shows that using specific size methods to get the 
 * big endian data and putting the shifting inline gives best performance.  
 * 
 */
public class UDPConnection implements ImaConnectionListener {
    ImaConnection  connect;          /* The base connection object this listener is associated with */
    ImaPlugin      plugin;           /* The plug-in object this listener is associated with         */
    UDPPlugin     listener;
    int            which;
    static Charset utf8 = Charset.forName("UTF-8");  /* UTF-8 encoding  */
    String clientID = "";
    boolean connected = false;
    int  channel = 0;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /**
     * Constructor for the connection listener object.
     * 
     * This is called when a connection is created and it saves the connection and plugin objects
     * for later use. 
     */
    UDPConnection(ImaConnection connect, ImaPlugin plugin, UDPPlugin listener, int which) {
        this.connect  = connect;
        this.plugin   = plugin;
        this.listener = listener;
        this.which    = which;
        if (which < 0) {
            channel = listener.getChannel();
        }
    }

    /**
     * Handle the close of a connection.
     * 
     * If one of the server connections closes, reopen it if we are not terminating
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onClose(int, java.lang.String)
     */
    public void onClose(int rc, String reason) {
        /*
         * In the case of an error termination, restart the connection 
         */
        if (which >= 0) {
            synchronized (this) {
                listener.virtconn[which] = null;
            }
            if (rc != ImaReturnCode.IMARC_ServerTerminating && rc != ImaReturnCode.IMARC_EndpointDisabled) {
                plugin.createConnection("udp!virt", listener.endpoint);
            }    
        }   
    }
    
    
    /**
     * Handle a request to check if the connection is alive.
     * 
     * There is no real concept of connection alive for UDP 
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        return true;  
    }
    
    
    /**
     * Handle data from the client.
     *
     * This plugin does not support HTTP
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data) {
    }

    
    /**
     * Handle data from the client.
     * 
     * This is only called when the virtual connection is first established
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
        clientID = "__UDP_" + which;
        connect.setKeepAlive(0);         /* We do not want to time out */
        connect.setIdentity(clientID, null, null, ImaConnection.Authenticated, false);
        return -1;
    }
    
        /**
     * Handle the completion of a connection and send a connection message to the client.
     *
     * We do not need to do anything as the http response will indicate the connection
     * status.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
        connected = true;
    }

    
    /**
     * Handle a message from the server.
     * 
     * This is not called as we have no subscriptions
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
    }

    
    /**
     * Handle a completion event.
     * 
     * The only completion is for a persistent message, and the correlation object is
     * the connection on which to send the response.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onComplete(java.lang.Object, int, java.lang.String)
     */
    public void onComplete(Object obj, int rc, String message) {
        if (plugin.isTraceable(9) || rc != 0) {
            plugin.trace("onComplete Connect=" + this + " Object= " + obj + " rc=" + rc);
        }
        if (obj != null) {
            if (obj instanceof ImaMessage) {
                listener.onComplete((ImaMessage)obj, rc, message);
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
        return "UDPConnection " + connect.getClientID();
    }
    
    
    /**
     * Handle a message from the server.
     * 
     * This is not called as we do not ask for retained messages.
     *  
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onGetMessage(Object correlate, int rc, ImaMessage msg) {
    }

}
