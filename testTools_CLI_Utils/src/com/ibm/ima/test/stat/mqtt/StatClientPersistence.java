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
package com.ibm.ima.test.stat.mqtt;

import java.util.Enumeration;
import java.util.Hashtable;

import org.eclipse.paho.client.mqttv3.MqttClientPersistence;
import org.eclipse.paho.client.mqttv3.MqttPersistable;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;

/**
 * StatClientPersistence is an MQTT persistence object that 
 * really does nothing. Since we do not care about persistence
 * at higher QoS levels with the automation stat application
 * the implementation of the MqttClientPersistence interface
 * pretty much does nothing...
 *
 */
public class StatClientPersistence implements MqttClientPersistence {
	
	/**
	 * create a new instance of StatClientPersistence
	 */
	public StatClientPersistence() {
		
	}
	
	@SuppressWarnings("rawtypes")
	private Hashtable cache = new Hashtable();

	@Override
	public void clear() throws MqttPersistenceException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void close() throws MqttPersistenceException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean containsKey(String arg0) throws MqttPersistenceException {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public MqttPersistable get(String arg0) throws MqttPersistenceException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Enumeration keys() throws MqttPersistenceException {

		/*
		 * The only thing this implementation does - just to avoid
		 * a NPE when it is called by the MqttClient.
		 * 
		 * return enumeration of empty cache...
		 */
		return cache.keys();
	}

	@Override
	public void open(String arg0, String arg1) throws MqttPersistenceException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void put(String arg0, MqttPersistable arg1)
			throws MqttPersistenceException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void remove(String arg0) throws MqttPersistenceException {
		// TODO Auto-generated method stub
		
	}

}
