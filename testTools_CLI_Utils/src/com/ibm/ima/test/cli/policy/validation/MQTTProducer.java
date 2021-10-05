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
package com.ibm.ima.test.cli.policy.validation;



import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;



public class MQTTProducer implements  MqttCallback, PolicyValidator {

	private static final String TCP = "tcp";

	private String clientId = null;
	private String server = null;
	private String port = null;
	private String topicString = null;
	private String username = null;
	private String password = null;
	private String msgToSend = null;
	private ValidationException error = null;


	public MQTTProducer(String server, String port, String topicString) {

		this.server = server;
		this.port = port;
		this.topicString = topicString;

	}

	private MqttClient getClient() throws MqttException {

		MqttClient pahoConn = new MqttClient(TCP + "://"+server+":"+port, clientId);
		pahoConn.setCallback(this);

		return pahoConn;

	}

	private void shutdownClient(MqttClient client) {

		try {
			client.disconnect();
		} catch (Throwable t) {
			// do not care about exception here...
		}

		try {
			client.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}


	public void startValidation()  {

		MqttClient client = null;

		try {

			client = getClient();
			MqttConnectOptions conOpt = new MqttConnectOptions();
			conOpt.setCleanSession(true);
			conOpt.setUserName(username);
			if (username != null) {
				conOpt.setPassword(password.toCharArray());
			}

			client.connect(conOpt); 


			byte[] payloadBytes = msgToSend.getBytes("UTF8");

			MqttTopic topic = client.getTopic(topicString);

			MqttMessage tmsg = new MqttMessage(payloadBytes);
			tmsg.setQos(0);
			MqttDeliveryToken token = topic.publish(tmsg);
			token.waitForCompletion();
			
		} catch (MqttException me) {
			if (me.getReasonCode() == 32103 || me.getReasonCode()== 32109) {
				error = new ValidationConnException(me);
			} else if (me.getReasonCode() == 5) {
				error = new ValidationAuthException(me);
			} else {
				error = new ValidationException(me);
			}

		} catch (Throwable t) {
			error = new ValidationException(t);
		
		} finally {
			shutdownClient(client);
		}



	}



	@Override
	public void connectionLost(Throwable arg0) {

	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// TODO Auto-generated method stub

	}

	@Override
	public void messageArrived(String arg0, MqttMessage arg1) throws Exception {
		// TODO Auto-generated method stub

	}

	public void setClientId(String clientId) {
		this.clientId = clientId;
	}

	public void setServer(String server) {
		this.server = server;
	}

	public void setPort(String port) {
		this.port = port;
	}


	public void setUsername(String username) {
		this.username = username;
	}

	public void setPassword(String password) {
		this.password = password;
	}


	public ValidationException getError() {
		return error;
	}


	public void setDestinationName(String destinationName) {
		topicString = destinationName;
		
	}


	public void setValidationMsg(String msg) {
		this.msgToSend = msg;
		
	}


	public void stopExecution(boolean stopExecution) {
		// no-op
		
	}




}
