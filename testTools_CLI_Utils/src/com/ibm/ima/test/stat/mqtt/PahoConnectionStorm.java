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

import java.util.Properties;

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



public class PahoConnectionStorm implements Runnable, MqttCallback  {

	private Config config = null;
	private String clientId = null;
	private boolean verboseOutput = false;
	private int exitCode = 0;
	private ExecutionMonitor monitor = null;
	private ExecutionStatus exitStatus = null;




	public PahoConnectionStorm(Config config, String clientId, ExecutionMonitor monitor, boolean verboseOutput) {

		this.config = config;
		this.clientId = clientId;
		this.monitor = monitor;
		this.verboseOutput = verboseOutput;

		exitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.MQTT_CONN_STORM, STATUS_TYPE.EXIT_STATUS);

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

		int goodConns = 0;
		Properties connProps = config.getMQTTSSLProps();
		boolean cleanStore = config.isCleanStore();
		String username = config.getUsername();
		String password = config.getPassword();
		startTime = System.currentTimeMillis();
		
		try {

			int numConnections = config.getConnStorm();

			for (int i=0; i<numConnections; i++) {
				client = getClient();
				MqttConnectOptions conOpt = new MqttConnectOptions();
				conOpt.setCleanSession(cleanStore);

				if (username != null) {
					conOpt.setUserName(username);
					conOpt.setPassword(password.toCharArray());
				}

				if (connProps != null) {
					conOpt.setSSLProperties(connProps);
				}

				client.connect(conOpt);
				closeClient(client);
				client = null;
				goodConns++;

			}

			exitCode = 0;

		} catch (Throwable t) {

			exitCode = 1;
			t.printStackTrace();
		} finally {

			long endTime = System.currentTimeMillis();
			closeClient(client);
			client = null;
			exitStatus.connStormExit(exitCode, startTime, endTime, goodConns);
			monitor.reportStatus(exitStatus);
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



	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// TODO Auto-generated method stub


	}

	@Override
	public void messageArrived(String topic, MqttMessage msg) throws Exception {




	}



}
