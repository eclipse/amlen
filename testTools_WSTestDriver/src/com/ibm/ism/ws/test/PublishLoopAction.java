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
 * TODO: QoS, retained, etc
 * For sending a set number of messages
 * Can take a counter or an incrementing messageID to start the loop index
 * publishes until reaches endindex
 *
 */
public class PublishLoopAction extends ApiCallAction {
	private final String _connectionID;
	private final String _topic;
	private final boolean _initializeCounter;
	private final int _endIndex;
	private final boolean _retained;
	private final String _qos;
	private final String _messageAttach;
	private final String _counterID;
	private final String _msgID;
	

	public PublishLoopAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
		_messageAttach = _actionParams.getProperty("messageAttach", "");
		_msgID = _actionParams.getProperty("message_id","");
		_counterID = _actionParams.getProperty("counterID","");
		_topic = _actionParams.getProperty("topic");
		_endIndex = Integer.parseInt(_actionParams.getProperty("endIndex", "10"));
		_retained = Boolean.parseBoolean(_actionParams.getProperty("retained", "false"));
		_initializeCounter = Boolean.parseBoolean(_actionParams.getProperty("initializeCounter", "true"));
		if (_actionParams.getProperty("qos") != null) {
			_qos = _actionParams.getProperty("qos", "0");
		} else if (_apiParameters.getProperty("qos") != null) {
			_qos = _apiParameters.getProperty("qos", "0");
		} else {
			_qos = "0";
		}
	}
	//TODO: change error constants

	@Override
	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.PUBLISHLOOP+1),
					"Failed to locate the IsmWSConnection object ({0}).", _connectionID);
			return false;
		}
        setConnection(con);

		Integer counterValue;
		if(_initializeCounter == true){
			_dataRepository.storeVar(_counterID, 1);
			counterValue = 1;
		} else {
			//get counter value
			counterValue = (Integer)_dataRepository.getVar(_counterID);
			if (null == counterValue) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+Constants.PUBLISHLOOP, "Cannot find counterID "+_counterID);
				throw new IsmTestException("ISMTEST"+Constants.PUBLISHLOOP, "Cannot find counterID "+_counterID);
			}
		}

		int loopIndex = counterValue.intValue();
		_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHLOOP+31),
				"Setting counter to {0}", loopIndex);
		
		MyMessage msg = null;
		msg = (MyMessage)_dataRepository.getVar(_msgID);
		if(msg == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHLOOP+31),
					"Did not find IsmWSMessage object ({0}). Will create new message..", _msgID);
		} else if (msg.isIncrementing() == true){
				loopIndex = msg.getCounter();
				_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHLOOP+31),
						"Got counter from message {0}, setting counter to {1}", _msgID, loopIndex);
		}
		
		if (loopIndex >= _endIndex){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.PUBLISHLOOP+33), 
					"Counter ({0}) >= endIndex ({1}), exiting..", loopIndex, _endIndex);
			return true;
		}
		
		for (int index=loopIndex; index <= _endIndex; index++) {
			String topic = _topic;
			String payload = _messageAttach + _id + ": " + index;
			
			// payload = "messageAttachpublishActionName: /topic: index"

			msg = new MyMessage(con.getConnectionType(), topic, payload.getBytes(), Integer.parseInt(_qos), _retained, con);

			
			/*if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.PUBLISHLOOP+2),
						"Action {0}: Connection object ({1}) is not connected. Exiting PublishLoop", _id, _connectionID);
				return true;
			}*/
			
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHLOOP+3), 
					"invokeApi: calling con.send...");
			con.send(msg, topic, /*retained*/_retained, /*QoS*/_qos, /*waitForAck*/false, /*WaitTime*/20000L);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHLOOP+32), 
					"sent message, index: {0}", index);
			_dataRepository.storeVar(_counterID, index);
		}
		
		return true;
	}
}
