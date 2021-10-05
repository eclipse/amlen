/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.mqcon.msgconversion;

import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.basetest.MQConnectivityTest;

public abstract class MqttHelper extends MQConnectivityTest{
	
	public abstract void closeConnections();
	
	public abstract void callback(MqttMessage msg);

}
