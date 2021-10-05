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

package com.ibm.ima.poc.filter;

import java.nio.charset.Charset;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.Map;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaSubscription;

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
public class FilterConnection implements ImaConnectionListener {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaConnection  connect;          /* The base connection object this listener is associated with */
    ImaPlugin      plugin;           /* The plug-in object this listener is associated with         */
    String         clientID;
    Map<String, ImaSubscription> subscriptions = new HashMap<String, ImaSubscription>();
    static Charset utf8 = Charset.forName("UTF-8");  /* All JSON text is in UTF-8 encoding          */
    
    /**
     * Constructor for the connection listener object.
     * 
     * This is called when a connection is created and it saves the connection and plugin objects
     * for later use. It then creates a clientID and authenticates it.
     * 
     * Using ImaConnection.Authenticated self-authenticates this connection
     */
    FilterConnection(ImaConnection connect, ImaPlugin plugin) {
        this.connect = connect;
        this.plugin = plugin;
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
    }
    
    /**
     * Handle a request to check if the connection is alive.
     * 
     * Send a ping to the client to check if the connection is still alive.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        return true;
    }

    /**
     * Handle data from the client.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
        clientID = makeClientID();
        connect.setKeepAlive(0);         /* We do not want to time out */
        connect.setIdentity(clientID, null, null, ImaConnection.Authenticated, false);
        return -1;
    }
    
    /**
     * Handle the completion of a connection.
     * 
     * A non-zero return code indicates that the connection is closed, and that is handled
     * by calling close.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
        plugin.trace(6, "onConnected Plugin=" + plugin.getName() + " ClientID=" + clientID + 
                " rc=" + rc + " Reason=\"" + reason + "\"");
    }
    
    /**
     * Handle a message from the server.
     * 
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
        /* TODO: do magic here */
    }
    
    /**
     * Handle a message from the server.
     * 
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onGetMessage(Object correlate, int rc, ImaMessage msg) {
    }

    /**
     * Handle a completion event.
     * 
     * The completion can come in on a message or subscription.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onComplete(java.lang.Object, int, java.lang.String)
     */
    public void onComplete(Object obj, int rc, String message) {
        plugin.trace(6, "onComplete Plugin=" + plugin.getName() + " ClientID=" + clientID + 
                " Object=" + obj + " rc=" + rc + " Reason=\"" + message + "\"");
    }
    
    /**
     * Give a simple string representation of this connection listener object.
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "FilterConnection " + connect.getClientID();
    }

    /**
     * This plug-in does not support HTTP interactions.
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data) {
        connect.close(2, "HTTP is not supported");
    }
    
    static char [] base62 = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
        'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
        'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
    };
    
    /*
     * Make a random client ID.
     * The generated clientID starts with _F_ followed by 8 base62 random digits.
     */
    String makeClientID() {
        char [] cidbuf = new char[12];
        cidbuf[0] = '_';
        cidbuf[1] = 'F';
        cidbuf[2] = '_';
        int where = 3;
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
        return new String(cidbuf, 0, where);
    }
}

