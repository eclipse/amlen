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
package com.ibm.ima.mqcon.resiliency;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;



public class MqttConnection {
	
	private String connectionString;
	
	
	public MqttConnection(String connection)
	{
		this.connectionString = connection;
		
	}


	public MqttClient getClient(String clientID) throws MqttException
	{
		return new MqttClient(connectionString, clientID);
		
	}
	
/*	public MqttClient getHAClient(String clientID, MqttHAFilePersistence persistenceClass) throws MqttException
	{
		return new MqttClient(connectionString, clientID, persistenceClass);
		
	}*/
}
