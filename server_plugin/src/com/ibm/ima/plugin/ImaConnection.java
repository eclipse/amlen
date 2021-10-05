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

import java.util.Set;

/**
 * A connection represents a logical communications channel.
 * 
 * The connection can represent a connection into the server, or out of the server
 * when the server is acting as a client.  It can also be a virtual connection which
 * does not represent a physical connection.
 * 
 * This is the Java representation of a connection object within the server.
 * 
 * The connection holds the security context and the connection must be authenticated
 * before it can be used.
 * 
 * The connection is not serialized.  If the connection is only processed within the 
 * ConnectionListerner methods, then no additional synchronization is required.
 *
 */
public interface ImaConnection {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /** The connection is authenticated and authorized */
    public static int Authenticated          = 1;
    /** The connection is not authorized and will be terminated */
    public static int NotAuthenticated       = 2;
    /** Continue authentication check with another module */
    public static int ContinueAuthentication = 3; 
    
    /** The connection is in protocol handshake */
    public static int State_Handshake        = 1;
    /** The connection has been accepted but the not authenticated or authorized */
    public static int State_Accepted        = 2;
    /** The connection is in the process of being authenticated and authorized */
    public static int State_Authenticating  = 3;
    /** The connection ia authorized and can process messages */
    public static int State_Open            = 4;
    /** The connection is closed */
    public static int State_Closed          = 5;
    
    /** 
     * Close the connection after sending the response.  
     * This option s used to force the connection to be a single request/response or to force 
     * a close in the case of an error. 
     */
    public static int HTTP_Option_Close     = 1;
    
    /** 
     * Increment the ReadMsg statistic for the connection and endpoint.
     * This is used when the plug-in wishes to show that an incomming message is
     * processed, even though it does not result in a message being sent from this connection.   
     */
    
    public static int HTTP_Option_ReadMsg   = 2;
    
    /** 
     * Increment the WriteMsg stats for the connection and endpoint.
     * This is used when the plug-in wishes to show that an outgoing message is
     * processed, even though no message has been received on this connection.    
     */
    public static int HTTP_Option_WriteMsg  = 4;
    
    /** 
     * Set the protocol for an incoming connection.
     * 
     * This action must be taken during an onProtocolCheck() callback in the plug-in listener.
     *
     * @param protocol  The name of the protocol to accept. 
     */
    public void setProtocol(String protocol);
    
    /**
     * Sets the keep alive timeout for the connection.
     * <p>
     * If no data received on the connection during timeout period onLivenessCheck method of the ImaConnectionListener
     * will be called.
     * 
     * @param timeout The keep alive timeout in seconds
     */
    public void setKeepAlive(int timeout);

    /**
     * Set the identity of the connection.
     * <p>
     * This sets the identity fields in the connection. This can be called during an onData() callback and will start
     * authentication processing.
     * 
     * @param clientid Set the clientID for the connection. Any connection which sends or receives messages must have a
     *            unique clientID.
     * @param user The name of the user which can be null.
     * @param password The password for the user which can be null
     */
    public void setIdentity(String clientid, String user, String password);
    
    /**
     * Set the identity of the connection and optionally start authenticate.
     * <p>
     * This sets the identity fields in the connection. This can be called during an onData() callback 
     * and will start authentication processing if the auth setting is ContinueAuthentication.
     * <p>     * 
     * It can be called after receiving an onAuthenticate() on this connection and can be used to reset 
     * any identity fields changed by the authentication and to unblock processing on this connection. 
     * 
     * @param clientid  Set the clientID for the connection.  Any connection which sends or receives 
     * messages must have a unique clientID. 
     * @param user      The name of the user which can be null.
     * @param password  The password for the user which can be null
     * @param auth      The authentication setting
     * @param allowSteal If true, allow this session to steal the clientID of an existing connection
     */
    public void setIdentity(String clientid, String user, String password, int auth, boolean allowSteal);
    
