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
/**
 * 
 */
package test.llm.sync;

import java.io.IOException;

import junit.framework.TestCase;

/**
 *
 */
public class SyncServerTest extends TestCase {

	/**
	 * @param name
	 */
	public SyncServerTest(String name) {
		super(name);
	}

	/**
	 * Test method for {@link test.llm.sync.SyncServer#SyncServer(java.lang.String, int)}.
	 */
	public void testSyncServer() throws IOException{
		String name = "test";
		int port = 35000;
		SyncServer test = new SyncServer(name, port);
		/* check connection activity*/
		assertEquals(port, test.getPort());
		assertEquals(name, test.getName());
		
	}

}
