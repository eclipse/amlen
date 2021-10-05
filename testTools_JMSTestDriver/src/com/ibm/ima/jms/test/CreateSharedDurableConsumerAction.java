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
package com.ibm.ima.jms.test;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateSharedDurableConsumerAction extends ApiCallAction {
	private final String _structureID;
	private final String _sessID;
	private final String _destID;
	private final String _durableName;
	private final String _selector;
	
	protected CreateSharedDurableConsumerAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null)
			throw new RuntimeException("structure_id is not defined for " + this);
	
		_sessID = _actionParams.getProperty("session_id");
		if (_sessID == null)
			throw new RuntimeException("session_id is not defined for " + this);
		
		_destID = _actionParams.getProperty("dest_id");
		if (_destID == null)
			throw new RuntimeException("dest_id is not defined for " + this);
		
		_durableName = _apiParameters.getProperty("durableName");
		_selector = _apiParameters.getProperty("selector");
	}

	@Override
	protected boolean invokeApi() throws JMSException {
		final MessageConsumer consumer;
		final Session ses = (Session) _dataRepository.getVar(_sessID);
		final Destination dest = (Destination) _dataRepository.getVar(_destID);

		if (ses == null)
			return err("Action %s failed to locate Session %s", _id, _sessID);
		
		if (dest == null)
			return err("Action %s failed to locate Destination %s", _id, _destID);

		if (! (dest instanceof Topic))
			return err("Action %s failed, Destination object %s for durable subscriber is not a topic", _id, _destID);

		Topic topic = (Topic) dest;
		
		if (_selector == null)
			consumer = ((ImaSubscription)ses).createSharedDurableConsumer(topic, _durableName);
		else
			consumer = ((ImaSubscription)ses).createSharedDurableConsumer(topic, _durableName, _selector);
		
		_dataRepository.storeVar(_structureID, consumer);
		return true;
	}

	private boolean err(String fmt, Object... args) {
		String s = String.format(fmt,  args);
		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", s, new Object[0]);
		return false;
	}
}
