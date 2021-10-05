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

import java.math.BigInteger;
import java.util.List;
import java.util.Vector;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CreateMessageAction extends ApiCallAction {
	private final 	String 		_structureID;
	private final	String		_connection;
	private final   String		_topic;
	private final	String		_topicVariable;
	private final 	int			_msgType;
	private final   int         _QoS;
	private final   boolean		_RETAIN;
	private final	String		_payload;
	private final   boolean		_incrementing;
	private final	boolean		_includeQoS;
	private final   int 		_subid;
    final long    _msgExpire;  /* uint32_t */
    final String  _contentType;
    final String  _responseTopic;
    final String  _correlationData;
    final Boolean _payloadFormatIndicator;
    final List<MqttUserProp> _userProps;
    final Integer _topicAlias;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateMessageAction(Element config,
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
		String temp = _apiParameters.getProperty("msgtype", "TEXT");
		if ("TEXT".equals(temp)) _msgType = MyMessage.TEXT;
		else if ("BINARY".equals(temp)) _msgType = MyMessage.BINARY;
		else _msgType = Integer.parseInt(temp);
		
		_topic = _apiParameters.getProperty("topic");
		_topicVariable = _apiParameters.getProperty("topicVariable");
		temp = _apiParameters.getProperty("QoS", "0");
		_QoS = Integer.valueOf(_apiParameters.getProperty("QoS", "0"));
		temp = _apiParameters.getProperty("RETAIN", "false");
		_RETAIN = Boolean.parseBoolean(temp);
		_payload = _apiParameters.getProperty("payload");
		_incrementing = Boolean.parseBoolean(_actionParams.getProperty("incrementing", "false"));
		_includeQoS = Boolean.parseBoolean(_actionParams.getProperty("includeQoS", "false"));
		_userProps = getUserProps("userprop");
		_subid = Integer.valueOf(_apiParameters.getProperty("subscriptionID","0"));
		_contentType = _apiParameters.getProperty("contentType");
	    _responseTopic = _apiParameters.getProperty("responseTopic");
	    _correlationData = _apiParameters.getProperty("correlationData");
	    temp = _apiParameters.getProperty("payloadFormatIndicator");
	    if (temp == null) {
	    	_payloadFormatIndicator = null;
	    } else {
	    	_payloadFormatIndicator = Boolean.parseBoolean(temp);
	    }
	    _msgExpire = Long.valueOf(_apiParameters.getProperty("msgExpire", "-1"));
	    temp = _apiParameters.getProperty("topicAlias");
	    if(temp == null){
	    	_topicAlias = null;
	    } else {
	    	_topicAlias = Integer.parseInt(temp);
	    }
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = null; 
		
		MyConnection con = (MyConnection)_dataRepository.getVar(_connection);
		String topic = _topic;
		if (null == topic && null != _topicVariable) {
			Object storeVal = _dataRepository.getVar(_topicVariable);
			
			if (null != storeVal) {
				if (storeVal instanceof NumberString) {
					topic = ((NumberString)storeVal).incrementAndGetName();
				} else if (storeVal instanceof String) {
					topic = (String)storeVal;
				}
			}
		}
		setConnection(con);
		
		/* See if it corresponds to a stored string */
		String payload = (String)_dataRepository.getVar(_payload);
		if (null == payload) {
			payload = _payload;
		}
		if (null == payload) {
			payload = "";
		}
		switch (_msgType) {
		case MyMessage.TEXT:
			switch (con.getConnectionType()) {
			case MyConnection.ISMWS:
	            msg = new MyMessage(con.getConnectionType(), topic, payload, _QoS, _RETAIN, _includeQoS, con);
	            break;
	            
			case MyConnection.ISMMQTT:
			case MyConnection.MQMQTT:
			case MyConnection.PAHO:
			case MyConnection.PAHOV5:
			case MyConnection.JSONTCP:
			case MyConnection.JSONWS:
				msg = new MyMessage(con.getConnectionType(), topic, payload, _QoS,
						  _RETAIN, _includeQoS, con);
				break;
			default:
			    throw new IsmTestException("ISMTEST"+(Constants.CREATEMESSAGE), "Unknown connection type "+con);
			}
			break;
			
		case MyMessage.BINARY:
			/*
			 * Generate a byte array message for now
			 */
			byte[] bytes = null;
			if (null != payload) {
				bytes = (new BigInteger(payload, 16)).toByteArray();
			}
			switch (con.getConnectionType()) {
			case MyConnection.ISMMQTT:
	             msg = new MyMessage(con.getConnectionType(), topic, bytes, _QoS,
	                        _RETAIN, MyMessage.BINARY, con);
			case MyConnection.ISMWS:
			case MyConnection.MQMQTT:
			case MyConnection.PAHO:
			case MyConnection.PAHOV5:
			case MyConnection.JSONTCP:
			case MyConnection.JSONWS:
				msg = new MyMessage(con.getConnectionType(), topic, bytes, _QoS,
						_RETAIN, MyMessage.BINARY, con);
				break;
			default: throw new IsmTestException("ISMTEST"+(Constants.CREATEMESSAGE+1), "Unknown connection type "+con.getConnectionType());
			}
			break;
		
		default:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.CREATEMESSAGE+2), "Failed to identify the IsmMqttMessage type ({0}).", _msgType);
			throw new IsmTestException(_apiParameters.getProperty("msgType","TEXT"), "message type is not valid");
		}
		msg.setIncrementing(_incrementing);
		switch (con.getConnectionType()) {
	    case MyConnection.ISMMQTT:
	    case MyConnection.PAHOV5:
            msg.setContentType(_contentType);
            msg.setExpiry(_msgExpire);
            msg.setUserProperties(_userProps);
//            if(_responseTopic != null){
            	msg.setResponseTopic(_responseTopic);            	
//            }
            msg.setCorrelationData(_correlationData);
            
            if (_payloadFormatIndicator != null){
            	msg.setPayloadFormatIndicator(_payloadFormatIndicator);
            }
            
            if (_topicAlias != null){
            	msg.setTopicAlias(_topicAlias);
            }
            
	        //set subscription id only used for creating template message to compare against
            if (_subid != 0) {
	            Vector<Integer> vi = new Vector<Integer>(1);
	            vi.add(_subid);
	            msg.setSubscriptionIdentifiers(vi);
	        }    
            
            
	        break;
		}
		
		_dataRepository.storeVar(_structureID, msg);
		return true;
	}

}
