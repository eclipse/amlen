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

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.test.stat.Config;
import com.ibm.ima.test.stat.ExecutionMonitor;
import com.ibm.ima.test.stat.ExecutionStatus;
import com.ibm.ima.test.stat.ExecutionStatus.CLIENT_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.STATUS_TYPE;


/**
 * PahoProducer can be used to send a specific number of messages
 * to a given topic or simply perform a timed run sending messages
 * to a topic. PahoProducer is highly configurable - QoS level
 * may be set as well as how fast to send messages to a topic. 
 * 
 * Execution can be configured to stop when a failure is detected
 * or automatic recovery may be performed.
 *
 */
public class PahoProducer implements Runnable, MqttCallback {

	private Config config = null;
	private String clientId = null;
	private boolean verboseOutput = false;
	private ExecutionMonitor monitor = null;
	private ExecutionStatus exitStatus = null;
	private int exitCode = 0;
	private long msgsProduced = 0;
	private long goodSends = 0;
	private long badSends = 0;
	private long startTime = 0;
	private String hostname = null;
	private String port = null;
	private String protocol = "tcp";

	private MqttClient client = null;
	private MqttTopic topic = null;
	private MqttConnectOptions connOpts = null;

	/**
	 * Create a PahoProducer instance and initialize it.
	 * 
	 * @param config			Config instance with execution parameters
	 * @param clientId			A unique client ID for this instance
	 * @param monitor			An ExecutionMonitor instance that should be used
	 * 							for notification upon completion.
	 * @param verboseOutput		Used for debugging or verbose output
	 */
	public PahoProducer(Config config, String clientId, ExecutionMonitor monitor, boolean verboseOutput) {

		this.config = config;
		this.clientId = clientId;
		this.monitor = monitor;
		this.verboseOutput = verboseOutput;
		
		/*
		 * Set up the ExecutionStatus instance so reporting can
		 * be done after execution of this thread is done. This
		 * instance should be reported to the ExecutionMonitor when
		 * execution is complete.
		 */
		exitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.MQTT_PRODUCER, STATUS_TYPE.EXIT_STATUS);
		
		// initialize connection information....
		hostname = config.getServer();
		port = config.getPort();
		if (Config.isSsl()) {
			protocol = "ssl";
		}
		
		// set up a connection options instance - instance can be re-used
		connOpts = new MqttConnectOptions();
		connOpts.setCleanSession(config.isCleanStore());
		connOpts.setUserName(config.getUsername());
		if (config.getUsername() != null) {
			connOpts.setPassword(config.getPassword().toCharArray());
		}
		
