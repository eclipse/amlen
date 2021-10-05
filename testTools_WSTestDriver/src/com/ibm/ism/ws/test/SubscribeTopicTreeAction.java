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
package com.ibm.ism.ws.test;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

/**
 * TODO: qos etc
 *
 */
public class SubscribeTopicTreeAction  extends ApiCallAction {
	private final String _connectionID;
	private final String _prefix;
	private final int _startIndex;
	private final int _endIndex;
	private final int _qos;
	
	public SubscribeTopicTreeAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _prefix = _actionParams.getProperty("prefix", "/clustertopic/");
		_startIndex = Integer.parseInt(_actionParams.getProperty("startIndex", "0"));
		_endIndex = Integer.parseInt(_actionParams.getProperty("endIndex", "10"));
		if (_actionParams.getProperty("qos") != null) {
			_qos = Integer.parseInt(_actionParams.getProperty("qos", "0"));
		} else if (_apiParameters.getProperty("qos") != null) {
			_qos = Integer.parseInt(_apiParameters.getProperty("qos", "0"));
		} else {
			_qos = 0;
		}
	}
	
	@Override
	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SUBSCRIBETOPICTREE+1),
					"Failed to locate the IsmWSConnection object ({0}).", _connectionID);
			return false;
		}
        setConnection(con);
		
		int numTopics = _endIndex - _startIndex + 1;
		String[] topics = new String[numTopics];
		int[] qos = new int[numTopics];
		
		for (int index=_startIndex, arrayIndex=0; index <= _endIndex; index++, arrayIndex++) {
			topics[arrayIndex] = _prefix + index;
			qos[arrayIndex] = _qos;
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.SUBSCRIBETOPICTREE+2), 
					"topics[{0}] = {1}, qos[{2}] = {3}", arrayIndex, topics[arrayIndex], arrayIndex, qos[arrayIndex]);
		}
		
		con.subscribe(topics, qos);
		
		return true;
	}
}
