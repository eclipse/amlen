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
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CompareMessageDataAction extends ApiCallAction {
	private final 	String 				_structureID;
	private final	String				_compareQoS;
	private final	String				_compareBody;
	private final	String				_compareBodyStart;
	private final	String				_compareTopic;
	private final	Boolean				_compareRetain;
	private final	Boolean				_compareDuplicate;
	private final	String[]			_topicAmong;
	private final 	int 				_mqttMessageVersion;
	private final	Map<String, Integer>					_compareQoSbyTopic;
    final long    _compareMsgExpire;  /* uint32_t */
    final String  _compareContentType;
    final Boolean _hasContentType;
    final String  _compareResponseTopic;
    final Boolean _hasResponseTopic;
    final String  _compareCorrelationData;
    final Boolean _hasCorrelationData;
    final Integer _compareSubscriptionIdentifier;
    final Boolean _hasSubscriptionIdentifier;
    final Integer _compareTopicAlias;
    final Boolean _hasTopicAlias;
    final Boolean _hasUserProperties;
    final Boolean _comparePayloadFormatIndicator;
    final List<MqttUserProp> _compareUserProps;
    final List<String> _hasPropertyKeys;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CompareMessageDataAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		_compareQoS = _actionParams.getProperty("compareQoS");
		_compareBody = _actionParams.getProperty("compareBody");
		_compareBodyStart = _actionParams.getProperty("compareBodyStart");
		_compareTopic = _actionParams.getProperty("compareTopic");
		String temp = _actionParams.getProperty("compareQoSbyTopic");
		if (null == temp) {
			_compareQoSbyTopic = null;
		} else {
			_compareQoSbyTopic = new HashMap<String, Integer>();
			String[] units = temp.split(":");
			for (int i=0; i<units.length; i++) {
				String[] pieces = units[i].split("=");
				_compareQoSbyTopic.put(pieces[0], new Integer(pieces[1]));
			}
		}
		temp = _actionParams.getProperty("topicAmong");
		if (null == temp) {
			_topicAmong = null;
		} else {
			
			_topicAmong = temp.split("=");
		}
		temp = _actionParams.getProperty("compareRetain");
		if (null == temp) {
			_compareRetain = null;
		} else {
			_compareRetain = new Boolean(temp);
		}
		temp = _actionParams.getProperty("compareDuplicate");
		if (null == temp) {
			_compareDuplicate = null;
		} else {
			_compareDuplicate = new Boolean(temp);
		}
	    _compareUserProps = getUserProps("compareUserprop");
	    
	    _hasPropertyKeys = getStringList("hasPropertyKeys");
	    
	    _compareContentType = _apiParameters.getProperty("compareContentType");
	    temp = _apiParameters.getProperty("hasContentType");
	    if (null == temp) {
	    	_hasContentType = null;
	    } else {
	    	_hasContentType = new Boolean(temp);
	    }
	    _compareResponseTopic = _apiParameters.getProperty("compareResponseTopic");
	    temp = _apiParameters.getProperty("hasResponseTopic");
	    if (null == temp) {
	    	_hasResponseTopic = null;
	    } else {
	    	_hasResponseTopic = new Boolean(temp);
	    }
	    _compareCorrelationData = _apiParameters.getProperty("compareCorrelationData");
	    temp = _apiParameters.getProperty("hasCorrelationData");
	    if (null == temp) {
	    	_hasCorrelationData = null;
	    } else {
	    	_hasCorrelationData = new Boolean(temp);
	    }
	    
	    
	    temp = _apiParameters.getProperty("hasUserProperties");
	    if (null == temp) {
	    	_hasUserProperties = null;
	    } else {
	    	_hasUserProperties = new Boolean(temp);
	    }
	    
	    temp = _apiParameters.getProperty("compareSubscriptionIdentifier");
	    if (null == temp) {
	    	_compareSubscriptionIdentifier = null;
	    } else {
	    	_compareSubscriptionIdentifier = new Integer(temp);
	    }
	    temp = _apiParameters.getProperty("hasSubscriptionIdentifier");
	    if (null == temp) {
	    	_hasSubscriptionIdentifier = null;
	    } else {
	    	_hasSubscriptionIdentifier = new Boolean(temp);
	    }
	    
	    _compareMsgExpire = Long.valueOf(_apiParameters.getProperty("compareMsgExpire", "-1"));
	    _mqttMessageVersion = Integer.valueOf(_actionParams.getProperty("mqttMessageVersion","4"));

	    temp = _apiParameters.getProperty("compareTopicAlias");
	    if (null == temp) {
	    	_compareTopicAlias = null;
	    } else {
	    	_compareTopicAlias = new Integer(temp);
	    }
	    temp = _apiParameters.getProperty("hasTopicAlias");
	    if (null == temp) {
	    	_hasTopicAlias = null;
	    } else {
	    	_hasTopicAlias = new Boolean(temp);
	    }
	    temp = _apiParameters.getProperty("comparePayloadFormatIndicator");
	    if (null == temp) { 
	    	_comparePayloadFormatIndicator = null;
	    } else {
	    	_comparePayloadFormatIndicator = new Boolean(temp);
	    }
	    
	}

	protected boolean invokeApi() throws IsmTestException {
		MyMessage msg = (MyMessage)_dataRepository.getVar(_structureID);
		if (null == msg) {
			throw new IsmTestException("ISMTEST"+Constants.COMPAREMESSAGEDATA,
					"message "+_structureID+" is not found");
		}
		
		if (null != _compareBody) {
			if (MyMessage.BINARY == msg.getType()) {
				byte[] msgBytes = null;
				byte[] compareBytes = null;

				try {
					msgBytes = msg.getPayloadBytes();
					compareBytes = (new BigInteger(_compareBody, 16)).toByteArray();
					if (msgBytes.length != compareBytes.length) {
						String log = "Compare failure, "+msg.toString()
						    + "\n\t expected body:="+TestUtils.bytesToHex(_compareBody.getBytes());
						log = log + "\n\t expected body as text:='"+(new String(_compareBody.getBytes(), StandardCharsets.UTF_8))+"'";
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, log);
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+4),
								"length of message, "+msgBytes.length+", does not equal length of expected message, "+compareBytes.length);
					}
					for (int i=0; i<msgBytes.length; i++) {
						if (msgBytes[i] != compareBytes[i]) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
									, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
									, "Compare failure, msg:QoS="+msg.getQos()
									+ " msg:topic="+msg.getTopic()
									+ " msg:string="+msg.getPayload());
							throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+5),
									"Message differs from expected at byte "+i+" expected "+compareBytes[i]+" received "+msgBytes[i]);
						}
					}
				} catch (NumberFormatException e) {
					String compareString = _compareBody;
					String msgString = null;
					msgString = new String(msgBytes, StandardCharsets.UTF_8);
					compareBytes = compareString.getBytes();
					if (msgBytes.length != compareBytes.length) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Compare failure, "+msg.toString()
								+ "\n\t expected body:="+TestUtils.bytesToHex(_compareBody.getBytes()));
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+7),
								"Received message length was "+msgString.length()+" expected "+compareString.length());
					}
					if (msgString.length() != compareString.length()) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Warning, byte array length matched but String lengths did not: expected "
									+compareString.length()+ "received "+msgString.length());
					}
					if (!java.util.Arrays.equals(compareBytes, msgBytes)
							&& !msgString.equals(compareString)) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Compare failure, msg:QoS="+msg.getQos()
								+ " msg:topic="+msg.getTopic()
								+ " msg:string="+msg.getPayload());
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+8),
								"Received message '"+msg.getPayload()+"' did not match expected message '"+compareString+"'");
					}
				}
			} else {
				String msgString = null;
				String compareString = null;
				msgString = msg.getPayload();
				compareString = _compareBody;
				if (msgString.length() != compareString.length()) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
							, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
							, "Compare failure, msg:QoS="+msg.getQos()
							+ " msg:topic="+msg.getTopic()
							+ " msg:string="+msg.getPayload());
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+7),
							"Received message length was "+msgString.length()+" expected "+compareString.length());
				}
				if (!msgString.equals(compareString)) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
							, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
							, "Compare failure, msg:QoS="+msg.getQos()
							+ " msg:topic="+msg.getTopic()
							+ " msg:string="+msg.getPayload());
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+8),
							"Received message '"+msg.getPayload()+"' did not match expected message '"+compareString+"'");
				}
			}
		}
		if (null != _compareBodyStart) {
			if (MyMessage.BINARY == msg.getType()) {
				byte[] msgBytes = null;
				byte[] compareBytes = null;

				try {
					msgBytes = msg.getPayloadBytes();
					compareBytes = (new BigInteger(_compareBodyStart, 16)).toByteArray();
					if (msgBytes.length < compareBytes.length) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Compare failure, msg:QoS="+msg.getQos()
								+ " msg:topic="+msg.getTopic()
								+ " msg:string="+msg.getPayload());
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+4),
								"length of message, "+msgBytes.length+", is shorter than length of expected message, "+compareBytes.length);
					}
					for (int i=0; i<compareBytes.length; i++) {
						if (msgBytes[i] != compareBytes[i]) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
									, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
									, "Compare failure, msg:QoS="+msg.getQos()
									+ " msg:topic="+msg.getTopic()
									+ " msg:string="+msg.getPayload());
							throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+5),
									"Message differs from expected at byte "+i+" expected "+compareBytes[i]+" received "+msgBytes[i]);
						}
					}
				} catch (NumberFormatException e) {
					String compareString = _compareBodyStart;
					String msgString = new String(msgBytes, StandardCharsets.UTF_8);
					if (msgString.length() < compareString.length()) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Compare failure, msg:QoS="+msg.getQos()
								+ " msg:topic="+msg.getTopic()
								+ " msg:string="+msg.getPayload());
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+7),
								"Received message length was "+msgString.length()+" expected at least "+compareString.length());
					}
					if (!(msgString.substring(0, compareString.length())).equals(compareString)) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
								, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
								, "Compare failure, msg:QoS="+msg.getQos()
								+ " msg:topic="+msg.getTopic()
								+ " msg:string="+msg.getPayload());
						throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+8),
								"Received message '"+msg.getPayload()+"' did not match expected message '"+compareString+"'");
					}
				}
			} else {
				String msgString = null;
				String compareString = null;
				msgString = msg.getPayload();
				compareString = _compareBodyStart;
				if (msgString.length() < compareString.length()) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
							, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
							, "Compare failure, msg:QoS="+msg.getQos()
							+ " msg:topic="+msg.getTopic()
							+ " msg:string="+msg.getPayload());
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+7),
							"Received message length was "+msgString.length()+" expected at least "+compareString.length());
				}
				if (!(msgString.substring(0, compareString.length())).equals(compareString)) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
							, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
							, "Compare failure, msg:QoS="+msg.getQos()
							+ " msg:topic="+msg.getTopic()
							+ " msg:string="+msg.getPayload());
					throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+8),
							"Received message '"+msg.getPayload()+"' did not match expected message '"+compareString+"'");
				}
			}
		}
		if (null != _compareQoS && msg.getQos() != Integer.valueOf(_compareQoS)) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
					, "Compare failure, msg:QoS="+msg.getQos()
					+ " msg:topic="+msg.getTopic()
					+ " msg:string="+msg.getPayload());
			throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+9),
					"Received message QoS was "+msg.getQos()+" expected "+_compareQoS);
		}
		if (null != _compareQoSbyTopic) {
			Integer value = _compareQoSbyTopic.get(msg.getTopic());
			if (null != value && value != msg.getQos()) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, msg:QoS="+msg.getQos()
						+ " msg:topic="+msg.getTopic()
						+ " msg:string="+msg.getPayload());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+10),
						"Received message on topic "+msg.getTopic()+" QoS was "+msg.getQos()+" expected "+value);
			}
		}
		if (null != _compareTopic) {
			if (!_compareTopic.equals(msg.getTopic())) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, msg:QoS="+msg.getQos()
						+ " msg:topic="+msg.getTopic()
						+ " msg:string="+msg.getPayload());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on topic "+msg.getTopic()+", QoS was "+msg.getQos()+" expected "+_compareTopic);
			}
		}
		if (null != _topicAmong) {
			boolean found = false;
			for (int i = 0; i< _topicAmong.length; i++) {
				if (_topicAmong[i].equals(msg.getTopic())) {
					found = true;
					break;
				}
			}
			if (!found) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+14),
						"Received message on topic "+msg.getTopic()+", QoS was "+msg.getQos()+
						" not among expected topics");
			}
		}
		if (null != _compareRetain) {
			if (_compareRetain != msg.isRetained()) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+12),
						"Received message on topic "+msg.getTopic()+", retain value was "+msg.isRetained()+" expected "+_compareRetain);
			}
		}
		if (null != _compareDuplicate) {
			if (_compareDuplicate != msg.isDuplicate()) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+12),
						"Received message on topic "+msg.getTopic()+", duplicate value was "+msg.isDuplicate()+" expected "+_compareDuplicate);
			}
		}
		
		
		if (_compareUserProps != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing User Properties");
		    if (!compareProps(_compareUserProps, msg.getUserProperties())){

		    	String actualProps = "ActualProps:";
		    	for(int i=0; i<msg.getUserProperties().size();i++){
		    		actualProps = actualProps + " " + msg.getUserProperties().get(i).getKey() + "=" + msg.getUserProperties().get(i).getValue();
		    	}
		    	String expectedProps = "ExpectedProps:";
		    	for(int i=0; i<_compareUserProps.size();i++){
		    		expectedProps = expectedProps + " " + _compareUserProps.get(i).getKey() + "=" + msg.getUserProperties().get(i).getValue();
		    	}
		    	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
    					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+12)
    					, "User Properties did not match, " + expectedProps + ", " + actualProps);
		    	throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+14),
		    			"Received message on topic "+msg.getTopic()+", user properties do not match");       
		    }
		}
		
		if(_hasPropertyKeys != null && _mqttMessageVersion >= 5){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking user property keys");
			if(!checkPropKeys(_hasPropertyKeys, msg.getUserProperties())){
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+14),
		                "Received message on topic "+msg.getTopic()+", specified property keys not found"); 
			}
		}
		
		
		if (_hasUserProperties != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has user properties");
			boolean msgHasUserProps = false;
			if (msg.getUserProperties() != null) {
				msgHasUserProps = true;
			}
			if (!(_hasUserProperties.equals(msgHasUserProps))){
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message has user properties: " + msgHasUserProps +" expected " + _hasUserProperties);
			}
		}
		
		if (_compareContentType != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing ContentType");
		    String ctype = msg.getContentType();
		    if (ctype == null || !ctype.equals(_compareContentType)) {
	            throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+15),
	                "Received message on topic "+msg.getTopic()+", content type value was " + ctype +" expected " + _compareContentType);
		    }
		}
		
		if (_hasContentType != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has content type");
			boolean msgHasContentType = false;
			if (msg.getContentType() != null) {
				msgHasContentType = true;
			}
			if (!(_hasContentType.equals(msgHasContentType))){
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message Has Content Type: " + msgHasContentType +" expected " + _hasContentType);
			}
		}
		
        if (_compareResponseTopic != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing ResponseTopic");
            String response = msg.getResponseTopic();
            if (response == null || !response.equals(_compareResponseTopic)) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+16),
                    "Received message on topic "+msg.getTopic()+", response topic value was " + response +" expected " + _compareResponseTopic);
            }    
        }
        
		if (_hasResponseTopic != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has response topic");
			boolean msgHasResponseTopic = false;
			if (msg.getResponseTopic() != null) {
				msgHasResponseTopic = true;
			}
			if (!(_hasResponseTopic.equals(msgHasResponseTopic))){
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message Has Response Topic: " + msgHasResponseTopic +" expected " + _hasResponseTopic);
			}
		}
        
	    if (_compareCorrelationData != null  && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing Correlation Data");
            String corr = msg.getCorrelationData();
            if (corr == null || !corr.equals(_compareCorrelationData)) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                    "Received message on topic "+msg.getTopic()+", correlation data value was " + corr +" expected " + _compareCorrelationData);
            }        
	    }
	    
		if (_hasCorrelationData != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has correlation data");
			boolean msgHasCorrelationData = false;
			if (msg.getCorrelationData() != null) {
				msgHasCorrelationData = true;
			}
			if (!(_hasCorrelationData.equals(msgHasCorrelationData))){
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message Has CorrelationData: " + msgHasCorrelationData +" expected " + _hasCorrelationData);
			}
		}

	    if (_compareTopicAlias != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing Topic Alias");
	    	Integer topicAlias = msg.getTopicAlias();
	    	System.out.println("topicalias: " + topicAlias);
	    	if(topicAlias != null){
	    		if(!(topicAlias.equals(_compareTopicAlias))) {
	    			throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
	    					"Received message on topic "+ msg.getTopic()+", TopicAlias value was " + topicAlias +" expected  " + _compareTopicAlias);
	    		}	    		
	    	}
	    }
	    
	    if (_hasTopicAlias != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has topic alias");
	    	boolean msgHasTopicAlias = false;
	    	if (msg.getTopicAlias() != null){
	    		msgHasTopicAlias = true;
	    	}
	    	if (!(_hasTopicAlias.equals(msgHasTopicAlias))){
	    		throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message has topic alias: " + msgHasTopicAlias +" expected " + _hasTopicAlias);
	    	}
	    }
	    
	    if (_mqttMessageVersion < 5) {
	    	_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+0),
						"mqtt message version: " + _mqttMessageVersion + ", not checking v5 properties");
	    }
	    
	    if (_compareSubscriptionIdentifier != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing SubscriptionID");
	        List<Integer> msgSubIds = msg.getSubscriptionIdentifiers();
	        if (msgSubIds.size() > 0) {
	        	int subid = msgSubIds.get(0);
	        	if (subid != _compareSubscriptionIdentifier) {
	        		throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
	                        "Received message on topic "+ msg.getTopic()+", SubscriptionIdentifier value was " + subid +" expected  " + _compareSubscriptionIdentifier);
	        	}
	        }
	    }
	    
	    if (_hasSubscriptionIdentifier != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Checking if message has subscription ID");
	    	List<Integer> msgSubIds = msg.getSubscriptionIdentifiers();
	    	boolean hasSubIds = false;
	    	if (msgSubIds.size() > 0) {
	    		hasSubIds = true;
	    	}
	    	if (!(_hasSubscriptionIdentifier.equals(hasSubIds))) {
	    		throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", message has sub id: " + hasSubIds +", expected " + _hasSubscriptionIdentifier);
	    	}
	    	
	    }
	    
	    if (_compareMsgExpire >= 0 && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing message expiry");
	        long expire = msg.getExpiry();
	        if (expire < 0 || expire > _compareMsgExpire) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+msg.getTopic()+", expiry value was " + expire +" expected <= " + _compareMsgExpire); 
	        }
	    } else if (_compareMsgExpire == -2) {
            long expire = msg.getExpiry();
            if (expire != -1) {
                throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+msg.getTopic()+", expiry value was " + expire +" expected no expiry"); 
            }
        }
	    
	    if (_comparePayloadFormatIndicator != null && _mqttMessageVersion >= 5) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+19)
					, "Comparing PayloadFormatIndicator");
	    	Boolean msgPayloadFormatIndicator = msg.getPayloadFormatIndicator();
	    	if(!(msgPayloadFormatIndicator.equals(_comparePayloadFormatIndicator))) {
	    		throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+17),
                        "Received message on topic "+ msg.getTopic()+", Payload Format Indicator value was " + msgPayloadFormatIndicator +" expected  " + _comparePayloadFormatIndicator);
	    	}
	    }
	    
		
		return true;
	}

	/*
	 * Compare user properties.
	 * We allow extra user properties, but the ones received are expected to be in order.
	 * This is a slightly higher requirement than MQTTv5 which only requires that properties
	 * of the same name be in order.
	 */
    boolean compareProps(List<MqttUserProp> expected, List<MqttUserProp> got) {
        if (_mqttMessageVersion < 5)                      /* No user properties before version 5 */
            return true;
        if (expected == null && got == null)  /* Both null they match */
            return true;
        if (expected == null || got == null)  /* Either is null and not the other they do not match */ {
        	System.out.println("expected: " + expected);
        	System.out.println("actual: " + got);        	
        	return false;
        }
        MqttUserProp[] gotarray = got.toArray(new MqttUserProp[0]);
        MqttUserProp[] exparray = expected.toArray(new MqttUserProp[0]);
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
            if (!good){
            	System.out.println("expected: " + exparray);
            	System.out.println("actual: " + gotarray);
            	return false;
            }
        }
        return true;
    }
    
    boolean checkPropKeys(List<String> expected, List<MqttUserProp> got){
    	if(_mqttMessageVersion < 5)
    		return true;
    	if(expected == null && got == null)
    		return true;
    	if(expected == null || got == null){
    		System.out.println("expected: " + expected);
    		System.out.println("actual: " + got);
    		return false;
    	}
    	MqttUserProp[] gotarray = got.toArray(new MqttUserProp[0]);
    	String[] expectedarray = expected.toArray(new String[0]);
    	for (int i=0;i<expectedarray.length;i++){
    		boolean keyFound = false;
    		for(int j=0;j<gotarray.length;j++){
    			if(gotarray[j].getKey().equals(expectedarray[i])){
    				keyFound = true;
    				System.out.println("user property key: " + expectedarray[i] + " found");
    			}
    		}
    		if(!keyFound){
    			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
    					, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
    					, "User Property with key: " + expectedarray[i] + " not found");
    			System.out.println("user property with key: " + expectedarray[i] + " not found");
    			return false;
    		}
    		
    	}
    	return true;
    	
    }
    
}