		if (config.getMQTTSSLProps() != null) {
			connOpts.setSSLProperties(config.getMQTTSSLProps());
		}
	
	}

	/**
	 * Create an MqttClient connection and obtain an MqttTopic
	 * reference. 
	 * 
	 * @throws MqttException	If there was a problem...
	 */
	private void connect() throws MqttException {

		/*
		 * We really do not care about persistence with this execution.
		 * We are simply trying to do timed runs and specific runs. We
		 * do not want retries or messages being written to local disk
		 * with higher QoS values. 
		 * 
		 * So we use our own persistence - StatClientPersistence
		 */
		StatClientPersistence store = new StatClientPersistence();
		
		// get the client and connect
		client = new MqttClient(protocol + "://"+hostname+":"+port, clientId, store);
		client.setCallback(this);
		client.connect(connOpts);
		
		// get the topic reference
		String topicName = config.getDestinationName();
		topic = client.getTopic(topicName);

	}

	/**
	 * Disconnect the MqttClient and clean up nicely..
	 * 
	 */
	private void disconnect() {
		
		// just because...
		topic = null;
		
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

	/**
	 * Execute this instance of the PahoProducer. The run will
	 * either be timed or exact. Keep track of the run and report
	 * status via the monitor when done. Catch all exceptions so 
	 * execution ends nicely... 
	 */
	public void run()  {

		try {
			
			// first determine if it is a timed or exact run.
				
			if (config.getMsgsToSend() <= 0) {

				// timed run....
				timedPublish();
				
			} else {
				
				// publish exact number of messages
				exactPublish();
				
			}
			
		} catch (Throwable t) {
			// something unexpected happened...
			exitCode = 1;
			t.printStackTrace();
		} finally {
			// shutdown gracefully and format report...
			long endTime = System.currentTimeMillis();
			disconnect();
			exitStatus.producerExit(exitCode, startTime, endTime, msgsProduced, goodSends, badSends);
			monitor.reportStatus(exitStatus);	
		}

	}
	
	/**
	 * Perform a timed run. Simply send messages to a topic until the
	 * desired amount of time has expired.
	 * 
	 * @throws MqttException					mqtt problem...
	 * @throws UnsupportedEncodingException		some sort of issue with the payload...
	 * @throws InterruptedException				a really bad problem...
	 */
	private void timedPublish() throws MqttException, UnsupportedEncodingException, InterruptedException {
		
		// initialize connection and topic
		connect();

		// get the initial payload bytes...
		String payload = config.getPayload();
		byte[] payloadBytes = payload.getBytes("UTF8");
		
		// obtain and report some initialization information...
		System.out.println("Paho Producer '" +  clientId + "' ready for timed run to destination: '" + config.getDestinationName() + ".");
		System.out.println("Paho Producer '" +  clientId + "' payload size is " + payloadBytes.length +" bytes" );
		
		// get the start time for the execution - this will be used for the final report
		startTime = System.currentTimeMillis();
		
		// timing information for this run...
		long currentTime = System.currentTimeMillis();
		long endTime = currentTime + (config.getTotalSeconds() * 1000);

		// send until time desired has been reached....
		while (currentTime < endTime) {

			try {
					
				msgsProduced++;
				if ((msgsProduced % 100) == 0) {
					payload = config.getPayload();
					payloadBytes = payload.getBytes();
				}
				MqttMessage tmsg = new MqttMessage(payloadBytes);
				tmsg.setQos(config.getQos());
				MqttDeliveryToken token = topic.publish(tmsg);
				
				// may not really need to wait - just makes it easier for this application...
			 	token.waitForCompletion();
				goodSends++;

			} catch (MqttException me) {
				badSends++;

				if (config.isIgnoreErrors() == false) {
					throw me;
				} else if (verboseOutput) {
					me.printStackTrace();
				}
			}

			Thread.sleep(config.getSendInterval());
			currentTime = System.currentTimeMillis();

		}
		
	}

	/**
	 * Publish an exact number of messages to a given topic.
	 * This number must be exact - that is do not try and re-send
	 * a message more than once. We need exact numbers that are
	 * predictable for use in test automation. 
	 * 
	 * @throws MqttException					mqtt problem...
	 * @throws UnsupportedEncodingException		some sort of issue with the payload...
	 * @throws InterruptedException				a really bad problem...
	 */
	private void exactPublish() throws MqttException, UnsupportedEncodingException, InterruptedException {
		
		// initialize connection and topic
		connect();
		
		// get the initial payload bytes...
		String payload = config.getPayload();
		byte[] payloadBytes = payload.getBytes("UTF8");
		
		// obtain and report some initialization information...
		System.out.println("Paho Producer '" +  clientId + "' ready for exact publish to destination: '" + config.getDestinationName() + ".");
		System.out.println("Paho Producer '" +  clientId + "' payload size is " + payloadBytes.length +" bytes" );
		
		// get the start time for the execution - this will be used for the final report
		startTime = System.currentTimeMillis();
		
		/*
		 * Attempt to send a specific number of messages to a topic. 
		 * This loop will continue to produce messages until a 
		 * specified number of messages to send has been reached. The
		 * loop does not guarantee that all messages have been sent
		 * successfully - it just makes sure that the attempt was
		 * successful.  
		 */
		for (int i = 0; i < config.getMsgsToSend(); i++) {
			
			try {
				
				// we have a good message and client is connected...
				msgsProduced++;

				MqttMessage tmsg = new MqttMessage(payloadBytes);
				tmsg.setQos(config.getQos());
				MqttDeliveryToken token = topic.publish(tmsg);
				
				// always wait since the client is asynchronous - need to be predictable
			 	token.waitForCompletion();
			 	goodSends++;
			 	
			} catch (MqttException me) {
				// something went wrong when sending the message...
				badSends++;
				if (config.isIgnoreErrors() == false) {
					throw me;
				} else if (verboseOutput) {
					me.printStackTrace();
				}
			} 
			Thread.sleep(config.getSendInterval());
		}
		
	}

	@Override
	public void connectionLost(Throwable cause) {
		
		/*
		 * A disconnect might happen if the destination is full.
		 * This seems to happen randomly. Our default behavior is
		 * to try and reconnect.
		 */
		if (verboseOutput) {
			System.out.println("Connection lost");
			cause.printStackTrace();
		}
		
		// just to clean up a bit...
		disconnect();
		
		// try to re-connect...
		try {
			connect();
		} catch (Exception ex) {
			ex.printStackTrace();
		}

	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void messageArrived(String arg0, MqttMessage arg1) throws Exception {
		// TODO Auto-generated method stub
		
	}
	
}
