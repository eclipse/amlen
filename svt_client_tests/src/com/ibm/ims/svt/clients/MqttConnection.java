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
package com.ibm.ims.svt.clients;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;



public class MqttConnection {
	
	private String connectionString;
	
	public MqttConnection(String connection)
	{
		this.connectionString = connection;
		
	}

	/**
	 * This will use the default file persistence 
	 * @param clientID
	 * @return
	 * @throws MqttException
	 */
	public MqttClient getClient(String clientID) throws MqttException
	{
		return new MqttClient(connectionString, clientID);
		
	}
	
	/**
	 * This will use in memory persistence so you may lose messages if the application failes
	 * @param clientID
	 * @return
	 * @throws MqttException
	 */
	public MqttClient getClientInMemoryPersistence(String clientID) throws MqttException
	{
		return new MqttClient(connectionString, clientID, null);
		
	}
	
	
}
