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
package com.ibm.ima.test.cli.policy.validation;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;


public class MQTTConsumer implements Runnable, MqttCallback, PolicyValidator  {

	private static final String TCP = "tcp";

	private String clientId = null;
	private String server = null;
	private String port = null;
	private String destinationName = null;
	private String username = null;
	private String password = null;
	private String expectedMsg = null;
	private long timeout = 5000;
	private BlockingQueue<String> taskMonitor = null;
	ValidationException error = null;
	boolean stopExecution = false;

	private BlockingQueue<MQTTConsumerMessage> msgQueue = new LinkedBlockingQueue<MQTTConsumerMessage>();


	public MQTTConsumer(String server, String port, String topicString, BlockingQueue<String> taskMonitor) {

		this.server = server;
		this.port = port;
		this.destinationName = topicString;
		this.taskMonitor = taskMonitor;

	}

	private MqttClient getClient() throws MqttException {

		MqttClient client = new MqttClient(TCP + "://"+server+":"+port, clientId);
		client.setCallback(this);

		return client;

	}


	private void closeClient(MqttClient client) {

		try {
			client.disconnect();
		} catch (Throwable t) {

		}

		try {
			client.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}

	public void startValidation() {
		Thread t = new Thread(this);
		t.start();
	}

	public void run()  {

		MqttClient client = null;
		boolean msgProcessed = false;

		try {

			client = getClient();
			MqttConnectOptions conOpt = new MqttConnectOptions();
			conOpt.setCleanSession(true);
			conOpt.setUserName(username);
			
			if (username != null) {
				conOpt.setPassword(password.toCharArray());
			}
			
			client.connect(conOpt);
			client.subscribe(destinationName, 0);
			taskMonitor.add("subscribed");

			long currentTime = System.currentTimeMillis();
			long endTime = currentTime + (timeout);

			while ((currentTime < endTime) && (stopExecution == false)) {

				long timeLeft = endTime - currentTime;
				if (timeLeft > 0) {

					MQTTConsumerMessage topicMsg = msgQueue.poll(100, TimeUnit.MILLISECONDS);
					if (topicMsg !=null) {
						System.out.println("polling...");
						processMsg(topicMsg);
						taskMonitor.add("received");
						msgProcessed = true;
						break;
					}
				}

				currentTime = System.currentTimeMillis();
				
			}

			if (msgProcessed == false) {
				throw new ValidationTimeOutException("MQTT consumer has timed out....");
			}
			

		} catch (MqttException me) {
			if (me.getReasonCode() == 32103 || me.getReasonCode()== 32109) {
				error = new ValidationConnException(me);
			} else if (me.getReasonCode() == 5) {
				error = new ValidationAuthException(me);
			} else {
				error = new ValidationException(me);
			}

			taskMonitor.add("failed");

		} catch (ValidationTimeOutException vtoe) {
			error = vtoe;
			taskMonitor.add("failed");
		} catch (Throwable t) {
			error = new ValidationException(t);
			taskMonitor.add("failed");
		} finally {
			closeClient(client);
		}
	}



	private void processMsg(MQTTConsumerMessage  topicMsg) throws Throwable  {


		if (topicMsg.errorReceived()) {
			Throwable reason = topicMsg.getReason();
			if (reason != null) {
				throw reason;
			} else {
				throw new Exception("client has been shutdown...");
			}
		}


		MqttMessage msg = topicMsg.getMsg();

		byte[] msgTextArray = msg.getPayload();

		String msgText = new String(msgTextArray, "UTF8");
		if (msgText.equals(expectedMsg) == false) {
			throw new Exception("Message expected [" + expectedMsg + "] message received [" + msgText + "]");
		}

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

	public void setDestinationName(String destinationName) {
		this.destinationName = destinationName;
	}

	public void setUsername(String username) {
		this.username = username;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public void setTimeout(long timeout) {
		this.timeout = timeout;
	}


	public void stopExecution(boolean stopExecution) {
		this.stopExecution = stopExecution;
	}

	public ValidationException getError() {
		return error;
	}

	public void setValidationMsg(String msg) {
		this.expectedMsg = msg;

	}

	@Override
	public void connectionLost(Throwable t) {

		MQTTConsumerMessage msg = new MQTTConsumerMessage(true, t);
		msgQueue.add(msg);

	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// TODO Auto-generated method stub

	}

	@Override
	public void messageArrived(String topic, MqttMessage msg) throws Exception {

		MQTTConsumerMessage topicMsg = new MQTTConsumerMessage(msg);
		msgQueue.add(topicMsg);

	}

	private class MQTTConsumerMessage {


		private MqttMessage msg = null;
		private boolean errorReceived = false;
		private Throwable reason = null;

		public MQTTConsumerMessage(MqttMessage msg) {
			this.msg = msg;
		}

		public MQTTConsumerMessage(boolean errorReceived, Throwable reason) {
			this.errorReceived = errorReceived;
			this.reason = reason;
		}

		public boolean errorReceived() {
			return errorReceived;
		}

		public Throwable getReason() {
			return reason;
		}


		public MqttMessage getMsg() {
			return msg;
		}


	}


}