    /**
     * Set the common name of the connection.
     * <p>
     * This sets the common name in the connection.  This can only be called before other authorization
     * and is used for cases where the protocol determines the certificate name.  When using transport
     * certificates the existing common name should not be changed.
     * @param certname  The certificate common name
     */
    public void setCommonName(String certname);
    
    /**
     * Authenticate the connection.
     * <p>
     * This can be called after receiving an onAuthenticate() and while the connection is blocked waiting 
     * for an authenticate.  It authenticates without updating the identity information.
     * <p>
     * If the auth setting is Authenticated, processing of the connection is unblocked and further processing
     * is done using the existing identity.  If the auth setting is NotAuthenticated, the connection is 
     * terminated.  If the auth setting is ContinueAuthentication, the connection remains blocked and
     * any other available authentication providers are tried.  
     * 
     * @param auth the authentication setting.
     */
    public void authenticate(int auth);
    
    /**
     * Close the connection normally.
     */
    public void close();
    
    /**
     * Close the connection.
     * @param rc     The MessageSight return code for this close
     * @param reason A reason string associated with this close (this is logged)
     */
    public void close(int rc, String reason);
    
    /**
     * Put an entry into the log.
     * 
     * This log entry is associated with this connection.
     * 
     * @param msgid     The message ID that can be any string.  IBM MessageSight uses an alphabetic string followed by four digits.
     * @param level     The level or severity of the log message (LOG_CRIT, LOG_ERROR, LOG_WARN, LOG_NOTICE, LOG_INFO) 
     * @param category  The category of the message.  This is placed in the log.  The categories "Connection", "Admin",
     *                  and "Security" will cause the entry to go into the associated log.  All other categories
     *                  will appear in the default log.
     * @param msgformat The format of the message in Java MessageFormat string form
     * @param args      A variable array of replacement values for the message format
     */
    public void log(String msgid, int level, String category, String msgformat, Object ... args);
    
    /**
     * Returns the connection listener associated with this connection.
     * 
     * The connection listener is set at the time onConnection() is invoked on the plug-in
     * listener and is null before onConnection() is called. 
     * @return The connection listener 
     */
    public ImaConnectionListener getConnectionListener();
    
    /**
     * Returns the endpoint associated with this connection.
     * The endpoint is a read-only object. 
     * @return The endpoint object or null to indicate there is no endpoint.
     */
    public ImaEndpoint getEndpoint();
    
    /**
     * Returns the index associated with this connection.
     * @return The index of a connection is used internally in the server and is made available here
     *         to allow correlation with problem determination of the server.
     */
    public int  getIndex();
    
    /**
     * Returns the protocol associated with this connection.
     * @return the name of the protocol or a zero length string to indicate the protocol is not set.
     */
    public String getProtocol();
    
    /**
     * Returns the protocol family associated with this connection.
     * @return the name of the protocol family or a zero length string to indicate the protocol is not set.
     */
    public String getProtocolFamily();
    
    /**
     * Returns the certificate name.
     * @return the certificate name.
     */
    public String getCertName();
    
    /**
     * Returns the client IP address associated with this connection.
     * 
     * Note that this represents the address of what is the other end of the connection and
     * might in fact be the server in the case where the connection is initiated from the plug-in.  
     * This string might be null.
     * @return the IP address of the other end of the connection
     */
    public String getClientAddress();
    
    /**
     * Returns the client ID.
     * @return the client ID.
     */
    public String getClientID();
    
    /**
     * Returns the client port number associated with this connection.
     * 
     * Note that this represents the port of the other end of the connection and
     * might in fact be the server in the case where we initiate the connection.  
     * This string might be null.
     * @return the port of the other end of the connection
     */
    public int getClientPort();

    /**
     * Returns the password in obfuscated form.
     * @return The obfuscated password
     */
    public String getPassword();
        
    /**
     * Returns the state of the connection.
     * @return the state of the connection as a State_ constant from this class.
     */
    public int    getState();
    
