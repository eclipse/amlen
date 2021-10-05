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

/**
 * TODO: QoS, retained, etc
 * 
 *
 */
public class PublishTopicTreeAction extends ApiCallAction {
	private final String _connectionID;
	private final String _prefix;
	private final int _startIndex;
	private final int _endIndex;
	private final boolean _retained;
	private final String _qos;
	private final boolean _clearRetained;
	private final String _messageAttach;
	private final String _customTopic;
	private final String _customMessage;
	
	public PublishTopicTreeAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_connectionID = _actionParams.getProperty("connection_id");
        if(_connectionID == null){
        	throw new RuntimeException("connection_id is not defined for " + this.toString());
        }
		_prefix = _actionParams.getProperty("prefix", "/clustertopic/");
		_messageAttach = _actionParams.getProperty("messageAttach", "");
		_startIndex = Integer.parseInt(_actionParams.getProperty("startIndex", "0"));
		_endIndex = Integer.parseInt(_actionParams.getProperty("endIndex", "10"));
		_retained = Boolean.parseBoolean(_actionParams.getProperty("retained", "false"));
		_clearRetained = Boolean.parseBoolean(_actionParams.getProperty("clearRetained", "false"));
		_customTopic = _actionParams.getProperty("customTopic","");
		_customMessage = _actionParams.getProperty("customMessage","");
		if (_actionParams.getProperty("qos") != null) {
			_qos = _actionParams.getProperty("qos", "0");
		} else if (_apiParameters.getProperty("qos") != null) {
			_qos = _apiParameters.getProperty("qos", "0");
		} else {
			_qos = "0";
		}
	}

	@Override
	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
		if(con == null){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.PUBLISHTOPICTREE+1),
					"Failed to locate the IsmWSConnection object ({0}).", _connectionID);
			return false;
		}
        setConnection(con);
		
		for (int index=_startIndex; index <= _endIndex; index++) {
			String topic = _prefix + index;
			
			String payload = _messageAttach + _id + ": " + topic;
			// default payload = "messageAttachpublishActionName: /prefix/index "
			
			
			// Custom topic and payload
			/* Must use custom topic to use custom payload
			 * Replace characters in topic or payload parameter
			 * (i) : index
			 * (rc) : random alphanumeric character 
			 * (rn) : random digit
			 * (rl) : random letter
			 */
			if (!(_customTopic.equals(""))){
				String characters = "abcdefghijklmnopqrstuvwxyz1234567890";
				Random rand = new Random();
				
				topic = _customTopic.replace("(i)",Integer.toString(index));
				while(topic.contains("(rc)") || topic.contains("(rn)") || topic.contains("(rl)")){
					topic = topic.replaceFirst("\\(rc\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()))));
					topic = topic.replaceFirst("\\(rn\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()-26)+26)));
					topic = topic.replaceFirst("\\(rl\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()-10))));
				}
			}
			
			if(!(_customMessage.equals(""))){
				String characters = "abcdefghijklmnopqrstuvwxyz1234567890";
				Random rand = new Random();
				
				payload = _customMessage.replace("(i)",Integer.toString(index));
				while(payload.contains("(rc)") || payload.contains("(rn)") || payload.contains("(rl)")){
					payload = payload.replaceFirst("\\(rc\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()))));
					payload = payload.replaceFirst("\\(rn\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()-26)+26)));
					payload = payload.replaceFirst("\\(rl\\)",String.valueOf(characters.charAt(rand.nextInt(characters.length()-10))));
				}
			}
			
			
			if (_clearRetained) {
				payload = "";
			}
			MyMessage msg = new MyMessage(con.getConnectionType(), topic, payload.getBytes(), Integer.parseInt(_qos), _retained, con);
			
			if (con.isConnected() == false) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.PUBLISHTOPICTREE+2),
						"Action {0}: Connection object ({1}) is not connected.", _id, _connectionID);
			}
			
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.PUBLISHTOPICTREE+3), 
					"invokeApi: calling con.send...");
			con.send(msg, topic, /*retained*/_retained, /*QoS*/_qos, /*waitForAck*/false, /*WaitTime*/20000L);
		}
		
		return true;
	}
}
