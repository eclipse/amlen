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
package com.ibm.ima.test;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;


/**
 * Just a sample
 *
 */
public class TestSampleJunit4 {

	@BeforeClass 
	public static void classSetup() throws Exception {
		
		System.out.println("Class Setup");
	}
	
	@Before 
	public void setUp() throws Exception {
		
		System.out.println("Setup");
	}
	
	@Test
	public void testQueueConnections1()
	{
		System.out.println("In the test 1");
		Assert.assertTrue(true);
	}
	
	@Test
	public void testQueueConnections2()
	{
		System.out.println("In the test 2");
		Assert.assertTrue(true);
	}
	
	@After
	public void teardown() throws Exception {
		System.out.println("Test done!");
	}
	
	@AfterClass
	public static void tearDownClass() throws Exception {
		System.out.println("Tear down!");
	}
}

