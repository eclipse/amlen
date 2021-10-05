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
package com.ibm.ism.test.mqtt;

import java.io.IOException;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttMessage;

public class ReceiveThread implements Runnable {

	private IsmMqttConnection _connection;
	private long _timeout;
	private int _numMessages;
	private int _messagesReceived = 0;
	
	public ReceiveThread(IsmMqttConnection connection, long timeout, int numMessages) {
		
		_connection = connection;
		_timeout = timeout;
		_numMessages = numMessages;
	}
	
	public void run() {
		
		while (_messagesReceived < _numMessages) {
			IsmMqttMessage message = null;
			try {
				if (_timeout != 0) {
					message=_connection.receive(_timeout);
				}
				else {				
					message=_connection.receive();
				} 
			} catch (IOException e) {
					e.printStackTrace();
			}
			if (message != null) { 
				_messagesReceived++;
			}
			else { 
				System.out.println("Null message received.  The receive may have timed out.");
			}
		}
	}
	
	public int getNumReceived () {
		return _messagesReceived;
	}

}
