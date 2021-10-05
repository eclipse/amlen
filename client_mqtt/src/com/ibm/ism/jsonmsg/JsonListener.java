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

package com.ibm.ism.jsonmsg;

/**
 * A listener for ISM MQTT messages must implement this callback.
 * The listener must be set before the connection is made.
 */
public interface JsonListener {
	/**
	 * Callback when a message is received. 
	 * @param msg The message which is received
	 */
    public void  onMessage(JsonMessage msg);
    
    /**
     * Callback on an error or disconnect.
     * This is called when the connection is disconnected after it is established.
     * If this happens on a synchronous method that method will also receieve an IOException.
     * If the disconnect happens when processing asynchronous messages, this is the only
     * indication of a connection failure.
     * @param rc  An error indicator.  These are commonly the WebSockets errors.
     * @param e   An object contianig the reason code and backtrace information.
     */
	public void  onDisconnect(int rc, Throwable e);
}
