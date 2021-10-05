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
package com.ibm.ima.test.cli.policy.cases;


import java.io.InputStream;
import java.util.Properties;


import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.ibm.ima.test.cli.policy.validation.PolicyValidation;
import com.ibm.ima.test.cli.policy.validation.PolicyValidation.VALIDATION_TYPE;
import com.ibm.ima.test.cli.policy.validation.ValidationConnException;
import com.ibm.ima.test.cli.policy.validation.ValidationException;
import com.ibm.ima.test.cli.util.CLICommandException;
import com.ibm.ima.test.cli.util.SSHExec;

public class PolicyValidationTest {

	private static SSHExec sshExec = null;
	private static Properties cmdProps = new Properties();
	private static int currentTest = 0;
	private static String currentTestId = null;
	private static String createPrefix = null;
	private static String updatePrefix = null;
	private static String cleanPrefix = null;

	// get the hostname/address of the server 
	private static final String A1_HOST = System.getProperty("A1_HOST");

	/**
	 * Obtain an SSH session to the server and create a Properties instance with
	 * all the commands.
	 * 
	 * @throws Exception  if an error occurs - no tests will be executed
	 */
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		
		sshExec = new SSHExec(A1_HOST, "admin", "admin");
		sshExec.connect();
		InputStream commandFile = ClassLoader.class.getResourceAsStream("/com/ibm/ima/test/cli/policy/cases/cli_commands.properties");
		// load the commands for the tests in this class
		cmdProps.load(commandFile);

	}

	/**
	 * Clean up after all tests are done.
	 * 
	 * @throws Exception  If an error occurrs.
	 */
	@AfterClass
	public static void tearDownAfterClass() throws Exception {

		if (sshExec != null) {
			sshExec.disconnect();
		}
	}

	/**
	 * Set up before each test case. If an error occurs the test case
	 * will not execute. All test cases in this class assume that
	 * setUp should be successful each and every time.
	 * 
	 * @throws CLICommandException  If an error occurs
	 */
	@Before
	public void setUp() throws CLICommandException {

		currentTest++;
		String currentTestString = "" + currentTest;
		currentTestId = ("0000"  + currentTestString).substring(currentTestString.length());
		createPrefix = currentTestId + "_create."; 
		updatePrefix = currentTestId + "_update."; 
		cleanPrefix = currentTestId + "_clean."; 
		sshExec.runCommands(createPrefix, cmdProps);

	}

	/**
	 * Clean up after each and every test case
	 * 
	 * @throws CLICommandException  If an error occurs
	 */
	@After
	public void cleanUp() throws CLICommandException {
		sshExec.runCommands(cleanPrefix, cmdProps);
	}

	/**
	 * Validate policy - this test case should always pass
	 * 
	 */
	@Test
	public void test_0001()  {

		PolicyValidation validator = new PolicyValidation(VALIDATION_TYPE.JMS_TO_JMS);

		validator.setConsumerClientId("CID13000_1");
		validator.setPort("13000");
		validator.setServer(A1_HOST);
		validator.setProducerUsername("USR13000_1");
		validator.setProducerPassword("ismtest");
		validator.setConsumerUsername("USR13000_1");
		validator.setConsumerPassword("ismtest");
		validator.setTopicString("ENDP13000_Topic");
		validator.setValidationMsg("Just some text...");
		validator.setProducerClientId("CID13000_2");

		try {
			validator.validate();
		} catch (ValidationException ve) {
			Assert.fail("Validation failed. An exception was thrown - " + ve.getMessage());
		}

	}
	
	/**
	 * Validate policy - A ValidationConnException is expected.
	 */
	@Test(expected=ValidationConnException.class)
	public void test_0002() throws ValidationException {

		PolicyValidation validator = new PolicyValidation(VALIDATION_TYPE.MQTT_TO_MQTT);

		validator.setConsumerClientId("CID13000_1");
		validator.setPort("13000");
		validator.setServer(A1_HOST);
		validator.setProducerUsername("USR13000_1");
		validator.setProducerPassword("ismtest");
		validator.setConsumerUsername("USR13000_1");
		validator.setConsumerPassword("ismtest");
		validator.setTopicString("ENDP13000_Topic");
		validator.setValidationMsg("Just some text...");
		validator.setProducerClientId("CID13000_2");

		validator.validate();
	

	}


}
