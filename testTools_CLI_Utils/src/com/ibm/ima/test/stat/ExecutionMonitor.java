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
package com.ibm.ima.test.stat;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import com.ibm.ima.test.stat.ExecutionStatus.CLIENT_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.STATUS_TYPE;


//import com.ibm.ima.test.stat.jms.Producer;

/**
 * ExecutionMonitor is used to keep track of all producer and
 * consumer threads. This is done using a BlockingQueue. Before
 * an ExecutionMonitor thread is started a List of client ids
 * must be provided. As each thread that corresponds to a client
 * id terminates it should report status to the ExecutionMonitor
 * thread.
 * 
 * The ExecutionMonitor thread will terminate the application once
 * all producer and consumer threads have reported status. If any
 * one of the producers or consumer provided a non-zero exit code
 * the application will be terminated with an exit code of 1.
 *
 */
public class ExecutionMonitor implements Runnable {

	// List of client ids to keep track of - all consumers and producers
	List<String> allClientsList = new ArrayList<String>();

	List<String> consumerClientList = new ArrayList<String>();

	List<Runnable> producerList = null;

	// ExecutionMonitor will process status reports from the BlockingQueue
	private BlockingQueue<ExecutionStatus> statusQueue = new LinkedBlockingQueue<ExecutionStatus>();

	private StringBuffer consumerStatus = new StringBuffer(ExecutionStatus.CONSUMER_HEADER);
	private StringBuffer producerStatus = new StringBuffer(ExecutionStatus.PRODUCER_HEADER);
	private StringBuffer connStormStatus = new StringBuffer(ExecutionStatus.CONN_STORM_HEADER);

	private int exitCode = 0;
	private boolean shutdown = false;
	private boolean producersStarted = false;
	private boolean saveSummary = false;
	
	private long totalProduced = 0;
	private long totalConsumed = 0;
	
	private long connectionStormTotal = 0;
	private long executionStartTime = 0;
	private long executionEndTime = 0;

	/**
	 * Default ExecutionMonitor constructor
	 */
	public ExecutionMonitor(boolean saveSummary) {
		this.saveSummary = saveSummary;
		executionStartTime = System.currentTimeMillis();
	}

	/**
	 * Consumer and producer threads use reportStatus to send exit
	 * status information to the ExecutionMonitor thread.
	 * 
	 * @param status  The exit status object of the producer or consumer
	 */
	public void reportStatus(ExecutionStatus status) {

		try {
			statusQueue.put(status);
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	/**
	 * Run the ExecutionMonitor thread. Shutdown only after all
	 * producers and consumers have reported status..
	 */
	public void run() {

		try {

			ExecutionStatus statusItem = null;

			while (shutdown == false) {

				if (consumerClientList.size() == 0 && producersStarted == false) {
					producersStarted = true;
					startProducers();
				}

				statusItem = statusQueue.take();

				if (statusItem.getStatusType() == STATUS_TYPE.INITIALIZED_STATUS) {
					processInit(statusItem);

				} else {
					processExit(statusItem);
				}



				if (allClientsList.size() == 0) {
					shutdown = true;
				}

			}

		} catch (InterruptedException ire) {
			ire.printStackTrace();
		}

		executionEndTime = System.currentTimeMillis();
		printReport();
		if (exitCode == 0) {
			System.out.println("\nTest Success!\n");
		}
		System.exit(exitCode);

	}

	private void processExit(ExecutionStatus statusItem) {

		if (statusItem.getExitCode() != 0) {
			exitCode = statusItem.getExitCode();
		}

		allClientsList.remove(statusItem.getClientId());
		consumerClientList.remove(statusItem.getClientId());
		CLIENT_TYPE clientType = statusItem.getClientType();

		if (CLIENT_TYPE.JMS_CONSUMER.equals(clientType) || CLIENT_TYPE.MQTT_CONSUMER.equals(clientType))  {
			consumerStatus.append(statusItem.getConsumerExitMsg());
			totalConsumed += statusItem.getMsgsConsumed();
		} else if (CLIENT_TYPE.JMS_PRODUCER.equals(clientType) || CLIENT_TYPE.MQTT_PRODUCER.equals(clientType)) {
			producerStatus.append(statusItem.getProducerExitMsg());
			totalProduced += statusItem.getGoodSends();
		} else if (CLIENT_TYPE.MQTT_CONN_STORM.equals(clientType)) {
			connStormStatus.append(statusItem.getConnStormExitMsg());
			connectionStormTotal += statusItem.getGoodConns();
		}

	}


	private void processInit(ExecutionStatus statusItem) {

		CLIENT_TYPE clientType = statusItem.getClientType();

		if (CLIENT_TYPE.JMS_CONSUMER.equals(clientType) || CLIENT_TYPE.MQTT_CONSUMER.equals(clientType)) {
			consumerClientList.remove(statusItem.getClientId());
		}
	}

	/**
	 * Print a pretty summary report
	 */
	private void printReport() {

		if (producerStatus.length() > 0) {
			System.out.println("-------------- Producer Summary --------------");
			System.out.println("\n" + producerStatus.toString() + "\n");
		}

		if (consumerStatus.length() > 0) {
			System.out.println("-------------- Consumer Summary --------------");
			System.out.println("\n" + consumerStatus.toString() + "\n");
		}
		
		if (connStormStatus.length() > 0) {
			System.out.println("-------------- Connection Storm Summary --------------");
			System.out.println("\n" + connStormStatus.toString() + "\n");
		}
		
		long totalRunTime = (executionEndTime - executionStartTime) / 1000;
		System.out.println("\nTotal Execution Time: " + totalRunTime);
		System.out.println("Total Messages Published: " + totalProduced);
		System.out.println("Total Messages Consumed: " + totalConsumed);
		System.out.println("Connection Storm Total: " + connectionStormTotal);

		if (saveSummary) {
			try {
				writeStatusToFile("producer.csv", producerStatus);
				writeStatusToFile("consumer.csv", consumerStatus);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

	}

	public static void writeStatusToFile(String pFilename, StringBuffer pData) throws IOException {  
		BufferedWriter out = new BufferedWriter(new FileWriter(pFilename));  
		out.write(pData.toString());  
		out.flush();  
		out.close();  
	} 

	/**
	 * Add a client id of a consumer or producer thread.
	 * 
	 * @param clientId  A client id of a consumer or producer
	 */
	public void addProducerClient(String clientId) {
		allClientsList.add(clientId);
	}

	public void addConsumerClient(String clientId) {
		allClientsList.add(clientId);
		consumerClientList.add(clientId);
	}

	private void startProducers() {

		if (producerList != null) {
			for (Runnable producer : producerList) {
				Thread t = new Thread(producer);
				t.start();
			}
		}
	}

	public void setProducerList(List<Runnable> producerList) {
		this.producerList = producerList;
	}



}
