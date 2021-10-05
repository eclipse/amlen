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

import java.util.Random;

public class SendMessageAction extends ApiCallAction {
	private final	String		_connectionID;
	private final	String		_msgID;
	private final	String		_topic;
	private final	String		_QoS;
	private final	boolean		_waitForAck;
	private final	long		_waitTime;
	private final	boolean		_retain;
	private final	boolean		_haveRetain;
	private final	int			_maxLength;
	private final	boolean		_CRC;
	private final   long		_seed;
	private final	Random		_rand;
	private final   int			_expectedrc;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public SendMessageAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);	
        //_structureID = _actionParams.getProperty("structure_id");
        //if(_structureID == null){
        //    throw new RuntimeException("structure_id is not defined for " + this.toString());
        //}
        _msgID = _actionParams.getProperty("message_id");
        if(_msgID == null){
        	throw new RuntimeException("message_id is not defined for " + this.toString());
        }
        _connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
		_topic = _actionParams.getProperty("topic");
		_QoS = _actionParams.getProperty("QoS");
		_waitForAck = Boolean.valueOf(_actionParams.getProperty("waitForAck", "false"));
		String temp = _actionParams.getProperty("waitTime", "20000");
		_waitTime = Long.valueOf(temp);
		temp = _actionParams.getProperty("RETAIN");
		if (null == temp) {
			_haveRetain = false;
			_retain = false;
		} else {
			_haveRetain = true;
			_retain = new Boolean(temp);
		}
		_expectedrc = Integer.valueOf(_apiParameters.getProperty("expectedrc", "-1"));
		_maxLength = Integer.valueOf(_actionParams.getProperty("maxLength", "500"));
		_CRC = Boolean.parseBoolean(_actionParams.getProperty("CRC", "false"));
		_seed = Long.parseLong(_actionParams.getProperty("seed", "0"));
		if (0 == _seed) {
			_rand = null;
		} else {
			_rand = new Random(_seed);
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SENDMESSAGE+1),
					"Failed to locate the IsmWSConnection object ({0}).", _connectionID);
			return false;
		}
        setConnection(con);
		
		String topic = (String)_dataRepository.getVar(_topic);
		if (null == topic) {
			topic = _topic;
		}
		MyMessage msg = null;
		if ("RANDOM".equals(_msgID)) {
			msg = new MyMessage(con.getConnectionType(), topic, randomPayload(), 0, false, con);
		} else {
			msg = (MyMessage)_dataRepository.getVar(_msgID);
		}
		if(msg == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SENDMESSAGE),
					"Failed to locate the IsmWSMessage object ({0}).", _msgID);
			return false;
		}

		if (con.isConnected() == false) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SENDMESSAGE+2),
					"Action {0}: Connection object ({1}) is not connected.", _id, _connectionID);
		}
		switch (con.getConnectionType()) {
		case MyConnection.ISMMQTT:
			if (null == topic) topic = msg.getTopic();
			break;
		case MyConnection.MQMQTT:
		case MyConnection.PAHO:
			if (null == topic) {
				topic = msg.getTopic();
			}
			if (null == topic) {
				throw new IsmTestException("ISMTEST"+(Constants.SENDMESSAGE+3),
					"Must set topic for MqttClient publish");
			}
			break;
		case MyConnection.PAHOV5:
			if (null == topic) {
				topic = msg.getTopic();
			}
			if (null == topic) {
				throw new IsmTestException("ISMTEST"+(Constants.SENDMESSAGE+3),
					"Must set topic for MqttClient publish");
			}
		
		}
		msg.incrementMessage();
		//  System.out.println("invokeApi: calling con.send... _haveRetain: "+_haveRetain+" _retain: "+_retain);
		con.send(msg, topic, _haveRetain ? _retain : msg.isRetained(), _QoS, _waitForAck, _waitTime, _expectedrc);
		return true;
	}
	
	public byte[] randomPayload() {
		Random rand = _rand;
		if (null == rand) {
			rand = TestUtils.rand; 
		}
		byte[] bytes = TestUtils.randomPayload(rand.nextInt(_maxLength), _rand);
		if (_CRC) {
			byte[] allBytes = new byte[bytes.length+8];
			java.util.zip.CRC32 crc32 = new java.util.zip.CRC32();
			crc32.update(bytes);
			long crc = crc32.getValue();
			java.nio.ByteBuffer buffer = java.nio.ByteBuffer.allocate(8);
			buffer.putLong(crc);
			System.arraycopy(buffer.array(), 0, allBytes, 0, 8);
			System.arraycopy(bytes, 0, allBytes, 8, bytes.length);
			bytes = allBytes;
		}
		return bytes;
	}

}
