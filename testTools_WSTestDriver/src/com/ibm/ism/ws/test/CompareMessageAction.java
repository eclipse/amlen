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

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.StringTokenizer;

public class CompareMessageAction extends ApiCallAction {
	private final 	String 				_structureID;
	private final	String				_message;
	private final	boolean				_compareQoS;
	private final	boolean				_compareBody;
	private final	String				_compareBodyContains;
	private final	boolean				_compareType;
	private final   boolean             _compareProperties;  /* Compare all v5 added properties */

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CompareMessageAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		_message = _actionParams.getProperty("compareTo");
		if (null == _message) {
			throw new RuntimeException("compareTo is not defined for "
					+ this.toString());
		}
		_compareQoS = Boolean.parseBoolean(_actionParams.getProperty("compareQoS", "false"));
		_compareBody = Boolean.parseBoolean(_actionParams.getProperty("compareBody", "true"));
		_compareBodyContains = _actionParams.getProperty("compareBodyContains");
		_compareType = Boolean.parseBoolean(_actionParams.getProperty("compareType", "true"));
	    _compareProperties = Boolean.parseBoolean(_actionParams.getProperty("compareProperties", "false"));
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = (MyMessage)_dataRepository.getVar(_structureID);
		if (null == msg) {
			throw new IsmTestException("ISMTEST"+Constants.COMPAREMESSAGE,
					"message "+_structureID+" is not found");
		}
		MyMessage compareMsg = (MyMessage)_dataRepository.getVar(_message);
		if (null == compareMsg) {
			throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+1),
					"message "+_message+" is not found");
		}
		if (_compareType && msg.getType() != compareMsg.getType()) {
			throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+2),
					"Message type was "+msg.getType()+"; expected "+compareMsg.getType());
		}
		if (MyMessage.BINARY == msg.getType()) {
			byte[] msgBytes = null;
			byte[] compareBytes = null;

			if (_compareBody) {
				msgBytes = msg.getPayloadBytes();
				compareBytes = compareMsg.getPayloadBytes();
				if (msgBytes.length != compareBytes.length) {
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+4),
							"length of message, "+msgBytes.length+", does not equal length of expected message, "+compareBytes.length);
				}
				for (int i=0; i<msgBytes.length; i++) {
					if (msgBytes[i] != compareBytes[i]) {
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+5),
								"Message differs from expected at byte "+i+" expected "+compareBytes[i]+" received "+msgBytes[i]);
					}
				}
			}
			if ((_compareBodyContains != null) && (_compareBodyContains.length() > 0)) {
				StringTokenizer compareStrings = new StringTokenizer(_compareBodyContains, "~");
				int counter = 0;
				StringBuffer compareStrBuf = new StringBuffer("");
				String msgStr = null;
				String compareStr = _compareBodyContains;
				boolean matchFound = false;
				msgBytes = msg.getPayloadBytes();
     			msgStr = new String(msgBytes, StandardCharsets.UTF_8);
				while (compareStrings.hasMoreElements()) {
					compareStr = compareStrings.nextToken();
					if (msgStr.contains(compareStr)) {
						matchFound = true;
						break;
					}
					if (counter == 0) {
						compareStrBuf.append(compareStr);
					} else {
						compareStrBuf.append(" or " + compareStr);
					}
					counter++;
				}
				if (!matchFound) {
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+4),
							"message, "+msgStr+", does not contain string, "+ compareStrBuf.toString());
				}
			}
			if (_compareQoS && msg.getQos() != compareMsg.getQos()) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+9),
						"Received message QoS was "+msg.getQos()+" expected "+compareMsg.getQos());
			}

			if (_compareQoS && msg.getQos() != compareMsg.getQos()) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+9),
						"Received message QoS was "+msg.getQos()+" expected "+compareMsg.getQos());
			}
		} else {
			String msgString = null;
			String compareString = null;
			if (_compareBody) {
				msgString = msg.getPayload();
				compareString = compareMsg.getPayload();
				if (msgString.length() != compareString.length()) {
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+7),
							"Received message length was "+msgString.length()+" expected "+compareString.length());
				}
				if (!msgString.equals(compareString)) {
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+8),
							"Received message did not match expected message");
				}
			}
			if (_compareQoS && msg.getQos() != compareMsg.getQos()) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGE+9),
						"Received message QoS was "+msg.getQos()+" expected "+compareMsg.getQos());
			}
		}
		
		if (_compareProperties) {
	        if (!compareProps(compareMsg.getUserProperties(), msg.getUserProperties())) {
	            throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+14),
	                "Received message on topic "+msg.getTopic()+", user properties do not match");
	                
	        }
		    if (!comparePropString(compareMsg.getContentType(), msg.getContentType())) {
	            throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+15),
	                "Received message on topic "+msg.getTopic()+", content type value was " + msg.getContentType() +
	                    " expected " + compareMsg.getContentType());
	        }
            if (!comparePropString(compareMsg.getResponseTopic(), msg.getResponseTopic())) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+16),
                    "Received message on topic "+msg.getTopic()+", response topic was " + msg.getResponseTopic() +
                        " expected " + compareMsg.getResponseTopic());
            }
            if (!comparePropString(compareMsg.getCorrelationData(), msg.getCorrelationData())) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                    "Received message on topic "+msg.getTopic()+", correlation data was " + msg.getCorrelationData() +
                        " expected " + compareMsg.getCorrelationData());
            }
            if((!(compareMsg.getSubscriptionIdentifiers().equals(msg.getSubscriptionIdentifiers())))){
            	throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+18),
            		"Received message on topic " + msg.getTopic()+", subscriptionID was " + msg.getSubscriptionIdentifiers() +
            			" expected " + compareMsg.getSubscriptionIdentifiers());
            	
            }
		}
		return true;
	}

	/*
	 * Compare properties stings accounting for version and nulls
	 */
    boolean comparePropString(String expected, String got) {
        if (version < 5)
            return true;
        if (expected == null && got == null)
            return true;
        if (expected == null || got == null)
            return false;
        return expected.equals(got);
    }
    
    /*
     * Compare user properties.
     * We allow extra user properties, but the ones received are expected to be in order.
     * This is a slightly higher requirement than MQTTv5 which only requires that properties
     * of the same name be in order.
     */
    boolean compareProps(List<MqttUserProp> expected, List<MqttUserProp> got) {
        /* TODO: implement this */
        if (version < 5)                      /* No user properties before version 5 */
            return true;
        if (expected == null && got == null)  /* Both null they match */
            return true;
        if (expected == null || got == null)  /* Either is null and not the other they do not match */
            return false;
        MqttUserProp[] gotarray = (MqttUserProp[])got.toArray();
        MqttUserProp[] exparray = (MqttUserProp[])expected.toArray();
        int gotpos = 0;
        for (int i=0; i<exparray.length; i++) {
            boolean good = false;
            for (; gotpos<gotarray.length; gotpos++) {
                if (gotarray[gotpos].getKey().equals(exparray[i].getKey()) &&
                    gotarray[gotpos].getValue().equals(exparray[i].getValue())) { 
                    good = true;
                    break;
                }
            }
            if (!good)
                return false;
        }
        return true;
    }

}