    /**
     * Returns the user name.
     * @return the user name of null if there is no user name.
     */
    public String getUser();

    /**
     * Returns whether this is an outgoing connection.
     * @return true if the plug-in originated the connection, and false otherwise
     */
    public boolean isClient();
    
    /**
     * Returns whether the connection is internal.
     * @return true if the connection was created from the plug-in.
     */
    public boolean isInternal();
    
    /**
     * Returns whether the connection is reliable.
     * @return true if this is a reliable connection such as TCP, false if it is an unreliable connection such as UDP.
     */
    public boolean isReliable();
    
    /**
     * Returns whether the connection is secure.
     * @return true if this is a secure connection (SSL/TLS), false otherwise.
     */
    public boolean isSecure();

    /**
     * Create a simple subscription name for non-durable subscriptions.
     * Durable subscriptions require a name which is the same when the consumer
     * is connected to the subscription.  Therefore this generated subscription name
     * must not be used for durable subscriptions.
     * @return A name which can be used as a unique name for a non-durable subscription. 
     */
    public String simpleSubscriptionName();

    /**
     * Create a new simple subscription object.
     * 
     * Create a non-durable subscription to a topic destination.  The subscription name is constructed.
     * 
     * @param topic     The topic name
     * @return A subscription object which you can then subscribe to
     */
    public ImaSubscription newSubscription(String topic);

    /**
     * Create a new subscription object.
     * 
     * A subscription can be durable, but a subscription
     * 
     * @param desttype  The destination type (topic or queue)
     * @param dest      The topic or queue name
     * @param name      The subscription name
     * @param subType   The subscription type (only for topics)
     * @param nolocal   Whether local messages should be received. 
     * @return A subscription object which you can then subscribe to
     */
    public ImaSubscription newSubscription(ImaDestinationType desttype, String dest, String name, 
            ImaSubscriptionType subType, boolean nolocal, boolean transacted);
    
    /**
     * Receive the retained message (if any) from a topic.
     * 
     * This call is asynchronous.
     * 
     * @param  topic    	The name of the topic
     * @param  correlate	the correlation object.
     * @throws ImaPluginException if an error occurs while recquesting the retained message.
     */
    public void getRetainedMessage(String topic, Object correlate);
    
    /**
     * Delete the retained message (if any) from a topic.
     *
     * No indication is returned about whether any retained message was actually deleted.
     * Note that deleting like sending a retained message is a race between multiple publishers
     * and the last one to be processed wins.
     * 
     * @param topic   The name of the topic
     */
    public void  deleteRetained(String topic);
    
    /**
     * Delete the retained message (if any) from a topic.
     *
     * Delete the retained message and send a completion when done.
     * 
     * @param topic   The name of the topic
     * @param correlate  The correlation object for onComplete.
     */
    public void  deleteRetained(String topic, Object correlate);
    
    /**
     * Specify the data required before the next onData call.
     * 
     * Specify the number of bytes which should be in the data before the next onData() 
     * is called.  This value is reset to zero on each call to onData().  This length
     * includes any unused bytes from the onData() return value.
     * <p>
     * This is used in cases such as protocols where the frame specifies a length.  
     * This allows the plug-in to indicate that data should be accumulated until a full
     * frame is available before calling onData().
     * <p>
     * Regardless of this setting, the onData callback should be prepared to handle
     * the case where fewer than the needed number of bytes is specified by onData().
     *
     * @param needed  The number of bytes required before re-invoking onData().
     */
    public void requiredData(int needed);
    
    /**
     * Send a message to a destination.
     * 
     * Between this call and the onComplete() callback for the message object, the message can be
     * modified by the connection and should not be modified by the invoker.
     * 
     * @param message   The message to send
     * @param desttype  The destination type (topic or queue)
     * @param dest      The destination name
     * @param ackObj    The object to send a complete on
     */
    public void send(ImaMessage message, ImaDestinationType desttype, String dest, Object ackObj);
    
