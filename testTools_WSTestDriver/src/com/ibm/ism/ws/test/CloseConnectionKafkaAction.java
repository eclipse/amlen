/*
 * Copyright (c) 2019-2021 Contributors to the Eclipse Foundation
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


package com.ibm.ism.ws.test;


import java.io.IOException;
import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;
/*
 * CloseConnectionKafkaAction is the action class for closing Kafka connection on completion
 */
public class CloseConnectionKafkaAction extends ApiCallAction {
	
	private final String _connectionID;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CloseConnectionKafkaAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_connectionID=_actionParams.getProperty("connectionID");

	}

	protected boolean invokeApi() throws IsmTestException {
		
		connectKafka = (MyConnectionKafka) _dataRepository.getVar(_connectionID);
		connectKafka.closeConnection();
		return true;
	}

}
