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
package com.ibm.mqst.mqxr.scada;

import java.io.IOException;

public class Socket extends java.net.Socket{
	java.net.Socket socket;
	
	Socket(String host, int port) throws IOException {
			super(java.net.InetAddress.getByName(host), port);
	}

}
