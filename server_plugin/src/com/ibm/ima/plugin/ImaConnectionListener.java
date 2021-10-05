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

package com.ibm.ima.plugin;

/**
 * The connection listener defines the callbacks associated with a connection. 
 * The plug-in which accepts a connection must create an implementation object
 * which implements this interface, and return that on the call to onConnection()
 * in the plug-in listener.
 * <br>
 * The object implementing this interface is the connection related object owned 
 * by the plug-in.
 */
public interface ImaConnectionListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Called when the connection is closing.
     * 
     * It is possible that the physical connection is already closed when this method is called.
     * Otherwise the physical connection will be closed after the return from this callback.
     * 
     * 
     * This provides the ability to clean up any resources attached to the connection.
     * @param rc      A return code 0=normal
     * @param reason  A human readable reason code
     */
    public void onClose(int rc, String reason);
    
    /**
     * Called when data is available on a connection.
     * 
     * If the onData() callback throws an exception, the connection is terminated.
     * 
     * @param data  The available data
     * @return The number of bytes used.  If this is negative or greater than the number of bytes
     * in the data then all of the bytes are consumed.  Otherwise it is the number of bytes used
     * and the remaining bytes will be resent on the next onData call when there is more data 
     * available.
     */
    public int onData(byte[] data, int offset, int length);
    
    /**
     * Called when a message from MessageSight is received for a connection.
     * 
     * When a message is received there is almost always an active subscription
     * which is the reason the message is received.  However this is not guaranteed.
     * For instance a message might arrive after a subscription is closed because
     * the message processing is asynchronous.  In this case the subscription is
     * returned as null so this case needs to be handled.
     *  
     * @param sub   The subscription on which the message comes.  This can be null.
     * @param msg   The message
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg);
    
    /**
     * Called when an asynchronous action completes.
     * 
     * Since the completion object can be something other than an ImaMessage, the callback
     * should always check the class of the object before casting it.
     * 
     * @param obj     The object which is complete.  For the completion of a send, the object is
     *                the message which was sent.
     * @param rc      The return code.  0=good 
     * @param message A human readable message
     */
    public void onComplete(Object obj, int rc, String message);
    
    /**
     * Called when the connection request completes.
     * 
     * When this method is called, all authentication and authorization of the
     * connection is complete.  If the return code is 0 the connection can be
     * used. 
     * 
     * @param rc      A return code 0=normal
     * @param reason  A human readable reason code
     */
    public void onConnected(int rc, String reason);

    /**
     * Called when no data was received on the connection during checkAlive timeout.
     * 
     * This is commonly called as the result of a keepalive timeout.  It can also be called when
     * another connection uses the same ClientID as this connection.
     * 
     * @return true if connection should be left active, false if connection should be closed
     */
    public boolean onLivenessCheck();
    
    /**
     * Called when data is received on an HTTP connection.
     * <p>
     * Additional information (such as host name, headers, and cookies) is available by query from the connection.
     * <p>
     * Note: this is currently not implemented
     * @param op      The operation to perform ("GET", "PUT", or "POST")  
     * @param path    The path not including the alias used to route to this plug-in
     * @param query   The query section of the URI, not including the leading question mark
     * @param content_type  The content type from a PUT or POST operation.  This might be NULL.
     * @param data    The byte data from a PUT or POST operation
     */
    public void onHttpData(String op, String path, String query, String content_type, byte [] data);
    
    /**
     * Called when an asynchronous get retained message action completes.
     * 
     * 
     * @param obj     The correlate object.
     * @param rc      The return code.  0=good 
     * @param msg  	  The message. If the return code is not zero, this parameter 
     *                will null.
     */    
    public void onGetMessage(Object correlate, int rc, ImaMessage message);
    
}
