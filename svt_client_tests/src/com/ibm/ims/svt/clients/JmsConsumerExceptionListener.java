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
package com.ibm.ims.svt.clients;

import javax.jms.ExceptionListener;
import javax.jms.JMSException;


/**
 * This class should be used by clients that need to be notified of all exceptions
 *
 */
public class JmsConsumerExceptionListener implements ExceptionListener {

	private JmsConsumerClient subscriber = null;
	
	public JmsConsumerExceptionListener(JmsConsumerClient subscriber)
	{
		this.subscriber = subscriber;
	}

	public void onException(JMSException arg0) {
		
		if(subscriber.trace != null && subscriber.trace.traceOn())
		{
			subscriber.trace.trace("In the consumer exception handler with error= " + arg0.getMessage());
			subscriber.trace.trace("Failure is true and calling reset connections");
		}
		if(!subscriber.isreconnecting())
		{
			
			subscriber.resetConnections();
		}
		else
		{
			if(subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("We are already trying to reconnect so do nothing but wait");
			}
		}
		

	}

}
