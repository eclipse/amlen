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


public class DeleteAllRetainedMessagesAction extends ApiCallAction {
	private final String 	_connectionID;
	private final long		_waitTime;
	private final Integer	_deleted;
	private final boolean   _checkEmpty;
	
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public DeleteAllRetainedMessagesAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        _waitTime = Long.valueOf(_actionParams.getProperty("waitTime", "1000"));
        String deleted = _actionParams.getProperty("deleted");
        if (null == deleted) {
        	_deleted = null;
        } else {
        	_deleted = new Integer(deleted);
        }
        _checkEmpty = Boolean.valueOf(_actionParams.getProperty("checkEmpty", "false"));
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = null;
		IsmTestException ex = null;
		
		if(_connectionID != null){
			final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);			
			if (con == null){
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.DELETEALLRETAINED), 
						"Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connectionID);
				return false;
			}
	        setConnection(con);
			if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.DELETEALLRETAINED+1),
						"Action {0}: Connection object ({1}) is not connected.", _id, _connectionID);
			}
			MyMessage zeroMsg = new MyMessage("", "", 0, true, con);
			// First subscribe to everything
			con.subscribe("#", 0);
			int msgs = 0;
			// Then for each message received, send a zero length RETAINed message to that topic.
			try {
				do {
					msg = con.myReceive(_waitTime);
					if (null != msg) {
						if (_checkEmpty && msg.isRetained() && 0 == msg.getMessageLength()) {
							// Should never get a zero length message with the RETAIN flag on 
							_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.DELETEALLRETAINED+3),
									"Received a zero length message with the RETAIN flag true on topic "+msg.getTopic());
							ex = new IsmTestException("ISMTEST"+(Constants.DELETEALLRETAINED+3),
									"Received a zero length message with the RETAIN flag true on topic "+msg.getTopic());
						}
					}
					if (null != msg && msg.isRetained()) {
						msgs++;
						String topic = msg.getTopic();
						_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.DELETEALLRETAINED+2),
								"Sending zero length RETAIN message for topic "+topic);
						zeroMsg.setTopic(topic);
						con.send(zeroMsg, true, 10000);
						// Get the topic and publish a zero length RETAINED message to this topic
					}
				} while (null != msg);
			} catch (IOException ie) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.DELETEALLRETAINED),
						"Received IOException trying to receive message");
				ex = new IsmTestException("ISMTEST"+(Constants.DELETEALLRETAINED),
						"Received IOException trying to receive message", ie);
			}
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.DELETEALLRETAINED+1),
					"Cleared out " + msgs + " RETAINed messages");
			con.unsubscribe("#");
			if (null != ex) throw ex;
			if (null != _deleted && _deleted != msgs) {
				throw new IsmTestException("ISMTEST"+(Constants.DELETEALLRETAINED+2),
						"Expected to receive "+_deleted+" RETAINed messages, received "+msgs);

			}
			
		}
		
		return true;
	}

}
