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


import java.io.IOException;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;


public class GetRetainedMessageAction extends ApiCallAction {
	private final String 	_connectionID;
	private final String	_structure_id;
	private final String	_topic;
	private final long		_waitTime;
	private final Integer	_ID;
	private final int       _messages;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public GetRetainedMessageAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _structure_id = _actionParams.getProperty("structure_id");
        _topic = _actionParams.getProperty("topic");
        _waitTime = Long.valueOf(_actionParams.getProperty("waitTime", "1000"));
        String id = _actionParams.getProperty("ID");
        if (null == id) {
        	_ID = null;
        } else {
        	_ID = new Integer(id);
        }
        _messages = new Integer(_actionParams.getProperty("messages", "1"));
        
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = null;
		IsmTestException ex = null;
		
		if(_connectionID != null){
			final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
			if(con == null){
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.GETRETAINED), 
						"Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
				return false;
			}
	        setConnection(con);
			if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.GETRETAINED+1),
						"Action {0}: Connection object ({1}) is not connected.", _id, _connectionID);
			}
			System.out.println("invokeApi: Calling con.getRetainedMessage("+_topic+", "+_ID+")");
			con.getRetainedMessage(_topic, _ID);
			int msgs = 0;
			try {
				do {
					msg = con.myReceive(_waitTime);
					if (null != msg && msg.isRetained()) {
						if (null != _structure_id) {
							_dataRepository.storeVar(_structure_id, msg);
						}
						msgs++;
						String topic = msg.getTopic();
						if (!_topic.equals(topic)) {
							throw new IsmTestException("ISMTEST"+(Constants.GETRETAINED+2)
								, "Expected message on topic '"+_topic+"', received message on topic '"+topic+"'");
						}
					}
				} while (null != msg);
			} catch (IOException ie) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.GETRETAINED),
						"Received IOException trying to receive message");
				ex = new IsmTestException("ISMTEST"+(Constants.GETRETAINED),
						"Received IOException trying to receive message", ie);
			}
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.GETRETAINED+1),
					"Received " + msgs + " RETAINed messages");
			if (null != ex) throw ex;
			if (_messages != msgs) {
				throw new IsmTestException("ISMTEST"+(Constants.GETRETAINED+2)
					, "Expected to receive "+_messages+" RETAINed messages, received "+msgs);

			}
			
		}
		
		return true;
	}

}
