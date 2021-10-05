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


import java.util.HashMap;
import java.util.Map;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;


public class ReceiveMessageAction extends ApiCallAction {
	private final String 	_connection_id;
	private final String 	_structureID;
	private final Long		_waitTime;
	private final boolean	_checkCRC;
	private final LogLevel	_msgTraceLevel;
	private final boolean	_firstOnTopicRetained;
	private final String	_monitorRecordPrefix;
	private final Boolean   _validateDup;
	private final Boolean   _printNumberReceived;

	private Map<String, String> topicRetain;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public ReceiveMessageAction(Element config, DataRepository dataRepository
			, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		// _structurID is used for the message in this case because the message is instantiated when received
        _structureID = _actionParams.getProperty("structure_id");
        if(_structureID == null){
            throw new RuntimeException("structure_id is not defined for " + this.toString());
        }
        _connection_id = _actionParams.getProperty("connection_id");
        if(_connection_id == null ){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
        String temp = _actionParams.getProperty("waitTime");
        if (null == temp) {
        	_waitTime = null;
        } else {
        	_waitTime = new Long(temp);
        }
        temp = _actionParams.getProperty("checkCRC", "false");
        _checkCRC = Boolean.valueOf(temp);
        temp = _actionParams.getProperty("msgTraceLevel"
        		, Integer.toString(TrWriter.LogLevel.toInt(_trWriter.getTraceLevel())));
        _msgTraceLevel = TrWriter.LogLevel.valueOf(Integer.valueOf(temp));
        _firstOnTopicRetained = Boolean.valueOf(_actionParams.getProperty("firstOnTopicRetained", "false"));
        if (_firstOnTopicRetained) topicRetain = new HashMap<String, String>();
        _monitorRecordPrefix = _actionParams.getProperty("monitorRecordPrefix");
        temp = _actionParams.getProperty("validateDup");
        if (null == temp) {
        	_validateDup = null;
        } else {
        	_validateDup = Boolean.valueOf(temp);
        }
				temp = _actionParams.getProperty("printNumberReceived", "false");
				_printNumberReceived = Boolean.valueOf(temp);

	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = null;

//		if(_connection_id != null){
			final MyConnection con = (MyConnection)_dataRepository.getVar(_connection_id);
			if(con == null){
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE)
					, "Action {0}: Failed to locate the IsmWSConnection object ({1}).",_id, _connection_id);
				return false;
			}
	        setConnection(con);
			
			if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE+1)
					, "Action {0}: Connection object ({1}) is not connected.", _id, _connection_id);
			}
			try {
				if (null == _waitTime) {
					msg = con.myReceive();
				} else {
					msg = con.myReceive(_waitTime);
				}
			} catch (IsmTestException e) {
				throw e;
			} catch (Exception ie) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE+2)
					, "Action {0}: IsmConnection object ({1}) received IOException: ({2})"
					, _id, _connection_id, ie.getMessage());
				throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+2)
					, ie.getMessage(), ie);
			}
			if(msg == null) {
				if(_printNumberReceived == true) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.RECEIVEMESSAGE+12)
						, "Action {0}: Received {1} messages on connection {2}.", _id, con.getMessagesReceived(), _connection_id);
				}
				if (con.getMessagesReceived() > 0) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE+8)
						, "Action {0}: Null message received after receiving {1} messages.", _id, con.getMessagesReceived());
				}
				throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+3)
					, "Null message from receive()");
			}
			con.incrementMessages(msg.getTopic());
			if (null != _validateDup) {
				if (_validateDup != msg.isDuplicate()) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE+11)
							, "Action {0}: Expected DUP flag of "+_validateDup+", but it was "+msg.isDuplicate()
							, _id);
				}
			}
			_dataRepository.storeVar(_structureID, msg);
			if (null != _monitorRecordPrefix
					&& msg.getTopic().startsWith("$SYS/ResourceStatistics/")) {
				long count = con.getMonitorRecordCount()+1;
				String store = _monitorRecordPrefix+count;
				MonitorRecord monitor = new MyMonitorRecord(msg).getMonitorRecord();
				con.countMonitorRecords(monitor);
				_dataRepository.storeVar(store, monitor);
			}
			if (_checkCRC) {
				byte[] msgData = msg.getPayloadBytes();
				if (msgData.length < 8) {
					throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+9)
						, "CRC expected but total message length was less than 8 bytes");
				} else {
					long expectedCRC = java.nio.ByteBuffer.allocate(8).getLong();
					java.util.zip.CRC32 crc32 = new java.util.zip.CRC32();
					crc32.update(msgData, 8, msgData.length-8);
					long actualCRC = crc32.getValue();
					if (expectedCRC != actualCRC) {
						throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+9)
							, "CRC did not match: expected "+expectedCRC
							+" calculated "+actualCRC);
					}
				}
			}

			try {
				if (msg.getType() == MyMessage.TEXT)  {
					_trWriter.writeTraceLine(_msgTraceLevel, "ISMTEST"+(Constants.RECEIVEMESSAGE+4)
							, "Action {0}: Received Text message, topic={1}, QoS={3}, RETAIN={4}, payload={2}"
							, _id, msg.getTopic(), msg.getPayload(), msg.getQos(), msg.isRetained());
				}
				else if (msg.getType() == MyMessage.BINARY) {
					_trWriter.writeTraceLine(_msgTraceLevel, "ISMTEST"+(Constants.RECEIVEMESSAGE+5)
							, "Action {0}: Received Binary message, topic={1}, QoS={4}, RETAIN={5}, length={3}, payloadBytes={2}"
							, _id, msg.getTopic(), new String(msg.getPayloadBytes()), msg.getPayloadBytes().length, msg.getQos(), msg.isRetained());
				}
				else {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.RECEIVEMESSAGE+6)
							, "Action {0}: Received unknown type message", _id);
					throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+6)
							, "Received unknown type message");
				}
			} catch (Exception e) {
				if (e instanceof IsmTestException) throw (IsmTestException)e;
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.RECEIVEMESSAGE+7)
						, "Action {0}: "+e.getMessage()+" thrown trying to get message payload", _id);
				throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+7)
						, e.getMessage()+" thrown trying to get message payload", e);
			}

			if (_firstOnTopicRetained) {
				if (topicRetain.containsKey(msg.getTopic())) {
					if (msg.isRetained()) {
						throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+10)
							, "not-First message for topic "+msg.getTopic()+" was a RETAINed message");
					}
				} else if (msg.isRetained()) {
					topicRetain.put(msg.getTopic(), "true");
				} else {
					throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+10)
						, "First message for topic "+msg.getTopic()+" was not a RETAINed message");
				}
			}
//		}

		return true;
	}

}
