/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
 * Implements the provider specific public classes for the IBM MessageSight JMS client.
 * <p>
 * Most of the interfaces used in JMS are defined by the common JMS
 * interfaces.  However, the JMS specification does not include the
 * classes and interfaces needed to configure the JMS client.
 * <p>
 * See <a href="http://download.oracle.com/javaee/6/api/javax/jms/package-summary.html" >
 * The JMS API documentation</a> for information about the JMS classes and methods.
 * 
 * <h2>Message properties extensions</h2>
 * <p>
 * IBM MesasgeSight defines a small set of provider specific properties on messages.  These are not real message properties
 * but the properties interfaces are used to set and retrieve information about the message.
 * <p>
 * When sending a message, the application can specify that a copy of a message should be retained so that it can be sent
 * to future subscribers of the topic. This is done in the IBM MessageSight JMS client by setting the message property 
 * JMS_IBM_Retain to the value 1.  This property is of type integer.  Any value other than 1 means that the message is not retained.
 * Retain has no meaning when doing point-to-point messaging (queues) and is ignored when a message is sent to a queue.
 * <p>  
 * If a message is received from a retained source, the message property JMS_IBM_Retain is set to the value 1, 
 * and is set to 0 otherwise. The returned value of JMS_IBM_Retain will always be an integer with the value of either 0 or 1. 
 * Before a message is sent, the value returned by JMS_IBM_Retain indicates the requested setting. 
 * When a message is received from a queue, the value will always be 0.
 * <p>
 * To set retained on a message do:
 * <pre>
 *     msg.setIntProperty("JMS_IBM_Retain", 1);
 * </pre> 
 * To see if the message came from a retained source do:
 * <pre>
 *     retained = msg.getIntProperty("JMS_IBM_Retain");
 * </pre>
 * <p>
 * When a message is received, IBM MessageSight creates an acknowledge sequence number which indicates the sequence within
 * the session.  This can be used for debugging and is available to the application as the message property IBM_JMS_ACK_SQN.  
 * This property cannot be set, and is only available after a message is received.  The type of the IBM_JMS_ACK_SQN property
 * is long.
 * <pre>
 *     acksqn = msg.getLongProperty("JMS_IBM_ACK_SQN");
 * </pre>
 */
package com.ibm.ima.jms;

// class xxx  {}
