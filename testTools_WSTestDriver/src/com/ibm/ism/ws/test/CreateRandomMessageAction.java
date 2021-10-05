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

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Random;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CreateRandomMessageAction extends ApiCallAction {
	private final 	String 			_structureID;
	private final	String			_connection;
	private final   String			_topic;
	private final	String			_topicVariable;
	private final   String			_QoS;
	private final   boolean			_RETAIN;
	private final	boolean			_includeQoS;
	private final	int				_length;
	private final	int				_msgType;
	private final	boolean			_incrementing;
	@SuppressWarnings("unused")
	private final	boolean			_CRC;
	@SuppressWarnings("unused")
	private final   boolean			_randomize;
    final long    _msgExpire;  /* uint32_t */
    final String  _contentType;
    final String  _responseTopic;
    final String  _correlationData;
    final List<MqttUserProp> _userProps;
	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateRandomMessageAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		_connection = _actionParams.getProperty("connection_id");
		if (null == _connection) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		String temp = _actionParams.getProperty("length");
		if (null == temp) {
			throw new RuntimeException("length is not defined for "
					+this.toString());
		}
		_length = Integer.parseInt(temp);
		
		_topic = _apiParameters.getProperty("topic");
		_topicVariable = _apiParameters.getProperty("topicVariable");
		temp = _apiParameters.getProperty("QoS", "0");
		_QoS = temp;
		temp = _apiParameters.getProperty("RETAIN", "false");
		_RETAIN = Boolean.parseBoolean(temp);
		temp = _apiParameters.getProperty("msgtype", "TEXT");
		if ("TEXT".equals(temp)) _msgType = MyMessage.TEXT;
		else if ("BINARY".equals(temp)) _msgType = MyMessage.BINARY;
		else _msgType = Integer.parseInt(temp);
		_incrementing = Boolean.parseBoolean(_actionParams.getProperty("incrementing", "false"));
		_CRC = Boolean.parseBoolean(_actionParams.getProperty("CRC", "false"));
		_randomize = Boolean.parseBoolean(_actionParams.getProperty("randomize", "false"));
		_seed = Long.parseLong(_actionParams.getProperty("seed", "0"));
		_includeQoS = Boolean.parseBoolean(_actionParams.getProperty("includeQoS", "false"));
	    _userProps = getUserProps("userprop");
        _contentType = _apiParameters.getProperty("contentType");
        _responseTopic = _apiParameters.getProperty("responseTopic");
        _correlationData = _apiParameters.getProperty("correlationData");
        _msgExpire = Long.valueOf(_apiParameters.getProperty("msgExpire", "-1"));
	}

	@SuppressWarnings("unused")
	private final   long			_seed;

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = null; 
		
		MyConnection con = (MyConnection)_dataRepository.getVar(_connection);
        setConnection(con);
		if (MyConnection.ISMMQTT == con.getConnectionType()) {
			
		}
		String topic = _topic;
		if (null == topic && null != _topicVariable) {
			topic = (String)_dataRepository.getVar(_topicVariable);
		}
		switch (_msgType) {
		case MyMessage.TEXT:
			//Random r = new Random();

			String _validCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";//`~!@$%^&*()[]{};':\",./<>?=_-";
			String payload;
			char[] chars = new char[_length-8];
			int charopts = _validCharacters.length();
			Random rand = new Random();
			for (int i=0; i<_length-8; i++) {
				char newChar;
				int j = rand.nextInt(charopts);
				newChar = _validCharacters.charAt(j);
				chars[i] = newChar;
			}
			payload = new String(chars);
			payload = "BEGIN"+payload+"END";
			/*String token = Long.toString(Math.abs(r.nextLong()), 36);
			StringBuffer sbuf = new StringBuffer();
			while (sbuf.length() < _length) {
				sbuf.append(token);
			}
			String payload = sbuf.substring(0, _length-1);*/
			switch (con.getConnectionType()) {
			case MyConnection.ISMWS:
			case MyConnection.ISMMQTT:
			case MyConnection.MQMQTT:
			case MyConnection.PAHO:
			case MyConnection.PAHOV5:
			case MyConnection.JSONTCP:
			case MyConnection.JSONWS:
				msg = new MyMessage(con.getConnectionType(), topic, payload,
						(null == _QoS) ? 0 : Integer.parseInt(_QoS), _RETAIN, _includeQoS, con);
				break;
			default: throw new IsmTestException("ISMTEST"+(Constants.CREATERANDOMMESSAGE), "Unknown connection type "+con);
			}
			break;
			
		case MyMessage.BINARY:
			/*
			 * Generate a byte array message for now
			 */
			ByteBuffer buff = ByteBuffer.allocate(_length);
			byte[] bytes= buff.array();
			Random gen = new Random();
			gen.nextBytes(bytes);
			switch (con.getConnectionType()) {
			case MyConnection.ISMMQTT:
			case MyConnection.ISMWS:
			case MyConnection.MQMQTT:
			case MyConnection.PAHO:
				msg = new MyMessage(con.getConnectionType(), topic, bytes, 
						(null == _QoS) ? 0 : Integer.parseInt(_QoS),
						_RETAIN, MyMessage.BINARY, con);
				break;
			default: throw new IsmTestException("ISMTEST"+(Constants.CREATERANDOMMESSAGE+1), 
					"Unknown connection type "+con);
			}
			break;
		
		default:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.CREATERANDOMMESSAGE+1),
					"Failed to identify the IsmMqttMessage type ({0}).", _msgType);
			throw new IsmTestException("ISMTEST"+(Constants.CREATERANDOMMESSAGE+2), "message type "+_msgType+" is not valid");
		}
		msg.setIncrementing(_incrementing);
	    switch (con.getConnectionType()) {
	    case MyConnection.ISMMQTT:
	    case MyConnection.PAHOV5:
	        msg.setContentType(_contentType);
	        msg.setExpiry(_msgExpire);
	        msg.setUserProperties(_userProps);
//	        if(_responseTopic != null){
	        	msg.setResponseTopic(_responseTopic);	        	
//	        }
	        msg.setCorrelationData(_correlationData);
	        break;
	    }
		_dataRepository.storeVar(_structureID, msg);
		return true;
	}

}
