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
package com.ibm.ima.mqcon.basetest;

public abstract class MQConnectivityTest {

	/**
	 * This class contains common code that will be required by the more  
	 * specialised MQ to IMA or IMA to MQ tests
	 */
	
	/** 
	 * the number of messages that will be sent by each publisher in the test
	 */
	protected int numberOfMessages;
	/** 
	 * the number of publishers used in the test
	 */	
	protected int numberOfPublishers;
	/** 
	 * the number of subscribers in the test
	 */	
	protected int numberOfSubscribers;
	/** 
	 * the name of the test case being executed
	 */	
	protected String testName;
	/** 
	 * the name of the test case being executed
	 */	
	protected String testNameFinal;
	/** 
	 * the address of the imaserver
	 */
	protected String imaServerHost;
	/** 
	 * the port number of the imaserver
	 */
	protected int imaServerPort;
	/** 
	 * whether the test has run cleanly or not
	 * assumed to be true unless explicitly set false
	 */
	private boolean success = true;
	
	/** 
	 * whether the test expects messages to arrive
	 * assumed to be true unless explicitly set false
	 */
	protected boolean messagesExpected = true;
	
	public static void main(String[] args) {


	}
	
	public void parseCommandLineArguments(String[] args) {
		// to be overridden by subclasses
	}

	public int getNumberOfMessages() {
		return numberOfMessages;
	}

	public void setNumberOfMessages(int numberOfMessages) {
		this.numberOfMessages = numberOfMessages;
	}

	public int getNumberOfPublishers() {
		return numberOfPublishers;
	}

	public void setNumberOfPublishers(int numberOfPublishers) {
		this.numberOfPublishers = numberOfPublishers;
	}

	public int getNumberOfSubscribers() {
		return numberOfSubscribers;
	}

	public void setNumberOfSubscribers(int numberOfSubscribers) {
		this.numberOfSubscribers = numberOfSubscribers;
	}

	public String getTestName() {
		return testName;
	}

	public void setTestName(String testName) {
		this.testName = testName;
	}

	public String getIamServerHost() {
		return imaServerHost;
	}

	public void setIamServerHost(String imaServerHost) {
		this.imaServerHost = imaServerHost;
	}

	public int getIamServerPort() {
		return imaServerPort;
	}

	public void setIamServerPort(int imaServerPort) {
		this.imaServerPort = imaServerPort;
	}

	public void setSuccess(boolean success) {
		this.success = success;
	}

	public boolean isSuccess() {
		return success;
	}

	public String getTestNameFinal() {
		return testNameFinal;
	}

	public void setTestNameFinal(String testNameFinal) {
		this.testNameFinal = testNameFinal;
	}

	public String getImaServerHost() {
		return imaServerHost;
	}

	public void setImaServerHost(String imaServerHost) {
		this.imaServerHost = imaServerHost;
	}

	public int getImaServerPort() {
		return imaServerPort;
	}

	public void setImaServerPort(int imaServerPort) {
		this.imaServerPort = imaServerPort;
	}
}
