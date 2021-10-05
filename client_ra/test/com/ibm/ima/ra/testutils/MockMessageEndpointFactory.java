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
package com.ibm.ima.ra.testutils;

import java.lang.reflect.Method;

import javax.resource.spi.UnavailableException;
import javax.resource.spi.endpoint.MessageEndpoint;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

public class MockMessageEndpointFactory implements MessageEndpointFactory {
	

	@Override
	public MessageEndpoint createEndpoint(XAResource arg0)
			throws UnavailableException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public MessageEndpoint createEndpoint(XAResource arg0, long arg1)
			throws UnavailableException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public boolean isDeliveryTransacted(Method arg0)
			throws NoSuchMethodException {
		// TODO Auto-generated method stub
		return false;
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
