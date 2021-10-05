/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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


package com.ibm.ism.mqtt;

/**
 * The IsmMqttListernerAsync interface is added as part of the MQTTv5 support.
 * This can be used for earlier versions of MQTT but you will not receive any
 * of the new information.
 */
public interface IsmMqttListenerAsync {
    /**
     * Callback when a message is received. 
     * @param msg The message which is received
     * @return An MQTT return code
     */
    public int onMessage(IsmMqttMessage msg);
    
    
    /**
     * Callback for completion of a publish, subscribe, or unsubscribe
     * @param result  The result object 
     */
    public void  onCompletion(IsmMqttResult result);
    
    
    /**
     * Callback on an error or disconnect.
     * This is called when the connection is disconnected after it is established.
     * If this happens on a synchronous method that method will also receieve an IOException.
     * If the disconnect happens when processing asynchronous messages, this is the only
     * indication of a connection failure.
     * @param result An object containing the result including the backtrace information.
     */
    public void  onDisconnect(IsmMqttResult result);
}
