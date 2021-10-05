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

import java.util.Map;

/**
 * A plug-in interface defines a set of methods which can be invoked for this plug-in.
 * 
 * The object is sent to the plug-in on the initialize() callback. 
 */
/* TODO: do we need to expose more of the properties of the plug-in */
public interface ImaPlugin {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    /** The log entry is critical */
    public static final int LOG_CRIT     = 1;
    /** The log entry represents an error */
    public static final int LOG_ERROR    = 2;
    /** The log entry represents a warning */
    public static final int LOG_WARN     = 3;
    /** The log entry is important but is not an error */
    public static final int LOG_NOTICE   = 4;
    /** The log entry is informational */
    public static final int LOG_INFO     = 5;
    
    /**
     * Create an internal virtual connection.
     * <p>
     * Most plug-in methods are done in service of a connection.  In most cases the 
     * connection results from a client connecting to the IBM MessageSight server, but
     * in some cases the plug-in needs to create its own virtual connection.
     * <p>
     * An internal connection is used as a virtual connection when there is no physical connection
     * created by a client connecting to the IBM MessageSight server.  An internal connection
     * can be used to subscribe and receive messages, but no data will come on the connection
     * and no data may be sent to the connection. 
     * <p>
     * At some point after making this call the plug-in will receive an OnConnection() for this
     * connection, and must set identity and authenticate before the connection can be used.
     * This is commonly done during the zero length onData callback which follows the onConnection.
     * It is common for a plug-in to self-authenticate virtual connections but it must still
     * call the setIdentity or authenticate methods and receive a onConnected response before
     * the connection can be used to subscribe and send messages. 
     * <p>
     * A virtual connection must have a unique clientID.  If the virtual connection does not have 
     * any durable subscriptions is can generate a unique clientID which is normally done by starting
     * the clientID with an underscore, the second character not being an underscore, and 
     * containing some number of random characters or digits.  A virtual connection may use a clientID
     * starting with two underscores which indicates a system clientID.  It must however be careful 
     * to avoid any conflict with the clientID encodings of other system components.
     * <p>
     * A virtual connection is associated with an endpoint.  THe endpoint must exist but it is not
     * requiered to be enabled.  The endpoint allows messaging policies to be attached and provides
     * for statistics.
     * <p>
     * @param protocol   The name of the protocol
     * @param endponit   The name of the endpoint used for authentication
     * 
     * @return The connection which is created
     */
    public ImaConnection createConnection(String protocol, String endpoint);
    
    /**
     * Create a message.
     * <p>
     * Create a message of the specified message type.  Additional ImaMessage methods are then
     * called to set the contents of the message.
     * 
     * @param mtype       The message type of the message
     * @return The message which is created
     */
    public ImaMessage createMessage(ImaMessageType mtype);
    
    /**
     * Get the plug-in configuration properties.
     * <p>
     * The configuration properties come from the plugin.json definition file for the plug-in.
     * and can be modified later.
     * <p>
     * Note: Currently, modifying the original configuration properties is not supported.
     * If you call this method, only the values set at install time will be used.
     * 
     * @return A map object with a set of configuration properties
     */
    public Map<String,Object>  getConfig();
    
    /**
     * Get the plug-in name.
     * @return The name of the plug-in
     */
    public String getName();
    
    /**
     * Get the protocol family of the plug-in. 
     * <p>
     * The protocol family is used define authorization and is commonly the name
     * for a group of related protocols. This could be for instance multiple versions of a protocol. A plug-in defines a
     * single protocol family, but can define multiple protocols.
     * 
     * @return The protocol family of the plug-in.
     */
    public String getProtocolFamily();
    
    /**
     * Get the author field of the plug-in.
     * @return The author field or null if it is not set
     */
    public String getAuthor();

    /**
     * Get the version field of the plug-in.
     * @return The version field or null if it is not set
     */
    public String getVersion();

    /**
     * Get the copyright field of the plug-in.
     * @return The copyright field or null if it is not set
     */
    public String getCopyright();

    /**
     * Get the build field of the plug-in.
     * @return The build field or null if it is not set
     */
    public String getBuild();

    /**
     * Get the description field of the plug-in.
     * @return The description field or null if it is not set
     */
    public String getDescription();

    /**
     * Get the license field of the plug-in.
     * @return The license field or null if it is not set
     */
    String getLicense();

    /**
     * Get the title field of the plug-in.
     * @return The title field or null if it is not set
     */
    String getTitle();
    
    /*
     * Put an entry into the log.
     * <p>
     * This log entry is not associated with a connection.
     * 
     * @param msgid     The message ID that can be any string.  IBM MessageSight uses an alphabetic string followed by four digits.
     * @param level     The level or severity of the log message (see LOG_CRIT to LOG_INFO)
     * @param category  The category of the message.  This is placed in the log.  The categories "Connection", "Admin",
     *                  and "Security" will cause the entry to go into the associated log.  All other categories will appear in the default log.
     * @param msgformat The format of the message in Java MessageFormat string form
     * @param args      A variable array of replacement values for the message format
     */
    public void log(String msgid, int level, String category, String msgformat, Object ... args);
    
    /**
     * Check if trace is allowed at the specified level.
     * <p>
     * @param level the trace level
     * @return true if the level is allowed to trace
     */
    public boolean isTraceable(int level);
    
    /**
     * Trace with a level specified.
     * 
     * @param level the level (1-9)
     * @param message the message
     */
    public void trace(int level, String message);
    
    /**
     * Trace unconditionally.
     *
     * @param message - the message
     */
    public void trace(String message);
    
    /**
     * Unconditionally write an exception stack trace to the trace file.
     * @param ex   The exception
     */
    public void traceException(Throwable ex);
    
    /**
     * Write an exception stack trace to the trace file at a specified level.
     * @param level The level
     * @param ex   The exception
     */
    public void traceException(int level, Throwable ex);
    
}
