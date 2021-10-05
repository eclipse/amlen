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

/**
 * Implementations of PolicyValidator are used to send or 
 * consume messages.
 *
 */
public interface PolicyValidator {

	/**
	 * Set the hostname/address of the server
	 * 
	 * @param server  The server where the validation will take place
	 */
	public void setServer(String server);
	
	/**
	 * The port to connect to on the server
	 * 
	 * @param port  the port on the server where validation will take place
	 */
	public void setPort(String port);
	
	/**
	 * The destination name. This will be the topic or queue that will
	 * be published or subscribed to.
	 * 
	 * @param destinationName  The topic or queue to publish or subscribe to
	 */
	public void setDestinationName(String destinationName);
	
	/**
	 * A client id that should be used to connect with
	 * 
	 * @param clientId  The client id used for the connection
	 */
	public void setClientId(String clientId);
	
	/**
	 * A username used to connect to the server
	 * 
	 * @param username  A username used to connect
	 */
	public void setUsername(String username);
	
	/**
	 * The password that should be used
	 * 
	 * @param password  A password used to connect
	 */
	public void setPassword(String password);
	
	/**
	 * A string that will be sent or validated. That is a 
	 * producer will send this message and a consumer will
	 * ensure this message is consumed.
	 *  
	 * @param msg  The message to validate
	 */
	public void setValidationMsg(String msg);
	
	/**
	 * Check for any validation error
	 * 
	 * @return  An exception encapsulating what went wrong
	 */
	public ValidationException getError();
	
	/**
	 * Set a notice to stop validation/execution
	 * 
	 * @param stopExecution  If true stop the execution
	 */
	public void stopExecution(boolean stopExecution);
	
	/**
	 * Start the validation. This could be synchronous or asynchronous
	 */
	public void startValidation();
	
	
}
