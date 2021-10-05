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



public class JmsProducerExceptionListener implements ExceptionListener {

	private JmsProducerClient producer = null;
	
	public JmsProducerExceptionListener(JmsProducerClient producer)
	{
		this.producer = producer;
	}


	public void onException(JMSException arg0) {
		
		
		if(producer.trace!= null && producer.trace.traceOn())
		{
			producer.trace.trace("In the producer exception handler with error= " + arg0.getMessage());
			producer.trace.trace("Failure is true and calling reset connections");
		}
		
		if(!producer.isReconnecting())
		{
			producer.resetConnections();
		}
		else
		{
			if(producer.trace!= null && producer.trace.traceOn())
			{
				producer.trace.trace("In the producer exception handler with error= " + arg0.getMessage());
				producer.trace.trace("We are already in reconnecting mode so nothing to do but wait");
			}
		}
		
		

	}
	
}