    /** 
     * Publish a message to a topic.
     * 
     * This is the simplified from of send which only supports pub/sub 
     *     
     * Between this call and the onComplete() callback for the message object, the message can be
     * modified by the connection and should not be modified by the invoker.
     * 
     * @param message   The message to publish
     * @param topic     The topic name
     * @param ackObj    The object to send a complete on
     */
    public void publish(ImaMessage message, String topic, Object ackObj);
    
    /**
     * Send text output data on the connection.
     * The data will be marked as text.
     * @param data   The string representing the data.
     */
    public void sendData(String data);
    
    /**
     * Send binary output data on the connection as a byte array.
     * The data will be marked as binary. 
     * @param data   The data to send    
     */
    public void sendData(byte [] data);
    
    /**
     * Send output data on the connection
     * @param data    The data to send    
     * @param offset  The starting offset in the data
     * @param len     The length of data to send 
     * @param isText  If true the data represents text data in UTF-8 encoding.
     */
    public void sendData(byte [] b, int offset, int len, boolean isText);

    /**
     * Destroy a subscription.
     * 
     * Destroy subscription is only required for durable subscriptions.  Non-durable subscriptions are
     * automatically destroyed when there are no more consumers.
     * 
     * @param obj     An object to return on completion
     * @param name    The name of the subscription
     * @param subType The subscription type of the object
     * @throws ImaPluginException if an error occurs while destroying a subscription.
     */
    public void destroySubscription(Object obj, String name, ImaSubscriptionType subType);
    
    /**
     * Send HTTP data.
     * 
     * This can only be done as a result of an onHttpData() call.  Each onHttpData() call but result in
     * one and only one sendHttpData().  All operations are assumed to allow content to be returned,
     * but the return code of 204 (No content) will force the content to be null.
     * <p>
     * For errors it is recommended that a description of the error be returned in "text/plain".  
     * <p>
     * If you return html text you must ensure that a less than sign or ampersand are not put in the 
     * stream unescaped 
     * <p>
     * The following return codes are allowed:
     * <dl>
     * <dt>200<dd>OK 
     * <dt>201<dd>Created 
     * <dt>202<dd>Accepted
     * <dt>203<dd>Non-authoratative
     * <dt>204<dd>No content (content_type and bytes are ignored for this return code)
     * <dt>205<dd>Reset content
     * <dt>400<dd>Bad request
     * <dt>403<dd>Forbidden
     * <dt>404<dd>Not found
     * <dt>405<dd>Method not allowed
     * <dt>406<dd>Not acceptable
     * <dt>409<dd>Conflict
     * <dt>410<dd>Gone
     * <dt>413<dd>Request too large
     * <dt>415<dd>Unsupported media type
     * <dt>500<dd>Server error
     * <dt>501<dd>Not implemented
     * <dt>503<dd>Service Unavailable
     * </dl>
     * @param rc  An HTTP return code.  
     * @param content_type  The content type of the result.  If null, "text/plain" is used.
     * @param bytes  The byte content of the result.  If null value results in a zero length content.
     * @throws IllegalArgumentException if an onHttpData is not active
     */
    public void sendHttpData(int rc, String content_type, byte [] bytes);
    
    /**
     * Send HTTP data with options.
     * 
     * This can only be done as a result of an onHttpData() call.  Each onHttpData() call but result in
     * one and only one sendHttpData().  All operations are assumed to allow content to be returned,
     * but the return code of 204 (No content) will force the content to be null.
     * <p>
     * The options allow special processing to be done as the result of sending this response.
     * <p>
     * @param rc  An HTTP return code.  
     * @param options HTTP send options (HTTP_Option_*) which can be ORed together
     * @param content_type  The content type of the result.  If null the content type is
     *               determined by examination of the content.
     * @param bytes  The byte content of the result/  A null value results in a zero length content.
     * @throws IllegalArgumentException if an onHttpData is not active
     */
    public void sendHttpData(int rc, int options, String content_type, byte [] bytes);
    
