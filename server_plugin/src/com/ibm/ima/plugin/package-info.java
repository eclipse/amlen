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

/**
 * Implements a set of interfaces defining the Java plug-in mechanism for IBM MessageSight.  These
 * interfaces combined with developer implementations of two listener interfaces allow you to 
 * extend protocol support in IBM MessageSight beyond the natively supported protocols.
 * <p>
 * A Java plug-in can be used to implement any combination of the following:
 * <ul>
 * <li>Define one or more protocols to accept from a TCP, UDP, or WebSockets connection
 * <li>Send messages to the MessageSight server and set up subscriptions which receive messages
 * from a MessageSight server.
 * <li>Authenticate a connection
 * <li>Run JavaSE code constrained by a security manager. 
 * </ul>
 * <p>
 * These functions can be combined together to easily define a messaging bridge or to map from
 * vendor or industry specific protocols.
 * <p>
 * This package primarily contains interfaces.  A Java plug-in to IBM MessageSight defines
 * a class implementing ImaPluginListener which is instantiated by IBM MessageSight.
 * The initialize() method is invoked passing in the ImaPlugin object.  The IBM MessageSight
 * server communicates with the plug-in by calling methods in ImaPluginListener and the
 * plug-in invokes methods in the ImaPlugin object to communicate with IBM MessageSight.
 * Additionally, to enable client connections, a Java plug-in to IBM MessageSight defines
 * a class implementing ImaConnectionListener.  The callback methods implemented for
 * the ImaConnectionListener interface permit clients using the plug-in protocol to 
 * send messages to and receive messages from IBM MessageSight via the target protocol.
 * <p>
 * 
 */
package com.ibm.ima.plugin;

// class xxx  {}
