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
 * The ImaPluginListener defines a set of callback methods which are called by MessageSight.  
 * The plug-in must implement these methods.
 */
public interface ImaPluginListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Callback when the plug-in is loaded.
     * 
     * This is called early in the startup of the plug-in process.  Methods which use
     * messaging function will not work correctly until the startMessaging callback is invoked.
     * <p>
     * It is expected that the plug-in will save the ImaPlugin object for later use.
     * 
     * @param plugin  The plug-in object which contains methods which the plug-in can call for services. 
     */
    public void initialize(ImaPlugin plugin);
    
    /**
     * Callback to request authentication of a connection.
     * <p>
     * Note that this function is not currently implemented and will not be called.  For
     * future compatibility the plug-in should return false.
     * <p>
     * This connection may have been accepted by this or any other protocol.
     * <p>
     * The connection is blocked (will not process data) until it has been authenticated.
     * <p>
     * If this method returns true, the plug-in is required to call the ImaConnection methods authenticate()
     * or setIdenity() with an auth option on the specified connection. 
     * This can be done either before returning from this callback or later.
     * If this method returns false, authentication processing is continued using other authenticators.
     * 
     * @param connect  The connection to authenticate
     * @return true if this plug-in has or will authenticate this connection, false otherwise.
     */
    public boolean onAuthenticate(ImaConnection connect);


    /**
     * Callback when a new connection is detected which might be for this plug-in.
     * 
     * This method allows the plug-in to choose whether this connection belongs to it.
     * The check should be as specific as possible with the fewest possible bytes.
     * <p>
     * If this callback returns 0 the connection is accepted.  If it returns a negative value the
     * connection is not accepted.  If it returns a positive value, this indicates the number of 
     * bytes of data it needs to determine if the connection belongs to it.
     * If another plug-in accepts the connection before more data is available, this method 
     * will not be reinvoked.
     * <p>
     * The data commonly has the first record sent by the client, but is only guaranteed to have 
     * a single byte.
     * <p>
     * Before returning, this method may set the protocol name in the connection using the 
     * connect.setProtocol() method.  If it does not do so, the name of the plug-in will be
     * used as the protocol.
     * 
     * @param connect  The connection object
     * @param data     Data from the first packet received
     * @return 0=accept, <0=not accepted, >0=amount of data needed to accept
     */
    public int onProtocolCheck(ImaConnection connect, byte[] data);
    
    /**
     * Callback for a new connection with a known protocol.
     * <p>
     * This is called when a connection is known to be accepted by this plug-in. This can either be due to returning a
     * zero value from onProtocolCheck(), or using a framer like WebSockets which declares the protocol.
     * <p>
     * If the plug-in is unwilling to accept this connection this method should return null.
     * 
     * @param connect The connection object
     * @param protocol The protocol of the connection
     * @return A connection listener object associated with this connection
     */
    public ImaConnectionListener onConnection(ImaConnection connect, String protocol);
    
    /**
     * Callback for a configuration update.
     * <p>
     * When a protocol is created it has a set of properties which it can receive using
     * ImaPlugin.getConfig().  This method allows the plug-in to be notified when
     * a property changes.
     * <p>
     * Note: This method is not currently invoked.
     * 
     * @param name     The name of the property
     * @param subname  If the base property is a Map object, the name of the inner property.
     * @param value    The value of the property
     */
    public void onConfigUpdate(String name, Object subname, Object value);
    
    /**
     * Callback when messaging is started within the server.
     * 
     * Some actions cannot be taken by the plug-in before messaging starts.
     * @param active  Specifies whether this server instance is the active peer 
     *                or the backup in a high availability pair, true for active
     *                and false for backup.
     */
    public void startMessaging(boolean active);
    
    /**
     * Callback to inform the plug-in it should release resources.
     * 
     * There is no guarantee that this is called, but either it will be called 
     * or the JVM running the plug-in will be terminated.
     * @param reason  The reason for the close as a return code.  0=normal
     */
    public void terminate(int reason);

}
