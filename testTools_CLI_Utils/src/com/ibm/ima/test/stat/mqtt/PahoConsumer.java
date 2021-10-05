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
package com.ibm.ima.test.stat.mqtt;

import java.io.UnsupportedEncodingException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.test.stat.Config;
import com.ibm.ima.test.stat.ExecutionMonitor;
import com.ibm.ima.test.stat.ExecutionStatus;
import com.ibm.ima.test.stat.ExecutionStatus.CLIENT_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.STATUS_TYPE;



public class PahoConsumer implements Runnable, MqttCallback  {

	private Config config = null;
	private String clientId = null;
	private boolean verboseOutput = false;
	private int exitCode = 0;
	private ExecutionMonitor monitor = null;
	private ExecutionStatus exitStatus = null;
	long msgsConsumed = 0;

	private BlockingQueue<ConsumerMsg> msgQueue = new LinkedBlockingQueue<ConsumerMsg>();


	public PahoConsumer(Config config, String clientId, ExecutionMonitor monitor, boolean verboseOutput) {

		this.config = config;
		this.clientId = clientId;
		this.monitor = monitor;
		this.verboseOutput = verboseOutput;

		exitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.MQTT_CONSUMER, STATUS_TYPE.EXIT_STATUS);

	}

	private MqttClient getClient() throws MqttException {

		String hostname = config.getServer();
		String port = config.getPort();
		String protocol = "tcp";
		if (Config.isSsl()) {
			protocol = "ssl";
		}
		MqttClient client = new MqttClient(protocol + "://"+hostname+":"+port, clientId);
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


	public void run()  {

		MqttClient client = null;
		long startTime = 0;



		try {

			client = getClient();
			MqttConnectOptions conOpt = new MqttConnectOptions();
			conOpt.setCleanSession(config.isCleanStore());
			conOpt.setUserName(config.getUsername());
			if (config.getUsername() != null) {
				conOpt.setPassword(config.getPassword().toCharArray());
			}
			//	if (config.getSocketFactory() != null) {
			//		conOpt.setSocketFactory(config.getSocketFactory());
			//	}
			if (config.getMQTTSSLProps() != null) {
				conOpt.setSSLProperties(config.getMQTTSSLProps());
			}
			client.connect(conOpt);

			client.subscribe(config.getDestinationName(), config.getQos());


			ExecutionStatus consumerInitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.MQTT_CONSUMER, STATUS_TYPE.INITIALIZED_STATUS);
			monitor.reportStatus(consumerInitStatus);

			System.out.println("Paho Consumer :" + clientId + " subscribed to dest: '" + config.getDestinationName() + "'.");
			startTime = System.currentTimeMillis();

			if (config.getMsgsToConsume() <= 0) {

				long currentTime = System.currentTimeMillis();
				long endTime = currentTime + (config.getTotalSeconds() * 1000);

				while ((currentTime < endTime) ) {

					long timeLeft = endTime - currentTime;
					if (timeLeft > 0) {

						ConsumerMsg topicMsg = msgQueue.poll(timeLeft, TimeUnit.MILLISECONDS);
						if (topicMsg !=null) {
							msgsConsumed++;
							processMsg(topicMsg);
						}
					}

					currentTime = System.currentTimeMillis();

				}

			} else { 				

				while (msgsConsumed < config.getMsgsToConsume() && client.isConnected()) {

					ConsumerMsg topicMsg = msgQueue.take();
					msgsConsumed++;
					processMsg(topicMsg);

				}

			}


			exitCode = 0;

		} catch (Throwable t) {

			exitCode = 1;
			t.printStackTrace();
		} finally {

			long endTime = System.currentTimeMillis();
			closeClient(client);
			exitStatus.consumerExit(exitCode, startTime, endTime, msgsConsumed);
			monitor.reportStatus(exitStatus);
		}
	}



	private void processMsg(ConsumerMsg  topicMsg) throws Throwable  {



		if (verboseOutput) {
			System.out.println("Paho Consumer client : " + clientId + " message received...");
		}

		if (topicMsg.errorReceived()) {
			Throwable reason = topicMsg.getReason();
			if (reason != null) {
				throw reason;
			} else {
				throw new Exception("client has been shutdown...");
			}
		}

		if (verboseOutput) {
			MqttMessage msg = topicMsg.getMsg();
			if (msg != null) {

				String topic = topicMsg.getTopic();

				String msgText = "";
				byte[] msgTextArray = msg.getPayload();
				if (msgText != null) {
					try {
						msgText = new String(msgTextArray, "UTF8");
					} catch (UnsupportedEncodingException e) {
						e.printStackTrace();
					}
				}


				System.out.println("Paho Consumer client : " + clientId  + " message from topic " +    
						topic + "' : " + msgText);
				System.out.println("Paho Consumer client : " + clientId + " has consumed " + msgsConsumed + " messages.");



			} else {


				System.out.println("Paho Consumer client : " + clientId + " received a null message...");


			}	
		}


	}


	public void setVerboseOutput(boolean verboseOutput) {
		this.verboseOutput = verboseOutput;
	}

	public int getExitCode() {
		return exitCode;
	}

	@Override
	public void connectionLost(Throwable t) {

		ConsumerMsg msg = new ConsumerMsg(true, t);
		msgQueue.add(msg);

	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// TODO Auto-generated method stub


	}

	@Override
	public void messageArrived(String topic, MqttMessage msg) throws Exception {

		ConsumerMsg topicMsg = new ConsumerMsg(topic, msg);
		msgQueue.add(topicMsg);

	}

	private class ConsumerMsg {

		private String topic = null;
		private MqttMessage msg = null;
		private boolean errorReceived = false;
		private Throwable reason = null;

		public ConsumerMsg(String topic, MqttMessage msg) {
			this.topic = topic;
			this.msg = msg;
		}

		public ConsumerMsg(boolean errorReceived, Throwable reason) {
			this.errorReceived = errorReceived;
			this.reason = reason;
		}

		public boolean errorReceived() {
			return errorReceived;
		}

		public Throwable getReason() {
			return reason;
		}

		public String getTopic() {
			return topic;
		}

		public MqttMessage getMsg() {
			return msg;
		}


	}


}
