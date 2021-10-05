/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : WsConnectionAction.java
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class SubscribeAction extends ApiCallAction {
	private final String _connectionID;
	private final int _QoS;   /* In MQTTv5 this is actually the subcription options */
	private final String _topic;
	private final String[] _topics;
	private final int _QoSlist[];
	private final boolean _durable;
	private final int [] _expectedrc;
	private final int [] _expectedrcv5;
	private final Integer _subid;
	private final List<MqttUserProp> _userprops;
	

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public SubscribeAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		
		super(config, dataRepository, trWriter);
		
		_connectionID = _actionParams.getProperty("connection_id");
		if (_connectionID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		_topic = _apiParameters.getProperty("topic");
		_durable = Boolean.valueOf(_apiParameters.getProperty("durable", "false"));
		// If topic is null, must specifies topics (multiple topic subscribe request)
		if (null == _topic) {
			String temp = _apiParameters.getProperty("topics");
			if (null == temp) {
				throw new RuntimeException("ISMTEST"+(Constants.SUBSCRIBE)
						+ " Either 'topic' or 'topics' must be supplied for Subscribe");
			}
			_topics = temp.split(":");
			_QoS = 0;
			temp = _apiParameters.getProperty("QoSlist");
			if (null == temp) {
				_QoSlist = null;
			} else {
				_QoSlist = new int[_topics.length];
				String[] qoslist = temp.split(":");
				for (int i = 0; i<_topics.length; i++) {
					if (i < qoslist.length) {
						_QoSlist[i] = Integer.parseInt(qoslist[i]);
					} else {
						_QoSlist[i] = 0;
					}
				}
			}
		} else {
			_topics = null;
			_QoSlist = null;
			_QoS = Integer.valueOf(_apiParameters.getProperty("QoS", "0"));
		}
		String temp = _apiParameters.getProperty("expectedrc");
		if (temp == null)
	        temp = _apiParameters.getProperty("expectedQoSlist");
		if (null == temp) {
			_expectedrc = null;
		} else {
			_expectedrc = new int[_topics == null ? 1 : _topics.length];
			String[] rclist = temp.split(":");
			for (int i = 0; i<_expectedrc.length; i++) {
				if (i < rclist.length) {
					_expectedrc[i] = Integer.parseInt(rclist[i]);
				} else {
					_expectedrc[i] = -1; // don't care
				}
			}
		}
	    temp = _apiParameters.getProperty("expectedrcv5");
	    if (temp == null) {
	        _expectedrcv5 = _expectedrc;
	    } else {
            _expectedrcv5 = new int[_topics == null ? 1 : _topics.length];
            String[] rclist = temp.split(":");
            for (int i = 0; i<_expectedrcv5.length; i++) {
                if (i < rclist.length) {
                    _expectedrcv5[i] = Integer.parseInt(rclist[i]);
                } else {
                    _expectedrcv5[i] = -1; // don't care
                }
            }	        
	    }
	    _userprops = getUserProps("userprop");

        temp = _apiParameters.getProperty("subscriptionID");
        if (temp != null) {
        	_subid = Integer.parseInt(_apiParameters.getProperty("subscriptionID"));
        } else {
        	_subid = null;
        }
	}

	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con = (MyConnection)_dataRepository.getVar(_connectionID);
		if (con == null) throw(new IsmTestException("ISMTEST"+(Constants.SUBSCRIBE)+2
				, "Cannot find connection named '"+_connectionID+"'"));
		setConnection(con);
		
		int[] rclist;
		if (null != _topic) {
			String topic = _topic;
			
			Object storeVar = _dataRepository.getVar(_topic);
			if (null != storeVar) {
				if (storeVar instanceof NumberString) {			
					topic = ((NumberString)storeVar).incrementAndGetName();
				} else if (storeVar instanceof String) {
					topic = (String)storeVar;
				}
			}			
			if (_durable) {
				int rc;
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.SUBSCRIBE)+10
						, "Subscribing to topic durable: "+ _topics + " at QoS: " + _QoSlist);
				if (0 != (rc = con.subscribe(topic, _QoS, _durable))) {
					throw new IsmTestException("ISMTEST"+(Constants.SUBSCRIBE)+3
						, "Error "+rc+" attempting to subscribe to topic '"+topic+"'");
				}
				return true;
			} else {
				rclist = con.subscribe(topic, _QoS, _subid, _userprops);
			}
		} else {
			if (_durable) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.SUBSCRIBE)+10
						, "Subscribing to topics durable: "+ _topics + " at QoS: " + _QoSlist);
				con.subscribe(_topics, _QoSlist, _durable);
				return true;
			} else {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.SUBSCRIBE)+10
						, "Subscribing to topic: "+ _topics + " at QoS: " + _QoSlist);
				rclist = con.subscribe(_topics, _QoSlist, _subid, _userprops);
			}
		}
		boolean result = true;
		int [] expected = version >=5 ? _expectedrcv5 : _expectedrc;
		if (null != rclist && null != expected) {
			for (int i = 0; i < expected.length; i++) {
				if (-1 != expected[i] && rclist[i] != expected[i]) {
					result = false;
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.SUBSCRIBE+4)
						, "Expected return code for element "+ i + " to be " + expected[i] + " got " + rclist[i]);
				} else {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST" + (Constants.SUBSCRIBE + 5),
							"Return code for element " + i + " match, expected: " + expected[i] + ", received: " + rclist[i]);
				}
			}
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST" + (Constants.SUBSCRIBE + 6),
					"Subscribe action not checking for specific return code");

			if (null != rclist && con.getVersion() == 5) {
				for (int i = 0; i < rclist.length; i++) {
					if (rclist[i] > 2) {
						result = false;
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST" + (Constants.SUBSCRIBE + 7),
								"Subscribe failed, received MQTTv5 return code " + rclist[i]);

					}
				}
			}
		}
	
		return result;
	}

}