    /**
     * Send HTTP data as a String.
     * 
     * This can only be done as a result of an onHttpData() call.  Each onHttpData() call but result in
     * one and only one sendHttpData().
     * @param rc  An HTTP return code.  Only a restricted set of HTTP return codes are allowed
     * @param content_type  The content type of the result.  If null, "text/plain" is used.
     * @param bytes  The byte content of the result.  If null an empty string is used.
     * @throws IllegalArgumentException if an onHttpData is not active
     * 
     * @param rc  An HTTP return code.  
     * @param content_type  The content type of the result.  If null the content type is
     *               determined by examination of the content.
     * @param data   The string content of the result/  A null value results in a zero length content.
     * @throws IllegalArgumentException if an onHttpData is not active
     */
    public void sendHttpData(int rc, String content_type, String data);
    
    /**
     * Send HTTP data as a string with options.
     * 
     * This can only be done as a result of an onHttpData() call.  Each onHttpData() call but result in
     * one and only one sendHttpData().  All operations are assumed to allow content to be returned,
     * but the return code of 204 (No content) will force the content to be null.
     * <p>
     * The options allow special processing to be done as the result of sending this response.
     * <p>
     * @param rc  An HTTP return code.  
     * @param options HTTP send options (HTTP_Option_*) which can be ORed together
     * @param content_type  The content type of the result.  If null the content type is
     *               determined by examination of the content.
     * @param data   The string content of the result.  If null an empty string is used.
     * @throws IllegalArgumentException if an onHttpData is not active
     */
    public void sendHttpData(int rc, int options, String content_type, String data);
    
    /**
     * Get an HTTP cookie by name.
     * 
     * @param name  The name of the cookie
     * @return The value of the coolie or null if the cookie is not set 
     */
    public String getHttpCookie(String name);
    
    /**
     * Get a Set of HTTP cookie names.
     * @return A set of the names of cookies from the current HTTP connection or null if there are none.
     */
    public Set<String> getHttpCookieSet();
    
    /**
     * Get an HTTP header from the current HTTP connection.
     * Only those headers defines in the HttpHeader property of the protocol definition
     * are available.
     * @param name   The name of the header
     * @return  The value of the header or null if the header is not set
     */
    public String getHttpHeader(String name);
    
    /**
     * Set an HTTP header field for the following sendHttpData().
     * <p>
     * This header is sent as a custom header file in the HTTP response.
     * @param name   The name of the header
     * @param value  The value of the header
     * @throws IllegalArgumentException if an onHttpData is not active
     */
    public void setHttpHeader(String name, String value);
    
    /**
     * Returns the user data associated with the connection.
     * <p>
     * Normally the ImaConnectionListener object returned from ImaPluginListener.onConnection 
     * defines the object associated with the connection, but this method returns another 
     * object which is attached to the connection.
     * @return user data object
     */
    public Object getUserData() ;
    
    /**
     * Sets the user data associated with the connection.
     * <p>
     * Normally the ImaConnectionListener object returned from ImaPluginListener.onConnection 
     * defines the object associated with the connection, but this method allows another 
     * object to be attached to the connection.
     */
    public void setUserData(Object userData) ;

    /**
     * Temporarily suspend message deliver.
     * <p>
     * This method is used to create a flow control and stop the delivery of messages to a 
     * connection so the the consumer can catch up.  The method resumeMessgeDelivery() should
     * be called when the receiver is ready to accept messages.
     */
    public void suspendMessageDelivery();

    /**
     * Resume message deliver.
     * <p>
     * This method is used to create a flow control and resume the delivery of messages to a 
     * connection so the the consumer can catch up.  
     */
    public void resumeMessageDelivery();

    /**
     * Create a transaction
     * <p>
     * A transaction allows a set of messages to be sent as an atomic set.
     * @param listener
     */
    public void createTransaction(ImaTransactionListener listener);
}
